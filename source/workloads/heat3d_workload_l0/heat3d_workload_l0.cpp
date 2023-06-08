/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
 * This program simulates the heat transfer in a 3D square block of solid material, placed in a
 * right-handed coordinate system. The heat equation is solved using the simplest explicit scheme
 * (7-point stencil).
 *
 *  Z
 *  ^   Y
 *  |  7
 *  | /
 *  |/
 *  +-------> X
 *
 *  Whenever we need to pick an ordering, the z-y-x order is used:
 *  1. Use all the available slots in the z-direction first, fix x & y
 *  2. When the z-column is filled up, we move to the next z-column in the y-direction
 *  3. When the yz-plane is filled up, we move to the next yz-plane in the x-direction
 *
 *  Therefore:
 *  1. Neighboring ranks will likely to have consecutive sub-domain z-coordinates
 *  2. When storing the mesh points in a 1D array, the z-columns of a yz-plane will be stored
 *  one-after-one, then we move to the next yz-plane
 *  3. As a consequence of (2), although we transfer data between a 3D array and a 2D array when we
 *  pack/unpack the buffers, we can use the same triple-loop to deal with different xyz-ranges and
 *  buffer dimensions, as long as the loop ordering is the same everywhere. This is because one of
 *  the i/j/k index is fixed during the process, so we traverse the sub-domain mesh and the buffer
 *  in the same order.
 *
 *  Each process handles a sub-domain of the simulation domain, these 3D sub-domains look like
 *  onions:
 *  1. The outer-most "shell" stores a single layer of ghost points for the six neighbors' facets,
 *  which will be updated at the end of every iteration, using the latest data sent by the
 *  neighbors. For each ghost facet, one of the coordinates is fixed to either [0] or [npt_* + 1],
 *  and the other two coordinates are in the range of [1, npt_*]. Therefore, the edges and the
 *  corners are unused, and the ghost points are stored in the inner (npt_* x npt_*) area.
 *  2. The next "shell" is the surface of the sub-domain managed by this process. For each facet of
 *  the sub-domain, one of the coordinates is fixed to either [1] or [npt_*], while the other two
 *  are in the range of [1, npt_*]. Note that there are overlaps between the facets under this
 *  definition, so we need to be careful to not update any points on the edges more than once when
 *  updating the facets. In this implementation, we update the full yz-facets, partial xz-facets
 *  and the inner-parts of the xy-facets. The full facets should still be copied to and from the
 *  send and the receive buffers, respectively.
 *  3. The most inner block is the interior of the sub-domain that does not need data from other
 *  sub-domains. All three coordinates of this block are in the rage of [2, npt_* - 1].
 *
 *  Since we cannot overwrite the temperature of un-updated mesh points, we allocate two buffers to
 *  store two copies of the sub-domain, and alternate them at the end of each iteration.
 */

#include "framework/l0/levelzero.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/linux/ipc.h"
#include "framework/utility/timer.h"
#include "framework/workload/register_workload.h"

#ifndef USE_PIDFD
const std::string masterSocketName{"/tmp/heat3d.socket"};
#endif // USE_PIDFD

struct Heat3DArguments : WorkloadArgumentContainer {
    IntegerArgument rank;
    PositiveIntegerArgument parentPid;
#ifdef USE_PIDFD
    StringArgument initBufferIpcHandle;
    StringArgument evPoolIpcHandle;
#endif // USE_PIDFD
    PositiveIntegerArgument nSubDomainX;
    PositiveIntegerArgument nSubDomainY;
    PositiveIntegerArgument nSubDomainZ;
    PositiveIntegerArgument nTimesteps;
    PositiveIntegerArgument meshLength;

    Heat3DArguments()
        : rank(*this, "rank", "Execution rank of this process"),
          parentPid(*this, "parentPid", "PID of the parent process"),
#ifdef USE_PIDFD
          initBufferIpcHandle(*this, "initBufferIpcHandle", "IPC handle of the shared initialization buffer"),
          evPoolIpcHandle(*this, "evPoolIpcHandle", "IPC handle of the Level Zero event pool"),
#endif // USE_PIDFD
          nSubDomainX(*this, "nSubDomainX", "Number of sub-domains in the X-direction"),
          nSubDomainY(*this, "nSubDomainY", "Number of sub-domains in the Y-direction"),
          nSubDomainZ(*this, "nSubDomainZ", "Number of sub-domains in the Z-direction"),
          nTimesteps(*this, "nTimesteps", "Number of simulation timesteps"),
          meshLength(*this, "meshLength", "Number of mesh points along each of the X-Y-Z directions") {
        synchronize = true;
    }
};

struct Heat3D : Workload<Heat3DArguments> {};

// Identify the six facets of a sub-domain
enum class FacetTy : uint8_t { XU = 0,
                               XD = 1,
                               YU = 2,
                               YD = 3,
                               ZU = 4,
                               ZD = 5,
                               LAST = 6 };

// Stores info about a particular facet in a unified form
struct FacetInfoTy {
    // Send, receive, and the neighbor's receive buffer of this facet
    float *sendBuffer, *recvBuffer, *recvBufferNeighbor;
    // Length of the buffers
    size_t bufferLen;
    // The neighbor's rank in this facet's direction
    int neighborRank;
    // The facet on the neighbor rank that is connected to this facet (U <-> D)
    FacetTy neighborFacet;
    // Indices below take ghost points into account and are exact, so use <= when iterating over the 3D matrices
    // The starting & ending indices for the outer shell ghost facets, use these to copy to the outer shell facets
    uint32_t outerFacetXStart, outerFacetXEnd, outerFacetYStart, outerFacetYEnd, outerFacetZStart, outerFacetZEnd;
    // The starting & ending indices for the inner shell sub-domain facets, use these to copy from the inner shell facets
    uint32_t innerFacetXStart, innerFacetXEnd, innerFacetYStart, innerFacetYEnd, innerFacetZStart, innerFacetZEnd;
    // The starting & ending indices for the inner shell sub-domain facets (de-duplicated), use these to update the inner shell facets
    uint32_t innerFacetXDedupStart, innerFacetXDedupEnd, innerFacetYDedupStart, innerFacetYDedupEnd, innerFacetZDedupStart, innerFacetZDedupEnd;
};

// Store various simulation parameters & variables
struct ParamsTy {
    ParamsTy(LevelZero &levelzero) : levelzero(levelzero) {}

    LevelZero &levelzero;
    ze_module_handle_t module;
    ze_kernel_handle_t kernelInitTemp, kernelPackSendBuf, kernelUnpackRecvBuf, kernelUpdateFacet, kernelUpdateInterior;
    ze_command_list_handle_t cmdlist; // Immediate
    void *initBuffer;
    ze_event_pool_handle_t barrierEvPool;
    std::vector<ze_event_handle_t> barrierEvents;

#ifndef USE_PIDFD
    int socketWorker;
#endif // USE_PIDFD

    uint32_t rank, nRanks;
    // Number of sub-domains in each direction
    uint32_t nSubDomainX, nSubDomainY, nSubDomainZ;
    // Sub-domain coordinates of this rank
    uint32_t subDomainCoordX, subDomainCoordY, subDomainCoordZ;
    uint32_t meshLength, nTimesteps;
    // Neighbor ranks in each direction, up & down
    // Assuming periodic boundary condition, so the neighbors wraparound
    uint32_t neighbors[int(FacetTy::LAST)];
    // Number of mesh points on each side of the sub-domain, sans the ghost facets
    uint32_t nPointsX, nPointsY, nPointsZ;
    // Thermal conductivity, space & time resolution
    float thermalConductivity, deltaSpace, deltaTime;
    // Store the sub-domain (incl. ghost facets) in flat storage, for current & next timestep
    float *subDomainOld, *subDomainNew;
    // Send buffers for the six facets
    float *sendBuffers[int(FacetTy::LAST)];
    // Receive buffers for the six facets
    float *recvBuffers[int(FacetTy::LAST)];
    // Store information of the six facets in the sub-domain, determined by the topology of the ranks, won't change during the simulation
    FacetInfoTy facetInfo[int(FacetTy::LAST)];
};

// Stores the PID and the IPC handle for the halo exchange receive buffers
struct HaloBufferInfoTy {
    pid_t pid;
    ze_ipc_mem_handle_t ipcHandle;
};

static TestResult ipcBarrierWorker(ParamsTy &params) {
    // Notify the master process
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(params.barrierEvents[params.rank]));
    // Wait for the master process
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(params.barrierEvents[params.rank + params.nRanks], UINT64_MAX));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(params.barrierEvents[params.rank + params.nRanks]));

    return TestResult::Success;
}

#ifdef USE_PIDFD

static TestResult deserializeAndPatchIpcHandle(const pid_t parentPid, std::string handleDataStr, void *handleData) {
    deserializeStrToBinary(handleData, handleDataStr);
    const int parentFd = *reinterpret_cast<int *>(handleData);
    if (parentFd <= 0) {
        return TestResult::Error;
    }
    int fd = -1;
    translateParentFd(parentPid, parentFd, fd);
    *reinterpret_cast<int *>(handleData) = fd;

    return TestResult::Success;
}

#endif // USE_PIDFD

static uint32_t subDomainCoordToRank(const uint32_t x, const uint32_t y, const uint32_t z, const ParamsTy &params) {
    return x * params.nSubDomainY * params.nSubDomainZ + y * params.nSubDomainZ + z;
}

static void findNeighbors(ParamsTy &params) {
    // Compute the coordinates for each of the directions
    const uint32_t coordXU = (params.subDomainCoordX + 1 + params.nSubDomainX) % params.nSubDomainX;
    const uint32_t coordXD = (params.subDomainCoordX - 1 + params.nSubDomainX) % params.nSubDomainX;
    const uint32_t coordYU = (params.subDomainCoordY + 1 + params.nSubDomainY) % params.nSubDomainY;
    const uint32_t coordYD = (params.subDomainCoordY - 1 + params.nSubDomainY) % params.nSubDomainY;
    const uint32_t coordZU = (params.subDomainCoordZ + 1 + params.nSubDomainZ) % params.nSubDomainZ;
    const uint32_t coordZD = (params.subDomainCoordZ - 1 + params.nSubDomainZ) % params.nSubDomainZ;

    // Compute the IDs of the neighboring PEs
    params.neighbors[int(FacetTy::XU)] = subDomainCoordToRank(coordXU, params.subDomainCoordY, params.subDomainCoordZ, params);
    params.neighbors[int(FacetTy::XD)] = subDomainCoordToRank(coordXD, params.subDomainCoordY, params.subDomainCoordZ, params);
    params.neighbors[int(FacetTy::YU)] = subDomainCoordToRank(params.subDomainCoordX, coordYU, params.subDomainCoordZ, params);
    params.neighbors[int(FacetTy::YD)] = subDomainCoordToRank(params.subDomainCoordX, coordYD, params.subDomainCoordZ, params);
    params.neighbors[int(FacetTy::ZU)] = subDomainCoordToRank(params.subDomainCoordX, params.subDomainCoordY, coordZU, params);
    params.neighbors[int(FacetTy::ZD)] = subDomainCoordToRank(params.subDomainCoordX, params.subDomainCoordY, coordZD, params);
}

static TestResult allocateStorage(ParamsTy &params) {
    void *ptr = nullptr;

    ze_device_mem_alloc_desc_t desc = {};
    desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    desc.pNext = nullptr;
    desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
    desc.ordinal = 0;

    // Storage for the sub-domain, including the ghost arrays (hence the +2)
    const size_t subDomainSz = sizeof(float) * (params.nPointsX + 2) * (params.nPointsY + 2) * (params.nPointsZ + 2);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, subDomainSz, 0, params.levelzero.device, &ptr));
    params.subDomainOld = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, subDomainSz, 0, params.levelzero.device, &ptr));
    params.subDomainNew = static_cast<float *>(ptr);

    // Allocate the send buffers for the ghost facets
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsY * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.sendBuffers[int(FacetTy::XU)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsY * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.sendBuffers[int(FacetTy::XD)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.sendBuffers[int(FacetTy::YU)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.sendBuffers[int(FacetTy::YD)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsY, 0, params.levelzero.device, &ptr));
    params.sendBuffers[int(FacetTy::ZU)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsY, 0, params.levelzero.device, &ptr));
    params.sendBuffers[int(FacetTy::ZD)] = static_cast<float *>(ptr);

    // Allocate the receive buffers for the ghost facets
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsY * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.recvBuffers[int(FacetTy::XU)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsY * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.recvBuffers[int(FacetTy::XD)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.recvBuffers[int(FacetTy::YU)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsZ, 0, params.levelzero.device, &ptr));
    params.recvBuffers[int(FacetTy::YD)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsY, 0, params.levelzero.device, &ptr));
    params.recvBuffers[int(FacetTy::ZU)] = static_cast<float *>(ptr);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(params.levelzero.context, &desc, sizeof(float) * params.nPointsX * params.nPointsY, 0, params.levelzero.device, &ptr));
    params.recvBuffers[int(FacetTy::ZD)] = static_cast<float *>(ptr);

    return TestResult::Success;
}

FacetTy reverseFacetUD(const FacetTy facet) {
    return FacetTy(((int(facet) / 2) * 2) + ((int(facet) % 2) ^ 1));
}

FacetInfoTy makeFacetInfo(const FacetTy facet, ParamsTy &params) {
    FacetInfoTy facetInfo;

    facetInfo.sendBuffer = params.sendBuffers[int(facet)];
    facetInfo.recvBuffer = params.recvBuffers[int(facet)];
    facetInfo.neighborRank = params.neighbors[int(facet)];
    facetInfo.neighborFacet = reverseFacetUD(facet);

    switch (facet) {
    case FacetTy::XU:
        facetInfo.bufferLen = params.nPointsY * params.nPointsZ;

        facetInfo.outerFacetXStart = params.nPointsX + 1; // X-Front of the outer shell
        facetInfo.outerFacetXEnd = params.nPointsX + 1;   // X-Front of the outer shell
        facetInfo.outerFacetYStart = 1;                   // Y range covers the whole
        facetInfo.outerFacetYEnd = params.nPointsY;       // sub-domain facet
        facetInfo.outerFacetZStart = 1;                   // Z range covers the whole
        facetInfo.outerFacetZEnd = params.nPointsZ;       // sub-domain facet

        facetInfo.innerFacetXStart = params.nPointsX; // X-Front of the inner shell
        facetInfo.innerFacetXEnd = params.nPointsX;   // X-Front of the inner shell
        facetInfo.innerFacetYStart = 1;               // Y range covers the whole
        facetInfo.innerFacetYEnd = params.nPointsY;   // sub-domain facet
        facetInfo.innerFacetZStart = 1;               // Z range covers the whole
        facetInfo.innerFacetZEnd = params.nPointsZ;   // sub-domain facet

        facetInfo.innerFacetXDedupStart = params.nPointsX; // X-Front of the inner shell
        facetInfo.innerFacetXDedupEnd = params.nPointsX;   // X-Front of the inner shell
        facetInfo.innerFacetYDedupStart = 1;               // Y range covers the whole
        facetInfo.innerFacetYDedupEnd = params.nPointsY;   // sub-domain facet
        facetInfo.innerFacetZDedupStart = 1;               // Z range covers the whole
        facetInfo.innerFacetZDedupEnd = params.nPointsZ;   // sub-domain facet
        break;

    case FacetTy::XD:
        facetInfo.bufferLen = params.nPointsY * params.nPointsZ;

        facetInfo.outerFacetXStart = 0;             // X-Back of the outer shell
        facetInfo.outerFacetXEnd = 0;               // X-Back of the outer shell
        facetInfo.outerFacetYStart = 1;             // Y range covers the whole
        facetInfo.outerFacetYEnd = params.nPointsY; // sub-domain facet
        facetInfo.outerFacetZStart = 1;             // Z range covers the whole
        facetInfo.outerFacetZEnd = params.nPointsZ; // sub-domain facet

        facetInfo.innerFacetXStart = 1;             // X-Back of the inner shell
        facetInfo.innerFacetXEnd = 1;               // X-Back of the inner shell
        facetInfo.innerFacetYStart = 1;             // Y range covers the whole
        facetInfo.innerFacetYEnd = params.nPointsY; // sub-domain facet
        facetInfo.innerFacetZStart = 1;             // Z range covers the whole
        facetInfo.innerFacetZEnd = params.nPointsZ; // sub-domain facet

        facetInfo.innerFacetXDedupStart = 1;             // X-Back of the inner shell
        facetInfo.innerFacetXDedupEnd = 1;               // X-Back of the inner shell
        facetInfo.innerFacetYDedupStart = 1;             // Y range covers the whole
        facetInfo.innerFacetYDedupEnd = params.nPointsY; // sub-domain facet
        facetInfo.innerFacetZDedupStart = 1;             // Z range covers the whole
        facetInfo.innerFacetZDedupEnd = params.nPointsZ; // sub-domain facet
        break;

    case FacetTy::YU:
        facetInfo.bufferLen = params.nPointsX * params.nPointsZ;

        facetInfo.outerFacetXStart = 1;                   // X range covers the whole
        facetInfo.outerFacetXEnd = params.nPointsX;       // sub-domain facet
        facetInfo.outerFacetYStart = params.nPointsY + 1; // Y-Front of the outer shell
        facetInfo.outerFacetYEnd = params.nPointsY + 1;   // Y-Front of the outer shell
        facetInfo.outerFacetZStart = 1;                   // Z range covers the whole
        facetInfo.outerFacetZEnd = params.nPointsZ;       // sub-domain facet

        facetInfo.innerFacetXStart = 1;               // X range covers the whole
        facetInfo.innerFacetXEnd = params.nPointsX;   // sub-domain facet
        facetInfo.innerFacetYStart = params.nPointsY; // Y-Front of the inner shell
        facetInfo.innerFacetYEnd = params.nPointsY;   // Y-Front of the inner shell
        facetInfo.innerFacetZStart = 1;               // Z range covers the whole
        facetInfo.innerFacetZEnd = params.nPointsZ;   // sub-domain facet

        facetInfo.innerFacetXDedupStart = 2;                 // Avoid duplicating the
        facetInfo.innerFacetXDedupEnd = params.nPointsX - 1; // X-Front and X-Back
        facetInfo.innerFacetYDedupStart = params.nPointsY;   // Y-Front of the inner shell
        facetInfo.innerFacetYDedupEnd = params.nPointsY;     // Y-Front of the inner shell
        facetInfo.innerFacetZDedupStart = 1;                 // Z range covers the whole
        facetInfo.innerFacetZDedupEnd = params.nPointsZ;     // sub-domain facet
        break;

    case FacetTy::YD:
        facetInfo.bufferLen = params.nPointsX * params.nPointsZ;

        facetInfo.outerFacetXStart = 1;             // X range covers the whole
        facetInfo.outerFacetXEnd = params.nPointsX; // sub-domain facet
        facetInfo.outerFacetYStart = 0;             // Y-Back of the outer shell
        facetInfo.outerFacetYEnd = 0;               // Y-Back of the outer shell
        facetInfo.outerFacetZStart = 1;             // Z range covers the whole
        facetInfo.outerFacetZEnd = params.nPointsZ; // sub-domain facet

        facetInfo.innerFacetXStart = 1;             // X range covers the whole
        facetInfo.innerFacetXEnd = params.nPointsX; // sub-domain facet
        facetInfo.innerFacetYStart = 1;             // Y-Back of the inner shell
        facetInfo.innerFacetYEnd = 1;               // Y-Back of the inner shell
        facetInfo.innerFacetZStart = 1;             // Z range covers the whole
        facetInfo.innerFacetZEnd = params.nPointsZ; // sub-domain facet

        facetInfo.innerFacetXDedupStart = 2;                 // Avoid duplicating the
        facetInfo.innerFacetXDedupEnd = params.nPointsX - 1; // X-Front and X-Back
        facetInfo.innerFacetYDedupStart = 1;                 // Y-Back of the inner shell
        facetInfo.innerFacetYDedupEnd = 1;                   // Y-Back of the inner shell
        facetInfo.innerFacetZDedupStart = 1;                 // Z range covers the whole
        facetInfo.innerFacetZDedupEnd = params.nPointsZ;     // sub-domain facet
        break;

    case FacetTy::ZU:
        facetInfo.bufferLen = params.nPointsX * params.nPointsY;

        facetInfo.outerFacetXStart = 1;                   // X range covers the whole
        facetInfo.outerFacetXEnd = params.nPointsX;       // sub-domain facet
        facetInfo.outerFacetYStart = 1;                   // Y range covers the whole
        facetInfo.outerFacetYEnd = params.nPointsY;       // sub-domain facet
        facetInfo.outerFacetZStart = params.nPointsZ + 1; // Z-Front of the outer shell
        facetInfo.outerFacetZEnd = params.nPointsZ + 1;   // Z-Front of the outer shell

        facetInfo.innerFacetXStart = 1;               // X range covers the whole
        facetInfo.innerFacetXEnd = params.nPointsX;   // sub-domain facet
        facetInfo.innerFacetYStart = 1;               // Y range covers the whole
        facetInfo.innerFacetYEnd = params.nPointsY;   // sub-domain facet
        facetInfo.innerFacetZStart = params.nPointsZ; // Z-Front of the inner shell
        facetInfo.innerFacetZEnd = params.nPointsZ;   // Z-Front of the inner shell

        facetInfo.innerFacetXDedupStart = 2;                 // Avoid duplicating the
        facetInfo.innerFacetXDedupEnd = params.nPointsX - 1; // X-Front and X-Back
        facetInfo.innerFacetYDedupStart = 2;                 // Avoid duplicating the
        facetInfo.innerFacetYDedupEnd = params.nPointsY - 1; // Y-Front and Y-Back
        facetInfo.innerFacetZDedupStart = params.nPointsZ;   // Z-Front of the inner shell
        facetInfo.innerFacetZDedupEnd = params.nPointsZ;     // Z-Front of the inner shell
        break;

    case FacetTy::ZD:
        facetInfo.bufferLen = params.nPointsX * params.nPointsY;

        facetInfo.outerFacetXStart = 1;             // X range covers the whole
        facetInfo.outerFacetXEnd = params.nPointsX; // sub-domain facet
        facetInfo.outerFacetYStart = 1;             // Y range covers the whole
        facetInfo.outerFacetYEnd = params.nPointsY; // sub-domain facet
        facetInfo.outerFacetZStart = 0;             // Z-Back of the outer shell
        facetInfo.outerFacetZEnd = 0;               // Z-Back of the outer shell

        facetInfo.innerFacetXStart = 1;             // X range covers the whole
        facetInfo.innerFacetXEnd = params.nPointsX; // sub-domain facet
        facetInfo.innerFacetYStart = 1;             // Y range covers the whole
        facetInfo.innerFacetYEnd = params.nPointsY; // sub-domain facet
        facetInfo.innerFacetZStart = 1;             // Z-Back of the inner shell
        facetInfo.innerFacetZEnd = 1;               // Z-Back of the inner shell

        facetInfo.innerFacetXDedupStart = 2;                 // Avoid duplicating the
        facetInfo.innerFacetXDedupEnd = params.nPointsX - 1; // X-Front and X-Back
        facetInfo.innerFacetYDedupStart = 2;                 // Avoid duplicating the
        facetInfo.innerFacetYDedupEnd = params.nPointsY - 1; // Y-Front and Y-Back
        facetInfo.innerFacetZDedupStart = 1;                 // Z-Back of the inner shell
        facetInfo.innerFacetZDedupEnd = 1;                   // Z-Back of the inner shell
        break;

    default:
        break;
    }

    return facetInfo;
}

static TestResult initParams(ParamsTy &params, const Heat3DArguments &arguments) {
    ze_command_queue_desc_t queueDesc = {};
    queueDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    queueDesc.pNext = nullptr;
    queueDesc.ordinal = 0;
    queueDesc.index = 0;
    queueDesc.flags = 0;
    queueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    queueDesc.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(params.levelzero.context, params.levelzero.device, &queueDesc, &params.cmdlist));

    params.rank = arguments.rank;
    params.nRanks = arguments.nSubDomainX * arguments.nSubDomainY * arguments.nSubDomainZ;
    params.nSubDomainX = arguments.nSubDomainX;
    params.nSubDomainY = arguments.nSubDomainY;
    params.nSubDomainZ = arguments.nSubDomainZ;

    // Initialize this rank's sub-domain coordinates
    params.subDomainCoordX = params.rank / (params.nSubDomainY * params.nSubDomainZ);
    params.subDomainCoordY = (params.rank - params.subDomainCoordX * params.nSubDomainY * params.nSubDomainZ) / params.nSubDomainZ;
    params.subDomainCoordZ = params.rank - params.subDomainCoordX * params.nSubDomainY * params.nSubDomainZ - params.subDomainCoordY * params.nSubDomainZ;

    EXPECT_EQ(subDomainCoordToRank(params.subDomainCoordX, params.subDomainCoordY, params.subDomainCoordZ, params), params.rank);

    params.meshLength = arguments.meshLength;
    params.nTimesteps = arguments.nTimesteps;

    findNeighbors(params);

    params.nPointsX = params.meshLength / params.nSubDomainX;
    params.nPointsY = params.meshLength / params.nSubDomainY;
    params.nPointsZ = params.meshLength / params.nSubDomainZ;

    // Thermal conductivity parameter
    params.thermalConductivity = 1.0;
    // Spacial discretization length, uniform in all three dimensions
    params.deltaSpace = 1.0 / params.meshLength;
    // Tempral discretization, respecting the convergence criterion
    params.deltaTime = params.deltaSpace * params.deltaSpace / (8.1 * params.thermalConductivity);

    allocateStorage(params);

    // When the buffers are ready and we know who our neighbors are, we can associate them with each of the six facets
    for (uint8_t i = 0; i < int(FacetTy::LAST); i++) {
        params.facetInfo[i] = makeFacetInfo(FacetTy(i), params);
    }

    params.barrierEvents.resize(2 * params.nRanks);

#ifdef USE_PIDFD
    ze_ipc_event_pool_handle_t evPoolIpcHandle{};
    deserializeAndPatchIpcHandle(arguments.parentPid, std::string(arguments.evPoolIpcHandle), evPoolIpcHandle.data);
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolOpenIpcHandle(params.levelzero.context, evPoolIpcHandle, &params.barrierEvPool));

    ze_ipc_mem_handle_t initBufferIpcHandle{};
    deserializeAndPatchIpcHandle(arguments.parentPid, std::string(arguments.initBufferIpcHandle), initBufferIpcHandle.data);
    ASSERT_ZE_RESULT_SUCCESS(zeMemOpenIpcHandle(params.levelzero.context, params.levelzero.device, initBufferIpcHandle, 0, &params.initBuffer));
#else
    socketCreate(params.socketWorker);
    socketConnect(params.socketWorker, masterSocketName);

    int fd = -1;

    ze_ipc_mem_handle_t initBufferIpcHandle{};
    std::fill_n(initBufferIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE, 0);
    socketRecvDataWithFd(params.socketWorker, fd, initBufferIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE);
    *reinterpret_cast<int *>(initBufferIpcHandle.data) = fd;
    ASSERT_ZE_RESULT_SUCCESS(zeMemOpenIpcHandle(params.levelzero.context, params.levelzero.device, initBufferIpcHandle, 0, &params.initBuffer));

    ze_ipc_event_pool_handle_t barrierEvPoolIpcHandle{};
    std::fill_n(barrierEvPoolIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE, 0);
    socketRecvDataWithFd(params.socketWorker, fd, barrierEvPoolIpcHandle.data, ZE_MAX_IPC_HANDLE_SIZE);
    *reinterpret_cast<int *>(barrierEvPoolIpcHandle.data) = fd;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolOpenIpcHandle(params.levelzero.context, barrierEvPoolIpcHandle, &params.barrierEvPool));
#endif // USE_PIDFD

    ze_event_desc_t barrierEventDesc = {};
    barrierEventDesc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    barrierEventDesc.pNext = nullptr;
    barrierEventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    barrierEventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    for (uint32_t i = 0; i < 2 * params.nRanks; i++) {
        barrierEventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(params.barrierEvPool, &barrierEventDesc, &params.barrierEvents[i]));
    }

    auto spirvModule = FileHelper::loadBinaryFile("heat3d_workload.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(params.levelzero.context, params.levelzero.device, &moduleDesc, &params.module, nullptr));

    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "init_temperature";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(params.module, &kernelDesc, &params.kernelInitTemp));
    kernelDesc.pKernelName = "pack_send_buffer";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(params.module, &kernelDesc, &params.kernelPackSendBuf));
    kernelDesc.pKernelName = "unpack_recv_buffer";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(params.module, &kernelDesc, &params.kernelUnpackRecvBuf));
    kernelDesc.pKernelName = "update_facet";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(params.module, &kernelDesc, &params.kernelUpdateFacet));
    kernelDesc.pKernelName = "update_interior";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(params.module, &kernelDesc, &params.kernelUpdateInterior));

#ifdef USE_PIDFD
    // Write receive buffers' IPC handles into shared init buffer
    for (int i = 0; i < int(FacetTy::LAST); i++) {
        const size_t initBufferOffset = (6 * params.rank + i) * 1024;
        HaloBufferInfoTy *haloBufferInfoPtr = reinterpret_cast<HaloBufferInfoTy *>(static_cast<uint8_t *>(params.initBuffer) + initBufferOffset);
        haloBufferInfoPtr->pid = getpid();
        std::fill_n(haloBufferInfoPtr->ipcHandle.data, ZE_MAX_IPC_HANDLE_SIZE, 0);
        ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(params.levelzero.context, params.recvBuffers[i], &haloBufferInfoPtr->ipcHandle));
    }

    ipcBarrierWorker(params);

    // Find each facet's neighbor rank's receive buffer, and store the address
    for (int i = 0; i < int(FacetTy::LAST); i++) {
        const uint32_t neighborRank = params.neighbors[i];
        const size_t neighborInitBufferOffset = (6 * neighborRank + int(reverseFacetUD(FacetTy(i)))) * 1024;
        HaloBufferInfoTy *haloBufferInfoPtr = reinterpret_cast<HaloBufferInfoTy *>(static_cast<uint8_t *>(params.initBuffer) + neighborInitBufferOffset);
        ze_ipc_mem_handle_t ipcHandle = haloBufferInfoPtr->ipcHandle;
        const int parentFd = *reinterpret_cast<int *>(ipcHandle.data);
        EXPECT_TRUE(parentFd > 0);
        int fd = -1;
        translateParentFd(haloBufferInfoPtr->pid, parentFd, fd);
        EXPECT_TRUE(fd > 0);
        *reinterpret_cast<int *>(ipcHandle.data) = fd;
        ASSERT_ZE_RESULT_SUCCESS(zeMemOpenIpcHandle(params.levelzero.context, params.levelzero.device, ipcHandle, 0, reinterpret_cast<void **>(&params.facetInfo[i].recvBufferNeighbor)));
    }
#else
    std::vector<std::vector<ze_ipc_mem_handle_t>> ipcHandles{params.nRanks};
    for (uint32_t r = 0; r < params.nRanks; r++) {
        ipcHandles[r].resize(int(FacetTy::LAST));
        for (int f = 0; f < int(FacetTy::LAST); f++) {
            std::fill_n(ipcHandles[r][f].data, ZE_MAX_IPC_HANDLE_SIZE, 0);
            if (r == params.rank) {
                ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(params.levelzero.context, params.recvBuffers[f], &ipcHandles[r][f]));
            }
        }
    }

    for (uint32_t r = 0; r < params.nRanks; r++) {
        if (params.nRanks == 1) {
            break;
        }
        int ipcSocket = -1;
        socketCreate(ipcSocket);
        EXPECT_TRUE(ipcSocket > 0);
        const std::string socketName = "/tmp/heat3d_" + std::to_string(r) + ".socket";

        ipcBarrierWorker(params);

        if (r == params.rank) {
            socketBindAndListen(ipcSocket, socketName);

            std::vector<int> socketOthers{};
            for (uint32_t rankOther = 0; rankOther < params.nRanks; rankOther++) {
                if (rankOther == r) {
                    continue;
                }
                int socketOther = -1;
                socketAccept(ipcSocket, socketOthers, socketOther);
                for (int f = 0; f < int(FacetTy::LAST); f++) {
                    const int fd = *reinterpret_cast<int *>(ipcHandles[r][f].data);
                    socketSendDataWithFd(socketOther, fd, ipcHandles[r][f].data, ZE_MAX_IPC_HANDLE_SIZE);
                }
            }
            for (int socketOther : socketOthers) {
                EXPECT_EQ(0, close(socketOther));
            }
            EXPECT_EQ(0, unlink(socketName.c_str()));
        } else {
            usleep(50000 * (params.nRanks - params.rank));
            socketConnect(ipcSocket, socketName);
            for (int f = 0; f < int(FacetTy::LAST); f++) {
                int fd = -1;
                socketRecvDataWithFd(ipcSocket, fd, ipcHandles[r][f].data, ZE_MAX_IPC_HANDLE_SIZE);
                EXPECT_TRUE(fd > 0);
                *reinterpret_cast<int *>(ipcHandles[r][f].data) = fd;
            }
        }

        ipcBarrierWorker(params);

        EXPECT_EQ(0, close(ipcSocket));
    }

    for (int f = 0; f < int(FacetTy::LAST); f++) {
        const uint32_t neighborRank = params.neighbors[f];
        const int neighborFacet = int(reverseFacetUD(FacetTy(f)));
        auto ipcHandle = ipcHandles[neighborRank][neighborFacet];
        ASSERT_ZE_RESULT_SUCCESS(zeMemOpenIpcHandle(params.levelzero.context, params.levelzero.device, ipcHandle, 0, reinterpret_cast<void **>(&params.facetInfo[f].recvBufferNeighbor)));
    }
#endif // USE_PIDFD

    return TestResult::Success;
}

static TestResult initTemperature(ParamsTy &params) {
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(params.kernelInitTemp, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 0, sizeof(params.subDomainNew), &params.subDomainNew));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 1, sizeof(params.nPointsX), &params.nPointsX));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 2, sizeof(params.nPointsY), &params.nPointsY));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 3, sizeof(params.nPointsZ), &params.nPointsZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 4, sizeof(params.subDomainCoordX), &params.subDomainCoordX));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 5, sizeof(params.subDomainCoordY), &params.subDomainCoordY));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 6, sizeof(params.subDomainCoordZ), &params.subDomainCoordZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelInitTemp, 7, sizeof(params.deltaSpace), &params.deltaSpace));

    const ze_group_count_t dispatch = {params.nPointsX, params.nPointsY, params.nPointsZ};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(params.cmdlist, params.kernelInitTemp, &dispatch, nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult packSendBuffer(FacetTy facet, ParamsTy &params) {
    const FacetInfoTy &facetInfo = params.facetInfo[int(facet)];

    // Copying from the inner shell facets
    const uint32_t xRange = facetInfo.innerFacetXEnd - facetInfo.innerFacetXStart + 1;
    const uint32_t yRange = facetInfo.innerFacetYEnd - facetInfo.innerFacetYStart + 1;
    const uint32_t zRange = facetInfo.innerFacetZEnd - facetInfo.innerFacetZStart + 1;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(params.kernelPackSendBuf, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 0, sizeof(params.subDomainNew), &params.subDomainNew));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 1, sizeof(facetInfo.sendBuffer), &facetInfo.sendBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 2, sizeof(params.nPointsX), &params.nPointsX));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 3, sizeof(params.nPointsY), &params.nPointsY));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 4, sizeof(params.nPointsZ), &params.nPointsZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 5, sizeof(facetInfo.innerFacetXStart), &facetInfo.innerFacetXStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 6, sizeof(facetInfo.innerFacetXEnd), &facetInfo.innerFacetXEnd));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 7, sizeof(facetInfo.innerFacetYStart), &facetInfo.innerFacetYStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 8, sizeof(facetInfo.innerFacetYEnd), &facetInfo.innerFacetYEnd));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 9, sizeof(facetInfo.innerFacetZStart), &facetInfo.innerFacetZStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelPackSendBuf, 10, sizeof(facetInfo.innerFacetZEnd), &facetInfo.innerFacetZEnd));

    const ze_group_count_t dispatch = {xRange, yRange, zRange};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(params.cmdlist, params.kernelPackSendBuf, &dispatch, nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult initPackSendBuffers(ParamsTy &params) {
    for (int i = 0; i < int(FacetTy::LAST); i++) {
        packSendBuffer(FacetTy(i), params);
    }

    return TestResult::Success;
}

static TestResult sendFacet(const FacetTy facet, ParamsTy &params) {
    FacetInfoTy &facetInfo = params.facetInfo[int(facet)];

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(params.cmdlist, facetInfo.recvBufferNeighbor, facetInfo.sendBuffer, facetInfo.bufferLen * sizeof(float), nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult initHaloExchange(ParamsTy &params) {
    for (int i = 0; i < int(FacetTy::LAST); i++) {
        sendFacet(FacetTy(i), params);
    }

    return TestResult::Success;
}

static TestResult unpackRecvBufferHelper(FacetTy facet, ParamsTy &params) {
    const FacetInfoTy &facetInfo = params.facetInfo[int(facet)];

    // Copying to the outer shell facets
    const uint32_t xRange = facetInfo.outerFacetXEnd - facetInfo.outerFacetXStart + 1;
    const uint32_t yRange = facetInfo.outerFacetYEnd - facetInfo.outerFacetYStart + 1;
    const uint32_t zRange = facetInfo.outerFacetZEnd - facetInfo.outerFacetZStart + 1;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(params.kernelUnpackRecvBuf, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 0, sizeof(params.subDomainNew), &params.subDomainNew));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 1, sizeof(facetInfo.recvBuffer), &facetInfo.recvBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 2, sizeof(params.nPointsX), &params.nPointsX));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 3, sizeof(params.nPointsY), &params.nPointsY));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 4, sizeof(params.nPointsZ), &params.nPointsZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 5, sizeof(facetInfo.outerFacetXStart), &facetInfo.outerFacetXStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 6, sizeof(facetInfo.outerFacetXEnd), &facetInfo.outerFacetXEnd));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 7, sizeof(facetInfo.outerFacetYStart), &facetInfo.outerFacetYStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 8, sizeof(facetInfo.outerFacetYEnd), &facetInfo.outerFacetYEnd));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 9, sizeof(facetInfo.outerFacetZStart), &facetInfo.outerFacetZStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUnpackRecvBuf, 10, sizeof(facetInfo.outerFacetZEnd), &facetInfo.outerFacetZEnd));

    const ze_group_count_t dispatch = {xRange, yRange, zRange};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(params.cmdlist, params.kernelUnpackRecvBuf, &dispatch, nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult unpackRecvBuffers(ParamsTy &params) {
    for (int i = 0; i < int(FacetTy::LAST); i++) {
        unpackRecvBufferHelper(FacetTy(i), params);
    }

    return TestResult::Success;
}

static TestResult updateFacet(const FacetTy facet, ParamsTy &params) {
    FacetInfoTy &facetInfo = params.facetInfo[int(facet)];

    const float weight = params.thermalConductivity * params.deltaTime / (params.deltaSpace * params.deltaSpace);

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(params.kernelUpdateFacet, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 0, sizeof(params.subDomainOld), &params.subDomainOld));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 1, sizeof(params.subDomainNew), &params.subDomainNew));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 2, sizeof(params.nPointsX), &params.nPointsX));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 3, sizeof(params.nPointsY), &params.nPointsY));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 4, sizeof(params.nPointsZ), &params.nPointsZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 5, sizeof(facetInfo.innerFacetXDedupStart), &facetInfo.innerFacetXDedupStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 6, sizeof(facetInfo.innerFacetYDedupStart), &facetInfo.innerFacetYDedupStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 7, sizeof(facetInfo.innerFacetZDedupStart), &facetInfo.innerFacetZDedupStart));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateFacet, 8, sizeof(weight), &weight));

    const uint32_t xRange = facetInfo.innerFacetXDedupEnd - facetInfo.innerFacetXDedupStart + 1;
    const uint32_t yRange = facetInfo.innerFacetYDedupEnd - facetInfo.innerFacetYDedupStart + 1;
    const uint32_t zRange = facetInfo.innerFacetZDedupEnd - facetInfo.innerFacetZDedupStart + 1;

    const ze_group_count_t dispatch = {xRange, yRange, zRange};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(params.cmdlist, params.kernelUpdateFacet, &dispatch, nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult updateInterior(ParamsTy &params) {
    const float weight = params.thermalConductivity * params.deltaTime / (params.deltaSpace * params.deltaSpace);

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(params.kernelUpdateInterior, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateInterior, 0, sizeof(params.subDomainOld), &params.subDomainOld));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateInterior, 1, sizeof(params.subDomainNew), &params.subDomainNew));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateInterior, 2, sizeof(params.nPointsX), &params.nPointsX));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateInterior, 3, sizeof(params.nPointsY), &params.nPointsY));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateInterior, 4, sizeof(params.nPointsZ), &params.nPointsZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(params.kernelUpdateInterior, 5, sizeof(weight), &weight));

    const uint32_t xRange = params.nPointsX - 2;
    const uint32_t yRange = params.nPointsY - 2;
    const uint32_t zRange = params.nPointsZ - 2;

    const ze_group_count_t dispatch = {xRange, yRange, zRange};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(params.cmdlist, params.kernelUpdateInterior, &dispatch, nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult cleanupParams(ParamsTy &params) {
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(params.cmdlist));

    for (uint8_t i = 0; i < int(FacetTy::LAST); i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(params.levelzero.context, params.sendBuffers[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(params.levelzero.context, params.recvBuffers[i]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(params.levelzero.context, params.subDomainOld));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(params.levelzero.context, params.subDomainNew));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(params.kernelInitTemp));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(params.kernelPackSendBuf));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(params.kernelUnpackRecvBuf));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(params.kernelUpdateFacet));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(params.kernelUpdateInterior));

    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(params.module));

    for (auto event : params.barrierEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCloseIpcHandle(params.barrierEvPool));
    ASSERT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(params.levelzero.context, params.initBuffer));

#ifndef USE_PIDFD
    EXPECT_EQ(0, close(params.socketWorker));
#endif

    return TestResult::Success;
}

TestResult run(const Heat3DArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {
    LevelZero levelzero{};

    uint32_t tilesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetSubDevices(levelzero.device, &tilesCount, nullptr));
    if (tilesCount > 1) {
        io.writeToConsole("This workload should run on a single tile\n");
        return TestResult::DeviceNotCapable;
    }

    ParamsTy params(levelzero);

    initParams(params, arguments);

    Timer timer{};

    for (auto i = 0u; i < arguments.iterations; i++) {
        initTemperature(params);
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(params.cmdlist, nullptr, 0, nullptr));

        initPackSendBuffers(params);
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(params.cmdlist, nullptr, 0, nullptr));

        initHaloExchange(params);
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(params.cmdlist, UINT64_MAX));

        // Wait for halo buffers to arrive
        ipcBarrierWorker(params);

        unpackRecvBuffers(params);
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(params.cmdlist, UINT64_MAX));

        std::swap(params.subDomainNew, params.subDomainOld);

        synchronization.synchronize(io);

        timer.measureStart();

        for (size_t t = 0; t < params.nTimesteps; t++) {
            for (int f = 0; f < int(FacetTy::LAST); f++) {
                updateFacet(FacetTy(f), params);
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(params.cmdlist, nullptr, 0, nullptr));
                packSendBuffer(FacetTy(f), params);
            }
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(params.cmdlist, nullptr, 0, nullptr));

            for (int f = 0; f < int(FacetTy::LAST); f++) {
                sendFacet(FacetTy(f), params);
            }

            updateInterior(params);
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(params.cmdlist, UINT64_MAX));

            ipcBarrierWorker(params);

            unpackRecvBuffers(params);

            std::swap(params.subDomainNew, params.subDomainOld);
        }

        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Unknown, MeasurementType::Unknown);
    }

    cleanupParams(params);

    return TestResult::Success;
}

int main(int argc, char **argv) {
    Heat3D workload;
    Heat3D::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}

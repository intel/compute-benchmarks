/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void init_temperature(global float* subDomain,
                             uint nPointsX, uint nPointsY, uint nPointsZ,
                             uint subDomainCoordX, uint subDomainCoordY, uint subDomainCoordZ,
                             float deltaSpace)
{
    // We do the loop from 1st to nPoints_th
    const size_t xId = get_global_id(0) + 1;
    const size_t yId = get_global_id(1) + 1;
    const size_t zId = get_global_id(2) + 1;

    // Put a high temperature ball in the center of the simulation domain
    const float ambient = 20.0f;
    const float hiTemp = 1000.0f;

    // Radius of the ball will be 1/10 of the side length
    const float ballR = 1.0f / 10.0f;
    const float ballR2 = ballR * ballR;

    // Go through all the non-ghost points in our sub-domain
    // If the distance between the current point and the center of the domain
    // is greater than r^2, then it is not part of the ball
    const float x = deltaSpace * (convert_float(subDomainCoordX * nPointsX + xId - 1) + 0.5f);
    const float diffX2 = pown(1.0f / 2.0f - x, 2);
    const float y = deltaSpace * (convert_float(subDomainCoordY * nPointsY + yId - 1) + 0.5f);
    const float diffY2 = pown(1.0f / 2.0f - y, 2);
    const float z = deltaSpace * (convert_float(subDomainCoordZ * nPointsZ + zId - 1) + 0.5f);
    const float diffZ2 = pown(1.0f / 2.0f - z, 2);

    const size_t flatId = xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId;
    if (diffX2 + diffY2 + diffZ2 > ballR2) {
        subDomain[flatId] = ambient;
    } else {
        subDomain[flatId] = hiTemp;
    }
}

kernel void pack_send_buffer(global float* subDomain, global float* sendBuffer,
                             uint nPointsX, uint nPointsY, uint nPointsZ,
                             uint innerFacetXStart, uint innerFacetXEnd,
                             uint innerFacetYStart, uint innerFacetYEnd,
                             uint innerFacetZStart, uint innerFacetZEnd)
{
    const size_t xId = get_global_id(0);
    const size_t yId = get_global_id(1);
    const size_t zId = get_global_id(2);

    const uint xRange = innerFacetXEnd - innerFacetXStart + 1;
    const uint yRange = innerFacetYEnd - innerFacetYStart + 1;
    const uint zRange = innerFacetZEnd - innerFacetZStart + 1;

    sendBuffer[xId * yRange * zRange + yId * zRange + zId] = subDomain[(xId + innerFacetXStart) * (nPointsY + 2) + (nPointsZ + 2) + (yId + innerFacetYStart) * (nPointsZ + 2) + (zId + innerFacetZStart)];
}

kernel void unpack_recv_buffer(global float* subDomain, global float* recvBuffer,
                               uint nPointsX, uint nPointsY, uint nPointsZ,
                               uint outerFacetXStart, uint outerFacetXEnd,
                               uint outerFacetYStart, uint outerFacetYEnd,
                               uint outerFacetZStart, uint outerFacetZEnd)
{
    const size_t xId = get_global_id(0);
    const size_t yId = get_global_id(1);
    const size_t zId = get_global_id(2);

    const size_t xRange = outerFacetXEnd - outerFacetXStart + 1;
    const size_t yRange = outerFacetYEnd - outerFacetYStart + 1;
    const size_t zRange = outerFacetZEnd - outerFacetZStart + 1;

    subDomain[(xId + outerFacetXStart) * (nPointsY + 2) * (nPointsZ + 2) + (yId + outerFacetYStart) * (nPointsZ + 2) + (zId + outerFacetZStart)] = recvBuffer[xId * yRange * zRange + yId * zRange + zId];
}

kernel void update_facet(global float* subDomainOld, global float* subDomainNew,
                         uint nPointsX, uint nPointsY, uint nPointsZ,
                         uint innerFacetXDedupStart, uint innerFacetYDedupStart, uint innerFacetZDedupStart,
                         const float weight)
{
    const size_t xId = get_global_id(0) + innerFacetXDedupStart;
    const size_t yId = get_global_id(1) + innerFacetYDedupStart;
    const size_t zId = get_global_id(2) + innerFacetZDedupStart;

    // Updating the inner shell facets, use deduplicated inner facet bounds
    // Couldn't pack the data into the send buffers b/c we are using deduplicated indices
    const float deltaTemp = weight * ( subDomainOld[(xId - 1) * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId]
                                     + subDomainOld[(xId + 1) * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + (yId - 1) * (nPointsZ + 2) + zId]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + (yId + 1) * (nPointsZ + 2) + zId]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + (zId - 1)]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + (zId + 1)]
                                     - 6.0f * subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId]);

    subDomainNew[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId] = subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId] + deltaTemp;
}

kernel void update_interior(global float* subDomainOld, global float* subDomainNew,
                            uint nPointsX, uint nPointsY, uint nPointsZ,
                            const float weight)
{
    const size_t xId = get_global_id(0) + 2;
    const size_t yId = get_global_id(1) + 2;
    const size_t zId = get_global_id(2) + 2;

    const float deltaTemp = weight * ( subDomainOld[(xId - 1) * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId]
                                     + subDomainOld[(xId + 1) * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + (yId - 1) * (nPointsZ + 2) + zId]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + (yId + 1) * (nPointsZ + 2) + zId]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + (zId - 1)]
                                     + subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + (zId + 1)]
                                     - 6.0f * subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId]);

    subDomainNew[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId] = subDomainOld[xId * (nPointsY + 2) * (nPointsZ + 2) + yId * (nPointsZ + 2) + zId] + deltaTemp;
}

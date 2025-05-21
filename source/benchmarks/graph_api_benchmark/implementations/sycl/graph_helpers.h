/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/graph_structure.h"
#include "framework/utility/error.h"

#include "../../definitions/graph_import.h"

#include <sycl/ext/oneapi/experimental/graph.hpp>
#include <sycl/sycl.hpp>

namespace graph_helpers {
namespace sycl_ext = sycl::ext::oneapi::experimental;

template <size_t NumNodes>
void buildGraphFromNodes(const std::array<graph_import::node, NumNodes> &NodeList, sycl_ext::command_graph<sycl_ext::graph_state::modifiable> &Graph) {
    // Storage for all the SYCL nodes
    std::vector<sycl_ext::node> SyclNodes;
    SyclNodes.reserve(NumNodes);
    // Dummy ptr parameters to trigger multiple set arg calls
    int *PtrArg1 = nullptr;
    int *PtrArg2 = nullptr;
    int *PtrArg3 = nullptr;
    int *PtrArg4 = nullptr;
    int *PtrArg5 = nullptr;
    int *PtrSrc = nullptr;
    [[maybe_unused]] auto DummyKernel = [=]() {
        *PtrArg1 = *PtrSrc;
        *PtrArg2 = *PtrSrc;
        *PtrArg3 = *PtrSrc;
        *PtrArg4 = *PtrSrc;
        *PtrArg5 = *PtrSrc;
    };
    // Create nodes in order using the explicit API so that we can later use successor indices to match SYCL node dependencies
    for (size_t i = 0; i < NumNodes; i++) {
        const graph_import::node &N = NodeList[i];
        switch (N.Type) {
        case graph_import::NodeType::Kernel: {
            auto KernelNode = Graph.add([&](sycl::handler &CGH) {
                CGH.single_task(DummyKernel);
            });
            SyclNodes.push_back(KernelNode);
            break;
        }
        case graph_import::NodeType::Barrier: {
            // Barriers are equivalent to empty nodes in the explicit API
            auto BarrierNode = Graph.add();
            SyclNodes.push_back(BarrierNode);
            break;
        }
        default:
            FATAL_ERROR("Unknown node type for graph import.");
        }
    }
    // Make edges between all nodes based on provided successors
    for (size_t i = 0; i < NumNodes; i++) {

        const graph_import::node &N = NodeList[i];
        for (size_t Index : N.Successors) {
            Graph.make_edge(SyclNodes[i], SyclNodes[Index]);
        }
    }
}
sycl_ext::command_graph<sycl_ext::graph_state::modifiable> constructGraph(GraphStructure Structure, const sycl::context &Context, const sycl::device &Device) {
    sycl_ext::command_graph<sycl_ext::graph_state::modifiable> Graph{Context, Device};
    switch (Structure) {
    case GraphStructure::Gromacs: {
        buildGraphFromNodes(graph_import::gromacs, Graph);
        break;
    }
    case GraphStructure::LLama: {
        buildGraphFromNodes(graph_import::llama, Graph);
        break;
    }
    default:
        FATAL_ERROR("Invalid graph structure specified.");
    }
    return Graph;
}
} // namespace graph_helpers

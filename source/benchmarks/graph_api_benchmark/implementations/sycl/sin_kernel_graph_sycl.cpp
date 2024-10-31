#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/sin_kernel_graph.h"
#include "sin_common.h"

#include <gtest/gtest.h>
#include <math.h>
#include <sycl/sycl.hpp>

static TestResult run(const SinKernelGraphArguments &arguments, Statistics &statistics) {

    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    std::size_t op_multiplier = arguments.multiplier;
    bool run_with_graph = arguments.run_with_graph;

    Timer timer;

    sycl::queue *q = new sycl::queue{sycl::gpu_selector_v, {sycl::property::queue::in_order()}};

    if (q == NULL) {
        std::cout << "failed to create sycl queue." << std::endl;
        return TestResult::Error;
    }

    g_devMemMgr.init(q);

    sycl::property_list prop_list = {sycl::property::queue::in_order()};

    sycl::queue second_q{sycl::gpu_selector_v, prop_list};

    // shape of model input (ABCD in general)
    size_t a = 2, b = 4, c = 8, d = 1024;

    // prepare host data for model input
    float *inp_h = sycl::malloc_host<float>(a * b * c * d, *q);
    for (size_t i = 0; i < a * b * c * d; ++i) {
        inp_h[i] = random_float();
    }

    // warm up
    float *gloden_h = new float[1];
    for (int i = 0; i < 20; ++i) {
        // Host2Device for model input
        Tensor4D inp(a, b, c, d);
        q->memcpy(inp.data, inp_h, inp.count() * sizeof(float));

        // we may add more special models such as run_model_dlrm for the demo purpose
        Tensor4D outp = run_model(inp, q, op_multiplier);

        delete[] gloden_h;
        gloden_h = new float[outp.count()];
        q->memcpy(gloden_h, outp.data, outp.count() * sizeof(float)).wait();
        g_devMemMgr.free(inp.data);
        g_devMemMgr.free(outp.data);
    }

    sycl::ext::oneapi::experimental::command_graph g{q->get_context(), q->get_device()};

    Tensor4D graph_inp(a, b, c, d);

    g.begin_recording({*q});
    Tensor4D graph_outp = run_model(graph_inp, q, op_multiplier);
    g.end_recording();

    auto execGraph = g.finalize();

    float *output_h = new float[graph_outp.count()];

    sycl::property_list exec_q_prop_list = {sycl::property::queue::in_order()};
    sycl::queue exec_q{sycl::gpu_selector_v, exec_q_prop_list};

    auto check_result = [&] {
        bool ret = true;
        for (int ai = 0; ai < graph_outp.A; ++ai) {
            for (int bi = 0; bi < graph_outp.B; ++bi) {
                for (int ci = 0; ci < graph_outp.C; ++ci) {
                    for (int di = 0; di < graph_outp.D; ++di) {
                        int index = di + ci * graph_outp.D + bi * graph_outp.C * graph_outp.D + ai * graph_outp.B * graph_outp.C * graph_outp.D;
                        if (fabs(output_h[index] - gloden_h[index]) > 0.0001f) {
                            ret = false;
                            std::cout << "at (" << ai << ", " << bi << ", " << ci << ", " << di << "), expect " << gloden_h[index] << ", but got " << output_h[index] << std::endl;
                        }
                    }
                }
            }
        }
        return ret;
    };

    // do the check
    exec_q.memset(graph_outp.data, 0, graph_outp.count() * sizeof(float)).wait();
    exec_q.memcpy(graph_inp.data, inp_h, graph_inp.count() * sizeof(float));
    exec_q.ext_oneapi_graph(execGraph);

    exec_q.memcpy(output_h, graph_outp.data, graph_outp.count() * sizeof(float)).wait();

    if (!check_result()) {
        std::cout << "Check FAILED" << std::endl;
        return TestResult::Error;
    }

    exec_q.wait(); // just for sure no pending GPU workloads

    int repeat = 100;
    for (std::size_t i = 0; i < arguments.iterations; ++i) {
        // run without sycl graph
        if (!run_with_graph) {
            Tensor4D bm_inp(a, b, c, d);
            Tensor4D bm_outp(1, 1, 1, 1);

            timer.measureStart();

            for (int i = 0; i < repeat; ++i) {
                g_devMemMgr.free(bm_outp.data);
                exec_q.memcpy(bm_inp.data, inp_h, bm_inp.count() * sizeof(float));
                bm_outp = run_model(bm_inp, &exec_q, op_multiplier);
            }
            exec_q.wait();
            timer.measureEnd();

            g_devMemMgr.free(bm_inp.data);
            g_devMemMgr.free(bm_outp.data);
        } else {
            // run with sycl graph
            std::vector<sycl::event> events;
            timer.measureStart();
            for (int i = 0; i < repeat; ++i) {
                exec_q.memcpy(graph_inp.data, inp_h, graph_inp.count() * sizeof(float));
                auto e = exec_q.ext_oneapi_graph(execGraph);
                events.push_back(e);
            }
            exec_q.wait();
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    g_devMemMgr.free(graph_inp.data);
    g_devMemMgr.free(graph_outp.data);
    delete[] output_h;
    delete[] gloden_h;
    sycl::free(inp_h, *q);

    // make sure all the GPU tasks are done when cleanup
    q->wait();
    g_devMemMgr.deinit();

    /*****/
    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SinKernelGraph> registerTestCase(run, Api::SYCL);
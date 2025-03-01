set(MODEL_LOADER_FRAMEWORK_SRC
    ${MODEL_LOADER_FRAMEWORK_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/model_loader.cc
    )

set(MSLITE_EXTEND_RUNTIME_SRC ${MSLITE_EXTEND_RUNTIME_SRC}
    ${MODEL_LOADER_FRAMEWORK_SRC})
if(MSLITE_ENABLE_CLOUD_FUSION_INFERENCE)
    set(ENABLE_CPU on)
    set(LOAD_PLUGIN_STATIC on)
    add_compile_definitions(USE_GLOG)
    string(REPLACE "-fno-rtti" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    string(REPLACE "-fno-rtti" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    add_compile_definitions(ENABLE_CLOUD_FUSION_INFERENCE)
    remove_definitions(-DBUILD_LITE_INFERENCE)
    set(MINDIR_MODEL_SRC
        ${MINDIR_MODEL_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/mindir_model.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/mindir_model_util.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/mindir_model_convertor.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/mindir_model_loader.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/kernel_mod_util.cc
        )

    set(MINDIR_KERNEL_SRC
        ${MINDIR_KERNEL_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/inner_kernel.cc)

    set(MSLITE_KERNEL_PLUGIN
        ${MSLITE_KERNEL_PLUGIN}
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/ascend/plugin/ascend_kernel_plugin.cc)

    set(MSLITE_EXTEND_RUNTIME_SRC ${MSLITE_EXTEND_RUNTIME_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/mindir_loader/mindir_model/mindir_model_util.cc
        ${MSLITE_KERNEL_PLUGIN}
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/file_utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/graph_util.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../litert/pack_weight_manager.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_mem_allocator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_mem_manager.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/subgraph_kernel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/numa_adapter.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/cpu/less_test_kernel_mod.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/cpu/transpose_kernel_mod.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/infer_session.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/session/single_op_session.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/infer_device_address.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/kernel_build_utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/kernel_graph_utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/tensor_utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/runtime_utils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/serialization.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/session/delegate_session.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/session/factory.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/delegate/factory.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/delegate/plugin/tensorrt_executor_plugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/delegate/tensorrt/distribution/distribution_base.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/delegate_graph_executor.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/convert/runtime_convert.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/session/optimizer/tensorrt_optimizer.cc
        )
    include_directories("${CCSRC_DIR}/ps/core")
    file(GLOB_RECURSE COMM_PROTO_IN RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CCSRC_DIR}/ps/core/protos/*.proto")
    ms_protobuf_generate(COMM_PROTO_SRCS COMM_PROTO_HDRS ${COMM_PROTO_IN})
    list(APPEND MSLITE_PROTO_SRC ${COMM_PROTO_SRCS})

    if(NOT ENABLE_SECURITY)
        include_directories("${CCSRC_DIR}/profiler/device/ascend")
        file(GLOB_RECURSE PROFILER_PROTO_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            "${CCSRC_DIR}/plugin/device/ascend/hal/profiler/memory_profiling.proto")
        ms_protobuf_generate(PROFILER_MEM_PROTO_SRC PROFILER_MEM_PROTO_HDRS ${PROFILER_PROTO_LIST})
        list(APPEND MSLITE_PROTO_SRC ${PROFILER_MEM_PROTO_SRC})
    endif()

    add_library(mindspore-lite-proto OBJECT ${MSLITE_PROTO_SRC})

    set(ANF_ALG_SRC ${ANF_ALG_SRC}
        ${CCSRC_DIR}/utils/anfalgo.cc
        ${CCSRC_DIR}/utils/utils.cc
        ${CCSRC_DIR}/utils/parallel_context.cc
        ${CCSRC_DIR}/utils/convert_utils.cc)
    add_library(mindspore-infer-anfalgo OBJECT ${ANF_ALG_SRC})

    set(KERNEL_GRAPH_SRC ${KERNEL_GRAPH_SRC}
        ${CCSRC_DIR}/backend/common/session/kernel_graph.cc
        ${CCSRC_DIR}/backend/common/session/anf_runtime_algorithm.cc
        ${CCSRC_DIR}/backend/common/somas/somas.cc
        ${CCSRC_DIR}/backend/common/somas/somas_tensor.cc
        ${CCSRC_DIR}/backend/common/somas/somas_solver_pre.cc
        ${CCSRC_DIR}/backend/common/somas/somas_solver_core.cc
        ${CCSRC_DIR}/backend/common/somas/somas_solver_alg.cc
        ${CCSRC_DIR}/backend/graph_compiler/graph_partition.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/segment_runner.cc
        ${CCSRC_DIR}/runtime/device/ms_device_shape_transfer.cc
        ${CCSRC_DIR}/runtime/device/kernel_info.cc
        ${CCSRC_DIR}/runtime/device/convert_tensor_utils.cc
        ${CCSRC_DIR}/runtime/device/kernel_runtime_manager.cc
        ${CCSRC_DIR}/runtime/device/bucket.cc
        ${CCSRC_DIR}/runtime/device/kernel_runtime.cc
        ${CCSRC_DIR}/runtime/device/memory_scheduler.cc
        ${CCSRC_DIR}/runtime/device/memory_offload_strategy.cc
        ${CCSRC_DIR}/runtime/device/memory_manager.cc
        ${CCSRC_DIR}/runtime/device/auto_mem_offload.cc
        ${CCSRC_DIR}/runtime/device/common_somas_allocator.cc
        ${CCSRC_DIR}/runtime/pynative/op_executor.cc
        ${CCSRC_DIR}/runtime/pynative/op_runtime_info.cc
        ${CCSRC_DIR}/runtime/hardware/device_type.cc
        ${CCSRC_DIR}/kernel/kernel_build_info.cc
        ${CCSRC_DIR}/kernel/common_utils.cc
        ${CCSRC_DIR}/kernel/kernel.cc
        ${CCSRC_DIR}/kernel/kash/kernel_pack.cc
        ${CCSRC_DIR}/kernel/oplib/oplib.cc
        ${CCSRC_DIR}/common/debug/anf_dump_utils.cc
        ${CCSRC_DIR}/common/debug/anf_ir_dump.cc
        ${CCSRC_DIR}/common/debug/common.cc
        ${CCSRC_DIR}/common/debug/env_config_parser.cc
        ${CCSRC_DIR}/common/thread_pool.cc
        ${CCSRC_DIR}/utils/scoped_long_running.cc
        ${CCSRC_DIR}/utils/cse.cc
        ${CCSRC_DIR}/utils/comm_manager.cc
        ${CCSRC_DIR}/utils/signal_util.cc
        ${CORE_DIR}/utils/status.cc
        )
    add_library(mindspore-kernel-graph OBJECT ${KERNEL_GRAPH_SRC})
    add_dependencies(mindspore-kernel-graph mindspore-lite-proto)

    add_library(mindspore-extendrt SHARED ${MSLITE_EXTEND_RUNTIME_SRC})
    add_dependencies(mindspore-extendrt fbs_inner_src)
    add_dependencies(mindspore-extendrt mindspore-lite-proto)
    add_dependencies(mindspore-extendrt mindspore-infer-anfalgo)
    add_dependencies(mindspore-extendrt mindspore-kernel-graph)
    add_subdirectory(cxx_api)
    add_subdirectory(delegate/graph_executor/litert)

    add_subdirectory(${CCSRC_DIR}/backend/common/pass common_pass)
    add_subdirectory(${CCSRC_DIR}/backend/common/optimizer mindspore_ccsrc_backend_cmmon_optimizer)
    target_link_libraries(mindspore-extendrt mindspore_infer_shared_lib_obj)
    target_link_libraries(mindspore-extendrt mindspore-infer-anfalgo
        mindspore-kernel-graph _mindspore_backend_common_optimizer_obj
        _mindspore_backend_common_pass_obj)
    target_link_libraries(mindspore-extendrt mindspore_core mindspore::protobuf)
    add_dependencies(mindspore-extendrt msplugin-ge-litert)
    target_link_libraries(mindspore-extendrt msplugin-ge-litert)

    if(NOT WIN32)
        target_link_libraries(mindspore-extendrt dl)
    endif()

    if(MSLITE_ENABLE_ACL)
        include_directories(${TOP_DIR}/graphengine/inc/external)
        add_subdirectory(kernel/ascend)
        if(MSLITE_ENABLE_HELPER)
            add_subdirectory(delegate/ascend_ge)
        endif()
    endif()

    if(SUPPORT_CUDA)
        set(CUDA_PATH $ENV{CUDA_HOME})
        include_directories(${CCSRC_DIR}/plugin/device/gpu/kernel)
        set(ENABLE_GPU on)
        add_definitions(-DENABLE_GPU)
        set(CUDA_VERSION 11.1)
        include_directories(${CUDA_PATH})
        include_directories(${CUDA_PATH}/include)
        find_package(CUDA)
        add_subdirectory(kernel/cuda)
        list(APPEND CUDA_NVCC_FLAGS -arch=sm_53 --expt-relaxed-constexpr)
        add_subdirectory(${CCSRC_DIR}/plugin/device/gpu/kernel/cuda_impl/cuda_ops cuda_ops)
        target_link_libraries(mindspore-extendrt cuda_lite_kernel_mid cuda_ops)
    endif()

    if(SUPPORT_TENSORRT)
        add_definitions(-DSUPPORT_TENSORRT)
        add_subdirectory(delegate/tensorrt)
    endif()

    if(MSLITE_ENABLE_CONVERTER)
        add_subdirectory(convert)
    endif()

    set(TEST_CLOUD_INFER on)

    if(TEST_CLOUD_INFER)
        set(CLOUD_INFER_TEST_SRC ${CLOUD_INFER_TEST_SRC}
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/cloud_infer_test.cc
            ${LITE_DIR}/tools/common/flag_parser.cc
            )

        add_executable(cloud_infer_test ${CLOUD_INFER_TEST_SRC})
        add_dependencies(cloud_infer_test mindspore-extendrt)
        target_link_libraries(cloud_infer_test mindspore-extendrt)
    endif()
else()
    add_library(mindspore-extendrt OBJECT ${MSLITE_EXTEND_RUNTIME_SRC})
    add_dependencies(mindspore-extendrt fbs_inner_src)
endif()

if(MSLITE_ENABLE_HELPER)
    target_link_libraries(mindspore-extendrt ascend_ge_plugin)
endif()
set_target_properties(mindspore-extendrt PROPERTIES OUTPUT_NAME "mindspore-lite")



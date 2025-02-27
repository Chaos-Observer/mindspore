if(DEFINED ENV{ASCEND_CUSTOM_PATH})
    set(TOOLCHAIN_PATH $ENV{ASCEND_CUSTOM_PATH}/latest/toolkit/toolchain)
elseif(EXISTS /usr/local/Ascend/latest/toolkit/toolchain)
    set(TOOLCHAIN_PATH /usr/local/Ascend/latest/toolkit/toolchain)
elseif(EXISTS /usr/local/Ascend/ascend-toolkit/latest/toolkit/toolchain)
    set(TOOLCHAIN_PATH /usr/local/Ascend/ascend-toolkit/latest/toolkit/toolchain)
else()
    set(TOOLCHAIN_PATH /usr/local/Ascend/toolkit/toolchain)
endif()
set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/hcc/bin/aarch64-target-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/hcc/bin/aarch64-target-linux-gnu-g++)

if(EXISTS ${CMAKE_C_COMPILER} AND EXISTS ${CMAKE_CXX_COMPILER})
    include(${CMAKE_SOURCE_DIR}/cmake/dependency_securec_arm.cmake)
    include(${CMAKE_SOURCE_DIR}/cmake/external_libs/protobuf_arm.cmake)

    set(AICPU_PROTO_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_op_proto/aicpu_tensor.proto
    )

    ms_protobuf_generate(PROTO_SRCS PROTO_HDRS ${AICPU_PROTO_SRC})

    set(AICPU_SRC
        ${PROTO_SRCS}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_base.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_log.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_sharder/aicpu_async_event.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_sharder/aicpu_context.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_sharder/aicpu_pulse.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_sharder/aicpu_sharder.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/random_choice_with_mask_kernels.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/gather_grad_kernels.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/environ/aicpu_environ_manager.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/environ/environ_create.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/environ/environ_set.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/environ/environ_get.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/environ/environ_destroy_all.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/replay_buffer/fifo_replay_buffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/replay_buffer/priority_replay_buffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/replay_buffer/priority_replay_buffer_kernels.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/concat_offset_kernel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/random_shuffle_kernel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/range_kernel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/replay_buffer/reservoir_replay_buffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/replay_buffer/reservoir_replay_buffer_kernels.cc
    )

    add_library(mindspore_aicpu_kernels SHARED
        ${AICPU_SRC}
    )

    target_compile_options(mindspore_aicpu_kernels PRIVATE
        -march=armv8-a
        -O2
        -fvisibility-inlines-hidden
        -fvisibility=hidden
        -fno-strict-aliasing
        -fno-common
    )

    target_link_libraries(mindspore_aicpu_kernels PRIVATE
        -ldl
        -shared
        PUBLIC
        ${SECUREC_ARM_LIBRARY}
        -Wl,--whole-archive
        -Wl,--no-whole-archive
        -Wl,-Bsymbolic
        -rdynamic
        mindspore::protobuf_arm
        -pthread
    )

    set(INSTALL_LIBRARY_DIR lib)
    install(TARGETS mindspore_aicpu_kernels OPTIONAL
        EXPORT mindspore_aicpu_kernels-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    )

endif()

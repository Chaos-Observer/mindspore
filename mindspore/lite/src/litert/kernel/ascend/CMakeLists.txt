include_directories(${TOP_DIR}/graphengine/inc/external)

find_library(ge_graph libgraph.so ${ASCEND_CANN_RUNTIME_PATH} ${ASCEND_TOOLKIT_RUNTIME_PATH})

aux_source_directory(src ACL_SRC)
add_library(ascend_kernel_mid OBJECT ${ACL_SRC})

add_dependencies(ascend_kernel_mid fbs_inner_src)
if("${MSLITE_REGISTRY_DEVICE}" STREQUAL "SD3403" AND PLATFORM_ARM64)
    find_library(ge_graph libgraph.so ${ASCEND_CANN_RUNTIME_PATH} ${ASCEND_TOOLKIT_RUNTIME_PATH})
    find_library(acl libascendcl.so ${ASCEND_CANN_RUNTIME_PATH} ${ASCEND_TOOLKIT_RUNTIME_PATH})
    find_library(acl_retr libacl_retr.so ${ASCEND_CANN_RUNTIME_PATH} ${ASCEND_TOOLKIT_RUNTIME_PATH})
    find_library(acl_cblas libacl_cblas.so ${ASCEND_CANN_RUNTIME_PATH} ${ASCEND_TOOLKIT_RUNTIME_PATH})
    find_library(acl_runtime libruntime.so ${ASCEND_CANN_RUNTIME_PATH} ${ASCEND_TOOLKIT_RUNTIME_PATH})
    target_link_libraries(ascend_kernel_mid ${ge_graph} ${acl} ${acl_retr} ${acl_cblas} ${acl_runtime})
else()
    target_link_libraries(ascend_kernel_mid ${ge_graph} ${ge_compiler}
        ${acl_retr} ${acl_cblas} ${acl_dvpp} ${acl_runtime} ${libplatform}
        ${libcompress} ${libopskernel} ${libaicore_utils} ${libaicpu_engine_common} ${acl})
endif()
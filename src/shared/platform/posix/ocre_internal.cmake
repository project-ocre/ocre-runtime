include(CheckPIESupported)
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
set (CMAKE_VERBOSE_MAKEFILE OFF)

# Build third-party libs
add_subdirectory(${OCRE_ROOT_DIR}/src/ocre/utils/c-smf)

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Set WAMR_BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "AARCH64[sub]", "ARM[sub]", "THUMB[sub]",
# "MIPS", "XTENSA", "RISCV64[sub]", "RISCV32[sub]"
if (NOT DEFINED WAMR_BUILD_TARGET)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
    set (WAMR_BUILD_TARGET "AARCH64")
    if (NOT DEFINED WAMR_BUILD_SIMD)
        set (WAMR_BUILD_SIMD 1)
    endif ()
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    set (WAMR_BUILD_TARGET "RISCV64")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (WAMR_BUILD_TARGET "X86_64")
    if (NOT DEFINED WAMR_BUILD_SIMD)
        set (WAMR_BUILD_SIMD 1)
    endif ()
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set (WAMR_BUILD_TARGET "X86_32")
    else ()
    message(SEND_ERROR "Unsupported build target platform!")
    endif ()
endif ()

# WAMR Options
set (WAMR_BUILD_PLATFORM "linux")
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_FAST_INTERP 0)
set (WAMR_BUILD_AOT 1)
set (WAMR_BUILD_JIT 0)
set (WAMR_BUILD_LIBC_BUILTIN 0)
set (WAMR_BUILD_LIBC_WASI 1)
set (WAMR_BUILD_LIB_PTHREAD 1)
set (WAMR_BUILD_REF_TYPES 1)
set (WASM_ENABLE_LOG 1)
set (WAMR_BUILD_SHARED_HEAP 1)

if (NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_POOL)
    set (WAMR_BUILD_GLOBAL_HEAP_POOL 1)
endif ()
if (NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_SIZE)
    set (WAMR_BUILD_GLOBAL_HEAP_SIZE 32767)
endif ()

set (WAMR_ROOT_DIR "wasm-micro-runtime")
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

check_pie_supported()

set(component_sources
    # Component support
    ${OCRE_ROOT_DIR}/src/ocre/component/component.c

    # Components
    ${OCRE_ROOT_DIR}/src/ocre/ocre_container_runtime/ocre_container_runtime.c
    ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/cs_main.c
    ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/cs_sm.c
    ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/cs_sm_impl.c
)

set(lib_sources
    ${OCRE_ROOT_DIR}/src/ocre/sm/sm.c
    # Platform
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_fs.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_thread.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_mutex.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_mq.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_misc.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_memory.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_timer.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_slist.c
    ${OCRE_ROOT_DIR}/src/shared/platform/posix/core_eventq.c
    # APIs
    ${OCRE_ROOT_DIR}/src/ocre/api/ocre_api.c
    ${OCRE_ROOT_DIR}/src/ocre/api/ocre_common.c
    ${OCRE_ROOT_DIR}/src/ocre/ocre_timers/ocre_timer.c
    ${OCRE_ROOT_DIR}/src/ocre/ocre_messaging/ocre_messaging.c
    # Utils
    ${OCRE_ROOT_DIR}/src/ocre/utils/strlcat.c
)

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

set(app_sources
    ${OCRE_ROOT_DIR}/src/samples-mini/posix/main.c
    ${UNCOMMON_SHARED_SOURCE}
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${lib_sources}
    ${component_sources}
)

add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
target_link_libraries(vmlib ${LLVM_AVAILABLE_LIBS} ${UV_A_LIBS} -lm -ldl -lpthread)

add_executable(app ${app_sources})
target_include_directories(app PUBLIC 
    ${OCRE_ROOT_DIR}/src 
    ${OCRE_ROOT_DIR}/src/ocre
    ${OCRE_ROOT_DIR}/src/shared/platform
)

set_target_properties(app PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(app smf vmlib -lm -lpthread)

add_compile_options(-O0 -Wno-unknown-attributes -Wall -Wextra)

add_dependencies(app generate_messages)

# Determine the ISA of the target and set appropriately
if (DEFINED CONFIG_ISA_THUMB2)
    set(TARGET_ISA THUMB)
elseif (DEFINED CONFIG_ISA_ARM)
    set(TARGET_ISA ARM)
elseif (DEFINED CONFIG_X86)
    set(TARGET_ISA X86_32)
elseif (DEFINED CONFIG_XTENSA)
    set(TARGET_ISA XTENSA)
elseif (DEFINED CONFIG_RISCV)
    set(TARGET_ISA RISCV32)
elseif (DEFINED CONFIG_ARCH_POSIX)
    set(TARGET_ISA X86_32)
else ()
    message(FATAL_ERROR "Unsupported ISA: ${CONFIG_ARCH}")
endif ()
message("Selected target ISA: ${TARGET_ISA}")

# WAMR Options
set(WAMR_BUILD_PLATFORM "zephyr")
set(WAMR_BUILD_TARGET ${TARGET_ISA})
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_REF_TYPES 1)
set(WAMR_BUILD_SHARED_HEAP 1)
set(WASM_ENABLE_LOG 1)

# DEBUG
#set(WAMR_BUILD_DEBUG_INTERP 1)

if(NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_POOL)
    set(WAMR_BUILD_GLOBAL_HEAP_POOL 1)
endif()
if(NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_SIZE)
    set(WAMR_BUILD_GLOBAL_HEAP_SIZE 32767)
endif()

enable_language (ASM)

set (WAMR_BUILD_PLATFORM "zephyr")

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../wasm-micro-runtime)

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

# include(${WAMR_ROOT_DIR}/core/iwasm/libraries/lib-socket/lib_socket_wasi.cmake)

add_library(vmlib)
target_sources(vmlib PRIVATE
    ${WAMR_RUNTIME_LIB_SOURCE})
target_link_libraries(vmlib zephyr_interface LITTLEFS)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(vmlib PUBLIC ${dirs})

get_property(defs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
target_compile_definitions(vmlib PRIVATE ${defs})

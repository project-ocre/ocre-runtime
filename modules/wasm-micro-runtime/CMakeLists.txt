cmake_minimum_required(VERSION 3.8.2)


# Determine the ISA of the target and set appropriately
if (DEFINED CONFIG_ISA_THUMB2)
set (TARGET_ISA THUMB)
elseif (DEFINED CONFIG_ISA_ARM)
set (TARGET_ISA ARM)
elseif (DEFINED CONFIG_X86)
set (TARGET_ISA X86_32)
elseif (DEFINED CONFIG_XTENSA)
set (TARGET_ISA XTENSA)
elseif (DEFINED CONFIG_RISCV)
set (TARGET_ISA RISCV32)
elseif (DEFINED CONFIG_ARCH_POSIX)
# Technically, this is not correct as the CPU architecture is not set.  This assumes POSIX is x86 32-bit
set (TARGET_ISA X86_32)  
else ()
message (FATAL_ERROR "Unsupported ISA: ${CONFIG_ARCH}")
endif ()
message("TARGET ISA: ${TARGET_ISA}")

############################
set (WAMR_BUILD_PLATFORM "zephyr")
set (WAMR_BUILD_TARGET ${TARGET_ISA})
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_FAST_INTERP 0)
set (WAMR_BUILD_AOT 0)
set (WAMR_BUILD_JIT 0)
set (WAMR_BUILD_LIBC_BUILTIN 1)
set (WAMR_BUILD_LIBC_WASI 0)
set (WAMR_BUILD_APP_FRAMEWORK 0)
set (WAMR_BUILD_APP_LIST WAMR_APP_BUILD_BASE )
set (WAMR_MPU_STACK_COUNT 5)

# Override the global heap usage
if (NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_POOL)
set (WAMR_BUILD_GLOBAL_HEAP_POOL 1)
endif ()

# Override the global heap size for small devices
if (NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_SIZE)
#set (WAMR_BUILD_GLOBAL_HEAP_SIZE 131072) # 128 KB
set (WAMR_BUILD_GLOBAL_HEAP_SIZE 32767) # 32 KB
endif ()

set (WAMR_ROOT_DIR ${ZEPHYR_WASM_MICRO_RUNTIME_MODULE_DIR})
message("**WAMR_ROOT_DIR: ${WAMR_ROOT_DIR}")

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

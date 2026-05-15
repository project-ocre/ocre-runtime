# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 1)
set (WAMR_BUILD_JIT 0)
set (WAMR_BUILD_LIBC_BUILTIN 0)
set (WAMR_BUILD_LIBC_WASI 1)
set (WAMR_BUILD_LIB_PTHREAD 0)
set (WAMR_BUILD_LIB_WASI_THREADS 1)
set (WAMR_BUILD_REF_TYPES 1)
set (WASM_ENABLE_LOG 1)
set (WAMR_BUILD_SHARED_HEAP 1)

set (WAMR_BUILD_PLATFORM "linux")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
  set (WAMR_BUILD_TARGET "AARCH64")
  set (WAMR_BUILD_SIMD 1)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set (WAMR_BUILD_TARGET "ARM")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
  set (WAMR_BUILD_TARGET "RISCV64")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set (WAMR_BUILD_TARGET "X86_64")
  set (WAMR_BUILD_SIMD 1)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "i686")
  set (WAMR_BUILD_TARGET "X86_32")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "mips")
  set (WAMR_BUILD_TARGET "MIPS")
else ()
  message(SEND_ERROR "Unsupported CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}!")
endif ()

add_subdirectory(wasm-micro-runtime)

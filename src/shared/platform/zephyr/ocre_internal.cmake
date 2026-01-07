# Zephyr-specific version defines
message("VERSION NUMBER: ${VERSION_NUMBER}")
zephyr_compile_options(-DVERSION_INFO="${VERSION_NUMBER}")
message("BUILD DATE: ${BUILD_DATE}")
zephyr_compile_options(-DVERSION_BUILD_DATE="${BUILD_DATE}")
message("BUILD MACHINE: ${BUILD_MACHINE}")
zephyr_compile_options(-DVERSION_BUILD_MACHINE="${BUILD_MACHINE}")
message("BUILD_INFO: ${BUILD_INFO}")
zephyr_compile_options(-DVERSION_BUILD_INFO="${BUILD_INFO}")

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
message("TARGET ISA: ${TARGET_ISA}")

add_compile_options(-O0 -Wno-unknown-attributes)

# WAMR Options
set(WAMR_BUILD_PLATFORM "zephyr")
set(WAMR_BUILD_TARGET ${TARGET_ISA})
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 0)
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 0)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_REF_TYPES 1)
set(WASM_ENABLE_LOG 1)

if(NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_POOL)
    set(WAMR_BUILD_GLOBAL_HEAP_POOL 1)
endif()
if(NOT DEFINED WAMR_BUILD_GLOBAL_HEAP_SIZE)
    set(WAMR_BUILD_GLOBAL_HEAP_SIZE 32767)
endif()

set(WAMR_ROOT_DIR ${OCRE_ROOT_DIR}/wasm-micro-runtime)
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

zephyr_include_directories(
    ${OCRE_ROOT_DIR}/src/
    ${OCRE_ROOT_DIR}/src/ocre
    ${OCRE_ROOT_DIR}/src/shared/platform
)

set(component_sources
    # Component support
    ${OCRE_ROOT_DIR}/src/ocre/component/component.c

    # Components
    ${OCRE_ROOT_DIR}/src/ocre/ocre_container_runtime/ocre_container_runtime.c
    ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/cs_main.c
    ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/cs_sm.c
    ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/cs_sm_impl.c
)

# Collect all sources in one list
set(lib_sources
    # Libraries
    ${OCRE_ROOT_DIR}/src/ocre/sm/sm.c
    ${OCRE_ROOT_DIR}/src/ocre/shell/ocre_shell.c
    # Platform
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_fs.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_thread.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_mutex.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_mq.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_eventq.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_misc.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_memory.c
    ${OCRE_ROOT_DIR}/src/shared/platform/zephyr/core_timer.c

    # Ocre APIs
    ${OCRE_ROOT_DIR}/src/ocre/api/ocre_api.c
    ${OCRE_ROOT_DIR}/src/ocre/api/ocre_common.c
)

# Conditionally add sources
if(CONFIG_OCRE_TIMER)
    list(APPEND lib_sources ${OCRE_ROOT_DIR}/src/ocre/ocre_timers/ocre_timer.c)
endif()

if(CONFIG_OCRE_SENSORS)
    list(APPEND lib_sources ${OCRE_ROOT_DIR}/src/ocre/ocre_sensors/ocre_sensors.c)
endif()

if(CONFIG_RNG_SENSOR)
    list(APPEND lib_sources ${OCRE_ROOT_DIR}/src/ocre/ocre_sensors/rng_sensor.c)
endif()

if(DEFINED CONFIG_OCRE_GPIO)
    list(APPEND lib_sources ${OCRE_ROOT_DIR}/src/ocre/ocre_gpio/ocre_gpio.c)
endif()

if(CONFIG_OCRE_CONTAINER_MESSAGING)
    list(APPEND lib_sources ${OCRE_ROOT_DIR}/src/ocre/ocre_messaging/ocre_messaging.c)
endif()

# Add all sources to the app target at once
target_sources(app PRIVATE
    ${WAMR_RUNTIME_LIB_SOURCE}
    ${lib_sources}
    ${component_sources}
    ${OCRE_ROOT_DIR}/src/samples-mini/zephyr/main.c
)

add_dependencies(app generate_messages)

if(NOT "${OCRE_INPUT_FILE}" STREQUAL "")
    add_dependencies(app generate_ocre_file)
endif()

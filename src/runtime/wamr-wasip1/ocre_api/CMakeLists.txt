# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

add_library(OcreRuntimeAPI)

target_sources(OcreRuntimeAPI
    PRIVATE
    ocre_api.c
    ocre_common.c
    ocre_dispatch/ocre_dispatch.c
    ocre_timers/ocre_timer.c
    ocre_messaging/ocre_messaging.c
    ocre_print/ocre_print.c
    utils/strlcat.c
    core/core_eventq.c
    core/core_misc.c
    core/core_timer.c
    core/core_mutex.c
    core/core_memory.c
)

if (CONFIG_OCRE_GPIO)
    target_sources(OcreRuntimeAPI
        PRIVATE
        ocre_gpio/ocre_gpio.c
    )
endif()

if (CONFIG_OCRE_ADC)
    target_sources(OcreRuntimeAPI
        PRIVATE
        ocre_adc/ocre_adc.c
    )
endif()

if (CONFIG_OCRE_HTTP_SERVER)
    target_sources(OcreRuntimeAPI
        PRIVATE
        ocre_http/ocre_http.c
    )
    zephyr_linker_sources(SECTIONS ocre_http/sections-rom.ld)
endif()

if (CONFIG_OCRE_HTTP_CLIENT)
    target_sources(OcreRuntimeAPI
        PRIVATE
        ocre_http_client/ocre_http_client.c
    )
endif()

target_link_libraries(OcreRuntimeAPI
    PUBLIC
    OcrePlatform
    vmlib
)

# Ensure generated headers are ready before compiling
if(TARGET zephyr_generated_headers)
    add_dependencies(OcreRuntimeAPI zephyr_generated_headers)
endif()

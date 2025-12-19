# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

add_library(OcreRuntimeAPI)

target_sources(OcreRuntimeAPI
    PRIVATE
    ocre_api.c
    ocre_common.c
    ocre_timers/ocre_timer.c
    ocre_messaging/ocre_messaging.c
    utils/strlcat.c
    core/core_eventq.c
    core/core_misc.c
    core/core_timer.c
    core/core_mutex.c
    core/core_memory.c
    ocre_gpio/ocre_gpio.c
)

target_link_libraries(OcreRuntimeAPI
    PUBLIC
    OcrePlatform
    vmlib
)

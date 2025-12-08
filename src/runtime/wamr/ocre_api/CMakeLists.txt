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
    core/core_slist.c
    core/core_memory.c

)

# target_include_directories(OcreRuntimeAPI
#     PUBLIC
#     include
# )

target_link_libraries(OcreRuntimeAPI
    PUBLIC
    # OcreRuntime
    OcrePlatform
    vmlib
)

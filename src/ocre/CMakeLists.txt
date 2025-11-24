add_library(OcreComponent)
target_sources(OcreComponent
    PRIVATE
    sm/sm.c
    # Component support
    component/component.c
    # Components
    ocre_container_runtime/ocre_container_runtime.c
    components/container_supervisor/cs_main.c
    components/container_supervisor/cs_sm.c
    components/container_supervisor/cs_sm_impl.c
    # APIs
    api/ocre_api.c
    api/ocre_common.c
    ocre_timers/ocre_timer.c
    ocre_messaging/ocre_messaging.c
    # Utils
    utils/strlcat.c
)

if(DEFINED CONFIG_OCRE_SHELL)
    target_sources(OcreComponent
        PRIVATE
        shell/ocre_shell.c
    )
endif()

# Conditionally add sources
if(CONFIG_OCRE_TIMER)
    target_sources(OcreComponent
        PRIVATE
        ocre_timers/ocre_timer.c
    )
endif()

if(CONFIG_OCRE_SENSORS)
target_sources(OcreComponent
    PRIVATE
    ocre_sensors/ocre_sensors.c
)
endif()

if(CONFIG_RNG_SENSOR)
target_sources(OcreComponent
    PRIVATE
    ocre_sensors/rng_sensor.c
)
endif()

if(DEFINED CONFIG_OCRE_GPIO)
    target_sources(OcreComponent
        PRIVATE
        ocre_gpio/ocre_gpio.c
    )
endif()

if(CONFIG_OCRE_CONTAINER_MESSAGING)
    target_sources(OcreComponent
        PRIVATE
        ocre_messaging/ocre_messaging.c
    )
endif()

target_include_directories(OcreComponent PRIVATE .)

target_link_libraries(OcreComponent PUBLIC OcreLibrary OcreMessages vmlib)

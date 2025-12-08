cmake_minimum_required(VERSION 3.20.0)

include(../cmake/state_information.cmake)

add_custom_command(OUTPUT user_data.littlefs
    COMMAND littlefs-python create --block-size ${CONFIG_OCRE_STORAGE_PARTITION_BLOCK_SIZE} --fs-size ${CONFIG_OCRE_STORAGE_PARTITION_SIZE} var/lib user_data.littlefs
    DEPENDS ${OCRE_IMAGES}
    COMMENT "Generating user_data.littlefs"
    VERBATIM
)

add_custom_command(OUTPUT user_data.littlefs.hex
    COMMAND bin2hex.py --offset ${CONFIG_OCRE_STORAGE_PARTITION_ADDR} user_data.littlefs user_data.littlefs.hex
    DEPENDS user_data.littlefs
    COMMENT "Generating user_data.littlefs.hex"
    VERBATIM
)

add_custom_target(user_data_partition ALL
    DEPENDS user_data.littlefs.hex
    COMMENT "Generating user data partition"
    VERBATIM
)

if (CONFIG_OCRE_MERGE_HEX)
    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE "${CMAKE_CURRENT_BINARY_DIR}/user_data.littlefs.hex")
    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE "zephyr.hex")
endif()

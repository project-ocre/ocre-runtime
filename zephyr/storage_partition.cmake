# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

add_custom_command(OUTPUT user_data.bin
    COMMAND littlefs-python create
        --block-size ${CONFIG_OCRE_STORAGE_PARTITION_BLOCK_SIZE}
        --fs-size ${CONFIG_OCRE_STORAGE_PARTITION_SIZE}
        ocre-core/var/lib user_data.bin
    DEPENDS ${OCRE_SDK_PRELOADED_IMAGES}
    COMMENT "Generating user_data.bin"
    VERBATIM
)

add_custom_command(OUTPUT user_data.hex
    COMMAND bin2hex.py
        --offset ${CONFIG_OCRE_STORAGE_PARTITION_ADDR}
        user_data.bin user_data.hex
    DEPENDS user_data.bin
    COMMENT "Generating user_data.hex"
    VERBATIM
)

add_custom_target(user_data_partition ALL
    DEPENDS user_data.hex
    COMMENT "Generating user data partition"
    VERBATIM
)

if (CONFIG_OCRE_MERGE_HEX)
    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE "${CMAKE_CURRENT_BINARY_DIR}/user_data.hex")
    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE "zephyr.hex")
endif()

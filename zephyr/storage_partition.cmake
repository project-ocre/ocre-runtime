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

if (CONFIG_OCRE_MERGE_HEX)
    add_custom_command(OUTPUT user_data.hex
        COMMAND bin2hex.py
            --offset ${CONFIG_OCRE_STORAGE_PARTITION_ADDR}
            user_data.bin user_data.hex
        DEPENDS user_data.bin
        VERBATIM
    )

    add_custom_target(user_data_partition ALL
        DEPENDS user_data.hex
        COMMENT "Generating user data partition (user_data.hex)"
        VERBATIM
    )

    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE "${CMAKE_CURRENT_BINARY_DIR}/user_data.hex")
    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE "zephyr.hex")
endif()

if (CONFIG_BOARD_NATIVE_SIM)
    # get the total flash size
    math(EXPR total_flash_size "${CONFIG_OCRE_STORAGE_PARTITION_ADDR} + ${CONFIG_OCRE_STORAGE_PARTITION_SIZE}")

    # convert to decimal if necessary
    math(EXPR flash_offset "${CONFIG_OCRE_STORAGE_PARTITION_ADDR}")

    # first we fill the total flash size with 0xff
    # then we rewrite the storage_partition
    add_custom_command(OUTPUT flash.bin
        COMMAND dd if=/dev/zero bs=${total_flash_size} count=1 | tr '\\0' '\\377' | dd of=${PROJECT_BINARY_DIR}/../flash.bin
        COMMAND dd if=user_data.bin of=${PROJECT_BINARY_DIR}/../flash.bin seek=1 bs=${flash_offset} conv=notrunc
        DEPENDS user_data.bin
        VERBATIM
    )

    add_custom_target(user_data_partition_native ALL
        DEPENDS flash.bin
        COMMENT "Generating user data partition (flash.bin)"
        VERBATIM
    )
endif()

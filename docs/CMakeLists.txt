# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(OcreDocumentation)

add_custom_target(docs ALL
    COMMAND OCRE_ROOT=${CMAKE_CURRENT_LIST_DIR}/.. /Users/marco/doxygen/build/bin/doxygen ${CMAKE_CURRENT_LIST_DIR}/Doxyfile
)

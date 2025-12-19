#!/bin/bash
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# Simple formatting checker for src folder
# Checks all C/C++ files against .clang-format rules
#

set -e

ARGUMENT="--dry-run"

if [ $# -eq 1 ]; then
    if [ "$1" == "-f" ]; then
        echo "Fixing files in place if necessary"
        ARGUMENT="-i"
    else
        echo "Invalid option: $1"
        exit 1
    fi
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_DIR="$ROOT_DIR/src"

echo "Checking formatting in $SRC_DIR/.."

find src -type f '(' -name '*.c' -o -name '*.h' ')' ! -name 'utlist.h' -print0 | \
    xargs -0 -n1 clang-format ${ARGUMENT} -Werror

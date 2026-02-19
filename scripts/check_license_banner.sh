#!/bin/sh
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# Checks all C/C++ files for license banner

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_DIR="$ROOT_DIR/src"

check_license_banner() {
    local file="$1"
    local file_header
    
    file_header=$(head -n 6 "$file")
    
    # Expected license banner
    local expected_banner="/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */"
    
    if [ "$file_header" != "$expected_banner" ]; then
        echo "ERROR: Missing or incorrect license banner in $file"
        return 1
    fi
    
    return 0
}

echo "Checking license banners."
ERROR_FOUND=0
for file in $(find . -type f \( -name '*.c' -o -name '*.h' \) \
    ! -name 'utlist.h' \
    ! -name 'CMakeCCompilerId.c' \
    ! -path './tests/Unity/*' \
    ! -path './build/*' \
    ! -path './wasm-micro-runtime/*' \
    ! -path './ocre-sdk/*' \
    ! -path './src/shell/sha256/*' \
    ! -path './src/runtime/wamr-wasip1/ocre_api/utils/*' \
    | sort); do

    if ! check_license_banner "$file"; then
        ERROR_FOUND=1
    fi
done

if [ $ERROR_FOUND -ne 0 ]; then
    echo "License check failed."
    exit 1
fi

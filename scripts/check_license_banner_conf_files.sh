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
    local start_line=1

    # Check first line and skip if it's a shebang line(only for .sh scripts)
    local first_line
    first_line=$(head -n 1 "$file")
    if [ "${first_line#\#\!/bin}" != "$first_line" ]; then
        start_line=2
    fi
    
    file_header=$(tail -n +$start_line "$file" | head -n 4)
    
    # Expected license banner
    local expected_banner="# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0"
    
    if [ "$file_header" != "$expected_banner" ]; then
        echo "ERROR: Missing or incorrect license banner in $file"
        return 1
    fi
    
    return 0
}

echo "Checking license banners."
ERROR_FOUND=0
for file in $(find . -type f \( -name 'CMakeLists.txt' -o -name '*.yaml' \
    -o -name '*.yml' -o -name '*.awk' -o -name '*.conf' -o -name 'Kconfig*' \
    -o -name '.gitignore' -o -name '.gitmodules' -o -name '*.sh' -o -name '*.cmake' \) \
    ! -name 'utlist.h' \
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

#!/bin/sh
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# Python formatter and linter
# Checks all .py files agains autopep8 rules
# Usage: check_py_formatting.sh [-f]
#   -f  Fix files in place(formatting)

set -e

ARGUMENT="--diff --exit-code"

if [ $# -eq 1 ]; then
    if [ "$1" = "-f" ]; then
        echo "Fixing files in place if necessary"
        ARGUMENT="--in-place"
    else
        echo "Invalid option: $1"
        exit 1
    fi
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

echo "Checking Python formatting."

find . -type f -name '*.py' \
    ! -path './tests/Unity/*' \
    ! -path './wasm-micro-runtime/*' \
    ! -path './ocre-sdk/*' \
    -print0 | xargs -0 autopep8 ${ARGUMENT}

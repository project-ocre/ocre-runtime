#!/bin/sh
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# Simple linter checker for markdown files
# Checks all markdown files with markdownlint

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Checking markdown files."

cd "$ROOT_DIR"

find . -type f -name '*.md' \
    ! -path '*/build/*' \
    ! -path './ocre-sdk/*' \
    ! -path './wasm-micro-runtime/*' \
    ! -path './tests/*' \
    -print0 | \
    xargs -0 mdl -s ./scripts/md_linter_rules.mdlrc

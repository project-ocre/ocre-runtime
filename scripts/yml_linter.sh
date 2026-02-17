#!/bin/bash
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# Simple linter checker for YAML files
# Checks all YAML files with yamllint

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Checking YAML files."

cd "$ROOT_DIR"

find . -type f \( -name '*.yml' -o -name '*.yaml' \) \
    ! -path '*/build/*' \
    ! -path './ocre-sdk/*' \
    ! -path './wasm-micro-runtime/*' \
    ! -path './tests/*' \
    -print0 | \
    xargs -0 yamllint -c "$SCRIPT_DIR/yml_linter_rules.yml"

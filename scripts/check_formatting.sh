#!/bin/bash
#
# Simple formatting checker for /src folder
# Checks all C/C++ files against .clang-format rules
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_DIR="$ROOT_DIR/src"

# Check if clang-format-16 is available
if ! command -v clang-format-16 &> /dev/null; then
    echo "Error: clang-format-16 not found"
    echo "Install it with: sudo apt-get install clang-format-16"
    exit 1
fi

# Check if src directory exists
if [ ! -d "$SRC_DIR" ]; then
    echo "Error: $SRC_DIR directory not found"
    exit 1
fi

echo "Checking formatting in $SRC_DIR/.."

# Find all C/C++ files and check formatting
FAILED_FILES=()
while IFS= read -r -d '' file; do
    if ! clang-format-16 --style=file --dry-run --Werror "$file" 2>&1; then
        FAILED_FILES+=("$file")
    fi
done < <(find "$SRC_DIR" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -print0)

# Report results
if [ ${#FAILED_FILES[@]} -eq 0 ]; then
    echo "All files are properly formatted"
    exit 0
else
    echo ""
    echo "The following files have formatting issues:"
    for file in "${FAILED_FILES[@]}"; do
        echo "  - $file"
    done
    echo ""
    echo "================================================================================"
    echo "CODE FORMATTING GUIDE"
    echo "================================================================================"
    echo ""
    echo "It is required to format the code before committing, or it will fail CI checks."
    echo ""
    echo "1. Install clang-format-16.0.0"
    echo ""
    echo "   You can download the package from:"
    echo "   https://github.com/llvm/llvm-project/releases"
    echo ""
    echo "   For Debian/Ubuntu:"
    echo "     sudo apt-get install clang-format-16"
    echo ""
    echo "   For macOS with Homebrew (part of llvm@14):"
    echo "     brew install llvm@14"
    echo "     /usr/local/opt/llvm@14/bin/clang-format"
    echo ""
    echo "2. Format C/C++ source files"
    echo ""
    echo "   Format a single file:"
    echo "     clang-format-16 --style=file -i path/to/file.c"
    echo ""
    echo "   Format all files in src/:"
    echo "     find src/ -type f \\( -name '*.c' -o -name '*.cpp' -o -name '*.h' \\) \\"
    echo "       -exec clang-format-16 --style=file -i {} +"
    echo ""
    echo "3. Disable formatting for specific code blocks (use sparingly)"
    echo ""
    echo "   Wrap code with clang-format off/on comments when formatted code"
    echo "   is not readable or friendly:"
    echo ""
    echo "     /* clang-format off */"
    echo "     your code here"
    echo "     /* clang-format on */"
    echo ""
    echo "================================================================================"
    exit 1
fi

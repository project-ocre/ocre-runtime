#!/bin/bash

# Function to display help
show_help() {
    echo "Usage: $0 -t <target> [-r] [-f <file1> [file2 ...]]"
    echo "  -t <target>   (Required) Specify the target. z for Zephyr and l for Linux"
    echo "  -r            (Optional) Specify whether run after the build"
    echo "  -f <file(s)>  (Optional) Specify one or more input files"
    echo "  -b            (Optional) Only Zephyr: build for b_u585i_iot02a instead of native_sim"
    echo "  -h             Display help"
    exit 0
}

RUN_MODE=false  # Default: Run mode disabled
INPUT_FILES=()
ZEPHYR_BOARD="native_sim"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t)
            TARGET="$2"
            shift 2
            ;;
        -r)
            RUN_MODE=true
            shift
            ;;
        -f)
            shift
            while [[ $# -gt 0 && ! "$1" =~ ^- ]]; do
                INPUT_FILES+=("$1")
                shift
            done
            ;;
        -b)
            ZEPHYR_BOARD=b_u585i_iot02a
            shift
            ;;
        -h)
            show_help
            ;;
        *)
            echo "Invalid option: $1" >&2
            show_help
            ;;
    esac
done

# Check if required argument is provided
if [[ "$TARGET" == "z" ]]; then
    echo "Target is: Zephyr's $ZEPHYR_BOARD"
    cd ..
    if [[ ${#INPUT_FILES[@]} -gt 0 ]]; then
        echo "Input files provided: ${INPUT_FILES[*]}"
        rm flash.bin 
        west build -p -b $ZEPHYR_BOARD ./application -d build -- \
        -DMODULE_EXT_ROOT=`pwd`/application -DOCRE_INPUT_FILE="${INPUT_FILES[0]}" -DTARGET_PLATFORM_NAME=Zephyr
        # Note: Only the first file is passed to OCRE_INPUT_FILE, adapt as needed for multiple files
    else
        rm flash.bin
        west build -p -b $ZEPHYR_BOARD  ./application -d build -- \
        -DMODULE_EXT_ROOT=`pwd`/application -DTARGET_PLATFORM_NAME=Zephyr
    fi
elif [[ "$TARGET" == "l" ]]; then
    echo "Target is: Linux x86_64"
    if [[ ! -d "build" ]]; then
        echo "build folder does not exist. Creating: build"
        mkdir -p "build"
    fi
    cd build
    cmake .. -DTARGET_PLATFORM_NAME=Linux -DWAMR_DISABLE_STACK_HW_BOUND_CHECK=1
    # Capture make output to a file
    make | tee build.log
    BUILD_SUCCESS=false
    if [[ $(tail -n 1 build.log) == "[100%] Built target app" ]]; then
        BUILD_SUCCESS=true
    fi
else
    echo "Target does not contain 'z' or 'l': exit"
    exit
fi

# Execute run mode if -r flag is set and build was successful
if [[ "$TARGET" == "z" && "$RUN_MODE" = true ]]; then
    west flash
elif [[ "$TARGET" == "l" && "$RUN_MODE" = true && "$BUILD_SUCCESS" = true ]]; then
    if [[ ${#INPUT_FILES[@]} -gt 0 ]]; then
        echo "Input files provided: ${INPUT_FILES[*]}"
        ./app "${INPUT_FILES[@]}"
    else
        ./app
    fi
fi

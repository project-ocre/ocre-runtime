#!/bin/bash

# Function to display help
show_help() {
    echo "Usage: $0 -t <target> [-r] [-f <file1> [file2 ...]]"
    echo "  -t <target>   (Required) Specify the target. z for Zephyr and l for Linux"
    echo "  -r            (Optional) Specify whether run after the build"
    echo "  -f <file(s)>  (Optional) Specify one or more input files"
    echo "  -b <board>    (Optional, Zephyr only) Select board:"
    echo "                  uw  -> b_u585i_iot02a + W5500"
    echo "                  ue  -> b_u585i_iot02a + ENC28J60"
    echo "  note: when no board is selected, native_sim is the default target for Zephyr"
    echo "  -h            Display help"
    exit 0
}

RUN_MODE=false  # Default: Run mode disabled
INPUT_FILES=()
BOARD_ARG="native_sim"

# resolve absolute paths (portable, no readlink -f)
abs_path() {
    local p="$1"
    if [[ "$p" = /* ]]; then
        echo "$p"
    else
        echo "$(cd "$(dirname "$p")" && pwd)/$(basename "$p")"
    fi
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t)
            if [[ -z "$2" || "$2" =~ ^- ]]; then
                echo "Error: -t requires a value (z or l)" >&2
                exit 1
            fi
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
            if [[ -z "$2" || "$2" =~ ^- ]]; then
                echo "Error: -b requires a board argument" >&2
                exit 1
            fi
            BOARD_ARG="$2"
            shift 2
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

# Normalize input files to absolute paths BEFORE any 'cd'
if [[ ${#INPUT_FILES[@]} -gt 0 ]]; then
    for i in "${!INPUT_FILES[@]}"; do
        INPUT_FILES[$i]="$(abs_path "${INPUT_FILES[$i]}")"
    done
fi

# Check if required argument is provided
if [[ "$TARGET" == "z" ]]; then
    echo "Target is: Zephyr"
    cd ..
    case "$BOARD_ARG" in
        uw)
            ZEPHYR_BOARD="b_u585i_iot02a"
            CONF_EXTRA=""
            echo "Building for b_u585i_iot02a with W5500 support"
            ;;
        ue)
            ZEPHYR_BOARD="b_u585i_iot02a"
            CONF_EXTRA="-DCONF_FILE=prj.conf\;boards/${ZEPHYR_BOARD}.conf\;boards/enc28j60.conf \
                        -DDTC_OVERLAY_FILE=boards/${ZEPHYR_BOARD}.overlay\;boards/enc28j60.overlay"
            echo "Building for b_u585i_iot02a with ENC28J60 support"
            ;;
        nrf5340)
            ZEPHYR_BOARD="nrf5340dk/nrf5340/cpuapp"
            CONF_EXTRA=""
            echo "Building for nrf5340dk board: App CPU"
            ;;
        *)
            ZEPHYR_BOARD="$BOARD_ARG"
            CONF_EXTRA=""
            echo "Building for board: $ZEPHYR_BOARD"
            ;;
    esac

    if [[ ${#INPUT_FILES[@]} -gt 0 ]]; then
        echo "Input files provided: ${INPUT_FILES[*]}"
        rm flash.bin
        west build -p -b $ZEPHYR_BOARD ./application -d build -- \
            -DMODULE_EXT_ROOT=`pwd`/application -DOCRE_INPUT_FILE="${INPUT_FILES[0]}" -DTARGET_PLATFORM_NAME=Zephyr $CONF_EXTRA || exit 1
    else
        rm flash.bin
        west build -p -b $ZEPHYR_BOARD ./application -d build -- \
            -DMODULE_EXT_ROOT=`pwd`/application -DTARGET_PLATFORM_NAME=Zephyr $CONF_EXTRA || exit 1
    fi
elif [[ "$TARGET" == "l" ]]; then
    echo "Target is: Linux"
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
    if [ "$BUILD_SUCCESS" == false ]; then
        exit 1
    fi
else
    echo "Target does not contain 'z' or 'l': exit"
    exit 1
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

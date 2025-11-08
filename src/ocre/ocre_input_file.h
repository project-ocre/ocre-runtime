#ifndef OCRE_INPUT_FILE_H
#define OCRE_INPUT_FILE_H

#include <stdint.h>
#include <stddef.h>

// Symbols created by objcopy from app.wasm
extern const uint8_t _binary_app_wasm_start[];
extern const uint8_t _binary_app_wasm_end[];

// Macros for compatibility with existing code
#define wasm_binary _binary_app_wasm_start
#define wasm_binary_len ((size_t)(_binary_app_wasm_end - _binary_app_wasm_start))

#endif

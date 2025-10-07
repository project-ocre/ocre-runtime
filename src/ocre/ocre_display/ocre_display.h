#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <stdbool.h>
#include "wasm_export.h"

typedef enum {
    COLOR_MODE_RGB565   = 0,
    COLOR_MODE_BGR565   = 1,
    COLOR_MODE_RGB888   = 2,
    COLOR_MODE_ARGB8888 = 3,
    COLOR_MODE_UNKNOWN  = 0xFFFFFFFFu,
} ocre_color_mode_t;

void ocre_display_init_internal(void);

// Exported WASM functions
extern int32_t ocre_display_init(wasm_exec_env_t exec_env);

extern int32_t ocre_display_get_capabilities(wasm_exec_env_t exec_env, uint32_t *out_width, 
                                 uint32_t *out_height, uint32_t *out_cpp,
                                 ocre_color_mode_t *out_color_mode);

extern void
ocre_display_flush(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
              int32_t y2, uint8_t *color);

extern void ocre_display_input_read(wasm_exec_env_t exec_env, int32_t *x, int32_t *y, bool *pressed, bool *more);

extern int ocre_time_get_ms(wasm_exec_env_t exec_env);

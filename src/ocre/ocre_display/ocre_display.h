#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <stdbool.h>
#include "wasm_export.h"

// TODO: pull in an LVGL header instead of redefining types here
#define LV_COLOR_DEPTH  16  // Matches NXP display

typedef union {
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    };
    uint32_t full;
} lv_color32_t;

typedef union
{
    struct
    {
#if LV_COLOR_16_SWAP == 0
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
#else
        uint16_t green_h : 3;
        uint16_t red : 5;
        uint16_t blue : 5;
        uint16_t green_l : 3;
#endif
    } ch;
    uint16_t full;
} lv_color16_t;

#if LV_COLOR_DEPTH == 32
typedef lv_color32_t lv_color_t;
#else
typedef lv_color16_t lv_color_t;
#endif

typedef uint8_t lv_indev_state_t;
typedef int16_t lv_coord_t;
typedef uint8_t lv_opa_t;

enum { LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR };
enum {
    LV_OPA_TRANSP = 0,
    LV_OPA_0 = 0,
    LV_OPA_10 = 25,
    LV_OPA_20 = 51,
    LV_OPA_30 = 76,
    LV_OPA_40 = 102,
    LV_OPA_50 = 127,
    LV_OPA_60 = 153,
    LV_OPA_70 = 178,
    LV_OPA_80 = 204,
    LV_OPA_90 = 229,
    LV_OPA_100 = 255,
    LV_OPA_COVER = 255,
};

void ocre_display_init(void);

extern int
time_get_ms(wasm_exec_env_t exec_env);

extern void display_init(void);

extern void
display_flush(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
              int32_t y2, lv_color_t *color);

extern void
display_fill(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
             int32_t y2, lv_color_t *color);

extern void
display_map(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
            int32_t y2, const lv_color_t *color);

extern void display_input_read(wasm_exec_env_t exec_env, int32_t *x, int32_t *y, bool *pressed, bool *more);

/*
 * OCRE display backend (SDL)
 *
 * Simple PoC backend that presents a window and accepts "touch"
 * via the mouse. It advertises ARGB8888 and expects flush buffers
 * to match that pixel layout.
 *
 * Build: link with SDL2 (-lSDL2); see notes below.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "wasm_export.h"
#include "ocre_display.h"
#include <SDL2/SDL.h>

// Debugging: draw a simple test pattern when display_init() is called from a container
// #define DRAW_SELFTEST

// Debugging: draw a magenta border around updated regions
// #define DRAW_UPDATE_BORDER

// SDL window size
// Mirror NXP RT1064 EVK LCD for now
#define OCRE_SDL_WIDTH  480
#define OCRE_SDL_HEIGHT 272

// Right now we're supporting only RGB565 as most embedded LCDs support
#define COLOR_MODE      COLOR_MODE_RGB565
#define BPP             2   

// SDL rendering must be done from the same thread that created the renderer
// Probably can remove, but here from prior testing
static Uint32 g_render_thread_id = 0; // SDL's thread id of the renderer owner
static inline Uint32 current_tid(void) { return SDL_ThreadID(); }

// Globals
static uint8_t  g_bpp        = BPP;
static ocre_color_mode_t g_color_mode = COLOR_MODE_RGB565;

static uint32_t g_width      = OCRE_SDL_WIDTH;
static uint32_t g_height     = OCRE_SDL_HEIGHT;

/* SDL objects */
static int sdl_initialized = 0;
static SDL_Window   *g_win   = NULL;
static SDL_Renderer *g_ren   = NULL;
static SDL_Texture  *g_tex   = NULL;

/* Input (mouse-as-touch) */
#define TOUCH_Q_CAP 8
struct touch_evt {
    uint16_t x;
    uint16_t y;
    uint8_t  pressed; /* 0/1 */
};

// Self-test pattern
#if defined(DRAW_SELFTEST)
/* Pack RGB into 565 (R:5,G:6,B:5). This matches SDL_PIXELFORMAT_RGB565. */
static inline uint16_t pack_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* If you switch your texture to SDL_PIXELFORMAT_BGR565, use this instead. */
static inline uint16_t pack_bgr565(uint8_t r, uint8_t g, uint8_t b) {
    /* Note the swapped positions for B and R in the bit layout */
    return (uint16_t)(((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}

/* Paints vertical color bars and a gray ramp into the current texture. */
static void selftest_fill_texture_565(SDL_Texture *tex, int use_bgr565)
{
    if (!tex) return;

    int w=0, h=0;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);

    SDL_Rect r = { .x = 0, .y = 0, .w = w, .h = h };
    void *pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(tex, &r, &pixels, &pitch) != 0) {
        printf("SDL_LockTexture(selftest) failed: %s\n", SDL_GetError());
        return;
    }

    /* Choose the packer based on pixel format selection. */
    uint16_t (*pack)(uint8_t,uint8_t,uint8_t) = use_bgr565 ? pack_bgr565 : pack_rgb565;

    /* Bars: [Red|Green|Blue|White|Black] then a gray ramp across the rest */
    const int bars = 5;
    const int bar_w = w / 7; /* leave room for the ramp */
    for (int y = 0; y < h; ++y) {
        uint16_t *row = (uint16_t *)((uint8_t*)pixels + (size_t)y * (size_t)pitch);
        for (int x = 0; x < w; ++x) {
            uint16_t px = 0;
            if (x < 1*bar_w)       px = pack(255,   0,   0);   /* Red */
            else if (x < 2*bar_w)  px = pack(  0, 255,   0);   /* Green */
            else if (x < 3*bar_w)  px = pack(  0,   0, 255);   /* Blue */
            else if (x < 4*bar_w)  px = pack(255, 255, 255);   /* White */
            else if (x < 5*bar_w)  px = pack(  0,   0,   0);   /* Black */
            else {
                /* Gray ramp from left to right across the remaining width */
                int ramp_x = x - 5*bar_w;
                int ramp_w = w - 5*bar_w;
                uint8_t g = (uint8_t)((ramp_x * 255) / (ramp_w > 0 ? ramp_w : 1));
                px = pack(g, g, g);
            }
            row[x] = px;
        }
    }
    SDL_UnlockTexture(tex);
}
#endif

// Ring buffer to mimic the Zephyr msgq behavior
static struct touch_evt touch_q[TOUCH_Q_CAP];
static int q_head = 0, q_tail = 0, q_count = 0;
static struct touch_evt last_evt = {0, 0, 0};

static void q_push(struct touch_evt e) {
    if (q_count == TOUCH_Q_CAP) {
        // drop oldest
        q_tail = (q_tail + 1) % TOUCH_Q_CAP;
        q_count--;
    }
    touch_q[q_head] = e;
    q_head = (q_head + 1) % TOUCH_Q_CAP;
    q_count++;
}

static bool q_pop(struct touch_evt *out) {
    if (q_count == 0) return false;
    *out = touch_q[q_tail];
    q_tail = (q_tail + 1) % TOUCH_Q_CAP;
    q_count--;
    return true;
}

// Process pending SDL events; push mouse-as-touch into queue
static void pump_input_events(void) {
    SDL_Event ev;
    bool any = false;
    while (SDL_PollEvent(&ev)) {
        any = true;
        if (ev.type == SDL_QUIT) {
            // Ignore window close events since the window is owned by the container
        } else if (ev.type == SDL_MOUSEMOTION) {
            struct touch_evt e = {
                .x = (uint16_t)ev.motion.x,
                .y = (uint16_t)ev.motion.y,
                .pressed = (uint8_t)((ev.motion.state & SDL_BUTTON_LMASK) ? 1 : 0),
            };
            if (e.x != last_evt.x || e.y != last_evt.y || e.pressed != last_evt.pressed) {
                q_push(e);
                last_evt = e;
            }
        } else if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
            if (ev.button.button == SDL_BUTTON_LEFT) {
                struct touch_evt e = {
                    .x = (uint16_t)ev.button.x,
                    .y = (uint16_t)ev.button.y,
                    .pressed = (uint8_t)(ev.type == SDL_MOUSEBUTTONDOWN ? 1 : 0),
                };
                if (e.x != last_evt.x || e.y != last_evt.y || e.pressed != last_evt.pressed) {
                    q_push(e);
                    last_evt = e;
                }
            }
        }
    }

    /* If nothing polled, still ensure we don't starve later calls */
    (void)any;
}

// Internal init, not needed for SDL
void ocre_display_init_internal(void) {}

/* ------------------------ Public API (shared header) ------------------------ */

void ocre_display_init(wasm_exec_env_t exec_env)
{
    (void)exec_env;
    if (sdl_initialized) return;

    // Record owning thread
    g_render_thread_id = current_tid();

    // Prevent SDL from installing signal handlers - otherwise it captures
    // SIGTERM from the runtime and prevents user shutdown handling.
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");  

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    // g_width  = OCRE_SDL_WIDTH;
    // g_height = OCRE_SDL_HEIGHT;
    // g_color_mode = COLOR_MODE_RGB565;
    // g_bpp = 2;

    // TODO: should use the container name here
    g_win = SDL_CreateWindow("Ocre Display",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        (int)g_width, (int)g_height, 0);
    if (!g_win) { printf("SDL_CreateWindow failed: %s\n", SDL_GetError()); return; }

    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_ren) { printf("SDL_CreateRenderer failed: %s\n", SDL_GetError()); return; }

    g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                              (int)g_width, (int)g_height);
    if (!g_tex) { printf("SDL_CreateTexture failed: %s\n", SDL_GetError()); return; }

    SDL_RenderClear(g_ren);
    SDL_RenderPresent(g_ren);

#if defined(DRAW_SELFTEST)
    selftest_fill_texture_565(g_tex, 0); // second arg = use RGB565
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
#endif

    printf("SDL ready on TID=%u  win=%p ren=%p tex=%p\n",
           (unsigned)g_render_thread_id, g_win, g_ren, g_tex);

    sdl_initialized = 1;
}


int32_t
ocre_display_get_capabilities(wasm_exec_env_t exec_env,
                              uint32_t *out_width,
                              uint32_t *out_height,
                              uint32_t *out_cpp,
                              ocre_color_mode_t *out_color_mode)
{
    if (!out_width || !out_height || !out_cpp || !out_color_mode) {
        return -1;
    }
    if (!sdl_initialized || g_bpp == 0 || g_color_mode == COLOR_MODE_UNKNOWN) {
        return -1;
    }
    *out_width      = g_width;
    *out_height     = g_height;
    *out_cpp        = (uint32_t)g_bpp;
    *out_color_mode = g_color_mode;
    return 0;
}

void
ocre_display_flush(wasm_exec_env_t exec_env, int32_t x1, int32_t y1, int32_t x2,
                   int32_t y2, uint8_t *color)
{
    (void)exec_env;

    if (!sdl_initialized || !g_tex || g_bpp != 2) return;

    // Ensure all SDL calls happen on the same thread that created the renderer
    if (g_render_thread_id && current_tid() != g_render_thread_id) {
        fprintf(stderr, "flush on wrong thread: owner=%u, current=%u (ignored)\n",
                (unsigned)g_render_thread_id, (unsigned)current_tid());
        return;
    }

    if (x2 < x1 || y2 < y1) return;

    // Clip to texture bounds
    int tex_w = 0, tex_h = 0;
    SDL_QueryTexture(g_tex, NULL, NULL, &tex_w, &tex_h);

    int cx1 = x1 < 0 ? 0 : x1;
    int cy1 = y1 < 0 ? 0 : y1;
    int cx2 = x2 >= tex_w ? (tex_w - 1) : x2;
    int cy2 = y2 >= tex_h ? (tex_h - 1) : y2;
    if (cx2 < cx1 || cy2 < cy1) return;

    const int cw = cx2 - cx1 + 1;
    const int ch = cy2 - cy1 + 1;

    // Source stride (bytes) – default to tight area rows; allow override in *pixels*
    const int rect_w = (int)(x2 - x1 + 1);
    int stride_px = rect_w;
    const size_t src_stride_bytes = (size_t)stride_px * (size_t)g_bpp;

    // Adjust source pointer for any clipping
    const uint8_t *src = color
                       + (size_t)(cy1 - y1) * src_stride_bytes
                       + (size_t)(cx1 - x1) * (size_t)g_bpp;

    // Lock the exact sub-rect (fallback to full lock if driver refuses sub-rect)
    SDL_Rect r = { .x = cx1, .y = cy1, .w = cw, .h = ch };
    void *tex_pixels = NULL;
    int tex_pitch = 0;
    if (SDL_LockTexture(g_tex, &r, &tex_pixels, &tex_pitch) != 0) {
        if (SDL_LockTexture(g_tex, NULL, &tex_pixels, &tex_pitch) != 0) {
            fprintf(stderr, "SDL_LockTexture failed: %s\n", SDL_GetError());
            return;
        }
        // Copy into full texture at the rect offset
        uint8_t *dst_base = (uint8_t*)tex_pixels
                          + (size_t)cy1 * (size_t)tex_pitch
                          + (size_t)cx1 * (size_t)g_bpp;
        for (int row = 0; row < ch; ++row) {
            memcpy(dst_base + (size_t)row * (size_t)tex_pitch,
                   src      + (size_t)row * src_stride_bytes,
                   (size_t)cw * (size_t)g_bpp);
        }
        SDL_UnlockTexture(g_tex);
    } else {
        // Direct write to locked sub-rect
        uint8_t *dst = (uint8_t *)tex_pixels;
        for (int row = 0; row < ch; ++row) {
            memcpy(dst + (size_t)row * (size_t)tex_pitch,
                   src + (size_t)row * src_stride_bytes,
                   (size_t)cw * (size_t)g_bpp);
        }

        // Optional 1-px outline to visualize updates
        #if defined(DRAW_UPDATE_BORDER)
            const uint16_t MAGENTA = 0xF81F; // RGB565
            // top
            uint16_t *row16 = (uint16_t *)(dst + 0 * (size_t)tex_pitch);
            for (int x = 0; x < cw; ++x) row16[x] = MAGENTA;
            // bottom
            row16 = (uint16_t *)(dst + (size_t)(ch - 1) * (size_t)tex_pitch);
            for (int x = 0; x < cw; ++x) row16[x] = MAGENTA;
            // sides
            for (int y = 0; y < ch; ++y) {
                row16 = (uint16_t *)(dst + (size_t)y * (size_t)tex_pitch);
                row16[0] = MAGENTA;
                row16[cw - 1] = MAGENTA;
            }
        #endif

        SDL_UnlockTexture(g_tex);
    }

    // Present
    SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
}


void ocre_display_input_read(wasm_exec_env_t exec_env,
                             int32_t *x, int32_t *y, bool *pressed, bool *more)
{
    if (!x || !y || !pressed || !more) return;

    /* Drain any pending SDL events into our queue */
    pump_input_events();

    struct touch_evt e;
    if (q_pop(&e)) {
        *x = e.x;
        *y = e.y;
        *pressed = (e.pressed != 0);
        *more = (q_count > 0);
        last_evt = e;
    } else {
        *x = last_evt.x;
        *y = last_evt.y;
        *pressed = (last_evt.pressed != 0);
        *more = false;
    }
}

int ocre_time_get_ms(wasm_exec_env_t exec_env)
{
    /* SDL ticks are milliseconds since SDL_Init() */
    return (int)SDL_GetTicks();
}

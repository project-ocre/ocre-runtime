#ifndef OCRE_PSRAM
#define OCRE_PSRAM

// PSRAM configuration - centralized for different platforms
#if defined(CONFIG_MEMC)
    // Board-specific PSRAM section attributes
    #if defined(CONFIG_BOARD_ARDUINO_PORTENTA_H7)
        #define PSRAM_SECTION_ATTR __attribute__((section("SDRAM1"), aligned(32)))
    #elif defined(CONFIG_BOARD_B_U585I_IOT02A)
        #define PSRAM_SECTION_ATTR __attribute__((section(".stm32_psram"), aligned(32)))
    #elif defined(CONFIG_BOARD_MIMXRT1064_EVK)
        #define PSRAM_SECTION_ATTR __attribute__((section("SDRAM"), aligned(32)))
    #else
        #define PSRAM_SECTION_ATTR __attribute__((aligned(32)))
    #endif

    PSRAM_SECTION_ATTR
    static char storage_heap_buf[CONFIG_OCRE_STORAGE_HEAP_BUFFER_SIZE] = {0};
    
    static struct k_heap storage_heap;
    #define storage_heap_init() k_heap_init(&storage_heap, storage_heap_buf, CONFIG_OCRE_STORAGE_HEAP_BUFFER_SIZE)
    #define storage_heap_alloc(size) k_heap_alloc(&storage_heap, (size), K_SECONDS(1))
    #define storage_heap_free(buffer) k_heap_free(&storage_heap, (void*)buffer)
#else
    // No PSRAM - use system malloc
    #define storage_heap_init() /* No initialization needed */
    #define storage_heap_alloc(size) malloc(size)
    #define storage_heap_free(buffer) free(buffer)
#endif

#endif /* OCRE_PSRAM*/

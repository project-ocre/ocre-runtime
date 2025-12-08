add_library(OcrePlatform)

target_sources(OcrePlatform
    PRIVATE
    memory.c
    file_mmap.c
    # file_alloc_read.c
)

target_include_directories(OcrePlatform
    PUBLIC
    include
    ../include
)

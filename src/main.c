/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>

#include <ocre/ocre.h>
#include <ocre/fs/fs.h>
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

void create_sample_container();

int main(int argc, char *argv[])
{
    ocre_cs_ctx ctx;
    char *container_filename = "hello.bin";

    ocre_app_storage_init();

    create_sample_container(container_filename);

    // Step 1:  Initialize the Ocre runtime
    ocre_container_runtime_init();
    printk("\n\nProject Ocre runtime started\n");

    // Step 2:  Create the container, this allocates and loads the container binary
    ocre_container_runtime_create_container(container_filename, &ctx);

    // Step 3:  Execute the container
    ocre_container_runtime_run_container(&ctx);

    // Loop forever, without this the application will exit and stop all execution
    while (true)
    {
        k_msleep(1000);
    }
}

/**
 * Creates a container image file using the sample "hello-word" WASM module
 * This is for demostration purposes only and does not perform any error checking.
 *
 * @param file_name a string containing the name of the file to create
 */

void create_sample_container(char *file_name)
{
    struct fs_file_t f;
    static char file_path[64];
    snprintf(&file_path, 64, "/lfs/ocre/images/%s", file_name);
    int res;

    fs_file_t_init(&f);
    res = fs_open(&f, file_path, FS_O_CREATE | FS_O_RDWR);

    unsigned char wasm_binary[] = {
        0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x00,
        0x00, 0x02, 0x0c, 0x01, 0x03, 0x65, 0x6e, 0x76, 0x04, 0x70, 0x75, 0x74, 0x73, 0x00, 0x00, 0x03, 0x03, 0x02,
        0x01, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x3c, 0x0a, 0x7f, 0x01, 0x41, 0x90, 0x28, 0x0b, 0x7f, 0x00,
        0x41, 0x80, 0x08, 0x0b, 0x7f, 0x00, 0x41, 0x8d, 0x08, 0x0b, 0x7f, 0x00, 0x41, 0x90, 0x08, 0x0b, 0x7f, 0x00,
        0x41, 0x90, 0x28, 0x0b, 0x7f, 0x00, 0x41, 0x80, 0x08, 0x0b, 0x7f, 0x00, 0x41, 0x90, 0x28, 0x0b, 0x7f, 0x00,
        0x41, 0x80, 0x80, 0x04, 0x0b, 0x7f, 0x00, 0x41, 0x00, 0x0b, 0x7f, 0x00, 0x41, 0x01, 0x0b, 0x07, 0xab, 0x01,
        0x0c, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x11, 0x5f, 0x5f, 0x77, 0x61, 0x73, 0x6d, 0x5f,
        0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x63, 0x74, 0x6f, 0x72, 0x73, 0x00, 0x01, 0x07, 0x6f, 0x6e, 0x5f, 0x69, 0x6e,
        0x69, 0x74, 0x00, 0x02, 0x0c, 0x5f, 0x5f, 0x64, 0x73, 0x6f, 0x5f, 0x68, 0x61, 0x6e, 0x64, 0x6c, 0x65, 0x03,
        0x01, 0x0a, 0x5f, 0x5f, 0x64, 0x61, 0x74, 0x61, 0x5f, 0x65, 0x6e, 0x64, 0x03, 0x02, 0x0b, 0x5f, 0x5f, 0x73,
        0x74, 0x61, 0x63, 0x6b, 0x5f, 0x6c, 0x6f, 0x77, 0x03, 0x03, 0x0c, 0x5f, 0x5f, 0x73, 0x74, 0x61, 0x63, 0x6b,
        0x5f, 0x68, 0x69, 0x67, 0x68, 0x03, 0x04, 0x0d, 0x5f, 0x5f, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x5f, 0x62,
        0x61, 0x73, 0x65, 0x03, 0x05, 0x0b, 0x5f, 0x5f, 0x68, 0x65, 0x61, 0x70, 0x5f, 0x62, 0x61, 0x73, 0x65, 0x03,
        0x06, 0x0a, 0x5f, 0x5f, 0x68, 0x65, 0x61, 0x70, 0x5f, 0x65, 0x6e, 0x64, 0x03, 0x07, 0x0d, 0x5f, 0x5f, 0x6d,
        0x65, 0x6d, 0x6f, 0x72, 0x79, 0x5f, 0x62, 0x61, 0x73, 0x65, 0x03, 0x08, 0x0c, 0x5f, 0x5f, 0x74, 0x61, 0x62,
        0x6c, 0x65, 0x5f, 0x62, 0x61, 0x73, 0x65, 0x03, 0x09, 0x0a, 0x14, 0x02, 0x02, 0x00, 0x0b, 0x0f, 0x00, 0x41,
        0x80, 0x88, 0x80, 0x80, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x00, 0x1a, 0x0b, 0x0b, 0x14, 0x01, 0x00, 0x41,
        0x80, 0x08, 0x0b, 0x0d, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x00};
    unsigned int wasm_binary_len = 323;

    fs_write(&f, &wasm_binary, wasm_binary_len);

    fs_close(&f);
}
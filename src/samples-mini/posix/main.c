/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "ocre_core_external.h"
#include <ocre/ocre.h>
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

#ifdef HAS_GENERATED_INPUT
#include <ocre/ocre_input_file.g>
#else
#include <ocre/ocre_input_file.h>
#endif

void create_sample_container();

int main(int argc, char *argv[]) {
    ocre_cs_ctx ctx;
    ocre_container_init_arguments_t args;
    char *container_filename = "hello";

    ocre_app_storage_init();

    // Step 1:  Initialize the Ocre runtime
    ocre_container_runtime_status_t ret = ocre_container_runtime_init(&ctx, &args);

    if (ret == RUNTIME_STATUS_INITIALIZED) {
        printf("\n\nOcre runtime started\n");

        if (argc > 1) {
        set_argc(argc);
        container_filename = argv[1]; // Use the filename as the container name/sha256
        }
        else {
            create_sample_container(container_filename);
        }

        // Step 2:  Create the container, this allocates and loads the container binary
        ocre_container_data_t ocre_container_data[CONFIG_MAX_CONTAINERS];
        int container_ID[CONFIG_MAX_CONTAINERS];
        ocre_container_runtime_cb callback[CONFIG_MAX_CONTAINERS];

        int i = 0;
        do {
            ocre_container_data[i].heap_size = 0;
            snprintf(ocre_container_data[i].name, sizeof(ocre_container_data[i].name), "Container%d", i);
            snprintf(ocre_container_data[i].sha256, sizeof(ocre_container_data[i].sha256), "%s", container_filename);
            ocre_container_data[i].timers = 0;
            ocre_container_data[i].watchdog_interval = 0;
            ocre_container_runtime_create_container(&ctx, &ocre_container_data[i], &container_ID[i], callback[i]);

            // Step 3:  Execute the container
            ocre_container_runtime_run_container(container_ID[i], callback[i]);
            core_sleep_ms(1000);

            i++;
            container_filename = argv[i+1];
        } while (i < argc - 1);

        // Loop forever, without this the application will exit and stop all execution
        while (true) {
            core_sleep_ms(1000);
        }

    } else {
        printf("\n\nOcre runtime failed to start.\n");
    }
}

/**
 * Creates a container image file using the sample "hello-word" WASM module
 * This is for demostration purposes only and does not perform any error checking.
 *
 * @param file_name a string containing the name of the file to create
 */

void create_sample_container(char *file_name) {
    static char file_path[64];
    snprintf(file_path, sizeof(file_path), "./ocre/images/%s.bin", file_name);

    // Create directories if they don't exist
    mkdir("./ocre", 0755);
    mkdir("./ocre/images", 0755);

    // Open the file for writing
    int fd = open(file_path, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    // Write the binary data to the file
    ssize_t bytes_written = write(fd, wasm_binary, wasm_binary_len);
    if (bytes_written == -1) {
        perror("Error writing to file");
    } else {
        printf("Wrote %zd bytes to %s\n", bytes_written, file_path);
    }

    // Close the file
    close(fd);
}

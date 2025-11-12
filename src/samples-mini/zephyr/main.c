/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include "ocre_core_external.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>

#include <ocre/ocre.h>
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

#ifdef HAS_GENERATED_INPUT
#include <ocre/ocre_input_file.g>
#else
#include <ocre/ocre_input_file.h>
#endif

void create_sample_container();
int ocre_network_init();

int main(int argc, char *argv[]) {
    ocre_cs_ctx ctx;
    ocre_container_init_arguments_t args;
#ifdef OCRE_INPUT_FILE_NAME
    const char *container_filename = OCRE_INPUT_FILE_NAME;
#else
    const char *container_filename = "hello-from-ocre";
#endif

#ifdef CONFIG_OCRE_NETWORKING
    int net_status = ocre_network_init();
    if (net_status < 0) {
        printf("Unable to connect to network\n");
    } else {
        printf("Network is UP\n");
    }
#endif

    ocre_app_storage_init();

    // Step 1:  Initialize the Ocre runtime
    ocre_container_runtime_status_t ret = ocre_container_runtime_init(&ctx, &args);

    if (ret == RUNTIME_STATUS_INITIALIZED) {
        printf("\n\nOcre runtime started\n");

        create_sample_container(container_filename);

        // Step 2:  Create the container, this allocates and loads the container binary
        ocre_container_data_t ocre_container_data;
        int container_ID;

        ocre_container_data.heap_size = 0;
        snprintf(ocre_container_data.name, sizeof(ocre_container_data.name), "%s", container_filename);
        snprintf(ocre_container_data.sha256, sizeof(ocre_container_data.sha256), "%s", container_filename);
        ocre_container_data.timers = 0;
        ocre_container_runtime_create_container(&ctx, &ocre_container_data, &container_ID, NULL);

        // Step 3:  Execute the container
        ocre_container_runtime_run_container(container_ID, NULL);
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
    struct fs_file_t f;
    snprintf((char *)&file_path, 64, "/lfs/ocre/images/%s.bin", file_name);
    int res;

    fs_file_t_init(&f);
    res = fs_open(&f, file_path, FS_O_CREATE | FS_O_RDWR);

    fs_write(&f, &wasm_binary, wasm_binary_len);
    fs_close(&f);
}

int ocre_network_init() {

    struct net_if *iface = net_if_get_default();
    net_dhcpv4_start(iface);

    printf("Waiting for network to be ready...\n");

    int sleep_cnt = 0;
    while (!net_if_is_up(iface) && (sleep_cnt < 10)) {
        k_sleep(K_MSEC(200));
        sleep_cnt++;
    }

    if (!net_if_is_up(iface)) {
        return -ENOTCONN;
    }

    return 0;
}

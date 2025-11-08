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

 #ifdef HAS_WASM_MANIFEST
 #include "wasm_manifest.h"
 #endif

 void create_containers_from_manifest(ocre_cs_ctx *ctx);
 int ocre_network_init();

 int main(int argc, char *argv[]) {
     ocre_cs_ctx ctx;
     ocre_container_init_arguments_t args;

 #ifdef CONFIG_OCRE_NETWORKING
     int net_status = ocre_network_init();
     if (net_status < 0) {
         printf("Unable to connect to network\n");
     } else {
         printf("Network is UP\n");
     }
 #endif

     ocre_app_storage_init();

     // Step 1: Initialize the Ocre runtime
     ocre_container_runtime_status_t ret = ocre_container_runtime_init(&ctx, &args);

     if (ret == RUNTIME_STATUS_INITIALIZED) {
         printf("\n\nOcre runtime started\n");

 #ifdef HAS_WASM_MANIFEST
         // Create and run all containers from manifest
         printf("Found %d WASM binaries to load\n", WASM_BINARY_COUNT);
         create_containers_from_manifest(&ctx);
 #else
         printf("ERROR: No WASM binaries provided. Build with -f option.\n");
 #endif

         // Loop forever, without this the application will exit and stop all execution
         while (true) {
             core_sleep_ms(1000);
         }

     } else {
         printf("\n\nOcre runtime failed to start.\n");
     }
 }

 #ifdef HAS_WASM_MANIFEST
 /**
  * Create and run all containers from the embedded WASM manifest
  */
  void create_containers_from_manifest(ocre_cs_ctx *ctx) {
    int container_IDs[WASM_BINARY_COUNT];

    // Step 1: Write all WASM binaries to filesystem
    for (int i = 0; i < WASM_BINARY_COUNT; i++) {
        const wasm_binary_entry_t *entry = wasm_get(i);
        size_t actual_size = wasm_get_size(i);  // ← Use the helper function!

        printf("Loading WASM: %s (%zu bytes) from flash\n", entry->name, actual_size);

        // Write to filesystem
        char file_path[64];
        struct fs_file_t f;
        snprintf(file_path, sizeof(file_path), "/lfs/ocre/images/%s.bin", entry->name);

        fs_file_t_init(&f);
        int res = fs_open(&f, file_path, FS_O_CREATE | FS_O_RDWR);
        if (res < 0) {
            printf("  ERROR: Failed to create file: %d\n", res);
            continue;
        }

        fs_write(&f, entry->data, actual_size);  // ← Use actual_size
        fs_close(&f);
        printf("  Written to: %s\n", file_path);
    }

    // Step 2: Create all containers
    for (int i = 0; i < WASM_BINARY_COUNT; i++) {
        const wasm_binary_entry_t *entry = wasm_get(i);

        ocre_container_data_t container_data;
        container_data.heap_size = 0;
        snprintf(container_data.name, sizeof(container_data.name), "%s", entry->name);
        snprintf(container_data.sha256, sizeof(container_data.sha256), "%s", entry->name);
        container_data.timers = 0;

        ocre_container_status_t status = ocre_container_runtime_create_container(ctx, &container_data,
                                                                                  &container_IDs[i], NULL);
        // Check for CREATED status, not 0!
        if (status == CONTAINER_STATUS_CREATED) {
            printf("Container requested: %s (ID: %d)\n", entry->name, container_IDs[i]);
            core_sleep_ms(100);
        } else {
            printf("ERROR: Failed to request container %s: %d\n", entry->name, status);
            container_IDs[i] = -1;
        }
    }

    // Give containers time to actually be created asynchronously
    printf("Waiting for containers to be created...\n");
    core_sleep_ms(500);  // Wait for async creation to complete

    // Run all containers
    for (int i = 0; i < WASM_BINARY_COUNT; i++) {
        if (container_IDs[i] >= 0) {
            const wasm_binary_entry_t *entry = wasm_get(i);
            printf("Starting container: %s\n", entry->name);
            ocre_container_runtime_run_container(container_IDs[i], NULL);
        }
    }

    printf("All containers started!\n");
}
 #endif

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

#ifndef OCRE_CS_SM_IMPL_H
#define OCRE_CS_SM_IMPL_H

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>
#include "../../api/ocre_api.h"
#include <ocre/ocre_container_runtime/ocre_container_runtime.h>

// Function prototypes
// RUNTIME
ocre_container_runtime_status_t CS_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args);
ocre_container_runtime_status_t CS_runtime_destroy(void);
// CONTAINER
ocre_container_status_t CS_create_container(ocre_cs_ctx *ctx, int container_id);
ocre_container_status_t CS_run_container(ocre_cs_ctx *ctx, int container_id);
ocre_container_status_t CS_get_container_status(ocre_cs_ctx *ctx, int container_id);
ocre_container_status_t CS_stop_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback);
ocre_container_status_t CS_destroy_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback);
ocre_container_status_t CS_restart_container(ocre_cs_ctx *ctx, int container_id, ocre_container_runtime_cb callback);
// DATA STRUCTURES
int CS_ctx_init(ocre_cs_ctx *ctx);
#endif // OCRE_CS_SM_IMPL_H

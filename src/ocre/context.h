/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ocre_context;

struct ocre_context *ocre_context_create(const char *workdir);
int ocre_context_destroy(struct ocre_context *context);
struct ocre_container *ocre_context_get_container_by_id_locked(const struct ocre_context *context, const char *id);

/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <ocre/ocre.h>

struct ocre_container;

struct ocre_container *ocre_container_create(const char *img_path, const char *workdir, const char *runtime,
					     const char *container_id, bool detached,
					     const struct ocre_container_args *arguments);
int ocre_container_destroy(struct ocre_container *container);

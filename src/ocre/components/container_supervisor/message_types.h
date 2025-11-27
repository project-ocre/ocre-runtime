/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CS_MESSAGING_TYPES_H
#define OCRE_CS_MESSAGING_TYPES_H

#include "ocre_core_external.h"

struct install {
	char name[OCRE_MODULE_NAME_LEN]; // <! Name of module (must be unique per installed instance)
	char sha256[OCRE_SHA256_LEN];	 // <! Sha256 of file (to be used in file path)
	int heap_size;			 // <! Heap size
	int timers;			 // <! Maximum number of timers
	int watchdog_interval;		 // <! Watchdog interval in {units}
};

struct uninstall {
	char name[OCRE_MODULE_NAME_LEN]; // <! Name of wasm module to uninstall
};

struct query {
	char name[OCRE_MODULE_NAME_LEN]; // <! Name of app to query.
};

struct app_manager_response {
	int msg_id;
};

#endif

/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ocre.h
 *
 * Public Interface of Ocre Runtime Library.
 *
 * This header define the interfaces of general purpose container management in Ocre.
 */

#ifndef OCRE_H
#define OCRE_H

/**
 * @mainpage Ocre Runtime Library Index
 * # Ocre Runtime Library
 *
 * Ocre is a runtime library for managing containers. It provides a set of APIs for creating, managing, and controlling
 * containers. It includes a WebAssembly runtime engine that can be used to run WebAssembly modules.
 *
 * This is just the auto-generated Doxygen documentation. Please refer to the full documentation and our website for
 * more information and detailed usage guidelines.
 *
 * # Public User API
 *
 * Used to control Ocre Library, manage contexts and containers.
 *
 * ## Ocre Library Control
 *
 * Used to initialize Ocre Runtime Library and create contexts.
 *
 * See ocre.h file documentation.
 *
 * ## Ocre Context Control: used to create and remove contexts.
 *
 * See ocre_context class for documentation.
 *
 * ## Ocre Container Control: used to manage containers.
 *
 * See ocre_container class for documentation.
 *
 * # Runtime Engine Virtual Table
 *
 * Used to add custom runtime engines to the Ocre Runtime Library.
 *
 * See ocre_runtime_vtable class for documentation.
 */

#include <ocre/common.h>
#include <ocre/library.h>
#include <ocre/context.h>
#include <ocre/container.h>

#endif /* OCRE_H */

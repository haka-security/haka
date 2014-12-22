/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_SYSTEM_H
#define HAKA_SYSTEM_H

#include <haka/types.h>

/**
 * Register a callback function that will be executed in signal
 * handler in case of fatal signal.
 */
bool system_register_fatal_cleanup(void (*callback)());

/**
 * Fatal exit.
 */
void fatal_exit(int rc);

/**
 * Get the haka path from default or env variable HAKA_PATH.
 */
const char *haka_path(void);

/**
 * Stop haka.
 */
void haka_exit(void);

/**
 * Get current process memory information.
 */
bool get_memory_size(size_t *vmsize, size_t *rss);

/**
 * Create a folder and all its parents if needed.
 */
bool mkdir_path(const char *path, int mode);

/**
 * Get a configuration value.
 */
const char *_get_config_value(const char *name, const char *def);

#define GET_CONFIG_VALUE(name) _get_config_value(#name, name)

#endif /* HAKA_SYSTEM_H */

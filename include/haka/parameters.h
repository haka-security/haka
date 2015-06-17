/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Abstract parameters utilities.
 *
 * It is mainly used to give generic parameters to modules.
 */

#ifndef HAKA_PARAMETERS_H
#define HAKA_PARAMETERS_H

#include <stddef.h>
#include <haka/types.h>


struct parameters; /**< Opaque structures. */

/**
 * Load the parameters from a file. The file must follow
 * the ini file format.
 */
struct parameters *parameters_open(const char *file);

/**
 * Create an empty parameter list.
 */
struct parameters *parameters_create();

/**
 * Release a paramerter list.
 */
void               parameters_free(struct parameters *params);

/**
 * Select a section in the parameter list.
 */
int                parameters_open_section(struct parameters *params, const char *section);

/**
 * Close the section.
 */
int                parameters_close_section(struct parameters *params);

/**
 * Get the parameter value as a string.
 */
const char        *parameters_get_string(struct parameters *params, const char *key, const char *def);

/**
 * Get the parameter value as a boolean.
 */
bool               parameters_get_boolean(struct parameters *params, const char *key, bool def);

/**
 * Get the parameter value as an integer.
 */
int                parameters_get_integer(struct parameters *params, const char *key, int def);

/**
 * Set or add a string parameter.
 */
bool               parameters_set_string(struct parameters *params, const char *key, const char *value);

/**
 * Set or add a boolean parameter.
 */
bool               parameters_set_boolean(struct parameters *params, const char *key, bool value);

/**
 * Set or add a integer parameter.
 */
bool               parameters_set_integer(struct parameters *params, const char *key, int value);


#endif /* HAKA_PARAMETERS_H */

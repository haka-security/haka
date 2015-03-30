/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Error handling functions.
 */

#ifndef HAKA_ERROR_H
#define HAKA_ERROR_H

#include <haka/types.h>
#include <haka/compiler.h>



/**
 * Set `error` to be reported to the callers. The function follows the printf API
 * and can be used in multi-threaded application. The error can then be retrieved using
 * clear_error() and check_error().
 *
 * If lua is at the origin of this call, the error will be converted to a lua error.
 */
void error(const char *error, ...) FORMAT_PRINTF(1, 2);

/**
 * Convert the `err` value to a human readable error message.
 *
 * The returned string will be erased by the next call to this function. This function
 * is thread-safe.
 */
const char *errno_error(int err);

/**
 * Check if an error has occurred. This function does not clear error flag.
 */
bool check_error();

/**
 * Get the error message and clear the error state.
 */
const char *clear_error();

#endif /* HAKA_ERROR_H */

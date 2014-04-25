/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Error handling functions.
 */

#ifndef _HAKA_ERROR_H
#define _HAKA_ERROR_H

#include <wchar.h>
#include <haka/types.h>



/**
 * Set an error message to be reported to the callers. The function follows the printf API
 * and can be used in multi-threaded application. The error can then be retrieved using
 * clear_error() and check_error().
 */
void error(const wchar_t *error, ...);

/**
 * Convert the errno value to a human readable error message.
 *
 * The returned string will be erased by the next call to this function. The function
 * thread-safe and can be used in multi-threaded application.
 */
const char *errno_error(int err);

/**
 * Check if an error has occurred. This function does not clear error flag.
 */
bool check_error();

/**
 * Get the error message and clear the error state.
 */
const wchar_t *clear_error();

#endif /* _HAKA_ERROR_H */

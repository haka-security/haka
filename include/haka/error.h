/**
 * @file haka/error.h
 * @brief Error utilities
 * @author Pierre-Sylvain Desse
 *
 * The file contains the error functions.
 */

/**
 * @defgroup Utilities Utilities
 * @brief Utilities functions and structures.
 * @ingroup API_C
 */

#ifndef _HAKA_ERROR_H
#define _HAKA_ERROR_H

#include <wchar.h>
#include <haka/types.h>


/**
 * Set an error message to be reported to the callers.
 * @param error Error formating string
 * @ingroup Utilities
 */
void error(const wchar_t *error, ...);

/**
 * Convert the errno value to a human readable error message.
 * @param err The errno value to convert.
 * @return The matching string. Note that this string will be
 * erased by the next call to this function.
 */
const char *errno_error(int err);

/**
 * Checks if an error has occurred. This function does not clear
 * the error flag.
 * @return true if an error has occurred.
 * @ingroup Utilities
 */
bool check_error();

/**
 * Gets the error message and clear the error state.
 * @return The error message.
 * @ingroup Utilities
 */
const wchar_t *clear_error();


#endif /* _HAKA_ERROR_H */

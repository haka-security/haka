
#ifndef _HAKA_ERROR_H
#define _HAKA_ERROR_H

#include <wchar.h>
#include <haka/types.h>


void error(const wchar_t *error, ...);
const char *errno_error(int err);
bool check_error();
const wchar_t *clear_error();

#endif /* _HAKA_ERROR_H */

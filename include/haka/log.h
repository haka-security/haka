
#ifndef _HAKA_LOG_H
#define _HAKA_LOG_H

#include <wchar.h>


typedef enum {
    LOG_FATAL,
	LOG_ERROR,
	LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,

    LOG_LEVEL_LAST
} log_level;

const char *level_to_str(log_level level);
void message(log_level level, const wchar_t *module, const wchar_t *message);
void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...);

#endif /* _HAKA_LOG_H */


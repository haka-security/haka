
#ifndef _HAKA_LOG_H
#define _HAKA_LOG_H


typedef enum {
    LOG_FATAL,
	LOG_ERROR,
	LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,

    LOG_LEVEL_LAST
} log_level;

const char *level_to_str(log_level level);
void message(log_level level, const char *module, const char *message);
void messagef(log_level level, const char *module, const char *fmt, ...);

#endif /* _HAKA_LOG_H */


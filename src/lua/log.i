%module log
%{
#include <haka/log.h>
%}

%rename(FATAL) LOG_FATAL;
%rename(ERROR) LOG_ERROR;
%rename(WARNING) LOG_WARNING;
%rename(INFO) LOG_INFO;
%rename(DEBUG) LOG_DEBUG;

enum log_level { LOG_FATAL, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG };

void message(log_level level, const char *module, const char *message);

%luacode {
    function log.messagef(level, module, fmt, ...) 
        log.message(level, module, string.format(fmt, unpack(arg)))
    end
}


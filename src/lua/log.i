%module log

%{
#include <haka/log.h>
%}

%include haka/wchar.i
%include haka/swig.i


%rename(FATAL) HAKA_LOG_FATAL;
%rename(ERROR) HAKA_LOG_ERROR;
%rename(WARNING) HAKA_LOG_WARNING;
%rename(INFO) HAKA_LOG_INFO;
%rename(DEBUG) HAKA_LOG_DEBUG;

enum log_level { HAKA_LOG_FATAL, HAKA_LOG_ERROR, HAKA_LOG_WARNING, HAKA_LOG_INFO, HAKA_LOG_DEBUG };

%rename(_message) message;
void message(log_level level, const wchar_t *module, const wchar_t *message);

%luacode {
	function log.message(level, module, fmt, ...)
		haka.log._message(level, module, string.format(fmt, ...))
	end

	function log.fatal(module, fmt, ...)
		haka.log.message(haka.log.FATAL, module, fmt, ...)
	end

	function log.error(module, fmt, ...)
		haka.log.message(haka.log.ERROR, module, fmt, ...)
	end

	function log.warning(module, fmt, ...)
		haka.log.message(haka.log.WARNING, module, fmt, ...)
	end

	function log.info(module, fmt, ...)
		haka.log.message(haka.log.INFO, module, fmt, ...)
	end

	function log.debug(module, fmt, ...)
		haka.log.message(haka.log.DEBUG, module, fmt, ...)
	end

	getmetatable(log).__call = function (_, module, fmt, ...)
		haka.log.info(module, fmt, ...)
	end
}


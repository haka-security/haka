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
		log._message(level, module, string.format(fmt, unpack(arg)))
	end

	function log.fatal(module, fmt, ...)
		log.message(log.FATAL, module, fmt, unpack(arg))
	end

	function log.error(module, fmt, ...)
		log.message(log.ERROR, module, fmt, unpack(arg))
	end

	function log.warning(module, fmt, ...)
		log.message(log.WARNING, module, fmt, unpack(arg))
	end

	function log.info(module, fmt, ...)
		log.message(log.INFO, module, fmt, unpack(arg))
	end

	function log.debug(module, fmt, ...)
		log.message(log.DEBUG, module, fmt, unpack(arg))
	end

	getmetatable(log).__call = function (_, module, fmt, ...)
		log.info(module, fmt, unpack(arg))
	end
}


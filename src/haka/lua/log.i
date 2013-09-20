%module log

%{
#include <haka/log.h>

static void setlevel1(const wchar_t *module, log_level level)
{
	setlevel(level, module);
}

static void setlevel0(log_level level)
{
	setlevel(level, NULL);
}
%}

%include "haka/lua/wchar.si"
%include "haka/lua/swig.si"


%rename(FATAL) HAKA_LOG_FATAL;
%rename(ERROR) HAKA_LOG_ERROR;
%rename(WARNING) HAKA_LOG_WARNING;
%rename(INFO) HAKA_LOG_INFO;
%rename(DEBUG) HAKA_LOG_DEBUG;

enum log_level { HAKA_LOG_FATAL, HAKA_LOG_ERROR, HAKA_LOG_WARNING, HAKA_LOG_INFO, HAKA_LOG_DEBUG };

%rename(_message) message;
void message(log_level level, const wchar_t *module, const wchar_t *message);

%rename(setlevel) setlevel1;
void setlevel1(const wchar_t *module, log_level level);

%rename(setlevel) setlevel0;
void setlevel0(log_level level);

%luacode {
	local this = unpack({...})

	function this.message(level, module, fmt, ...)
		this._message(level, module, string.format(fmt, ...))
	end

	function this.fatal(module, fmt, ...)
		this.message(this.FATAL, module, fmt, ...)
	end

	function this.error(module, fmt, ...)
		this.message(this.ERROR, module, fmt, ...)
	end

	function this.warning(module, fmt, ...)
		this.message(this.WARNING, module, fmt, ...)
	end

	function this.info(module, fmt, ...)
		this.message(this.INFO, module, fmt, ...)
	end

	function this.debug(module, fmt, ...)
		this.message(this.DEBUG, module, fmt, ...)
	end

	getmetatable(this).__call = function (_, module, fmt, ...)
		this.info(module, fmt, ...)
	end
}

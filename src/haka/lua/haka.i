%module haka

%{
#include <stdint.h>
#include <wchar.h>

#include "app.h"
#include "state.h"
#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/module.h>
#include <haka/stream.h>
#include <haka/config.h>

char *module_prefix = HAKA_MODULE_PREFIX;
char *module_suffix = HAKA_MODULE_SUFFIX;

%}

%include "haka/lua/swig.si"
%include "haka/lua/stream.si"
%include "time.si"

%nodefaultctor;
%nodefaultdtor;

%rename(current_thread) thread_get_id;
int thread_get_id();

%rename(module_path) module_get_path;
const char *module_get_path();

%immutable;
char *module_prefix;
char *module_suffix;

struct stream {
};
BASIC_STREAM(stream)

struct time_lua {
	int    seconds;
	int    micro_seconds;

	~time_lua() {
		free($self);
	}
};

%luacode {
	haka = unpack({...})

	function string.split(str, delim)
		local ret = {}
		local last_end = 1
		local s, e = str:find(delim, 1)

		while s do
			cap = str:sub(last_end, e-1)
			table.insert(ret, cap)

			last_end = e+1
			s, e = str:find(delim, last_end)
		end

		if last_end <= #str then
			cap = str:sub(last_end)
			table.insert(ret, cap)
		end

		return ret
	end

	local function addpath(dst, paths, exts)
		local pathtable = string.split(dst, ';')

		for _, path in pairs(string.split(paths, ';')) do
			for _, ext in pairs(exts) do
				local p = string.gsub(path, '*', ext)
				table.insert(pathtable, p)
			end
		end

		return table.concat(pathtable, ';')
	end

	haka._on_exit = {}

	function haka._exiting()
		for _, f in pairs(haka._on_exit) do
			f()
		end
	end

	function haka.on_exit(func)
		table.insert(haka._on_exit, func)
	end

	package.cpath = addpath(package.cpath, haka.module_path(), { haka.module_prefix .. '?' .. haka.module_suffix })
	package.path = addpath(package.path, haka.module_path(), { '?.bc', '?.lua' })

	require('rule')
}

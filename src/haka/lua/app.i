%module app
%{
#include <stdint.h>
#include <wchar.h>

#include "app.h"
#include "state.h"
#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/config.h>

%}

%include "haka/lua/swig.si"

void exit(int);

%rename(current_thread) thread_get_id;
int thread_get_id();

%luacode {
	app._on_exit = {}

	function app._exiting()
		for _, f in pairs(haka.app._on_exit) do
			f()
		end
	end

	function haka.on_exit(func)
		table.insert(haka.app._on_exit, func)
	end
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module luadebug

%{
	#include <haka/luadebug/debugger.h>
	#include <haka/luadebug/interactive.h>
	#include "luadebug/debugger.h"

	#define lua_luadebug_interactive_enter(single, multi, msg) luadebug_interactive_enter(L, single, multi, msg, 0, NULL)

	#define lua_luadebug_debugger_start(break_immediatly) luadebug_debugger_start(L, break_immediatly)
	#define lua_luadebug_debugger_stop()                  luadebug_debugger_stop(L)
	#define lua_luadebug_debugger_break()                 luadebug_debugger_break(L)


	void lua_pushpdebugger(lua_State *L, struct luadebug_debugger *dbg)
	{
		SWIG_NewPointerObj(L,dbg,SWIGTYPE_p_luadebug_debugger,1);
	}

	struct luadebug_debugger *lua_getpdebugger(lua_State *L, int index)
	{
		struct luadebug_debugger *dbg = NULL;
		if (!SWIG_IsOK(SWIG_ConvertPtr(L, index, (void**)&dbg, SWIGTYPE_p_luadebug_debugger, 0))) {
			return NULL;
		}
		return dbg;
	}
%}

%include "haka/lua/swig.si";

%nodefaultctor;
%nodefaultdtor;

%rename(__interactive_enter) lua_luadebug_interactive_enter;
void lua_luadebug_interactive_enter(const char *single, const char *multi, const char *msg);

%rename(__debugger) luadebug_debugger;
struct luadebug_debugger {
	%extend {
		luadebug_debugger() {
			return NULL;
		}

		~luadebug_debugger() {
			luadebug_debugger_cleanup($self);
		}
	}
};

%rename(__debugger_start) lua_luadebug_debugger_start;
void lua_luadebug_debugger_start(bool break_immediatly=false);

%rename(__debugger_stop) lua_luadebug_debugger_stop;
void lua_luadebug_debugger_stop();

%rename(__debugger_break) lua_luadebug_debugger_break;
void lua_luadebug_debugger_break();

%luacode {
	local this = unpack({...})
	local color = require("color")
	local class = require('class')

	function debug.hide_underscore(name, value)
		if type(name) == "string" then
			return name:sub(1, 1) == "_"
		else
			return false
		end
	end

	function debug.hide_function(name, value)
		if type(value) == "function" then
			return true
		else
			return false
		end
	end

	function debug.__printwrapper(p, data)
		return function (...)
			p(data, ...)
		end
	end

	local function _hidden(key, value, hidden)
		for _,i in pairs(hidden) do
			if i(key, value) then
				return true
			end
		end
		return false
	end

	local __pprint

	local function __pprint_swig(obj, indent, name, visited, hidden, depth, out, title, meta)
		title = table.concat({title, color.cyan, color.bold, "userdata", color.clear})

		if meta[".type"] then
			title = table.concat({title, " ", tostring(meta[".type"])})
		end

		if meta[".fn"] and meta[".fn"].__tostring then
			out(title, tostring(obj))
		else
			if depth == 0 then
				out(title)
			else
				local has_value = false

				for key, _ in sorted_pairs(meta[".get"]) do
					local success, child_obj = pcall(function () return obj[key] end)
					if not _hidden(key, child_obj, hidden) then
						if not has_value then
							out(title, "{")
							has_value = true
						end

						if success then
							__pprint(child_obj, indent .. "  ", key, visited, hidden, depth-1, out)
						else
							local error = table.concat({indent, "  ", color.blue, color.bold, key, color.clear, " : ", color.red, child_obj, color.clear})
							out(error)
						end
					end
				end

				if has_value then
					out(indent .. "}")
				else
					out(title)
				end
			end
		end
	end
	local function __pprint_cdata(obj, indent, name, visited, hidden, depth, out, title, meta)
		title = table.concat({title, color.cyan, color.bold, "cdata", color.clear})

		if meta.mt.__tostring then
			out(title, tostring(obj))
		else
			if depth == 0 then
				out(title)
			else
				local has_value = false

				for key, prop in sorted_pairs(meta.prop) do
					local success, child_obj = pcall(function () return obj[key] end)
					if not _hidden(key, child_obj, hidden) then
						if not has_value then
							out(title, "{")
							has_value = true
						end

						if success then
							__pprint(child_obj, indent .. "  ", key, visited, hidden, depth-1, out)
						else
							local error = table.concat({indent, "  ", color.blue, color.bold, key, color.clear, " : ", color.red, child_obj, color.clear})
							out(error)
						end
					end
				end

				if has_value then
					out(indent .. "}")
				else
					out(title)
				end
			end
		end
	end

	local function __print_table(obj, indent, name, visited, hidden, depth, out, title)
		title = table.concat({title, color.cyan, color.bold, "table", color.clear})
		if depth == 0 then
			out(title)
		else
			out(title, "{")

			for key, value in sorted_pairs(obj) do
				if not _hidden(key, value, hidden) then
					__pprint(value, indent .. "  ", tostring(key), visited, hidden, depth-1, out)
				end
			end

			out(indent .. "}")
		end
	end

	local function __print_class(obj, indent, name, visited, hidden, depth, out, title, meta)
		title = table.concat({title, color.cyan, color.bold, "class", color.clear})

		title = table.concat({title, " ", tostring(meta.name)})

		if depth == 0 then
			out(title)
		elseif obj.__pprint then
			obj:__pprint(indent, out)
		else
			out(title, "{")

			local vars = {}

			for key, value in sorted_pairs(obj) do
				if key ~= '__property' and (not _hidden(key, value, hidden)) then
					__pprint(value, indent .. "  ", tostring(key), visited, hidden, depth-1, out)
					vars[key] = true
				end
			end

			local property = rawget(obj, '__property')
			if property then
				for key, _ in sorted_pairs(property) do
					local success, child_obj = pcall(function () return obj[key] end)
					if not vars[key] and (not _hidden(key, child_obj, hidden)) then
						vars[key] = true
						if success then
							__pprint(child_obj, indent .. "  ", key, visited, hidden, depth-1, out)
						else
							local error = table.concat({indent, "  ", color.blue, color.bold, key, color.clear, " : ", color.red, child_obj, color.clear})
							out(error)
						end
					end
				end
			end

			for key, _ in sorted_pairs(meta.property) do
				local success, child_obj = pcall(function () return obj[key] end)
				if not vars[key] and (not _hidden(key, child_obj, hidden)) then
					vars[key] = true
					if success then
						__pprint(child_obj, indent .. "  ", key, visited, hidden, depth-1, out)
					else
						local error = table.concat({indent, "  ", color.blue, color.bold, key, color.clear, " : ", color.red, child_obj, color.clear})
						out(error)
					end
				end
			end

			out(indent .. "}")
		end
	end

	__pprint = function(obj, indent, name, visited, hidden, depth, out)
		local type = type(obj)

		local title
		if name then
			title = table.concat({indent, color.blue, color.bold, name, color.clear, " : "})
		else
			title = ""
		end

		if type == "userdata" or type == "table" or type == "cdata" then
			if visited[obj] then
				out(table.concat({title, color.red, "recursive value", color.clear}))
				return
			end

			visited[obj] = true
		end

		if type == "table" then
			local meta = getmetatable(obj)
			if meta and class.isclass(meta) then
				__print_class(obj, indent, name, visited, hidden, depth, out, title, meta)
			else
				__print_table(obj, indent, name, visited, hidden, depth, out, title)
			end
		elseif type == "userdata" then
			local meta = getmetatable(obj)
			if meta and meta[".type"] and meta[".get"] then
				__pprint_swig(obj, indent, name, visited, hidden, depth, out, title, meta)
			else
				out(table.concat({title, color.cyan, color.bold, "userdata", color.clear}))
			end
		elseif type == "cdata" then
			local success, meta = pcall(function () return obj[".meta"] end)
			if success and meta then
				__pprint_cdata(obj, indent, name, visited, hidden, depth, out, title, meta)
			else
				out(table.concat({title, color.cyan, color.bold, "cdata", color.clear}))
			end
		elseif type == "function" then
			out(table.concat({title, color.cyan, color.bold, "function", color.clear}))
		elseif type == "string" then
			out(table.concat({title, color.magenta, color.bold, "\"", obj, "\"", color.clear}))
		elseif type == "boolean" then
			out(table.concat({title, color.magenta, color.bold, tostring(obj), color.clear}))
		elseif type == "thread" then
			out(table.concat({title, color.cyan, color.bold, "thread", color.clear}))
		else
			out(title .. tostring(obj))
		end

		if type == "userdata" or type == "table" then
			visited[obj] = nil
		end
	end

	function debug.pprint(obj, indent, depth, hide, out)
		if not hide then
			hide = {}
		elseif type(hide) == "function" then
			hide = { hide }
		end

		if num then
			__pprint(obj, indent or "", nil, {}, hide, depth or -1, out or print)
		else
			__pprint(obj, indent or "", nil, {}, hide, depth or -1, out or print)
		end
	end

	debug.debugger = {}
	debug.debugger.start = this.__debugger_start
	debug.debugger.stop = this.__debugger_stop
	debug.breakpoint = this.__debugger_break

	debug.interactive = {}
	debug.interactive.enter = this.__interactive_enter
}

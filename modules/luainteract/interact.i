%module luainteract

%{
	#include "session.h"

	#define new_luainteract_session()   luainteract_session_create(L)
%}

%rename(session) luainteract_session;

struct luainteract_session {
	%extend {
		luainteract_session();

		~luainteract_session() {
			luainteract_session_cleanup($self);
		}

		void start() {
			luainteract_session_enter($self);
		}

		void setprompt(const char *single, const char *multi) {
			luainteract_session_setprompts($self, single, multi);
		}
	}
};

%luacode {
	haka.session = luainteract.session()

	local color = require("haka/color")

	local function __hidden(name)
		if type(name) == "string" then
			return name:sub(1, 1) == "_"
		else
			return false
		end
	end

	local function __pprint(obj, indent, name, visited, hidden)
		local type = type(obj)

		local title
		if name then
			title = indent .. " " .. color.blue .. color.bold .. name .. color.clear .. " :"
		else
			title = ""
		end

		if type == "userdata" or type == "table" then
			if visited[obj] then
				print(title, color.red .. "recursive value" .. color.clear)
				return
			end

			visited[obj] = true
		end

		if type == "table" then
			print(title, color.cyan .. color.bold .. "table" .. color.clear, "{")

			for key, value in pairs(obj) do
				if not hidden or not hidden(key) then
					__pprint(value, indent .. "  ", key, visited, hidden)
				end
			end

			print(indent, "}")
		elseif type == "userdata" then
			local meta = getmetatable(obj)
			if meta then
				title = title .. " " .. color.cyan .. color.bold .. "userdata" .. color.clear

				if meta[".fn"] and meta[".fn"].__tostring then
					print(title, tostring(obj))
				else
					local has_value = false

					for key, _ in pairs(meta[".get"]) do
						if not hidden or not hidden(key) then
							if not has_value then
								print(title, "{")
								has_value = true
							end

							__pprint(obj[key], indent .. "  ", key, visited, hidden)
						end
					end

					if has_value then
						print(indent, "}")
					elseif meta[".type"] then
						print(title, meta[".type"])
					else
						print(title)
					end
				end
			else
				print(title, color.cyan .. color.bold .. "userdata" .. color.clear)
			end
		elseif type == "function" then
			print(title, color.cyan .. color.bold .. "function" .. color.clear)
		elseif type == "string" then
			print(title, color.magenta .. color.bold .. "\"" .. obj .. "\"" .. color.clear)
		elseif type == "boolean" then
			print(title, color.magenta .. color.bold .. tostring(obj) .. color.clear)
		elseif type == "thread" then
			print(title, color.cyan .. color.bold .. "thread" .. color.clear)
		else
			print(title, obj)
		end

		visited[obj] = nil
	end

	function luainteract.pprint(obj, num)
		if num then
			__pprint(obj, "", tostring(num), {}, __hidden)
		else
			__pprint(obj, "", nil, {}, __hidden)
		end
	end

	function luainteract.interactive_rule(self, input)
		haka.log("luainteract", "entering interactive rule")
		luainteract.pprint(input, "input")
		haka.session:setprompt(color.green .. self.hook .. color.bold .. ">  " .. color.clear,
			color.green .. self.hook .. color.bold .. ">> " .. color.clear)
		haka.session:start()
		haka.log("luainteract", "continue")
	end
}

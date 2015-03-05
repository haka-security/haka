-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local log = haka.log_section("grammar")

local module = class.class("CComp")

local suffix = "_grammar"

function module.method:__init(name, _debug)
	self._swig = haka.config.ccomp.swig
	self._name = name..suffix
	self._nameid = self._name.."_"..haka.genuuid():gsub('-', '_')
	self._debug = _debug or false
	self._cfile = haka.config.ccomp.runtime_dir..self._nameid..".c"
	self._sofile = haka.config.ccomp.runtime_dir..self._nameid..".so"
	self._store = {} -- Store some lua object to access it from c

	-- Open and init c file
	log.debug("generating c code at '%s'", self._cfile)
	self._fd = assert(io.open(self._cfile, "w"))

	-- Create c grammar
	self._tmpl = require("grammar_c")
	self._ctx = {
		-- Expose some of our upvalue
		class = class,
		-- Configure current template
		name = name,
		nameid = self._nameid,
		_swig = haka.config.ccomp.swig,
		_parsers = {},
	}

	self.waitcall = self:store(function (ctx)
		ctx.iter:wait()
	end)
end

local function register(parser, node)
	local id = parser.nodes[node]
	if id then
		return id
	end
	parser.nodes_count = parser.nodes_count + 1
	parser.nodes[node] = parser.nodes_count
	return parser.nodes_count
end

function module.method:add_parser(name, dgraph)
	self._ctx._parsers[#self._ctx._parsers + 1] = {
		name = name,
		fname = "parse_"..name,
		dgraph = dgraph,
		nodes = {}, -- Store all encountered nodes
		nodes_count = 0, -- Count of encountered nodes
		written_nodes = {}, -- Store written nodes
		register = register,
	}
end

function module.method:store(func)
	assert(func, "cannot store nil")
	assert(type(func) == 'function', "cannot store non function type")
	local id = #self._store + 1

	self._store[id] = func
	return id
end

function module.method:call(id, name)
	self:write([[
			call = %d;                                /* %s */
]], id, name)
end

local function escape_string(str)
	local esc = str:gsub("([\\%%\'\"])", "\\%1")
	return esc
end

function module.method:trace_node(node, desc)
	local id = self._parser.nodes[node]

	self:write([[
#ifdef HAKA_DEBUG_GRAMMAR
			{
				char dump[21];
				char dump_safe[81];
				struct vbuffer_sub sub;
				vbuffer_sub_create_from_position(&sub, ctx->iter, 20);
				safe_string(dump_safe, dump, vbuffer_asstring(&sub, dump, 21));

				LOG_DEBUG(grammar, "in rule '%%s' field %%s gid %d: %%s\n\tat byte %%d: %%s...",
					node_debug_%s[%d-1].rule, node_debug_%s[%d-1].id, "%s", ctx->iter->meter, dump_safe);
			}
#endif
]], node.gid, self._parser.name, id, self._parser.name, id, escape_string(desc))
end

function module.method:apply_node(node)
	assert(self._parser, "cannot apply node without started parser")
	assert(node)

	self:call(self:store(function (ctx)
		node:_apply(ctx)
	end), "node:_apply(ctx)")
end

function module.method:mark(readonly)
	readonly = readonly and "true" or "false"
	self:write([[
			parse_ctx_mark(ctx, %s);
]], readonly)
end

function module.method:unmark()
	self:write([[
			parse_ctx_unmark(ctx);
]])
end

local numtab={}
for i=0,255 do
	numtab[string.char(i)]=("%3d,"):format(i)
end

local function lua2c(lua)
	local content = string.dump(assert(load(lua)))
	return (content:gsub(".", numtab):gsub(("."):rep(80), "%0\n"))
end

function module.method:compile()
	local binding = [[
local parse_ctx = require("parse_ctx")
]]
	if haka.config.ccomp.ffi then

		binding = binding..[[
local ffibinding = require("ffibinding")
local lib = ffibinding.load[[
]]

		-- Expose all parser
		for _, value in pairs(self._ctx._parsers) do
			binding = binding..string.format("int parse_%s(struct parse_ctx *ctx);\n", value.name)
		end

		binding = binding.."]]"
	else
		binding = binding..[[
local lib = unpack({...})
]]
	end

	binding = binding..[[

return { ctx = parse_ctx, grammar = lib }
]]

	self._ctx.binding_code = binding
	self._ctx.binding_bytecode = lua2c(binding)
	self:write(self._tmpl.render(self._ctx))
	self._fd:close()

	-- Compile c grammar
	local flags = string.format("%s -I%s -I%s/haka/lua/", haka.config.ccomp.flags, haka.config.ccomp.include_path, haka.config.ccomp.include_path)
	if self._debug then
		flags = flags.." -DHAKA_DEBUG_GRAMMAR"
	end
	local compile_command = string.format("%s %s -o %s %s", haka.config.ccomp.cc, flags, self._sofile, self._cfile)
	log.info("compiling grammar '%s'", self._name)
	log.debug(compile_command)
	local ret = os.execute(compile_command)
	if not ret then
		error("grammar compilation failed `"..compile_command.."`")
	end

	return self._nameid
end

function module.method:write(string, ...)
	assert(self._fd, "uninitialized template")
	assert(self._fd:write(string.format(string, ...)))
end

return module

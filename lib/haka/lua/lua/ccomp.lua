-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local log = haka.log_section("grammar")

local module = class.class("CComp")

local suffix = "_grammar"

local function escape_string(str)
	local esc = str:gsub("([\\%%\'\"])", "\\%1")
	return esc
end

local function debug_symbols(parser)
	local id2node = {}

	for node, id in pairs(parser.nodes) do
		id2node[id] = node
	end

	return id2node
end

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
		escape_string = escape_string,
		grammar_dg = require('grammar_dg'),
		-- Configure current template
		name = name,
		nameid = self._nameid,
		_swig = haka.config.ccomp.swig,
		_parsers = {},
		ccomp = self,
		debug_symbols = debug_symbols,
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
	self._fd:write(self._tmpl.render(self._ctx))
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

return module

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')
local http = require('protocol/http')

local function count_table(tab)
	local index = 0
	for k, _ in pairs(tab) do
		index = index + 1
	end
	return index
end

local function dump_table(tab)
	for k, v in pairs(tab) do
		if type(v) == 'table' then
			dump_table(v)
		else
			print('[' .. k .. '] = [' .. v .. ']')
		end
	end
end

local function compare_table(tab1, tab2)
	local verif = true
	if (tab1 and tab2) then
		if type(tab1) ~= 'table' or type(tab2) ~= 'table' then return false end
		if count_table(tab1) ~= count_table(tab2) then return false end
		for k, v in pairs(tab1) do
			if type(v) == 'table' then
				verif = compare_table(v, tab2[k])
			elseif tab2[k] ~= v then
				return false
			end
		end
		return verif
	else
		return false
	end
end

TestUriSplit = {}

local _test = {
	['test1'] = {
		'HTTP://example.com/foo',
		{
			scheme = 'HTTP',
			authority = 'example.com',
			path = '/foo',
			host = 'example.com',
		}
	},
	['test2'] = {
		'http://myname:mypass@www.example.com:8888/a/b/page?a=1&b=2#frag',
		{
			scheme = 'http',
			authority = 'myname:mypass@www.example.com:8888',
			host = 'www.example.com',
			user = 'myname',
			pass = 'mypass',
			port = '8888',
			args = {a = '1', b = '2'},
			path = '/a/b/page',
			fragment = 'frag',
			userinfo = 'myname:mypass',
			query = 'a=1&b=2',
		}
	},
	['test3'] = {
		'www.example.com:8888/foo',
		{
			authority = 'www.example.com:8888',
			host = 'www.example.com',
			port = '8888',
			path = '/foo',
		}
	},
	['test4'] = {
		'/page?a=1',
		{
			args = {a = '1'},
			path = '/page',
			query = 'a=1',
		}
	},
	['test5'] = {
		'/page?a=1&b=2',
		{
			args = {a = '1', b='2'},
			path = '/page',
			query = 'a=1&b=2',
		}
	},
	['test6'] = {
		'/page?a=1&a=2',
		{
			args = {a = '1', a='2'},
			path = '/page',
			query = 'a=1&a=2',
		}
	},
	['test7'] = {
		'http://www.example.com/#',
		{
			scheme = 'http',
			authority = 'www.example.com',
			host = 'www.example.com',
			path = '/',
		}
	},
	['test8'] = {
		'/',
		{
			path = '/',
		}
	},
	['test9'] = {
		'www.example.com/',
		{
			authority = 'www.example.com',
			host = 'www.example.com',
			path = '/',
		}
	}
}

for k, v in pairs(_test) do
	TestUriSplit[k] = function (self)
		local split = http.uri.split(v[1])
		assertTrue(compare_table(split, v[2]))
	end
end

TestCookieSplit = {}

local _cookie = {
	['test1'] = { 'a=3;b=5', {a = '3', b='5'} },
	['test2'] = { 'a=3', {a = '3'} },
	['test3'] = { 'login=administrateur;password=pass;utm__z=0124645648645646', {login = 'administrateur', password='pass', utm__z='0124645648645646'} },
	['test4'] = { '', { } },
	['test5'] = { 'a=2%3bb=c', { a = '2%3bb=c'} },
	['test6'] = { 'a=t t;b=c', { a = 't t'; b='c'} },
	['test7'] = { 'a=2;;b=3', {a='2', b='3'} },
	['test8'] = { ';a=2', {a = '2'} },
	['test9'] = { 'a=2;', {a = '2'} }
}

for k, v in pairs(_cookie) do
	TestCookieSplit[k] = function (self)
		local split = http.cookies.split(v[1])
		assertTrue(compare_table(split, v[2]))
	end
end

LuaUnit:setVerbosity(1)
assert(LuaUnit:run('TestUriSplit', 'TestCookieSplit') == 0)

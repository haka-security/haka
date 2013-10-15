local http = require('protocol/http')

local _test = {
	['HTTP://example.com/foo'] = {
		scheme = 'HTTP',
		authority = 'example.com',
		path = '/foo',
		host = 'example.com',
	},

	['http://myname:mypass@www.example.com:8888/a/b/page?a=1&b=2#frag'] = {
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
	},

	['www.example.com:8888/foo'] = {
		authority = 'www.example.com:8888',
		host = 'www.example.com',
		port = '8888',
		path = '/foo',
	},

	['/page?a=1'] = {
		args = {a = '1'},
		path = '/page',
		query = 'a=1',
	},

	['/page?a=1&b=2'] = {
		args = {a = '1', b='2'},
		path = '/page',
		query = 'a=1&b=2',
	},

	['/page?a=1&a=2'] = {
		args = {a = '1', a='2'},
		path = '/page',
		query = 'a=1&a=2',
	},

	['http://www.example.com/#'] = {
		scheme = 'http',
		authority = 'www.example.com',
		host = 'www.example.com',
		path = '/',
	},

	['/'] = {
		path = '/',
	},

	['www.example.com/'] = {
		authority = 'www.example.com',
		host = 'www.example.com',
		path = '/',
	}
}

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


for k, v in pairs(_test) do
	local split = http.uri.split(k)
	print('splitting uri: ', k)
	dump_table(split)
	if compare_table(split, v) then
	else
		print('----> expected:')
		dump_table(v)
		print('----> got:')
		dump_table(split)
		error('missing and/or extra components while splitting uri')
	end
end

local _cookie = {
	[ 'a=3;b=5' ] = 
		{a = '3', b='5'},

	[ 'a=3' ] = 
		{a = '3'},

	[ 'login=administrateur;password=pass;utm__z=0124645648645646' ] =
		{login = 'administrateur', password='pass', utm__z='0124645648645646'},

	[ '' ] = 
		{ },

	[ 'a=2%3bb=c' ] = 
		{ a = '2%3bb=c'},

	[ 'a=t t;b=c' ] = 
		{ a = 't t'; b='c'},

	[ 'a=2;;b=3' ] = 
		{a='2', b='3'},

	[ ';a=2' ] = 
		{a = '2'},

	[ 'a=2;' ] = 
		{a = '2'}

}

for k, v in pairs(_cookie) do
	local split = http.cookies.split(k)
	print('splitting cookie:', k)
	dump_table(split)
	if compare_table(split, v) then
	else
		print('----> expected:')
		dump_table(v)
		print('----> got:')
		dump_table(split)
		error('missing and/or extra components while splitting cookie')
	end
end

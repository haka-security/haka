local http = require('protocol/http')

local _test = {{'HTTP://example.com/foo',
	               {scheme = 'HTTP',
				    authority = 'example.com',
					path = '/foo',
					host = 'example.com'}
			        },
	           {'http://myname:mypass@www.example.com:8888/a/b/page?a=1&b=2#frag',
			       {scheme = 'http',
				    authority = 'myname:mypass@www.example.com:8888',
					host = 'www.example.com',
			        user = 'myname',
					pass = 'mypass',
					port = '8888',
					args = {a = '1', b = '2'},
					path = '/a/b/page',
					fragment = 'frag',
					userinfo = 'myname:mypass',
					query = 'a=1&b=2'}
	}
}

function dump_table(tab)
	for k, v in pairs(tab) do
		if type(v) == 'table' then
			dump_table(v)
		else
			print('[' .. k .. '] = [' .. v .. ']')
		end
	end
end

function compare_table(tab1, tab2)
	verif = true
	if (tab1 and tab2) then
		if type(tab1) ~= 'table' or type(tab2) ~= 'table' then return false end
		if #tab1 ~= #tab2 then return false end
		for k, v in pairs(tab1) do
			if type(v) == 'table' then
				verif = compare_table(v, tab2[k])
			elseif (tab2[k] ~= v) then
				return false
			end
		end
		return verif
	else
		return false
	end
end


for _, uri in pairs(_test) do
	local split = http.uri.split(uri[1])
	io.write('splitting uri: ', uri[1])
	if compare_table(split, uri[2]) then
		print(' (success)')
	else
		print(' (failed)')
		print('----> expected:')
		dump_table(uri[2])
		print('----> got:')
		dump_table(split)
		error('')
	end
end

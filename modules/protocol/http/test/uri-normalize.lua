-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local http = require('protocol/http')

local _test = {
	['HTTP://example.com/'] =  'http://example.com/',
    ['http://EXAMPLE.COM/'] = 'http://example.com/',
    ['http://example.com/%7Efoo'] = 'http://example.com/~foo',
    ['http://example.com/?q=1%2f2'] = 'http://example.com/?q=1%2F2',
    ['http://example.com/a/./b'] = 'http://example.com/a/b',
    ['http://example.com/../a/b'] = 'http://example.com/a/b',
    ['http://example.com/dir../a'] = 'http://example.com/dir../a',
    ['http://example.com/.dir/a'] = 'http://example.com/.dir/a',
    ['http://example.com/a/%2e/b'] = 'http://example.com/a/b',
    ['http://example.com/..../a'] = 'http://example.com/..../a',
    ['http://example.com/..dir/a'] = 'http://example.com/..dir/a',
    ['http://example.com/%2e%2e/a/b'] = 'http://example.com/a/b',
    ['http://example.com/.../a'] = 'http://example.com/.../a',
    ['http://example.com/a/../a/b'] = 'http://example.com/a/b',
    ['http://example.com/.%2e/a/b'] = 'http://example.com/a/b',
    ['http://example.com/%2e./a/b'] = 'http://example.com/a/b',
    ['http://example.com/../../../../../a/b'] = 'http://example.com/a/b',
    ['http://example.com/../../a/../../../a/b'] = 'http://example.com/a/b',
    ['http://example.com/z/../../../x/y'] = 'http://example.com/x/y',
    ['http://example.com'] = 'http://example.com/',
    ['http://example.com/ABCD/ef/'] = 'http://example.com/ABCD/ef/',
    ['http://example.com:80/'] = 'http://example.com/',
    ['htTp://wWw.foo.coM:80/foo/./baz/../bar/%7Eb%61r?test=1#noise'] = 'http://www.foo.com/foo/bar/~bar?test=1#noise',
    ['http://USER:pass@www.Example.COM/foo/bar'] = 'http://USER:pass@www.example.com/foo/bar',
    ['http://www.w3.org/2000/01/rdf-schema'] = 'http://www.w3.org/2000/01/rdf-schema',
    ['www.example.com/foo/bar'] = 'http://www.example.com/foo/bar',
    ['/'] = '/',

	-- check normalisation errors
	[''] =  '',
	['://'] = '',
	['#'] = '',
	[':@'] = '/',
	['http:///'] = '/',
	['http://http://'] = 'http://http://'
}

for k, v in pairs(_test) do
	local normalized = http.uri.normalize(k)
	print('normal =', '[' ..normalized .. ']')
	print('normalizing uri : ' .. k)
	if v ~= normalized then
		error('\nfailed to normalize: ' .. k .. '\nexpected: ' .. v .. ' got ' .. normalized)
	end
end


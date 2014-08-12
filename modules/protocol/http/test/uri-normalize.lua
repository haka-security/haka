-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestHttpNormalize = {}

function TestHttpNormalize:setUp()
	self.http = require('protocol/http')
end

local _tests = {
	['test_normalize_should_lower_case_scheme'] = { 'HTTP://example.com/', 'http://example.com/' },
	['test_normalize_should_lower_case_domain'] = { 'http://EXAMPLE.COM/', 'http://example.com/' },
	['test_normalize_should_expand_ecaped_chars'] = { 'http://example.com/%7Efoo', 'http://example.com/~foo' },
	['test_normalize_should_upper_case_escaped_chars'] = { 'http://example.com/?q=1%2f2', 'http://example.com/?q=1%2F2' },
	['test_normalize_should_resolv_current_path'] = { 'http://example.com/a/./b', 'http://example.com/a/b' },
	['test_normalize_should_resolv_parent_path'] = { 'http://example.com/../a/b', 'http://example.com/a/b' },
	['test_normalize_should_not_resolv_parent_path_that_are_beyond_root'] = { 'http://example.com/dir../a', 'http://example.com/dir../a' },
	['test_normalize_should_not_resolv_invalid_path'] = { 'http://example.com/.dir/a', 'http://example.com/.dir/a' },
	['test_normalize_should_not_resolv_invalid_path_2'] = { 'http://example.com/a/%2e/b', 'http://example.com/a/b' },
	['test_normalize_should_not_resolv_invalid_path_3'] = { 'http://example.com/..../a', 'http://example.com/..../a' },
	['test_normalize_should_not_resolv_invalid_path_4'] = { 'http://example.com/..dir/a', 'http://example.com/..dir/a' },
	['test_normalize_should_not_resolv_invalid_path_5'] = { 'http://example.com/%2e%2e/a/b', 'http://example.com/a/b' },
	['test_normalize_should_expand_escaped_chars_and_resolv_current_path'] = { 'http://example.com/.../a', 'http://example.com/.../a' },
	['test_normalize_should_expand_escaped_chars_and_resolv_parent_path'] = { 'http://example.com/a/../a/b', 'http://example.com/a/b' },
	['test_normalize_should_expand_escaped_chars_and_resolv_parent_path_2'] = { 'http://example.com/.%2e/a/b', 'http://example.com/a/b' },
	['test_normalize_should_expand_escaped_chars_and_resolv_parent_path_3'] = { 'http://example.com/%2e./a/b', 'http://example.com/a/b' },
	['test_normalize_should_resolv_multiple_parent_path'] = { 'http://example.com/../../../../../a/b', 'http://example.com/a/b' },
	['test_normalize_should_resolv_multiple_parent_path_2'] = { 'http://example.com/../../a/../../../a/b', 'http://example.com/a/b' },
	['test_normalize_should_resolv_multiple_parent_path_3'] = { 'http://example.com/z/../../../x/y', 'http://example.com/x/y' },
	['test_normalize_should_always_add_slash_to_end_of_uri'] = { 'http://example.com', 'http://example.com/' },
	['test_normalize_should_keep_case_for_path'] = { 'http://example.com/ABCD/ef/', 'http://example.com/ABCD/ef/' },
	['test_normalize_should_drop_default_port'] = { 'http://example.com:80/', 'http://example.com/' },
	['test_normalize_should_do_its_job_with_complexe_uri'] = { 'htTp://wWw.foo.coM:80/foo/./baz/../bar/%7Eb%61r?test=1#noise', 'http://www.foo.com/foo/bar/~bar?test=1#noise' },
	['test_normalize_should_not_fail_with_user_and_password'] = { 'http://USER:pass@www.Example.COM/foo/bar', 'http://USER:pass@www.example.com/foo/bar' },
	['test_normalize_should_add_http_scheme'] = { 'www.example.com/foo/bar', 'http://www.example.com/foo/bar' },
	['test_normalize_should_do_nothing_on_path_only'] = { '/', '/' },
	['test_normalize_should_not_fail_on_error'] = { '', '' },
	['test_normalize_should_not_fail_on_error_2'] = { '://', '' },
	['test_normalize_should_not_fail_on_error_3'] = { '#', '' },
	['test_normalize_should_not_fail_on_error_4'] = { ':@', '/' },
	['test_normalize_should_not_fail_on_error_5'] = { 'http:///', '/' },
	['test_normalize_should_not_fail_on_error_6'] = { 'http://http://', 'http://http://' }
}

for k, v in pairs(_tests) do
	TestHttpNormalize[k] = function (self)
		-- Given
		uri = v[1]
		-- When
		local normalized = self.http.uri.normalize(uri)
		-- Assert
		assertEquals(normalized, v[2])
	end
end

addTestSuite('TestHttpNormalize')

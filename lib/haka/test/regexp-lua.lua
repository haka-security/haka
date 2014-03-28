-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Given
local module = os.getenv("HAKA_MODULE")
assert(module, "/!\\ TEST REQUIRES ENV VAR : HAKA_MODULE")
local rem = require("regexp/" .. module)

function test_match_should_not_fail ()
        -- When
        local ret, msg = pcall(function () rem.re:match(".*", "aaa") end)
        -- Then
        assert(ret, string.format("Regexp match should not failed but failed with message %s", msg))
end

function test_match_should_be_successful ()
        -- When
        local ret = rem.re:match(".*", "aaa")
        -- Then
        assert(ret, "Matching pattern expected to match but failed")
end

function test_match_should_fail_when_pattern_do_not_match ()
        -- Given nothing
        -- When
        local ret = rem.re:match("abc", "aaa")
        -- Then
        assert(not ret, "Non-matching pattern expected to failed but return match")
end

function test_match_should_return_results ()
        -- Given nothing
        -- When
        local result = rem.re:match("bar", "foo bar foo")
        -- Then
        assert(result, "Matching pattern expected to return non-empty result but got nil")
end

function test_match_should_return_nil_results_when_pattern_do_not_match ()
        -- Given nothing
        -- When
        local result = rem.re:match("bar", "foo")
        -- Then
        assert(not result, "Matching pattern expected to return nil result but got result")
end

function test_match_should_be_successful_using_new_escaping_char ()
		-- When
		local ret = rem.re:match("%d", "666")
		-- Then
		assert(ret, "Matching pattern with special char '%' expected to match but failed")
end

function test_compile_should_not_fail ()
	-- When
	local ret, msg = pcall(function () rem.re:compile(".*") end)
	-- Then
	assert(ret , string.format("Regexp compile should not failed but failed with message %s", msg))
end

function test_compile_should_be_successful ()
        -- When
        local re = rem.re:compile(".*")
        -- Then
        assert(re, "Compilation of '.*' expected to be successful but failed")
end

function test_compile_should_fail_with_bad_pattern ()
        -- When
        local ret, msg = pcall(function () rem.re:compile("?") end)
        -- Then
        assert(not ret, "Compilation of '?' expected to fail but compile return non nil value")
end

function test_compile_should_be_successful_using_new_escaping_char ()
        -- When
        local ret, msg = pcall(function() rem.re:compile("%d%s") end)
        -- Then
        assert(ret, "Compilation of valid regex using special char '%' should not failed but failed")
end

function test_exec_should_not_fail ()
	-- Given
	local re = rem.re:compile(".*")
	-- When
	local ret, msg = pcall(function () re:match("aaa") end)
	-- Then
	assert(ret , string.format("Regexp exec should not failed but failed with message %s", msg))
end

function test_exec_should_be_successful ()
        -- Given
        local re = rem.re:compile(".*")
        -- When
        local ret = re:match("aaa")
        -- Then
        assert(ret, "Matching pattern expected to match but failed")
end

function test_exec_should_fail_when_pattern_do_not_match ()
        -- Given
        local re = rem.re:compile("abc")
        -- When
        local ret = re:match("aaa")
        -- Then
        assert(not ret, "Non-matching pattern expected to failed but return match")
end

function test_exec_should_return_results ()
        -- Given
        local re = rem.re:compile("bar")
        -- When
        local result = re:match("foo bar foo")
        -- Then
        assert(result == "bar", string.format("Matching pattern expected to return 'bar' but return '%s'", result))
end

function test_exec_should_return_nil_results_when_pattern_do_not_match ()
        -- Given
        local re = rem.re:compile("bar")
        -- When
        local ret = re:match("foo")
        -- Then
        assert(not ret, "Matching pattern expected to return nil result but got result")
end

function test_create_sink_should_be_successful ()
        -- Given
        local re = rem.re:compile(".*")

        -- When
        local ret, msg = pcall(function () re:create_sink() end)

        -- Then
        assert(ret, string.format("Regexp stream should not failed but failed with message %s", msg))
end

function test_feed_should_not_fail ()
	-- Given
	local re = rem.re:compile(".*")
        local sink = re:create_sink()
	-- When
	local ret, msg = pcall(function () sink:feed("aaa", true) end)
	-- Then
	assert(ret, string.format("Regexp feed should not failed but failed with message %s", msg))
end

function test_feed_should_match_accross_two_string ()
        -- Given
        local re = rem.re:compile("ab")
        local sink = re:create_sink()
        -- When
        local ret = sink:feed("aaa")
        ret = sink:feed("bbb", true)
        -- Then
        assert(ret, "Matching pattern expected to match over two string but failed")
end

function test_feed_should_return_results ()
        -- Given
        local re = rem.re:compile("bar")
        local sink = re:create_sink()
        -- When
        local ret, result = sink:feed("foo bar foo", true)
        -- Then
        assert(ret, "Matching pattern expected to match but failed")
        assert(result.offset == 4, "Matching pattern expected to return offset = 4 but found offset = %d", result.offset)
        assert(result.size == 3, "Matching pattern expected to return size = 3 but found size = %d", result.size)
end

function test_feed_should_return_nil_results_when_pattern_do_not_match ()
        -- Given
        local re = rem.re:compile("bar")
        local sink = re:create_sink()
        -- When
        local ret, result = sink:feed("foo", true)
        -- Then
        assert(not ret, "Matching pattern expected to feed but failed")
        assert(not result, "Matching pattern expected to return nil result but got result")
end

function test_feed_should_set_sink_to_partial ()
        -- Given
        local re = rem.re:compile("abc")
        local sink = re:create_sink()
        local ret, result = sink:feed("aaa", true)
        -- When
	local partial = sink:ispartial()
        -- Then
        assert(partial, "Partial matching pattern expected to set sink to partial result don't")
end

function test_match_should_not_match_different_case_without_option ()
        -- When
        local ret = rem.re:match("camel case", "CaMeL CaSe")
        -- Then
        assert(not ret, "Matching insensitive pattern expected to match but failed")
end

function test_match_should_allow_case_insensitive ()
        -- When
        local ret = rem.re:match("camel case", "CaMeL CaSe", rem.re.CASE_INSENSITIVE)
        -- Then
        assert(ret, "Matching insensitive pattern expected to match but failed")
end

function test_match_can_work_on_iterator ()
	-- Given
	local re = rem.re:compile("foo")
	local vbuf = haka.vbuffer_from("bar fo")
	vbuf:append(haka.vbuffer_from("o bar"))
	local iter = vbuf:pos("begin")
	-- When
	local ret = re:match(iter)
	-- Then
	assert(ret, "Matching on iterator expected to be successful but failed")
end

function test_match_on_iterator_should_return_a_subbuffer ()
	-- Given
	local re = rem.re:compile("foo")
	local vbuf = haka.vbuffer_from("bar fo")
	vbuf:append(haka.vbuffer_from("o bar"))
	local iter = vbuf:pos("begin")
	-- When
	local ret = re:match(iter, true)
	-- Then
	assert(ret:asstring() == 'foo', string.format("Matching on iterator expected return 'foo' but got '%s'", ret:asstring()))
end

function test_can_match_twice_with_same_iterator ()
	-- Given
	local re = rem.re:compile("foo")
	local vbuf = haka.vbuffer_from("bar fo")
	vbuf:append(haka.vbuffer_from("o foo"))
	local iter = vbuf:pos("begin")
	local ret = re:match(iter, true)
	-- When
	local ret = re:match(iter, true)
	-- Then
	assert(ret, "Matching on iterator expected to be successful but failed")
	assert(ret:asstring() == 'foo', string.format("Matching on iterator expected return 'foo' but got '%s'", ret:asstring()))
end

test_match_should_not_fail()
test_match_should_be_successful()
test_match_should_be_successful_using_new_escaping_char()
test_match_should_fail_when_pattern_do_not_match()
test_match_should_return_results()
test_match_should_return_nil_results_when_pattern_do_not_match()
test_compile_should_not_fail()
test_compile_should_be_successful()
test_compile_should_fail_with_bad_pattern()
test_compile_should_be_successful_using_new_escaping_char ()
test_exec_should_not_fail()
test_exec_should_be_successful()
test_exec_should_fail_when_pattern_do_not_match()
test_exec_should_return_results()
test_exec_should_return_nil_results_when_pattern_do_not_match()
test_create_sink_should_be_successful()
test_feed_should_not_fail()
test_feed_should_match_accross_two_string()
test_feed_should_return_results()
test_feed_should_return_nil_results_when_pattern_do_not_match()
test_feed_should_set_sink_to_partial()
test_match_should_allow_case_insensitive()
test_match_should_not_match_different_case_without_option()
test_match_can_work_on_iterator()
test_match_on_iterator_should_return_a_subbuffer()
test_can_match_twice_with_same_iterator()

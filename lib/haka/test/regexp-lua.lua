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
        local ret, result = rem.re:match("bar", "foo bar foo")
        -- Then
        assert(ret, "Matching pattern expected to match but failed")
        assert(result.offset == 4, "Matching pattern expected to return offset = 4 but found offset = %d", result.offset)
        assert(result.size == 3, "Matching pattern expected to return size = 3 but found size = %d", result.size)
end

function test_match_should_return_nil_results_when_pattern_do_not_match ()
        -- Given nothing
        -- When
        local ret, result = rem.re:match("bar", "foo")
        -- Then
        assert(not ret, "Matching pattern expected to match but failed")
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
        local ret, result = re:match("foo bar foo")
        -- Then
        assert(ret, "Matching pattern expected to match but failed")
        assert(result.offset == 4, "Matching pattern expected to return offset = 4 but found offset = %d", result.offset)
        assert(result.size == 3, "Matching pattern expected to return size = 3 but found size = %d", result.size)
end

function test_exec_should_return_nil_results_when_pattern_do_not_match ()
        -- Given
        local re = rem.re:compile("bar")
        -- When
        local ret, result = re:match("foo")
        -- Then
        assert(not ret, "Matching pattern expected to exec but failed")
        assert(not result, "Matching pattern expected to return nil result but got result")
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

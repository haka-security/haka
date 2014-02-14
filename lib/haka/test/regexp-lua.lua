-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- Given
local module = os.getenv("HAKA_MODULE")
print("got env var " .. module)
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
        -- When nothing
        local ret = rem.re:match("abc", "aaa")
        -- Then
        assert(not ret, "Non-matching pattern expected to failed but return match")
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

function test_exec_should_not_fail ()
	-- Given
	local re = rem.re:compile(".*")
	-- When
	local ret, msg = pcall(function () re:exec("aaa") end)
	-- Then
	assert(ret , string.format("Regexp exec should not failed but failed with message %s", msg))
end

function test_exec_should_be_successful ()
        -- Given
        local re = rem.re:compile(".*")
        -- When
        local ret = re:exec("aaa")
        -- Then
        assert(ret, "Matching pattern expected to match but failed")
end

function test_exec_should_fail_when_pattern_do_not_match ()
        -- Given
        local re = rem.re:compile("abc")
        -- When
        local ret = re:exec("aaa")
        -- Then
        assert(not ret, "Non-matching pattern expected to failed but return match")
end

function test_get_ctx_should_be_successful ()
        -- Given
        local re = rem.re:compile(".*")

        -- When
        local ret, msg = pcall(function () re:get_ctx() end)

        -- Then
        assert(ret, string.format("Regexp stream should not failed but failed with message %s", msg))
end

function test_feed_should_not_fail ()
	-- Given
	local re = rem.re:compile(".*")
        local re_ctx = re:get_ctx()
	-- When
	local ret, msg = pcall(function () re_ctx:feed("aaa") end)
	-- Then
	assert(ret, string.format("Regexp feed should not failed but failed with message %s", msg))
end

function test_feed_should_match_accross_two_string ()
        -- Given
        local re = rem.re:compile("ab")
        local re_ctx = re:get_ctx()
        -- When
        local ret = re_ctx:feed("aaa")
        ret = re_ctx:feed("bbb")
        -- Then
        assert(ret, "Matching pattern expected to match over two string but failed")
end

function test_exec_should_accept_vbuffer ()
        -- Given
        local re = rem.re:compile("aaa")
        local vbuf = "todo"
end


test_match_should_not_fail()
test_match_should_be_successful()
test_match_should_fail_when_pattern_do_not_match()
test_compile_should_not_fail()
test_compile_should_be_successful()
test_compile_should_fail_with_bad_pattern()
test_exec_should_not_fail ()
test_exec_should_be_successful()
test_exec_should_fail_when_pattern_do_not_match()
test_get_ctx_should_be_successful()
test_feed_should_not_fail()
test_feed_should_match_accross_two_string ()

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')


TestRegexpModule = {}

function TestRegexpModule:setUp()
	local module = os.getenv("HAKA_MODULE")
	assert(module, "/!\\ TEST REQUIRES ENV VAR : HAKA_MODULE")
	self.rem = require("regexp/" .. module)
end

function TestRegexpModule:test_match_should_not_fail ()
        -- When
        local ret = self.rem.re:match(".*", "aaa")
        -- Then
        assertTrue(ret)
end

function TestRegexpModule:test_match_should_be_successful ()
        -- When
        local ret = self.rem.re:match(".*", "aaa")
        -- Then
        assertTrue(ret)
end

function TestRegexpModule:test_match_should_fail_when_pattern_do_not_match ()
        -- Given nothing
        -- When
        local ret = self.rem.re:match("abc", "aaa")
        -- Then
        assertTrue(not ret)
end

function TestRegexpModule:test_match_should_return_results ()
        -- Given nothing
        -- When
        local result = self.rem.re:match("bar", "foo bar foo")
        -- Then
        assert(result, "Matching pattern expected to return non-empty result but got nil")
end

function TestRegexpModule:test_match_should_return_nil_results_when_pattern_do_not_match ()
        -- Given nothing
        -- When
        local result = self.rem.re:match("bar", "foo")
        -- Then
        assert(not result, "Matching pattern expected to return nil result but got result")
end

function TestRegexpModule:test_match_should_be_successful_using_new_escaping_char ()
		-- When
		local ret = self.rem.re:match("%d", "666")
		-- Then
		assertTrue(ret)
end

function TestRegexpModule:test_compile_should_not_fail ()
	-- When
	local ret, msg = self.rem.re:compile(".*")
	-- Then
	assertTrue(ret)
end

function TestRegexpModule:test_compile_should_be_successful ()
        -- When
        local re = self.rem.re:compile(".*")
        -- Then
        assertTrue(re)
end

function TestRegexpModule:test_compile_should_fail_with_bad_pattern ()
        -- When
        local ret, msg = pcall(function () self.rem.re:compile("?") end)
        -- Then
        assertTrue(not ret)
end

function TestRegexpModule:test_compile_should_be_successful_using_new_escaping_char ()
        -- When
        local ret, msg = self.rem.re:compile("%d%s")
        -- Then
        assertTrue(ret)
end

function TestRegexpModule:test_exec_should_not_fail ()
	-- Given
	local re = self.rem.re:compile(".*")
	-- When
	local ret, msg = re:match("aaa")
	-- Then
	assertTrue(ret)
end

function TestRegexpModule:test_exec_should_be_successful ()
        -- Given
        local re = self.rem.re:compile(".*")
        -- When
        local ret = re:match("aaa")
        -- Then
        assertTrue(ret)
end

function TestRegexpModule:test_exec_should_fail_when_pattern_do_not_match ()
        -- Given
        local re = self.rem.re:compile("abc")
        -- When
        local ret = re:match("aaa")
        -- Then
        assertTrue(not ret)
end

function TestRegexpModule:test_exec_should_return_results ()
        -- Given
        local re = self.rem.re:compile("bar")
        -- When
        local result = re:match("foo bar foo")
        -- Then
        assert(result == "bar", string.format("Matching pattern expected to return 'bar' but return '%s'", result))
end

function TestRegexpModule:test_exec_should_return_nil_results_when_pattern_do_not_match ()
        -- Given
        local re = self.rem.re:compile("bar")
        -- When
        local ret = re:match("foo")
        -- Then
        assert(not ret, "Matching pattern expected to return nil result but got result")
end

function TestRegexpModule:test_create_sink_should_be_successful ()
        -- Given
        local re = self.rem.re:compile(".*")

        -- When
        local ret, msg = re:create_sink()

        -- Then
        assertTrue(ret)
end

function TestRegexpModule:test_feed_should_not_fail ()
	-- Given
	local re = self.rem.re:compile(".*")
        local sink = re:create_sink()
	-- When
	local ret, msg = sink:feed("aaa", true)
	-- Then
	assertTrue(ret)
end

function TestRegexpModule:test_feed_should_match_accross_two_string ()
        -- Given
        local re = self.rem.re:compile("ab")
        local sink = re:create_sink()
        -- When
        local ret = sink:feed("aaa")
        ret = sink:feed("bbb", true)
        -- Then
        assertTrue(ret)
end

function TestRegexpModule:test_feed_should_return_results ()
        -- Given
        local re = self.rem.re:compile("bar")
        local sink = re:create_sink()
        -- When
        local ret, result = sink:feed("foo bar foo", true)
        -- Then
        assertTrue(ret)
        assertEquals(result.offset, 4)
        assertEquals(result.size, 3)
end

function TestRegexpModule:test_feed_should_return_nil_results_when_pattern_do_not_match ()
        -- Given
        local re = self.rem.re:compile("bar")
        local sink = re:create_sink()
        -- When
        local ret, result = sink:feed("foo", true)
        -- Then
        assertTrue(not ret)
        assertTrue(not result)
end

function TestRegexpModule:test_feed_should_set_sink_to_partial ()
        -- Given
        local re = self.rem.re:compile("abc")
        local sink = re:create_sink()
        local ret, result = sink:feed("aaa", true)
        -- When
	local partial = sink:ispartial()
        -- Then
        assert(partial, "Partial matching pattern expected to set sink to partial result don't")
end

function TestRegexpModule:test_match_should_not_match_different_case_without_option ()
        -- When
        local ret = self.rem.re:match("camel case", "CaMeL CaSe")
        -- Then
        assert(not ret, "Matching insensitive pattern expected to match but failed")
end

function TestRegexpModule:test_match_should_allow_case_insensitive ()
        -- When
        local ret = self.rem.re:match("camel case", "CaMeL CaSe", self.rem.re.CASE_INSENSITIVE)
        -- Then
        assert(ret, "Matching insensitive pattern expected to match but failed")
end

function TestRegexpModule:test_match_can_work_on_iterator ()
	-- Given
	local re = self.rem.re:compile("foo")
	local vbuf = haka.vbuffer_from("bar fo")
	vbuf:append(haka.vbuffer_from("o bar"))
	local iter = vbuf:pos("begin")
	-- When
	local ret = re:match(iter)
	-- Then
	assert(ret, "Matching on iterator expected to be successful but failed")
end

function TestRegexpModule:test_match_on_iterator_should_return_a_subbuffer ()
	-- Given
	local re = self.rem.re:compile("foo")
	local vbuf = haka.vbuffer_from("bar fo")
	vbuf:append(haka.vbuffer_from("o bar"))
	local iter = vbuf:pos("begin")
	-- When
	local ret = re:match(iter, true)
	-- Then
	assert(ret:asstring() == 'foo', string.format("Matching on iterator expected return 'foo' but got '%s'", ret:asstring()))
end

function TestRegexpModule:test_can_match_twice_with_same_iterator ()
	-- Given
	local re = self.rem.re:compile("foo")
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

LuaUnit:setVerbosity(1)
LuaUnit:run('TestRegexpModule')

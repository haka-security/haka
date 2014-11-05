-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestRef = {}


function TestRef:test_haka_provide_state()
	-- Given

	-- When
	local state = haka.state

	-- Then
	assertNotEquals(state, nil)
end

function TestRef:test_ref_can_be_set_and_get()
	-- Given
	local ffi = require("ffi")
	local ref = ffi.new("struct lua_ref")

	-- When
	ref:set("foo")
	local obj = ref:get()

	-- Then
	assertEquals(obj, "foo")
end


--[[
-- Test ref internals was never committed so keep it there
-- should be tested against luajit ref behavior
function TestRef:test_ref_should_return_valid_ref()
	-- Given
	local myobj = "foo"

	-- When
	local ref = haka.ref(myobj)

	-- Then
	assertNotEquals(ref, nil)
	assertNotEquals(ref, 0)
end

function TestRef:test_ref_on_nil_should_return_special_ref()
	-- Given
	local myobj = nil

	-- When
	local ref = haka.ref(myobj)

	-- Then
	assertEquals(ref, -1)
end

function TestRef:test_multiple_ref_call_should_not_return_same_ref()
	-- Given
	local myobj1 = "foo"
	local myobj2 = "bar"
	local ref1 = haka.ref(myobj1)

	-- When
	local ref2 = haka.ref(myobj2)

	-- Then
	assertNotEquals(ref1, ref2)
end

function TestRef:test_getref_should_return_object()
	-- Given
	local myobj = "foo"
	local ref = haka.ref(myobj)

	-- When
	local myobj_from_ref = haka.refget(ref)

	-- Then
	assertEquals(myobj_from_ref, myobj)
end

function TestRef:test_unref_should_free_a_ref()
	-- Given
	local myobj = "foo"
	local ref = haka.ref(myobj)
	haka.unref(ref)

	-- When
	myobj = haka.refget(ref)

	-- Then
	assertIsNil(myobj)
end

function TestRef:test_ref_after_unref_should_reuse_freed_ref()
	-- Given
	local myobj1 = "foo"
	local myobj2 = "bar"
	local ref1 = haka.ref(myobj1)
	haka.unref(ref1)

	-- When
	local ref2 = haka.ref(myobj2)

	-- Then
	assertEquals(ref2, ref1)
end

function TestRef:test_weakref_should_return_valid_weakref()
	-- Given
	local myobj = "foo"

	-- When
	local weakref = haka.weakref(myobj)

	-- Then
	assertNotEquals(weakref, nil)
	assertNotEquals(weakref, 0)
end

function TestRef:test_weakref_on_nil_should_return_special_weakref()
	-- Given
	local myobj = nil

	-- When
	local weakref = haka.weakref(myobj)

	-- Then
	assertEquals(weakref, -1)
end

function TestRef:test_multiple_weakref_call_should_not_return_same_weakref()
	-- Given
	local myobj1 = "foo"
	local myobj2 = "bar"
	local weakref1 = haka.weakref(myobj1)

	-- When
	local weakref2 = haka.weakref(myobj2)

	-- Then
	assertNotEquals(weakref1, weakref2)
end

function TestRef:test_getweakref_should_return_object()
	-- Given
	local myobj = "foo"
	local weakref = haka.weakref(myobj)

	-- When
	local myobj_from_weakref = haka.weakrefget(weakref)

	-- Then
	assertEquals(myobj_from_weakref, myobj)
end

function TestRef:test_unweakref_should_free_a_weakref()
	-- Given
	local myobj = "foo"
	local weakref = haka.weakref(myobj)
	haka.unweakref(weakref)

	-- When
	myobj = haka.weakrefget(weakref)

	-- Then
	assertIsNil(myobj)
end

function TestRef:test_weakref_after_unweakref_should_reuse_freed_weakref()
	-- Given
	local myobj1 = "foo"
	local myobj2 = "bar"
	local weakref1 = haka.weakref(myobj1)
	haka.unweakref(weakref1)

	-- When
	local weakref2 = haka.weakref(myobj2)

	-- Then
	assertEquals(weakref2, weakref1)
end ]]--

addTestSuite('TestRef')

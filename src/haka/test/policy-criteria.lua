-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestPolicy = {}

function TestPolicy:test_range_criterion_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.range(1, 3),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = 2, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicy:test_range_criterion_does_not_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.range(1, 3),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = 4, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, false)
end

function TestPolicy:test_out_of_range_criterion_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.outofrange(1, 3),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = 4, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicy:test_set_criterion_does_not_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.outofrange(1, 3),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = 2, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, false)
end

function TestPolicy:test_set_criterion_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.set{1, 3},
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = 1, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicy:test_set_criterion_does_not_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.set{1, 2},
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = 3, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, false)
end

function TestPolicy:test_present_criterion_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.present(),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = true, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicy:test_present_criterion_does_not_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.present(),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = nil, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, false)
end

function TestPolicy:test_absent_criterion_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.absent(),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = nil, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicy:test_absent_criterion_does_not_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.absent(),
		action = function ()
			action_triggered = true
		end
	}

	-- When
	test_policy:apply{ values = { value = true, }, ctx = {}, desc = {} }

	-- Then
	assertEquals(action_triggered, false)
end

addTestSuite('TestPolicy')

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestPolicyLearning = {}

function TestPolicyLearning:test_range_learning()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.learn(haka.policy.range),
		action = function ()
			action_triggered = true
		end
	}

	haka.policy.learning = true
	test_policy:apply{ values = { value = 2, }, ctx = {} }
	test_policy:apply{ values = { value = 5, }, ctx = {} }
	test_policy:apply{ values = { value = 10, }, ctx = {} }

	action_triggered = false

	-- When
	haka.policy.learning = false
	test_policy:apply{ values = { value = 11, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, false)

	-- and When
	test_policy:apply{ values = { value = 3, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicyLearning:test_set_learning()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	haka.policy{
		on = test_policy,
		value = haka.policy.learn(haka.policy.set),
		action = function ()
			action_triggered = true
		end
	}

	haka.policy.learning = true
	test_policy:apply{ values = { value = 2, }, ctx = {} }
	test_policy:apply{ values = { value = 5, }, ctx = {} }
	test_policy:apply{ values = { value = 10, }, ctx = {} }

	action_triggered = false

	-- and When
	haka.policy.learning = false
	test_policy:apply{ values = { value = 3, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, false)

	-- and When
	test_policy:apply{ values = { value = 5, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, true)
end

function TestPolicyLearning:test_learning_on_invalid_criterion()
	-- When
	local success, msg = pcall(function () haka.policy.learn(haka.policy.present) end)
	assertEquals(not success and string.sub(msg, -45), "cannot learn on haka.policy.present criterion")
end

function TestPolicyLearning:test_do_not_learn_if_other_criterion_do_not_match()
	-- Given
	local action_triggered = false
	local test_policy = haka.policy.new("Test policy")
	-- a_pass and z_pass ensure value is evaluated before z_pass
	haka.policy{
		on = test_policy,
		a_pass = true,
		value = haka.policy.learn(haka.policy.set),
		z_pass = true,
		action = function ()
			action_triggered = true
		end
	}

	-- When
	haka.policy.learning = true
	test_policy:apply{ values = { a_pass = true, value = 2, z_pass = false }, ctx = {} }

	-- Then
	haka.policy.learning = false
	test_policy:apply{ values = { a_pass = true, value = 2, z_pass = true }, ctx = {} }
	assertEquals(action_triggered, false)
end

addTestSuite('TestPolicyLearning')

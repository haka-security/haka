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

	-- When
	haka.policy.learning = true
	test_policy:apply{ values = { value = 2, }, ctx = {} }
	test_policy:apply{ values = { value = 5, }, ctx = {} }
	test_policy:apply{ values = { value = 10, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, false)

	-- and When
	haka.policy.learning = false
	test_policy:apply{ values = { value = 3, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, true)

	-- and When
	haka.policy.learning = false
	action_triggered = false
	test_policy:apply{ values = { value = 11, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, false)
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

	-- When
	haka.policy.learning = true
	test_policy:apply{ values = { value = 2, }, ctx = {} }
	test_policy:apply{ values = { value = 5, }, ctx = {} }
	test_policy:apply{ values = { value = 10, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, false)

	-- and When
	haka.policy.learning = false
	test_policy:apply{ values = { value = 5, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, true)

	-- and When
	haka.policy.learning = false
	action_triggered = false
	test_policy:apply{ values = { value = 3, }, ctx = {} }

	-- Then
	assertEquals(action_triggered, false)
end

function TestPolicyLearning:test_learning_on_invalid_criterion()
	-- When
	local success, msg = pcall(function () haka.policy.learn(haka.policy.present) end)
	assertEquals(not success and string.sub(msg, -45), "cannot learn on haka.policy.present criterion")
end

addTestSuite('TestPolicyLearning')

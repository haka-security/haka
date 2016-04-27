-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestDissector = {}

function TestDissector:test_error_when_accessing_unknown_dissector()
	-- Given
	-- No dissector defined

	-- When
	local my_unknown_dissector = haka.dissectors.my_unknown_dissector

	-- Then
	assertEquals(my_unknown_dissector, nil)
end

addTestSuite('TestDissector')

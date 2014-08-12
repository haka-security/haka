-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

function console.setloglevel(level, module)
	hakactl.remote('any', function ()
		return haka.log.setlevel(level, module)
	end)
end

function console.stop()
	hakactl.remote('any', function ()
		haka.log("core", "request to stop haka received")
		return haka.exit()
	end)

	haka.exit()
end

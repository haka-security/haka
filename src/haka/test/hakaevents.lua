-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

haka.rule{
	on = haka.events.exiting,
	eval = function ()
		haka.log.info("exiting")
	end
}

haka.rule{
	on = haka.events.started,
	eval = function ()
		haka.log.info("started")
	end
}

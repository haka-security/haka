-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local smtp = require('smtp')

smtp.install_tcp_rule(25)

haka.rule{
	hook = smtp.events.mail_content,
	options = {
		streamed = true,
	},
	eval = function (flow, iter)
		local mail_content = {}

		for sub in iter:foreach_available() do
			table.insert(mail_content, sub:asstring())
		end

		print("== Mail Content ==")
		print(table.concat(mail_content))
		print("== End Mail Content ==")
	end
}

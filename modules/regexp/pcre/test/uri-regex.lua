-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/http")
local rem = require("regexp/pcre")

--
-- Testing regex from modsecurity rule set
-- Should block the following payload:
--
-- OR 1# 
-- DROP sampletable;--
-- admin'--
-- DROP/*comment*/sampletable
-- DR/**/OP/*bypass blacklisting*/sampletable
-- SELECT/*avoid-spaces*/password/**/FROM/**/Members
-- SELECT /*!32302 1/0, */ 1 FROM tablename
-- ‘ or 1=1# 
-- ‘ or 1=1-- -
-- ‘ or 1=1/*
-- ' or 1=1;\x00
-- 1='1' or-- -
-- ' /*!50000or*/1='1
-- ' /*!or*/1='1
-- 0/**/union/*!50000select*/table_name`foo`/**/
--

local modsec_regex = "(/%*!?|%*/|[';]--|--[%s%r%n%v%f]|(?:--[^-]*?-)|([^%-&])#.*?[%s%r%n%v%f]|;?%00)"

local re = rem.re:compile(modsec_regex)

haka.rule {
	on = haka.dissectors.http.events.request,
	eval = function (http, request)
		local uri = request.uri
		if re:match(uri) then
			haka.alert{
				severity = 'medium',
				method = {
					description = string.format("SQLi comment sequences detected in %s", uri)
				},
				sources = { haka.alert.address(http.flow.srcip) },
			}
		end
	end
}

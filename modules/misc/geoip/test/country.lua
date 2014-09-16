-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require('protocol/ipv4')
local geoip = require('misc/geoip')

TestGeoipCountry = {}

function TestGeoipCountry:test_country_check_no_error()
	local db = geoip.open('/usr/share/GeoIP/GeoIP.dat')

	assertEquals(db:country(ipv4.addr('8.8.8.8')), "US")
end

addTestSuite('TestGeoipCountry')

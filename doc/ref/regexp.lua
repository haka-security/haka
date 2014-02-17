-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local rem = require('regexp/pcre')

local re = rem.re:compile("foo")
local re_ctx = re:get_ctx()

local ret = re_ctx:feed("some fo")
ret = re_ctx:feed("o over two string")

print(ret)

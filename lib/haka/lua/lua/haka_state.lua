-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <haka/config.h>

local state = unpack({...})

#ifdef HAKA_FFI
local ffi = require("ffi")

state = ffi.cast("void *", state)
#endif

haka.state = state

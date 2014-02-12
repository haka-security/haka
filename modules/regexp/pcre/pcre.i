/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module pcre

%{

#include <haka/regexp_module.h>

extern struct regexp_module HAKA_MODULE;

static struct regexp_module *re = &HAKA_MODULE;

%}

struct regexp_module *re;

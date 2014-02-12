/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <haka/regexp_module.h>
%}

struct regexp {
        %extend {
                bool exec(const char *buffer, int len) {
                        return $self->module->exec($self, buffer, len);
                }

                ~regexp() {
                        $self->module->free($self);
                }
        }
};

%newobject regexp_module::compile;
struct regexp_module {
        %extend {
                bool match(const char *pattern, const char *buffer, int len) {
                        return $self->match(pattern, buffer, len);
                }

                struct regexp *compile(const char *pattern) {
                        return $self->compile(pattern);
                }
        }
};

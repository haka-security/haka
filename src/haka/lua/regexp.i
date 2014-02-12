/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <haka/regexp_module.h>
%}

struct regexp {
        %extend {
                bool exec(const char *STRING, size_t SIZE) {
                        return $self->module->exec($self, STRING, SIZE);
                }

                bool feed(const char *STRING, size_t SIZE) {
                        return $self->module->feed($self, STRING, SIZE);
                }

                bool exec(struct vbuffer *vbuf) {
                        return $self->module->vbexec($self, vbuf);
                }

                bool feed(struct vbuffer *vbuf) {
                        return $self->module->vbfeed($self, vbuf);
                }

                ~regexp() {
                        $self->module->free($self);
                }
        }
};

%newobject regexp_module::compile;
struct regexp_module {
        %extend {
                bool match(const char *pattern, const char *STRING, size_t SIZE) {
                        return $self->match(pattern, STRING, SIZE);
                }

                struct regexp *compile(const char *pattern) {
                        return $self->compile(pattern);
                }
        }
};

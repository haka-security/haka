/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%{
#include <haka/regexp_module.h>
%}

struct regexp_ctx {
        %extend {
                bool feed(const char *STRING, size_t SIZE) {
                        return $self->regexp->module->feed($self, STRING, SIZE);
                }

                bool feed(struct vbuffer *vbuf) {
                        return $self->regexp->module->vbfeed($self, vbuf);
                }

                ~regexp_ctx() {
                        $self->regexp->module->free_regexp_ctx($self);
                }
        }
};

%newobject regexp::get_ctx;
struct regexp {
        %extend {
                bool exec(const char *STRING, size_t SIZE) {
                        return $self->module->exec($self, STRING, SIZE);
                }

                bool exec(struct vbuffer *vbuf) {
                        return $self->module->vbexec($self, vbuf);
                }

                struct regexp_ctx *get_ctx() {
                        return $self->module->get_ctx($self);
                }

                ~regexp() {
                        $self->module->release_regexp($self);
                }
        }
};

%newobject regexp_module::compile;
struct regexp_module {
        %extend {
                bool match(const char *pattern, const char *STRING, size_t SIZE) {
                        return $self->match(pattern, STRING, SIZE);
                }

                bool match(const char *pattern, struct vbuffer *vbuf) {
                        return $self->vbmatch(pattern, vbuf);
                }

                struct regexp *compile(const char *pattern) {
                        return $self->compile(pattern);
                }
        }
};

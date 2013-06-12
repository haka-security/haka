%module stream

%{
#include <haka/stream.h>
%}

%include "haka/lua/swig.si"
%include "haka/lua/stream.si"

%nodefaultctor;

struct stream {
};

BASIC_STREAM(stream)

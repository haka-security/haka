%module stream

%{
#include <haka/stream.h>
%}

%include haka/swig.si
%include haka/stream.si

%nodefaultctor;

struct stream {
};

BASIC_STREAM(stream)

%module stream

%{
#include <haka/stream.h>
%}

%include haka/swig.i
%include haka/stream.i

%nodefaultctor;

struct stream {
};

BASIC_STREAM(stream)

%module tcp
%{
#include "haka/tcp.h"
%}

%include haka/swig.i

%nodefaultctor;

struct tcp {
	%extend {
		~tcp()
		{
			tcp_release($self);
		}
	}
};

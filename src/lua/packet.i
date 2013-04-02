%module packet
%{
#include <haka/packet.h>
#include <haka/packet_module.h>
%}

%rename(ACCEPT) FILTER_ACCEPT;
%rename(DROP) FILTER_DROP;

enum filter_result { FILTER_ACCEPT, FILTER_DROP };


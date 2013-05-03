%module stream

%{
#include <haka/stream.h>
%}

%include haka/swig.i
%include haka/buffer.i

%nodefaultctor;

struct stream {
	%extend {
		~stream()
		{
			stream_destroy($self);
		}

		%rename(read) _read;
		struct buffer *_read(unsigned int size)
		{
			struct buffer *buf = allocate_buffer(size);
			buf->size = stream_read($self, buf->data, size);
			if (check_error()) {
				free_buffer(buf);
				return NULL;
			}
			return buf;
		}

		%rename(available) _available;
		unsigned int available()
		{
			return stream_available($self);
		}
	}
};

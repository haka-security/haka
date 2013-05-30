/**
 * @file stream.h
 * @brief Stream API
 * @author Pierre-Sylvain Desse
 *
 * The file contains the stream API functions.
 */

/**
 * @defgroup Stream Stream
 * @brief Stream API functions and structures.
 * @ingroup API_C
 */

#ifndef _HAKA_STREAM_H
#define _HAKA_STREAM_H

#include <stddef.h>

#include <haka/types.h>


struct stream;

/**
 * Stream function definition.
 * @ingroup Stream
 */
struct stream_ftable {
	/**
	 * Called when the stream is destroyed.
	 */
	bool      (*destroy)(struct stream *s);

	/**
	 * Read data from the stream.
	 */
	size_t    (*read)(struct stream *s, uint8 *data, size_t length);

	/**
	 * Get number of available bytes on the stream.
	 */
	size_t    (*available)(struct stream *s);

	/**
	 * Insert data at the current stream position.
	 */
	size_t    (*insert)(struct stream *s, uint8 *data, size_t length);

	/**
	 * Replace data at the current stream position.
	 */
	size_t    (*replace)(struct stream *s, uint8 *data, size_t length);

	/**
	 * Erase data at the current stream position.
	 */
	size_t    (*erase)(struct stream *s, size_t length);
};


/**
 * Opaque stream structure.
 * @ingroup Stream
 */
struct stream {
	struct stream_ftable *ftable;
};


/**
 * Read data from a stream
 * @param stream Opaque stream.
 * @param data Destination buffer.
 * @param length Length to read.
 * @return Length read or (size_t)-1 in case of an error (use clear_error() to
 * get details about the error).
 * @ingroup Stream
 */
static inline size_t stream_read(struct stream *stream, uint8 *data, size_t length)
{
	return stream->ftable->read(stream, data, length);
}

/**
 * Get the number of contiguous bytes available from a stream.
 * @param stream Opaque stream.
 */
static inline size_t stream_available(struct stream *stream)
{
	return stream->ftable->available(stream);
}

/**
 * Destroy a stream.
 * @param stream Opaque stream.
 * @return True if successful (use clear_error() to get details about the error).
 * In case of an error, the stream is not freed.
 */
static inline bool stream_destroy(struct stream *stream)
{
	return stream->ftable->destroy(stream);
}

#endif /* _HAKA_STREAM_H */

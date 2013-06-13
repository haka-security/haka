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
#include <haka/compiler.h>
#include <haka/error.h>


struct stream;
struct stream_mark;

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
	size_t    (*insert)(struct stream *s, const uint8 *data, size_t length);

	/**
	 * Replace data at the current stream position.
	 */
	size_t    (*replace)(struct stream *s, const uint8 *data, size_t length);

	/**
	 * Erase data at the current stream position.
	 */
	size_t    (*erase)(struct stream *s, size_t length);

	struct stream_mark *(*mark)(struct stream *s);
	bool      (*unmark)(struct stream *s, struct stream_mark *mark);
	bool      (*seek)(struct stream *s, struct stream_mark *mark, bool unmark);
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
INLINE size_t stream_read(struct stream *stream, uint8 *data, size_t length)
{
	return stream->ftable->read(stream, data, length);
}

/**
 * Get the number of contiguous bytes available from a stream.
 * @param stream Opaque stream.
 */
INLINE size_t stream_available(struct stream *stream)
{
	return stream->ftable->available(stream);
}

/**
 * Destroy a stream.
 * @param stream Opaque stream.
 * @return True if successful (use clear_error() to get details about the error).
 * In case of an error, the stream is not freed.
 */
INLINE bool stream_destroy(struct stream *stream)
{
	return stream->ftable->destroy(stream);
}

INLINE size_t stream_insert(struct stream *stream, const uint8 *data, size_t length)
{
	if (!stream->ftable->insert) {
		error(L"usupported operation");
		return 0;
	}

	return stream->ftable->insert(stream, data, length);
}

INLINE size_t stream_replace(struct stream *stream, const uint8 *data, size_t length)
{
	if (!stream->ftable->replace) {
		error(L"usupported operation");
		return 0;
	}

	return stream->ftable->replace(stream, data, length);
}

INLINE size_t stream_erase(struct stream *stream, size_t length)
{
	if (!stream->ftable->erase) {
		error(L"usupported operation");
		return 0;
	}

	return stream->ftable->erase(stream, length);
}

INLINE struct stream_mark *stream_mark(struct stream *stream)
{
	if (!stream->ftable->mark) {
		error(L"usupported operation");
		return NULL;
	}

	return stream->ftable->mark(stream);
}

INLINE bool stream_unmark(struct stream *stream, struct stream_mark *mark)
{
	if (!stream->ftable->unmark) {
		error(L"usupported operation");
		return false;
	}

	return stream->ftable->unmark(stream, mark);
}

INLINE bool stream_seek(struct stream *stream, struct stream_mark *mark, bool unmark)
{
	if (!stream->ftable->seek) {
		error(L"usupported operation");
		return false;
	}

	return stream->ftable->seek(stream, mark, unmark);
}

#endif /* _HAKA_STREAM_H */

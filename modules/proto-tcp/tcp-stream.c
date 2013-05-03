
#include <haka/tcp-stream.h>
#include <haka/error.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>


static bool tcp_stream_destroy(struct stream *s);
static size_t tcp_stream_read(struct stream *s, uint8 *data, size_t length);
static size_t tcp_stream_available(struct stream *s);
static size_t tcp_stream_insert(struct stream *s, uint8 *data, size_t length);
static size_t tcp_stream_replace(struct stream *s, uint8 *data, size_t length);
static size_t tcp_stream_erase(struct stream *s, size_t length);

struct stream_ftable tcp_stream_ftable = {
	destroy: tcp_stream_destroy,
	read: tcp_stream_read,
	available: tcp_stream_available,
	insert: tcp_stream_insert,
	replace: tcp_stream_replace,
	erase: tcp_stream_erase,
};


struct tcp_stream_chunck {
	struct tcp               *tcp;
	size_t                    start_seq;
	size_t                    end_seq;
	struct tcp_stream_chunck *next;
};

struct tcp_stream {
	struct stream             stream;
	size_t                    start_seq;
	size_t                    current_seq;
	struct tcp_stream_chunck *first;   /* first packet in the stream */
	struct tcp_stream_chunck *last;    /* last continuous packet */
	struct tcp_stream_chunck *current; /* packet at the current stream position */
};

#define TCP_STREAM(s) struct tcp_stream *tcp_s = (struct tcp_stream *)(s); \
	assert((s)->ftable == &tcp_stream_ftable);


struct stream *tcp_stream_create(size_t start_seq)
{
	struct tcp_stream *tcp_s = malloc(sizeof(struct tcp_stream));
	if (!tcp_s) {
		error(L"memory error");
		return NULL;
	}

	tcp_s->stream.ftable = &tcp_stream_ftable;
	tcp_s->start_seq = start_seq;
	tcp_s->current_seq = 0;
	tcp_s->first = NULL;
	tcp_s->last = NULL;
	tcp_s->current = NULL;

	return &tcp_s->stream;
}

static bool tcp_stream_destroy(struct stream *s)
{
	TCP_STREAM(s);

	if (tcp_s->first) {
		error(L"tcp stream must be empty to be destroyed");
		return false;
	}

	free(s);
	return true;
}

bool tcp_stream_push(struct stream *s, struct tcp *tcp)
{
	struct tcp_stream_chunck *chunk;
	TCP_STREAM(s);

	chunk = malloc(sizeof(struct tcp_stream_chunck));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	chunk->tcp = tcp;
	chunk->start_seq = tcp_get_seq(tcp);
	if (chunk->start_seq < tcp_s->start_seq) {
		error(L"invalid sequence number");
		free(chunk);
		return false;
	}

	chunk->start_seq -= tcp_s->start_seq;
	chunk->end_seq = chunk->start_seq + tcp_get_payload_length(tcp);

	/* Search for insert point */
	if (!tcp_s->last) {
		tcp_s->first = chunk;
		tcp_s->last = chunk;
		chunk->next = NULL;
	}
	else {
		struct tcp_stream_chunck **iter = &tcp_s->last;
		while (*iter && (*iter)->start_seq < chunk->start_seq) {
			iter = &((*iter)->next);
		}

		chunk->next = *iter;
		*iter = chunk;
	}

	return true;
}

struct tcp *tcp_stream_pop(struct stream *s)
{
	struct tcp *tcp;
	struct tcp_stream_chunck *chunk;
	TCP_STREAM(s);

	chunk = tcp_s->first;
	if (chunk && chunk->start_seq > tcp_s->current_seq) {
		tcp = chunk->tcp;
		tcp_s->first = tcp_s->first->next;

		free(chunk);
		return tcp;
	}

	return NULL;
}

static struct tcp_stream_chunck *tcp_stream_current(struct tcp_stream *tcp_s)
{
	struct tcp_stream_chunck *iter;

	iter = tcp_s->current;
	while (iter && iter->end_seq < tcp_s->current_seq) {
		iter = iter->next;
	}

	tcp_s->current = iter;
	return iter;
}

static size_t tcp_stream_read(struct stream *s, uint8 *data, size_t length)
{
	struct tcp_stream_chunck *iter;
	size_t offset;
	size_t left_len = length;
	TCP_STREAM(s);

	offset = tcp_s->current_seq;
	iter = tcp_stream_current(tcp_s);

	while (iter && left_len > 0) {
		if (offset < iter->start_seq || offset >= iter->end_seq)
			break;

		const size_t local_offset = offset - iter->start_seq;
		const uint8 *src = tcp_get_payload(iter->tcp) + local_offset;
		size_t len = offset - iter->end_seq;
		if (len > left_len) len = left_len;

		memcpy(data, src, len);

		left_len -= len;
		data += len;
		offset += len;
		iter = iter->next;
	}

	return length - left_len;
}

static size_t tcp_stream_available(struct stream *s)
{
	struct tcp_stream_chunck *iter;
	size_t offset;
	size_t available_len = 0;
	TCP_STREAM(s);

	offset = tcp_s->current_seq;
	iter = tcp_stream_current(tcp_s);

	while (iter) {
		if (offset < iter->start_seq || offset >= iter->end_seq)
			break;

		size_t len = offset - iter->end_seq;

		available_len += len;
		offset += len;
		iter = iter->next;
	}

	return available_len;
}

static size_t tcp_stream_insert(struct stream *s, uint8 *data, size_t length)
{
	//TCP_STREAM(s);
	return -1;
}

static size_t tcp_stream_replace(struct stream *s, uint8 *data, size_t length)
{
	//TCP_STREAM(s);
	return -1;
}

static size_t tcp_stream_erase(struct stream *s, size_t length)
{
	//TCP_STREAM(s);
	return -1;
}


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

static struct tcp_stream_chunck *tcp_stream_current(struct tcp_stream *tcp_s)
{
	struct tcp_stream_chunck *iter;

	iter = tcp_s->current;
	if (!iter) iter = tcp_s->first;

	while (iter && iter->end_seq < tcp_s->current_seq) {
		iter = iter->next;
	}

	tcp_s->current = iter;
	return iter;
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
		struct tcp_stream_chunck **iter;

		if (tcp_s->last->start_seq < chunk->start_seq) {
			iter = &tcp_s->last;
			do {
				iter = &((*iter)->next);
			}
			while (*iter && (*iter)->start_seq < chunk->start_seq);
		}
		else {
			tcp_stream_current(tcp_s); /* Validate tcp_s->current field */
			iter = &tcp_s->current;
			do {
				iter = &((*iter)->next);
			}
			while (*iter && (*iter)->start_seq < chunk->start_seq);
		}

		if (*iter) chunk->next = (*iter)->next;
		else chunk->next = NULL;
		*iter = chunk;

		tcp_s->last = chunk;
	}

	return true;
}

struct tcp *tcp_stream_pop(struct stream *s)
{
	struct tcp *tcp;
	struct tcp_stream_chunck *chunk;
	TCP_STREAM(s);

	chunk = tcp_s->first;
	if (chunk && chunk->end_seq <= tcp_s->current_seq) {
		tcp = chunk->tcp;
		tcp_s->first = tcp_s->first->next;

		if (tcp_s->current == chunk) tcp_s->current = NULL;
		if (tcp_s->last == chunk) {
			assert(chunk->next == NULL);
			tcp_s->last = NULL;
		}

		free(chunk);
		return tcp;
	}

	return NULL;
}

static size_t tcp_stream_read(struct stream *s, uint8 *data, size_t length)
{
	struct tcp_stream_chunck *iter;
	size_t offset, last_seq;
	size_t left_len = length;
	TCP_STREAM(s);

	offset = tcp_s->current_seq;
	iter = tcp_stream_current(tcp_s);

	if (!iter || tcp_s->current_seq < iter->start_seq)
		return 0;

	offset = tcp_s->current_seq;
	last_seq = iter->start_seq;

	do {
		if (last_seq != iter->start_seq)
			break;

		const size_t local_offset = offset - iter->start_seq;
		const uint8 *src = tcp_get_payload(iter->tcp) + local_offset;
		size_t len = iter->end_seq - offset;
		if (len > left_len) len = left_len;
		memcpy(data, src, len);
		offset = iter->end_seq;
		left_len -= len;

		last_seq = iter->end_seq;
		iter = iter->next;
	}
	while (iter && left_len > 0);

	tcp_s->current_seq += length - left_len;

	return length - left_len;
}

static size_t tcp_stream_available(struct stream *s)
{
	struct tcp_stream_chunck *iter;
	size_t last_seq = 0;
	TCP_STREAM(s);

	iter = tcp_stream_current(tcp_s);
	if (!iter || tcp_s->current_seq < iter->start_seq)
		return 0;

	last_seq = iter->start_seq;

	do {
		if (last_seq != iter->start_seq)
			break;

		last_seq = iter->end_seq;
		iter = iter->next;
	}
	while (iter);

	return last_seq - tcp_s->current_seq;
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

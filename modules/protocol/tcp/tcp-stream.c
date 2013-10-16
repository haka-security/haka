
#include <haka/tcp-stream.h>
#include <haka/tcp.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/container/list.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/*
 * Stream interface declaration
 */

static bool tcp_stream_destroy(struct stream *s);
static size_t tcp_stream_read(struct stream *s, uint8 *data, size_t length);
static size_t tcp_stream_available(struct stream *s);
static size_t tcp_stream_insert(struct stream *s, const uint8 *data, size_t length);
static size_t tcp_stream_replace(struct stream *s, const uint8 *data, size_t length);
static size_t tcp_stream_erase(struct stream *s, size_t length);
static struct stream_mark *tcp_stream_mark(struct stream *s, bool readonly);
static bool tcp_stream_unmark(struct stream *s, struct stream_mark *mark);
static bool tcp_stream_seek(struct stream *s, struct stream_mark *mark, bool unmark);

struct stream_ftable tcp_stream_ftable = {
	destroy: tcp_stream_destroy,
	read: tcp_stream_read,
	available: tcp_stream_available,
	insert: tcp_stream_insert,
	replace: tcp_stream_replace,
	erase: tcp_stream_erase,
	mark: tcp_stream_mark,
	unmark: tcp_stream_unmark,
	seek: tcp_stream_seek
};


/*
 * Stream structures
 */

enum tcp_modif_type {
	TCP_MODIF_INSERT,
	TCP_MODIF_ERASE
};

struct tcp_stream;

struct tcp_stream_chunk_modif {
	struct list                     list;

	enum tcp_modif_type             type;
	uint64                          position;
	size_t                          length;
	uint8                           data[0];
};

struct tcp_stream_chunk {
	struct list                     list;

	struct tcp                     *tcp;
	uint8                          *data;
	uint64                          start_seq;
	uint64                          end_seq;
	int64                           offset_seq;
	struct tcp_stream_chunk_modif  *modifs;
};

struct tcp_stream_position {
	uint64                          chunk_seq;         /* chunk position seq (without modifs) */
	uint64                          chunk_seq_modif;   /* chunk position seq (with modifs) */
	uint64                          current_seq_modif; /* current position seq (with modifs) */
	struct tcp_stream_chunk        *chunk;             /* chunk at the current stream position */
	size_t                          chunk_offset;      /* current offset in the current chunk (without modifs) */
	struct tcp_stream_chunk_modif  *modif;             /* current or previous modif */
	size_t                          modif_offset;      /* position in the modif or -1 */
};

struct tcp_stream_mark {
	struct list                     list;

	struct tcp_stream              *stream;
	uint64                          chunk_seq;
	size_t                          chunk_offset;
	size_t                          modif_offset;
	bool                            readonly;
};

struct tcp_stream {
	struct stream                   stream;
	uint32                          start_seq;
	uint64                          last_seq;

	struct tcp_stream_chunk        *begin;
	struct tcp_stream_chunk        *end;
	struct tcp_stream_chunk        *first;
	int                             first_offset_seq;
	struct tcp_stream_chunk        *last;    /* last inserted packet */
	struct tcp_stream_chunk        *sent;
	struct tcp_stream_chunk        *last_sent;
	int                             sent_offset_seq;

	struct tcp_stream_position      current_position;
	struct tcp_stream_position      mark_position;
	struct tcp_stream_mark         *marks;
	struct tcp_stream_chunk_modif  *pending_modif;
};


/*
 * Stream position functions
 */

bool tcp_stream_position_isvalid(struct tcp_stream_position *pos)
{
	return (pos->current_seq_modif != (uint64)-1);
}

void tcp_stream_position_invalidate(struct tcp_stream_position *pos)
{
	pos->current_seq_modif = (uint64)-1;
	pos->chunk = NULL;
	pos->modif = NULL;
}

static struct tcp_stream_chunk_modif *tcp_stream_position_modif(struct tcp_stream_position *pos,
		size_t *modif_offset, struct tcp_stream_chunk_modif **prev, struct tcp_stream_chunk_modif **next)
{
	if (pos->modif && pos->modif->position == pos->chunk_offset) {
		if ((pos->modif->type == TCP_MODIF_INSERT &&
			pos->modif_offset >= pos->modif->length) ||
			(pos->modif->type == TCP_MODIF_ERASE &&
			pos->modif_offset != 0)) {
			if (prev) *prev = pos->modif;
			if (next) *next = list_next(pos->modif);
			return NULL;
		}
		else {
			*modif_offset = pos->modif_offset;
			if (prev) *prev = list_prev(pos->modif);
			if (next) *next = list_next(pos->modif);
			return pos->modif;
		}
	}
	else {
		if (pos->modif) {
			if (prev) *prev = pos->modif;
			if (next) *next = list_next(pos->modif);
		}
		else {
			if (prev) *prev = NULL;
			if (next) *next = pos->chunk ? pos->chunk->modifs : NULL;
		}
		return NULL;
	}
}

static void tcp_stream_position_update_modif(struct tcp_stream_position *pos)
{
	struct tcp_stream_chunk_modif *next_modif = NULL;

	if (pos->modif) {
		if (pos->chunk_offset == pos->modif->position) {
			if (pos->modif->type == TCP_MODIF_INSERT &&
				pos->modif_offset >= pos->modif->length) {
				next_modif = list_next(pos->modif);
			}
			else if (pos->modif->type == TCP_MODIF_ERASE &&
				pos->modif_offset != 0) {
				next_modif = list_next(pos->modif);
			}
		}
		else {
			next_modif = list_next(pos->modif);
		}
	}
	else {
		next_modif = pos->chunk ? pos->chunk->modifs : NULL;
		if (next_modif && next_modif->position < pos->chunk_offset) {
			next_modif = NULL;
		}
	}

	assert(!next_modif || next_modif->position >= pos->chunk_offset);

	if (next_modif && next_modif->position == pos->chunk_offset) {
		pos->modif = next_modif;
		pos->modif_offset = 0;
	}
}

static bool tcp_stream_position_chunk_at_end(struct tcp_stream_position *pos)
{
	struct tcp_stream_chunk_modif *modif;

	if (pos->chunk->start_seq + pos->chunk_offset != pos->chunk->end_seq) {
		return false;
	}

	modif = pos->modif;
	if (!modif) modif = pos->chunk->modifs;

	if (!modif) {
		return true;
	}
	else if (list_next(modif)) {
		assert(list_next(modif)->position >= pos->chunk_offset);
		return false;
	}
	else {
		assert(modif->position <= pos->chunk_offset);
		return modif->position != pos->chunk_offset ||
				pos->modif_offset >= modif->length;
	}
}

static bool tcp_stream_position_next_chunk(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos)
{
	assert(pos->chunk);

	if (!list_next(pos->chunk) || list_next(pos->chunk)->start_seq == pos->chunk->end_seq) {
		pos->chunk_seq += pos->chunk_offset;
		pos->chunk_seq_modif += pos->chunk_offset + pos->chunk->offset_seq;
		pos->chunk = list_next(pos->chunk);
		assert(!pos->chunk || pos->chunk->start_seq == pos->chunk_seq);
		pos->chunk_offset = 0;
		pos->modif = NULL;
		pos->modif_offset = (size_t)-1;
		assert(!tcp_s->pending_modif);
		return true;
	}
	else {
		return false;
	}
}

static bool tcp_stream_position_chunk_is_before(struct tcp_stream_position *pos,
		struct tcp_stream_chunk *chunk)
{
	return (pos->chunk != chunk) && (pos->chunk_seq + pos->chunk_offset >= chunk->end_seq);
}

static bool tcp_stream_position_advance(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos)
{
	if (!pos->chunk) {
		if (!tcp_s->first || tcp_s->first->start_seq > pos->chunk_seq) {
			if (!pos->modif) {
				if (tcp_s->pending_modif) {
					pos->modif = tcp_s->pending_modif;
					pos->modif_offset = 0;
				}
				else {
					return false;
				}
			}
		}
		else {
			struct tcp_stream_chunk *chunk = tcp_s->first;
			int seq_offset = tcp_s->first_offset_seq;

			while (chunk && chunk->start_seq != pos->chunk_seq) {
				assert(chunk->start_seq < pos->chunk_seq);
				seq_offset += chunk->offset_seq;
				chunk = list_next(chunk);
			}

			if (!chunk) {
				return false;
			}

			pos->chunk = chunk;
			pos->chunk_offset = 0;
			pos->chunk_seq_modif = pos->chunk->start_seq + seq_offset;
			assert(pos->chunk_seq == pos->chunk->start_seq);

			if (pos->modif) {
				assert(pos->modif == pos->chunk->modifs);
				assert(pos->current_seq_modif == pos->chunk_seq_modif + pos->modif_offset);
			}
			else {
				pos->modif = NULL;
				pos->modif_offset = (size_t)-1;
				assert(pos->current_seq_modif == pos->chunk_seq_modif);
			}
		}
	}

	while (true) {
		struct tcp_stream_chunk_modif *cur_modif;
		size_t modif_offset;

		tcp_stream_position_update_modif(pos);

		if (pos->chunk) {
			if (tcp_stream_position_chunk_at_end(pos)) {
				if (!list_next(pos->chunk) || !tcp_stream_position_next_chunk(tcp_s, pos)) {
					return false;
				}
			}
			else {
				assert(pos->chunk->start_seq + pos->chunk_offset <= pos->chunk->end_seq);
			}
		}
		else if (pos->modif) {
			/* Pending modif case */
			assert(pos->modif == tcp_s->pending_modif);
			assert(list_next(pos->modif) == NULL);

			if (pos->modif_offset >= pos->modif->length) {
				return false;
			}
		}
		else {
			return false;
		}

		cur_modif = tcp_stream_position_modif(pos, &modif_offset, NULL, NULL);
		if (cur_modif) {
			if (cur_modif->type == TCP_MODIF_ERASE) {
				assert(pos->chunk);
				pos->chunk_offset += cur_modif->length;
				assert(pos->chunk->start_seq+pos->chunk_offset <= pos->chunk->end_seq);
				pos->modif = cur_modif;
				pos->modif_offset = 1;
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}

	return true;
}

static size_t tcp_stream_position_read_step(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos, uint8 *data, size_t length)
{
	struct tcp_stream_chunk_modif *next_modif = NULL;
	struct tcp_stream_chunk_modif *current_modif = NULL;
	size_t modif_offset;

	if (!tcp_stream_position_advance(tcp_s, pos)) {
		return (size_t)-1;
	}

	current_modif = tcp_stream_position_modif(pos, &modif_offset, NULL, &next_modif);
	if (current_modif) {
		pos->modif = current_modif;
		pos->modif_offset = modif_offset;

		assert(current_modif->type == TCP_MODIF_INSERT);
		assert(pos->modif_offset < current_modif->length);

		const size_t maxlength = MIN(length, current_modif->length - pos->modif_offset);
		if (data) memcpy(data, current_modif->data + pos->modif_offset, maxlength);
		pos->modif_offset += maxlength;
		pos->current_seq_modif += maxlength;
		return maxlength;
	}
	else if (pos->chunk) {
		size_t maxlength;

		if (next_modif) {
			maxlength = MIN(next_modif->position - pos->chunk_offset, length);
		}
		else {
			maxlength = MIN((pos->chunk->end_seq - pos->chunk->start_seq) - pos->chunk_offset, length);
		}

		if (data) {
			if (pos->chunk->tcp) {
				memcpy(data, tcp_get_payload(pos->chunk->tcp) + pos->chunk_offset, maxlength);
			}
			else {
				memcpy(data, pos->chunk->data + pos->chunk_offset, maxlength);
			}
		}

		pos->chunk_offset += maxlength;
		pos->current_seq_modif += maxlength;
		return maxlength;
	}
	else {
		return (size_t)-1;
	}
}

static size_t tcp_stream_position_read(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos, uint8 *data, size_t length)
{
	size_t left_len = length;

	while (left_len > 0) {
		const size_t len = tcp_stream_position_read_step(tcp_s, pos, data, left_len);
		if (len == (size_t)-1) {
			break;
		}

		left_len -= len;
		if (data) data += len;
	}

	return length - left_len;
}

static size_t tcp_stream_position_skip_available(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos)
{
	size_t chunk_length, length, total_length  = 0;

	while (true) {
		if (!tcp_stream_position_advance(tcp_s, pos)) {
			break;
		}

		if (pos->chunk) {
			chunk_length = pos->chunk->end_seq - pos->chunk->start_seq + pos->chunk->offset_seq;
			length = chunk_length - (pos->current_seq_modif - pos->chunk_seq_modif);
			pos->chunk_offset = pos->chunk->end_seq - pos->chunk->start_seq;

			if (!pos->modif) pos->modif = pos->chunk->modifs;

			if (pos->modif) {
				while (list_next(pos->modif)) {
					pos->modif = list_next(pos->modif);

					switch (pos->modif->type) {
					case TCP_MODIF_INSERT:
						pos->modif_offset = pos->modif->length;
						break;

					case TCP_MODIF_ERASE:
						pos->modif_offset = 1;
						break;

					default:
						assert(0);
					}
				}
			}
			else {
				pos->modif_offset = (size_t)-1;
			}
		}
		else if (pos->modif) {
			assert(pos->modif->type == TCP_MODIF_INSERT);
			length = pos->modif->length - pos->modif_offset;
			pos->modif_offset = pos->modif->length;
		}
		else {
			break;
		}

		pos->current_seq_modif += length;
		total_length += length;
	}

	return total_length;
}

static void tcp_stream_position_try_advance_chunk(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos, struct tcp_stream_chunk *chunk)
{
	if (pos->chunk && pos->chunk == chunk) {
		if (tcp_stream_position_chunk_at_end(pos)) {
			tcp_stream_position_next_chunk(tcp_s, pos);
		}
	}
}

UNUSED static bool tcp_stream_position_isbefore(struct tcp_stream_position *pos1,
		struct tcp_stream_position *pos2)
{
	return (pos1->current_seq_modif <= pos2->current_seq_modif);
}

static void tcp_stream_position_getmark(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos, struct tcp_stream_mark *mark)
{
	struct tcp_stream_position advanced_pos = *pos;
	struct tcp_stream_chunk_modif *modif;
	size_t modif_offset;

	mark->chunk_seq = advanced_pos.chunk_seq;
	mark->chunk_offset = advanced_pos.chunk_offset;

	modif = tcp_stream_position_modif(&advanced_pos, &modif_offset, NULL, NULL);
	if (modif) {
		if (modif->type == TCP_MODIF_INSERT)
			mark->modif_offset = modif_offset;
		else
			mark->modif_offset = (size_t)-1;
	}
	else {
		mark->modif_offset = 0;
	}
}

static int tcp_stream_mark_compare(struct tcp_stream_mark *pos1,
		struct tcp_stream_mark *pos2)
{
	if (pos1->chunk_seq < pos2->chunk_seq) {
		return -1;
	}
	else if (pos1->chunk_seq == pos2->chunk_seq) {
		if (pos1->chunk_offset < pos2->chunk_offset) {
			return -1;
		}
		else if (pos1->chunk_offset == pos2->chunk_offset) {
			if (pos1->modif_offset != (size_t)-1) {
				if (pos2->modif_offset != (size_t)-1) {
					if (pos1->modif_offset < pos2->modif_offset) return -1;
					else if (pos1->modif_offset == pos2->modif_offset) return 0;
					else return +1;
				}
				else {
					return -1;
				}
			}
			else {
				if (pos2->modif_offset != (size_t)-1) {
					return +1;
				}
				else {
					return 0;
				}
			}
		}
		else {
			return +1;
		}
	}
	else {
		return +1;
	}
}

static bool tcp_stream_position_moveforward(struct tcp_stream *tcp_s,
		struct tcp_stream_position *pos, struct tcp_stream_mark *mark)
{
	size_t chunk_length, length, total_length  = 0;
	struct tcp_stream_mark current;
	struct tcp_stream_chunk_modif *modif;
	size_t modif_offset;

	tcp_stream_position_getmark(tcp_s, pos, &current);
	if (tcp_stream_mark_compare(&current, mark) > 0) {
		error(L"invalid mark");
		return false;
	}

	while (true) {
		if (!tcp_stream_position_advance(tcp_s, pos)) {
			break;
		}

		assert(!pos->chunk || pos->chunk->start_seq <= mark->chunk_seq);

		if (pos->chunk) {
			if (pos->chunk->end_seq > mark->chunk_seq) {
				/* Correct chunk */
				struct tcp_stream_position saved_pos = *pos;
				while (saved_pos.chunk_offset < mark->chunk_offset) {
					*pos = saved_pos;
					tcp_stream_position_read_step(tcp_s, &saved_pos, NULL, (size_t)-1);
				}

				pos->chunk_offset = mark->chunk_offset;

				if (!tcp_stream_position_advance(tcp_s, pos)) {
					break;
				}

				if (pos->chunk_offset == mark->chunk_offset) {
					modif = tcp_stream_position_modif(pos, &modif_offset, NULL, NULL);
					if (modif) {
						assert(modif->type == TCP_MODIF_INSERT);
						pos->modif_offset = mark->modif_offset;
						if (pos->modif_offset > modif->length) {
							pos->modif_offset = modif->length;
						}
					}
				}

				break;
			}
			else {
				/* Advance to next chunk */
				chunk_length = pos->chunk->end_seq - pos->chunk->start_seq + pos->chunk->offset_seq;
				length = chunk_length - (pos->current_seq_modif - pos->chunk_seq_modif);
				pos->chunk_offset = pos->chunk->end_seq - pos->chunk->start_seq;

				if (!pos->modif) pos->modif = pos->chunk->modifs;

				if (pos->modif) {
					while (list_next(pos->modif)) {
						pos->modif = list_next(pos->modif);

						switch (pos->modif->type) {
						case TCP_MODIF_INSERT:
							pos->modif_offset = pos->modif->length;
							break;

						case TCP_MODIF_ERASE:
							pos->modif_offset = 1;
							break;

						default:
							assert(0);
						}
					}
				}
				else {
					pos->modif_offset = (size_t)-1;
				}
			}
		}
		else if (pos->modif) {
			assert(pos->modif->type == TCP_MODIF_INSERT);
			length = pos->modif->length - pos->modif_offset;
			pos->modif_offset = pos->modif->length;
		}
		else {
			break;
		}

		pos->current_seq_modif += length;
		total_length += length;
	}

	tcp_stream_position_getmark(tcp_s, pos, &current);
	return tcp_stream_mark_compare(&current, mark) == 0;
}


/*
 * Stream functions
 */

#define TCP_STREAM(s) struct tcp_stream *tcp_s = (struct tcp_stream *)(s); \
	assert((s)->ftable == &tcp_stream_ftable);

struct stream *tcp_stream_create()
{
	struct tcp_stream *tcp_s = malloc(sizeof(struct tcp_stream));
	if (!tcp_s) {
		error(L"memory error");
		return NULL;
	}

	memset(tcp_s, 0, sizeof(struct tcp_stream));

	lua_object_init(&tcp_s->stream.lua_object);
	tcp_s->start_seq = (uint32)-1;
	tcp_stream_position_invalidate(&tcp_s->mark_position);

	tcp_s->stream.ftable = &tcp_stream_ftable;
	return &tcp_s->stream;
}

static void tcp_stream_chunk_release(struct tcp_stream_chunk *chunk, bool modif)
{
	if (modif) {
		struct tcp_stream_chunk_modif *iter = chunk->modifs;
		while (iter) {
			struct tcp_stream_chunk_modif *modif = iter;
			iter = list_next(iter);
			free(modif);
		}
		chunk->modifs = NULL;
	}

	free(chunk->data);
	chunk->data = NULL;
}

static bool tcp_stream_destroy(struct stream *s)
{
	struct tcp_stream_chunk *iter;
	struct tcp_stream_mark *mark;
	TCP_STREAM(s);

	lua_object_release(&tcp_s->stream, &tcp_s->stream.lua_object);

	iter = tcp_s->begin;
	while (iter) {
		struct tcp_stream_chunk *tmp = iter;

		if (tmp->tcp) tcp_release(tmp->tcp);
		tcp_stream_chunk_release(tmp, true);

		iter = list_next(tmp);
		free(tmp);
	}

	mark = tcp_s->marks;
	while (mark) {
		struct tcp_stream_mark *tmp = mark;
		mark = list_next(mark);
		free(tmp);
	}

	free(tcp_s->pending_modif);
	free(s);

	return true;
}

void tcp_stream_init(struct stream *s, uint32 seq)
{
	TCP_STREAM(s);

	tcp_s->start_seq = seq;
	tcp_s->last_seq = 0;
}

static uint64 tcp_remap_seq(uint32 seq, uint64 ref)
{
	uint64 ret = seq + ((ref>>32)<<32);

	if (ret + (1LL<<31) < ref) {
		ret += (1ULL<<32);
	}

	return ret;
}

bool tcp_stream_push(struct stream *s, struct tcp *tcp)
{
	struct tcp_stream_chunk *chunk;
	size_t ref_seq;
	TCP_STREAM(s);

	if (tcp_s->start_seq == (size_t)-1) {
		error(L"uninitialized stream");
		return false;
	}

	chunk = malloc(sizeof(struct tcp_stream_chunk));
	if (!chunk) {
		error(L"memory error");
		return false;
	}

	chunk->tcp = tcp;
	chunk->data = NULL;
	list_init(chunk);

	if (tcp_s->last_sent) ref_seq = tcp_s->last_sent->start_seq + tcp_s->start_seq;
	else ref_seq = tcp_s->start_seq;

	chunk->start_seq = tcp_remap_seq(tcp_get_seq(tcp), ref_seq);
	chunk->start_seq -= tcp_s->start_seq;
	chunk->end_seq = chunk->start_seq + tcp_get_payload_length(tcp);
	chunk->offset_seq = 0;
	chunk->modifs = NULL;

	if (chunk->start_seq < tcp_s->current_position.chunk_seq + tcp_s->current_position.chunk_offset ||
		chunk->end_seq < tcp_s->current_position.chunk_seq + tcp_s->current_position.chunk_offset) {
		message(HAKA_LOG_WARNING, L"tcp-connection", L"retransmit packet (ignored)");
		tcp_action_drop(tcp);
		free(chunk);
		return false;
	}

	/* Search for insert point */
	if (!tcp_s->last) {
		assert(!tcp_s->first);
		list_insert_after(chunk, NULL, &tcp_s->begin, &tcp_s->end);
		tcp_s->first = chunk;
		tcp_s->last = chunk;
	}
	else {
		struct tcp_stream_chunk *iter, *prev = NULL, **start;

		if (tcp_s->last->start_seq < chunk->start_seq) {
			start = &tcp_s->last;
		}
		else {
			start = &tcp_s->first;
		}

		iter = *start;
		assert(iter);

		while (iter && iter->start_seq <= chunk->start_seq) {
			prev = iter;
			iter = list_next(iter);
		}

		if (iter && chunk->end_seq > iter->start_seq) {
			message(HAKA_LOG_WARNING, L"tcp-connection", L"retransmit packet (ignored)");
			tcp_action_drop(tcp);
			free(chunk);
			return false;
		}

		if (prev) {
			list_insert_after(chunk, prev, &tcp_s->begin, &tcp_s->end);
		}
		else {
			list_insert_before(chunk, *start, &tcp_s->begin, &tcp_s->end);
			*start = chunk;
		}

		if (tcp_s->pending_modif) {
			assert(!chunk->modifs);
			assert(!list_next(tcp_s->pending_modif) && !list_prev(tcp_s->pending_modif));
			assert(tcp_s->pending_modif->position == 0);
			chunk->modifs = tcp_s->pending_modif;
			chunk->offset_seq += chunk->modifs->length;
			tcp_s->pending_modif = NULL;
		}

		tcp_s->last = chunk;
	}

	return true;
}

void tcp_stream_seq(struct stream *s, struct tcp *tcp)
{
	if (packet_mode() == MODE_NORMAL) {
		uint32 ack;

		TCP_STREAM(s);

		ack = tcp_get_seq(tcp) + tcp_s->first_offset_seq;
		if (ack != tcp_get_seq(tcp)) {
			tcp_set_seq(tcp, ack);
		}
	}
}

uint32 tcp_stream_lastseq(struct stream *s)
{
	TCP_STREAM(s);

	return tcp_s->start_seq + tcp_s->last_seq;
}

struct tcp *tcp_stream_pop(struct stream *s)
{
	struct tcp *tcp;
	struct tcp_stream_chunk *chunk;
	struct tcp_stream_position local_pos;
	struct tcp_stream_position *read_pos, *write_pos;
	TCP_STREAM(s);

	chunk = tcp_s->first;

	assert(!tcp_stream_position_isvalid(&tcp_s->mark_position) ||
			tcp_stream_position_isbefore(&tcp_s->mark_position, &tcp_s->current_position));

	read_pos = &tcp_s->current_position;
	tcp_stream_position_advance(tcp_s, read_pos);

	if (tcp_stream_position_isvalid(&tcp_s->mark_position)) {
		struct tcp_stream_mark *tcp_mark = tcp_s->marks;
		assert(tcp_mark);

		tcp_stream_position_try_advance_chunk(tcp_s, read_pos, chunk);

		read_pos = &tcp_s->mark_position;

		tcp_stream_position_advance(tcp_s, read_pos);
		tcp_stream_position_try_advance_chunk(tcp_s, read_pos, chunk);

		local_pos = *read_pos;
		write_pos = &local_pos;

		while (tcp_mark->readonly && list_next(tcp_mark)) {
			tcp_stream_position_moveforward(tcp_s, write_pos, list_next(tcp_mark));
			tcp_mark = list_next(tcp_mark);
		}

		tcp_stream_position_advance(tcp_s, write_pos);

		if (tcp_mark->readonly) {
			tcp_stream_position_skip_available(tcp_s, write_pos);
		}

		tcp_stream_position_try_advance_chunk(tcp_s, write_pos, chunk);
	}
	else {
		/* Force a skip of all available data */
		tcp_stream_position_skip_available(tcp_s, read_pos);
		tcp_stream_position_try_advance_chunk(tcp_s, read_pos, chunk);

		write_pos = read_pos;
	}

	if (chunk && tcp_stream_position_chunk_is_before(write_pos, chunk)) {
		const bool keepdata = !tcp_stream_position_chunk_is_before(read_pos, chunk);

		tcp = chunk->tcp;
		assert(tcp || chunk->data);

		if (keepdata && !chunk->data) {
			const size_t size = tcp_get_payload_length(chunk->tcp);

			chunk->data = malloc(size);
			if (!chunk->data) {
				error(L"memory error");
				return NULL;
			}

			memcpy(chunk->data, tcp_get_payload(chunk->tcp), size);
		}

		if (chunk->modifs) {
			/*
			 * Apply modifs to tcp packet
			 */
			const size_t new_size = chunk->end_seq - chunk->start_seq + chunk->offset_seq;
			struct tcp_stream_position pos;
			uint8 *buffer, *payload;
			UNUSED size_t size;

			assert(tcp);

			if (chunk->end_seq - chunk->start_seq > 0 && new_size == 0) {
				tcp_action_drop(tcp);
			}
			else {
				pos.chunk = chunk;
				pos.chunk_offset = 0;
				pos.modif = NULL;
				pos.modif_offset = 0;
				pos.chunk_seq = chunk->start_seq;
				pos.chunk_seq_modif = chunk->start_seq + tcp_s->first_offset_seq;
				pos.current_seq_modif = pos.chunk_seq_modif;

				buffer = malloc(new_size);
				if (!buffer) {
					error(L"memory error");
					return NULL;
				}

				size = tcp_stream_position_read(tcp_s, &pos, buffer, new_size);
				assert(size == new_size);

				payload = tcp_resize_payload(tcp, new_size);
				if (!payload) {
					assert(check_error());
					free(buffer);
					return NULL;
				}

				memcpy(payload, buffer, new_size);
				free(buffer);
			}
		}

		if (tcp && tcp->packet) {
			tcp_stream_seq(s, tcp);
		}

		tcp_s->first_offset_seq += chunk->offset_seq;
		tcp_s->last_seq = chunk->end_seq + tcp_s->first_offset_seq;
		tcp_s->first = list_next(tcp_s->first);

		if (tcp_s->last == chunk) {
			tcp_s->last = list_next(chunk);
		}

		if (tcp_s->last_sent) {
			assert(tcp_s->last_sent->end_seq == chunk->start_seq);
			assert(tcp_s->last_sent == list_prev(chunk));
			tcp_s->last_sent = chunk;
		}
		else {
			tcp_s->last_sent = chunk;
			tcp_s->sent = chunk;
		}

		chunk->tcp = NULL;

		tcp_stream_chunk_release(chunk, false);

		if (tcp) {
			return tcp;
		}
	}

	if (packet_mode() == MODE_PASSTHROUGH) {
		/*
		 * If no packet as been sent, send our last received packet.
		 */
		chunk = tcp_s->last;
		if (chunk && chunk->tcp) {
			tcp = chunk->tcp;
			size_t size = tcp_get_payload_length(tcp);

			chunk->data = malloc(size);
			if (!chunk->data) {
				error(L"memory error");
				return NULL;
			}

			memcpy(chunk->data, tcp_get_payload(tcp), size);

			chunk->tcp = NULL;
			return tcp;
		}
	}

	return NULL;
}

void tcp_stream_ack(struct stream *s, struct tcp *tcp)
{
	if (packet_mode() == MODE_NORMAL) {
		uint64 ack = tcp_get_ack_seq(tcp);
		uint64 seq, new_seq;
		struct tcp_stream_chunk *iter;
		TCP_STREAM(s);

		iter = tcp_s->sent;
		if (!iter) {
			return;
		}

		seq = tcp_s->sent_offset_seq + tcp_s->sent->start_seq;
		ack = tcp_remap_seq(ack, seq);
		ack -= tcp_s->start_seq;

		new_seq = tcp_s->sent->start_seq;

		while (iter && !iter->tcp) {
			if (seq + (iter->end_seq - iter->start_seq) + iter->offset_seq > ack) {
				break;
			}

			seq += (iter->end_seq - iter->start_seq) + iter->offset_seq;
			new_seq = iter->end_seq;
			if (ack <= seq) {
				break;
			}

			assert(!list_next(iter) || list_next(iter)->tcp || list_next(iter)->start_seq == iter->end_seq);

			iter = list_next(iter);
		}

		/* Find the exact ack position taking into account the modifs */
		if (iter)
		{
			int offset = 0;

			struct tcp_stream_chunk_modif *modif = iter->modifs;
			while (modif) {
				if (seq + (modif->position - offset) >= ack) {
					break;
				}
				else {
					new_seq = iter->start_seq + modif->position;
					seq += modif->position - offset;
					offset = modif->position;

					switch (modif->type) {
					case TCP_MODIF_INSERT:
						if (seq + modif->length > ack) {
							seq = ack;
						}
						else {
							seq += modif->length;
						}
						break;

					case TCP_MODIF_ERASE:
						new_seq += modif->length;
						offset += modif->length;
						break;

					default:
						assert(0);
						break;
					}
				}

				modif = list_next(modif);
			}
		}

		/* Need to account for offset (case of the FIN for instance) */
		new_seq += ack-seq;
		new_seq += tcp_s->start_seq;

		if (tcp_get_ack_seq(tcp) != new_seq) {
			tcp_set_ack_seq(tcp, new_seq);
		}
	}
}

static size_t tcp_stream_read(struct stream *s, uint8 *data, size_t length)
{
	TCP_STREAM(s);
	return tcp_stream_position_read(tcp_s, &tcp_s->current_position, data, length);
}

static size_t tcp_stream_available(struct stream *s)
{
	struct tcp_stream_position position;
	TCP_STREAM(s);

	position = tcp_s->current_position;
	return tcp_stream_position_skip_available(tcp_s, &position);
}

static struct tcp_stream_chunk_modif *tcp_stream_create_insert_modif(struct tcp_stream *tcp_s,
		struct tcp_stream_chunk_modif *prev, struct tcp_stream_chunk_modif *next,
		struct tcp_stream_chunk_modif **head, const uint8 *data, size_t length)
{
	struct tcp_stream_chunk_modif *new_modif = NULL;
	struct tcp_stream_position *pos;

	pos = &tcp_s->current_position;

	new_modif = malloc(sizeof(struct tcp_stream_chunk_modif) + length);
	if (!new_modif) {
		error(L"memory error");
		return NULL;
	}

	list_init(new_modif);
	new_modif->type = TCP_MODIF_INSERT;
	new_modif->position = pos->chunk_offset;
	new_modif->length = length;
	memcpy(new_modif->data, data, length);

	if (prev) {
		list_insert_after(new_modif, prev, head, NULL);
	}
	else {
		list_insert_before(new_modif, next, head, NULL);
	}

	if (pos->chunk) pos->chunk->offset_seq += length;

	pos->modif = new_modif;
	pos->modif_offset = length;
	pos->current_seq_modif += length;

	return new_modif;
}

static size_t tcp_stream_update_insert_modif(struct tcp_stream *tcp_s,
		struct tcp_stream_chunk_modif *cur_modif, struct tcp_stream_chunk_modif **modif_head,
		const uint8 *data, size_t length, size_t modif_offset)
{
	struct tcp_stream_chunk_modif *new_modif = NULL;
	struct tcp_stream_position *pos;

	assert(cur_modif->type == TCP_MODIF_INSERT);
	pos = &tcp_s->current_position;

	new_modif = malloc(sizeof(struct tcp_stream_chunk_modif) + length + cur_modif->length);
	if (!new_modif) {
		error(L"memory error");
		return -1;
	}

	list_init(new_modif);
	new_modif->type = TCP_MODIF_INSERT;
	new_modif->position = cur_modif->position;
	new_modif->length = cur_modif->length + length;

	memcpy(new_modif->data, cur_modif->data, modif_offset);
	memcpy(new_modif->data + modif_offset, data, length);
	memcpy(new_modif->data + modif_offset + length,
			cur_modif->data + modif_offset,
			cur_modif->length - modif_offset);

	list_insert_before(new_modif, cur_modif, modif_head, NULL);
	list_remove(cur_modif, modif_head, NULL);

	if (pos->chunk) pos->chunk->offset_seq += length;
	free(cur_modif);

	pos->modif = new_modif;
	pos->modif_offset = modif_offset + length;
	pos->current_seq_modif += length;

	return length;
}

static size_t tcp_stream_insert(struct stream *s, const uint8 *data, size_t length)
{
	struct tcp_stream_chunk_modif *prev_modif = NULL;
	struct tcp_stream_chunk_modif *next_modif = NULL;
	struct tcp_stream_chunk_modif *cur_modif = NULL;
	struct tcp_stream_position *pos;
	size_t modif_offset;
	TCP_STREAM(s);

	pos = &tcp_s->current_position;
	tcp_stream_position_advance(tcp_s, pos);

	if (!pos->chunk) {
		if (tcp_s->pending_modif) {
			return tcp_stream_update_insert_modif(tcp_s, tcp_s->pending_modif,
					&tcp_s->pending_modif, data, length, pos->modif_offset);
		}
		else {
			tcp_s->pending_modif = tcp_stream_create_insert_modif(tcp_s, NULL,
					NULL, &tcp_s->pending_modif, data, length);
			if (!tcp_s->pending_modif) {
				return -1;
			}
			return length;
		}
	}
	else {
		if (!pos->chunk->tcp) {
			error(L"stream is read-only at this position (data already sent)");
			return -1;
		}

		cur_modif = tcp_stream_position_modif(pos, &modif_offset, &prev_modif, &next_modif);
		if (cur_modif) {
			return tcp_stream_update_insert_modif(tcp_s, cur_modif, &pos->chunk->modifs, data, length,
					modif_offset);
		}
		else {
			cur_modif = tcp_stream_create_insert_modif(tcp_s, prev_modif, next_modif,
					&pos->chunk->modifs, data, length);
			if (!cur_modif) {
				return -1;
			}
			return length;
		}
	}
}

static size_t tcp_stream_replace(struct stream *s, const uint8 *data, size_t length)
{
	const size_t ret = tcp_stream_insert(s, data, length);
	tcp_stream_erase(s, length);
	return ret;
}

static size_t tcp_stream_erase(struct stream *s, size_t length)
{
	struct tcp_stream_chunk_modif *prev_modif = NULL;
	struct tcp_stream_chunk_modif *next_modif = NULL;
	struct tcp_stream_chunk_modif *cur_modif = NULL;
	struct tcp_stream_position *pos;
	size_t modif_offset, erase_length;
	TCP_STREAM(s);

	pos = &tcp_s->current_position;

	if (!tcp_stream_position_advance(tcp_s, pos)) {
		return 0;
	}

	if (pos->chunk && !pos->chunk->tcp) {
		error(L"stream is read-only at this position (data already sent)");
		return -1;
	}

	cur_modif = tcp_stream_position_modif(pos, &modif_offset, &prev_modif, &next_modif);
	if (cur_modif) {
		assert(cur_modif->type == TCP_MODIF_INSERT);
		struct tcp_stream_chunk_modif *new_modif = NULL;
		size_t max_erase;

		max_erase = cur_modif->length - pos->modif_offset;
		erase_length = MIN(max_erase, length);

		if (cur_modif->length == erase_length) {
			/* Remove modif */
			list_remove(cur_modif, &pos->chunk->modifs, NULL);

			if (pos->modif == cur_modif) {
				pos->modif = prev_modif;
				pos->modif_offset = (size_t)-1;
			}
		}
		else {
			/* Modify an existing modif */
			new_modif = malloc(sizeof(struct tcp_stream_chunk_modif) + cur_modif->length - erase_length);
			if (!new_modif) {
				error(L"memory error");
				return -1;
			}

			list_init(new_modif);
			new_modif->type = TCP_MODIF_INSERT;
			new_modif->position = cur_modif->position;
			new_modif->length = cur_modif->length - erase_length;

			memcpy(new_modif->data, cur_modif->data, modif_offset);
			memcpy(new_modif->data + modif_offset,
					cur_modif->data + modif_offset + erase_length,
					cur_modif->length - modif_offset - erase_length);

			list_insert_after(new_modif, cur_modif, &pos->chunk->modifs, NULL);
			list_remove(cur_modif, &pos->chunk->modifs, NULL);

			if (pos->modif == cur_modif) {
				pos->modif = new_modif;
			}
		}

		pos->chunk->offset_seq -= erase_length;
		free(cur_modif);
	}
	else {
		size_t max_erase;

		if (next_modif) {
			max_erase = next_modif->position - pos->chunk_offset;
		}
		else {
			max_erase = (pos->chunk->end_seq - pos->chunk->start_seq) - pos->chunk_offset;
		}

		erase_length = MIN(max_erase, length);

		/* Create a new modif */
		cur_modif = malloc(sizeof(struct tcp_stream_chunk_modif));
		if (!cur_modif) {
			error(L"memory error");
			return -1;
		}

		list_init(cur_modif);
		cur_modif->type = TCP_MODIF_ERASE;
		cur_modif->position = pos->chunk_offset;
		cur_modif->length = erase_length;

		if (prev_modif) {
			list_insert_after(cur_modif, prev_modif, &pos->chunk->modifs, NULL);
		}
		else {
			list_insert_before(cur_modif, next_modif, &pos->chunk->modifs, NULL);
		}

		pos->chunk->offset_seq -= erase_length;

		pos->modif = cur_modif;
		pos->modif_offset = 1;
		pos->chunk_offset += erase_length;
	}

	if (erase_length > 0 && erase_length < length) {
		return erase_length + tcp_stream_erase(s, length-erase_length);
	}
	else {
		return erase_length;
	}
}

static struct stream_mark *tcp_stream_mark(struct stream *s, bool readonly)
{
	struct tcp_stream_mark *mark = NULL;
	TCP_STREAM(s);

	mark = malloc(sizeof(struct tcp_stream_mark));
	if (!mark) {
		error(L"memory error");
		return NULL;
	}

	tcp_stream_position_getmark(tcp_s, &tcp_s->current_position, mark);
	mark->stream = tcp_s;
	list_init(mark);

	{
		struct tcp_stream_mark *iter = tcp_s->marks;
		if (!iter) {
			tcp_s->marks = mark;
		}
		else {
			while (list_next(iter) && tcp_stream_mark_compare(list_next(iter), mark) <= 0) {
				iter = list_next(iter);
			}

			list_insert_after(mark, iter, &tcp_s->marks, NULL);
		}
	}

	if (!tcp_stream_position_isvalid(&tcp_s->mark_position)) {
		tcp_s->mark_position = tcp_s->current_position;
	}

	mark->readonly = readonly;

	return (struct stream_mark *)mark;
}

static void tcp_stream_chunk_release_data(struct tcp_stream *tcp_s, struct tcp_stream_chunk *chunk,
		struct tcp_stream_position *pos)
{
	tcp_stream_position_advance(tcp_s, pos);

	while (chunk) {
		tcp_stream_position_try_advance_chunk(tcp_s, pos, chunk);

		if (!tcp_stream_position_chunk_is_before(pos, chunk)) {
			break;
		}

		tcp_stream_chunk_release(chunk, false);
		chunk = list_next(chunk);
	}
}

static bool tcp_stream_unmark(struct stream *s, struct stream_mark *mark)
{
	struct tcp_stream_mark *tcp_mark = (struct tcp_stream_mark *)mark;
	TCP_STREAM(s);

	assert(tcp_mark);

	if (tcp_mark->stream != tcp_s) {
		error(L"invalid mark");
		return false;
	}

	if (!list_prev(tcp_mark)) {
		if (list_next(tcp_mark)) {
			struct tcp_stream_chunk *chunk;
			struct tcp_stream_mark current;
			tcp_stream_position_getmark(tcp_s, &tcp_s->mark_position, &current);

			if (tcp_stream_mark_compare(&current, tcp_mark) > 0) {
				error(L"invalid mark");
				return false;
			}

			chunk = tcp_s->mark_position.chunk;

			tcp_stream_position_moveforward(tcp_s, &tcp_s->mark_position, list_next(tcp_mark));

			tcp_stream_chunk_release_data(tcp_s, chunk, &tcp_s->mark_position);
		}
		else {
			tcp_stream_position_invalidate(&tcp_s->mark_position);
		}
	}

	list_remove(tcp_mark, &tcp_s->marks, NULL);

	free(tcp_mark);
	return true;
}

static bool tcp_stream_seek(struct stream *s, struct stream_mark *mark, bool unmark)
{
	struct tcp_stream_mark current;
	struct tcp_stream_mark *tcp_mark = (struct tcp_stream_mark *)mark;
	TCP_STREAM(s);

	assert(tcp_mark);

	if (tcp_mark->stream != tcp_s) {
		error(L"invalid mark");
		return false;
	}

	if (!tcp_stream_position_isvalid(&tcp_s->mark_position)) {
		error(L"invalid mark");
		return false;
	}

	tcp_stream_position_getmark(tcp_s, &tcp_s->current_position, &current);
	if (tcp_stream_mark_compare(&current, tcp_mark) > 0) {
		tcp_s->current_position = tcp_s->mark_position;
	}

	tcp_stream_position_moveforward(tcp_s, &tcp_s->current_position, tcp_mark);

	if (unmark) {
		tcp_stream_unmark(s, mark);
	}

	return true;
}

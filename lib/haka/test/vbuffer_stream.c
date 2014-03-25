/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <check.h>
#include <haka/vbuffer_stream.h>
#include <haka/error.h>
#include <haka/types.h>

#define ck_check_error     if (check_error()) { ck_abort_msg("Error: %ls", clear_error()); return; }

#define NUM 5
static const int sizes[NUM] = { 150, 213, 10, 0, 1523 };
static const int mark_offsets[NUM] = { 12, 213, 0, 0, 1244 };


START_TEST(test_create)
{
	struct vbuffer_stream stream;
	ck_assert(vbuffer_stream_init(&stream, NULL));
	ck_check_error;

	vbuffer_stream_clear(&stream);
	ck_check_error;
}
END_TEST

START_TEST(test_push_pop)
{
	int i;
	struct vbuffer_stream stream;
	ck_assert(vbuffer_stream_init(&stream, NULL));
	ck_check_error;

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		vbuffer_create_new(&buffer, sizes[i], true);
		vbuffer_stream_push(&stream, &buffer, NULL);
		ck_assert_int_eq(vbuffer_size(vbuffer_stream_data(&stream)), sizes[i]);
		ck_check_error;

		ck_assert(vbuffer_stream_pop(&stream, &buffer, NULL));
		ck_assert_int_eq(vbuffer_size(&buffer), sizes[i]);
		vbuffer_clear(&buffer);
		ck_check_error;
	}

	vbuffer_stream_finish(&stream);

	vbuffer_stream_clear(&stream);
	ck_check_error;
}
END_TEST


START_TEST(test_push_pop_interleaved)
{
	int i;
	size_t size = 0;
	struct vbuffer_stream stream;
	ck_assert(vbuffer_stream_init(&stream, NULL));
	ck_check_error;

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		vbuffer_create_new(&buffer, sizes[i], true);
		vbuffer_stream_push(&stream, &buffer, NULL);
		size += sizes[i];
		ck_assert_int_eq(vbuffer_size(vbuffer_stream_data(&stream)), size);
		ck_check_error;
	}

	vbuffer_stream_finish(&stream);

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		ck_assert(vbuffer_stream_pop(&stream, &buffer, NULL));
		ck_assert_int_eq(vbuffer_size(&buffer), sizes[i]);
		vbuffer_clear(&buffer);
		ck_check_error;
	}

	vbuffer_stream_clear(&stream);
	ck_check_error;
}
END_TEST

START_TEST(test_mark)
{
	int i;
	size_t size = 0;
	struct vbuffer_stream stream;
	struct vbuffer_iterator marks[NUM];
	ck_assert(vbuffer_stream_init(&stream, NULL));
	ck_check_error;

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		vbuffer_create_new(&buffer, sizes[i], true);
		vbuffer_position(&buffer, &marks[i], mark_offsets[i]);
		ck_assert(vbuffer_iterator_mark(&marks[i], false));

		vbuffer_stream_push(&stream, &buffer, NULL);
		size += sizes[i];
		ck_assert_int_eq(vbuffer_size(vbuffer_stream_data(&stream)), size);
		ck_check_error;
	}

	vbuffer_stream_finish(&stream);

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		ck_assert(!vbuffer_stream_pop(&stream, &buffer, NULL));
		ck_assert(vbuffer_iterator_unmark(&marks[i]));

		ck_assert(vbuffer_stream_pop(&stream, &buffer, NULL));
		ck_assert_int_eq(vbuffer_size(&buffer), sizes[i]);
		size -= sizes[i];
		ck_assert_int_eq(vbuffer_size(vbuffer_stream_data(&stream)), size);
		vbuffer_clear(&buffer);
		ck_check_error;
	}

	vbuffer_stream_clear(&stream);
	ck_check_error;
}
END_TEST

START_TEST(test_eof)
{
	int i;
	size_t size = 0;
	struct vbuffer_stream stream;
	struct vbuffer_iterator iter;
	ck_assert(vbuffer_stream_init(&stream, NULL));
	ck_check_error;

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		vbuffer_create_new(&buffer, sizes[i], true);
		vbuffer_stream_push(&stream, &buffer, NULL);
		size += sizes[i];
		ck_assert_int_eq(vbuffer_size(vbuffer_stream_data(&stream)), size);
		ck_check_error;
	}

	vbuffer_begin(vbuffer_stream_data(&stream), &iter);
	ck_assert(!vbuffer_iterator_iseof(&iter));

	vbuffer_iterator_advance(&iter, 1<<30);
	ck_assert_int_eq(vbuffer_iterator_available(&iter), 0);
	ck_assert(!vbuffer_iterator_iseof(&iter));

	vbuffer_stream_finish(&stream);

	ck_assert(vbuffer_iterator_iseof(&iter));

	for (i=0; i<NUM; ++i) {
		struct vbuffer buffer;
		ck_assert(vbuffer_stream_pop(&stream, &buffer, NULL));
		ck_assert_int_eq(vbuffer_size(&buffer), sizes[i]);
		vbuffer_clear(&buffer);
		ck_check_error;
	}

	vbuffer_stream_clear(&stream);
	ck_check_error;
}
END_TEST

int main(int argc, char *argv[])
{
	int number_failed;

	Suite *suite = suite_create("vbuffer_stream_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, test_create);
	tcase_add_test(tcase, test_push_pop);
	tcase_add_test(tcase, test_push_pop_interleaved);
	tcase_add_test(tcase, test_mark);
	tcase_add_test(tcase, test_eof);
	suite_add_tcase(suite, tcase);

	SRunner *runner = srunner_create(suite);
#ifdef HAKA_DEBUG
	srunner_set_fork_status(runner, CK_NOFORK);
#endif
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <check.h>
#include <haka/vbuffer.h>
#include <haka/error.h>

#define ck_check_error     if (check_error()) { ck_abort_msg("Error"); return; }

/*
 * vbuffer
 */

START_TEST(vbuffer_test_create)
{
	clear_error();

	struct vbuffer *buffer = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer != NULL);
	vbuffer_free(buffer);
	ck_check_error;
}
END_TEST

START_TEST(vbuffer_test_insert)
{
	clear_error();

	struct vbuffer *buffer = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer != NULL);

	struct vbuffer *insert = vbuffer_create_new(12);
	ck_check_error;
	ck_assert(insert != NULL);

	vbuffer_insert(buffer, 15, insert);
	ck_check_error;
	ck_assert(vbuffer_size(buffer) == 150+12);

	vbuffer_free(buffer);
	ck_check_error;
}
END_TEST

START_TEST(vbuffer_test_insert_begin)
{
	clear_error();

	struct vbuffer *buffer = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer != NULL);

	struct vbuffer *insert = vbuffer_create_new(103);
	ck_check_error;
	ck_assert(insert != NULL);

	vbuffer_insert(buffer, 0, insert);
	ck_check_error;
	ck_assert(vbuffer_size(buffer) == 150+103);

	vbuffer_free(buffer);
	ck_check_error;
}
END_TEST

START_TEST(vbuffer_test_select)
{
	clear_error();

	struct vbuffer *buffer = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer != NULL);

	struct vbuffer *ref;
	struct vbuffer *select = vbuffer_select(buffer, 15, 100, &ref);
	ck_check_error;
	ck_assert(select != NULL);
	ck_assert(vbuffer_size(buffer) == 150-100);
	ck_assert(vbuffer_size(select) == 100);

	vbuffer_restore(ref, select);
	ck_check_error;
	ck_assert(vbuffer_size(buffer) == 150);

	vbuffer_free(buffer);
	ck_check_error;
}
END_TEST

START_TEST(vbuffer_test_select_begin)
{
	clear_error();

	struct vbuffer *buffer = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer != NULL);

	struct vbuffer *ref;
	struct vbuffer *select = vbuffer_select(buffer, 0, 100, &ref);
	ck_check_error;
	ck_assert(select != NULL);
	ck_assert(vbuffer_size(buffer) == 150-100);
	ck_assert(vbuffer_size(select) == 100);

	vbuffer_restore(ref, select);
	ck_check_error;
	ck_assert(vbuffer_size(buffer) == 150);

	vbuffer_free(buffer);
	ck_check_error;
}
END_TEST

Suite* vbuffer_suite(void)
{
	Suite *suite = suite_create("vbuffer_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, vbuffer_test_create);
	tcase_add_test(tcase, vbuffer_test_insert);
	tcase_add_test(tcase, vbuffer_test_insert_begin);
	tcase_add_test(tcase, vbuffer_test_select);
	tcase_add_test(tcase, vbuffer_test_select_begin);
	suite_add_tcase(suite, tcase);
	return suite;
}


/*
 * vbuffer_stream
 */

START_TEST(vbuffer_stream_test_create)
{
	clear_error();

	struct vbuffer_stream *stream = vbuffer_stream();
	ck_check_error;
	ck_assert(stream != NULL);
	vbuffer_stream_free(stream);
	ck_check_error;
}
END_TEST

START_TEST(vbuffer_stream_test_push_pop)
{
	clear_error();

	struct vbuffer_stream *stream = vbuffer_stream();
	ck_check_error;
	ck_assert(stream != NULL);

	struct vbuffer *buffer1 = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer1 != NULL);
	struct vbuffer *buffer2 = vbuffer_create_new(213);
	ck_check_error;
	ck_assert(buffer2 != NULL);

	vbuffer_stream_push(stream, buffer1);
	ck_check_error;
	buffer1 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer1 != NULL);
	ck_assert(vbuffer_size(buffer1) == 150);

	vbuffer_stream_push(stream, buffer2);
	ck_check_error;
	buffer2 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer2 != NULL);
	ck_assert(vbuffer_size(buffer2) == 213);

	if (buffer1) vbuffer_free(buffer1);
	if (buffer2) vbuffer_free(buffer2);
	vbuffer_stream_free(stream);
}
END_TEST

START_TEST(vbuffer_stream_test_push_pop_interleaved)
{
	clear_error();

	struct vbuffer_stream *stream = vbuffer_stream();
	ck_check_error;
	ck_assert(stream != NULL);

	struct vbuffer *buffer1 = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer1 != NULL);
	struct vbuffer *buffer2 = vbuffer_create_new(213);
	ck_check_error;
	ck_assert(buffer2 != NULL);

	vbuffer_stream_push(stream, buffer1);
	ck_check_error;
	vbuffer_stream_push(stream, buffer2);
	ck_check_error;

	buffer1 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer1 != NULL);
	ck_assert(vbuffer_size(buffer1) == 150);

	buffer2 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer2 != NULL);
	ck_assert(vbuffer_size(buffer2) == 213);

	if (buffer1) vbuffer_free(buffer1);
	if (buffer2) vbuffer_free(buffer2);
	vbuffer_stream_free(stream);
	ck_check_error;
}
END_TEST

START_TEST(vbuffer_stream_test_mark)
{
	clear_error();

	struct vbuffer_stream *stream = vbuffer_stream();
	ck_check_error;
	ck_assert(stream != NULL);

	struct vbuffer *buffer1 = vbuffer_create_new(150);
	ck_check_error;
	ck_assert(buffer1 != NULL);
	struct vbuffer *buffer2 = vbuffer_create_new(213);
	ck_check_error;
	ck_assert(buffer2 != NULL);

	vbuffer_stream_push(stream, buffer1);
	ck_check_error;

	struct vbuffer_iterator mark;
	vbuffer_iterator(vbuffer_stream_available(stream), &mark, 15, false, false);
	vbuffer_iterator_register(&mark);
	ck_check_error;

	vbuffer_stream_push(stream, buffer2);
	ck_check_error;

	buffer1 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer1 == NULL);

	vbuffer_iterator_unregister(&mark);
	ck_check_error;

	buffer1 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer1 != NULL);
	ck_assert(vbuffer_size(buffer1) == 150);

	buffer2 = vbuffer_stream_pop(stream);
	ck_check_error;
	ck_assert(buffer2 != NULL);
	ck_assert(vbuffer_size(buffer2) == 213);

	if (buffer1) vbuffer_free(buffer1);
	if (buffer2) vbuffer_free(buffer2);
	vbuffer_stream_free(stream);
	ck_check_error;
}
END_TEST

Suite* vbuffer_stream_suite(void)
{
	Suite *suite = suite_create("vbuffer_stream_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, vbuffer_stream_test_create);
	tcase_add_test(tcase, vbuffer_stream_test_push_pop);
	tcase_add_test(tcase, vbuffer_stream_test_push_pop_interleaved);
	tcase_add_test(tcase, vbuffer_stream_test_mark);
	suite_add_tcase(suite, tcase);
	return suite;
}


int main(int argc, char *argv[])
{
	int number_failed;

	SRunner *runner = srunner_create(vbuffer_suite());
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	runner = srunner_create(vbuffer_stream_suite());
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed += srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}

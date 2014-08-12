/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <check.h>
#include <haka/config.h>
#include <haka/vbuffer.h>
#include <haka/error.h>
#include <haka/types.h>

#define ck_check_error     if (check_error()) { ck_abort_msg("Error: %ls", clear_error()); return; }


static const size_t size = 150;
static const char *string = "abcdefghijklmnopqrstuvwxyz";

/* Build a buffer with 2 chunks */
static void vbuffer_test_build(struct vbuffer *buffer1)
{
	struct vbuffer buffer2 = vbuffer_init;

	ck_assert(vbuffer_create_new(buffer1, size, true));
	ck_check_error;

	ck_assert(vbuffer_create_from(&buffer2, string, strlen(string)));
	ck_check_error;

	vbuffer_append(buffer1, &buffer2);
	ck_check_error;

	vbuffer_release(&buffer2);
	ck_check_error;

	ck_assert(vbuffer_size(buffer1) == size+strlen(string));
}

START_TEST(test_create)
{
	struct vbuffer buffer = vbuffer_init;

	/* Test if the buffer creation is ok */
	vbuffer_test_build(&buffer);
	ck_check_error;

	/* Cleanup */
	vbuffer_release(&buffer);
	ck_check_error;
}
END_TEST

START_TEST(test_iterator_available)
{
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_iterator iter;

	vbuffer_test_build(&buffer);
	ck_check_error;

	/* Check the available size from the beginning */
	vbuffer_begin(&buffer, &iter);
	ck_assert_int_eq(vbuffer_iterator_available(&iter), size+strlen(string));
	ck_assert(vbuffer_iterator_check_available(&iter, size+10, NULL));
	vbuffer_iterator_clear(&iter);

	/* Check the available size from the middle of it */
	vbuffer_position(&buffer, &iter, 48);
	ck_assert_int_eq(vbuffer_iterator_available(&iter), size+strlen(string)-48);
	ck_assert(vbuffer_iterator_check_available(&iter, 10, NULL));
	vbuffer_iterator_clear(&iter);

	/* Check the available size from the end */
	vbuffer_end(&buffer, &iter);
	ck_assert_int_eq(vbuffer_iterator_available(&iter), 0);
	ck_assert(!vbuffer_iterator_check_available(&iter, 10, NULL));
	vbuffer_iterator_clear(&iter);

	vbuffer_release(&buffer);
	vbuffer_iterator_clear(&iter);
	ck_check_error;
}
END_TEST

static const size_t insert_offset = 21;

/* Build an interleaved buffer with 2 chunks */
static void vbuffer_test_build2(struct vbuffer *buffer1)
{
	struct vbuffer buffer2 = vbuffer_init;
	struct vbuffer_iterator iter;

	ck_assert(vbuffer_create_new(buffer1, size, true));
	ck_check_error;

	ck_assert(vbuffer_create_from(&buffer2, string, strlen(string)));
	ck_check_error;

	vbuffer_begin(buffer1, &iter);

	/* Do not use ck_assert_int_eq here as it will call vbuffer_iterator_advance
	 * twice in this case.
	 */
	ck_assert(vbuffer_iterator_advance(&iter, insert_offset) == insert_offset);

	ck_assert_int_eq(vbuffer_iterator_available(&iter), size-insert_offset);
	ck_check_error;

	vbuffer_iterator_insert(&iter, &buffer2, NULL);
	ck_check_error;

	vbuffer_release(&buffer2);
	ck_check_error;

	ck_assert_int_eq(vbuffer_size(buffer1), size+strlen(string));

	vbuffer_iterator_clear(&iter);
	ck_check_error;
}

START_TEST(test_insert)
{
	struct vbuffer buffer = vbuffer_init;
	vbuffer_test_build2(&buffer);
	vbuffer_clear(&buffer);
	ck_check_error;
}
END_TEST

START_TEST(test_erase)
{
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build2(&buffer);

	/* Erase part of the buffer */
	vbuffer_sub_create(&sub, &buffer, 0, 30);
	ck_check_error;

	vbuffer_erase(&sub);
	ck_assert_int_eq(vbuffer_size(&buffer), size+strlen(string)-30);
	vbuffer_sub_clear(&sub);

	/* Erase the rest of the buffer */
	vbuffer_sub_create(&sub, &buffer, 0, ALL);
	vbuffer_erase(&sub);
	ck_assert_int_eq(vbuffer_size(&buffer), 0);
	vbuffer_sub_clear(&sub);

	vbuffer_release(&buffer);
	ck_check_error;
}
END_TEST

START_TEST(test_extract)
{
	struct vbuffer buffer = vbuffer_init, extract = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build2(&buffer);

	/* Extract part of it */
	vbuffer_sub_create(&sub, &buffer, 12, 100);
	ck_check_error;

	vbuffer_extract(&sub, &extract);
	ck_assert_int_eq(vbuffer_size(&extract), 100);
	ck_assert_int_eq(vbuffer_size(&buffer), size+strlen(string)-100);
	vbuffer_clear(&extract);
	vbuffer_sub_clear(&sub);

	/* Extract the rest */
	vbuffer_sub_create(&sub, &buffer, 0, ALL);
	ck_check_error;

	vbuffer_extract(&sub, &extract);
	ck_assert_int_eq(vbuffer_size(&buffer), 0);
	vbuffer_release(&extract);
	vbuffer_sub_clear(&sub);

	vbuffer_release(&buffer);
	ck_check_error;
}
END_TEST

START_TEST(test_mmap)
{
	struct vbuffer_sub_mmap mmapiter = vbuffer_mmap_init;
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	int count = 0;
	size_t len = 0;
	uint8 *ptr;

	vbuffer_test_build2(&buffer);
	ck_check_error;

	vbuffer_sub_create(&sub, &buffer, 0, ALL);
	ck_check_error;

	/* mmap the whole buffer and check the returned memory blocks */
	while ((ptr = vbuffer_mmap(&sub, &len, false, &mmapiter, NULL))) {
		++count;
		ck_check_error;

		switch (count) {
		case 1:  ck_assert_int_eq(len, insert_offset); break;
		case 2:  ck_assert_int_eq(len, strlen(string)); ck_assert_int_eq(memcmp(ptr, string, strlen(string)), 0); break;
		case 3:  ck_assert_int_eq(len, size-insert_offset); break;
		default: ck_abort_msg("too many mmap blocks"); break;
		}
	}

	vbuffer_release(&buffer);
	ck_check_error;
}
END_TEST

START_TEST(test_select)
{
	struct vbuffer select = vbuffer_init;
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	struct vbuffer_iterator ref;
	vbuffer_test_build2(&buffer);

	/* Select sub buffer */
	vbuffer_sub_create(&sub, &buffer, insert_offset/2, strlen(string)+insert_offset);
	ck_check_error;

	ck_assert(vbuffer_select(&sub, &select, &ref));
	ck_check_error;

	ck_assert_int_eq(vbuffer_size(&buffer), size-insert_offset);
	ck_assert_int_eq(vbuffer_size(&select), strlen(string)+insert_offset);

	/* Restore it */
	ck_assert(vbuffer_restore(&ref, &select, false));
	ck_check_error;
	ck_assert_int_eq(vbuffer_size(&buffer), size+strlen(string));

	vbuffer_release(&buffer);
	ck_check_error;
}
END_TEST

START_TEST(test_flatten)
{
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build2(&buffer);

	/* Check flatness of part of the buffer */
	vbuffer_sub_create(&sub, &buffer, insert_offset+2, strlen(string)/2);
	ck_check_error;
	ck_assert(vbuffer_sub_isflat(&sub));
	vbuffer_sub_clear(&sub);
	ck_check_error;

	/* Check flatness of the whole buffer */
	vbuffer_sub_create(&sub, &buffer, 0, ALL);
	ck_check_error;
	ck_assert(!vbuffer_sub_isflat(&sub));
	ck_check_error;

	/* Flatten the whole buffer */
	ck_assert(vbuffer_sub_flatten(&sub, NULL) != NULL);
	ck_assert(vbuffer_sub_isflat(&sub));
	ck_check_error;
	ck_assert_int_eq(vbuffer_size(&buffer), size+strlen(string));

	vbuffer_release(&buffer);
	vbuffer_sub_clear(&sub);
	ck_check_error;
}
END_TEST

START_TEST(test_compact)
{
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	struct vbuffer_iterator iter;

	ck_assert(vbuffer_create_new(&buffer, size, true));
	ck_check_error;

	/* Add a mark and remove it to split the buffer */
	vbuffer_position(&buffer, &iter, insert_offset);
	vbuffer_iterator_mark(&iter, true);
	vbuffer_iterator_unmark(&iter);

	vbuffer_sub_create(&sub, &buffer, 0, ALL);
	ck_check_error;
	ck_assert(!vbuffer_sub_isflat(&sub));
	ck_check_error;

	/* Compact */
	ck_assert(vbuffer_sub_compact(&sub));
	ck_assert(vbuffer_sub_isflat(&sub));
	ck_check_error;
	ck_assert_int_eq(vbuffer_size(&buffer), size);

	vbuffer_release(&buffer);
	vbuffer_sub_clear(&sub);
	ck_check_error;
}
END_TEST

START_TEST(test_number)
{
	static const int number = 0xdeadbeef;
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build2(&buffer);

	/* Check number conversion on a single chunk */
	vbuffer_sub_create(&sub, &buffer, insert_offset+2, 4);
	ck_check_error;
	ck_assert(vbuffer_sub_isflat(&sub));

	/* Big endian */
	ck_assert(vbuffer_setnumber(&sub, true, number));
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, true), number);
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, false), (int)SWAP_int32(number));

	/* Little endian */
	ck_assert(vbuffer_setnumber(&sub, false, number));
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, false), number);
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, true), (int)SWAP_int32(number));

	vbuffer_sub_clear(&sub);
	ck_check_error;

	/* Check number conversion over a chunk boundary */
	vbuffer_sub_create(&sub, &buffer, insert_offset-2, 4);
	ck_check_error;
	ck_assert(!vbuffer_sub_isflat(&sub));

	/* Big endian */
	ck_assert(vbuffer_setnumber(&sub, true, number));
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, true), number);
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, false), (int)SWAP_int32(number));

	/* Little endian */
	ck_assert(vbuffer_setnumber(&sub, false, number));
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, false), number);
	ck_assert_int_eq((int)vbuffer_asnumber(&sub, true), (int)SWAP_int32(number));

	vbuffer_sub_clear(&sub);
	ck_check_error;

	vbuffer_release(&buffer);
	vbuffer_sub_clear(&sub);
	ck_check_error;
}
END_TEST

START_TEST(test_bits)
{
	static const int number = 0x555;
	static const int offset = 3;
	static const int bits = 11;

	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build2(&buffer);

	/* Check number conversion on a single chunk */
	vbuffer_sub_create(&sub, &buffer, insert_offset+2, 2);
	ck_check_error;
	ck_assert(vbuffer_sub_isflat(&sub));

	/* Little endian */
	ck_assert(vbuffer_setnumber(&sub, false, 0));
	ck_assert(vbuffer_setbits(&sub, offset, bits, false, number));
	ck_assert_int_eq((int)vbuffer_asbits(&sub, offset, bits, false), number);

	/* Big endian */
	ck_assert(vbuffer_setnumber(&sub, true, 0));
	ck_assert(vbuffer_setbits(&sub, offset, bits, true, number));
	ck_assert_int_eq((int)vbuffer_asbits(&sub, offset, bits, true), number);

	vbuffer_sub_clear(&sub);
	ck_check_error;

	/* Check number conversion over a chunk boundary */
	vbuffer_sub_create(&sub, &buffer, insert_offset-1, 2);
	ck_check_error;
	ck_assert(!vbuffer_sub_isflat(&sub));

	/* Little endian */
	ck_assert(vbuffer_setbits(&sub, offset, bits, false, number));
	ck_assert_int_eq((int)vbuffer_asbits(&sub, offset, bits, false), number);

	/* Big endian */
	ck_assert(vbuffer_setbits(&sub, offset, bits, true, number));
	ck_assert_int_eq((int)vbuffer_asbits(&sub, offset, bits, true), number);

	vbuffer_sub_clear(&sub);
	ck_check_error;

	vbuffer_release(&buffer);
	vbuffer_sub_clear(&sub);
	ck_check_error;
}
END_TEST

START_TEST(test_bits_endian)
{
	static const int number = 0x5;
	static const int offset = 0;
	static const int bits = 4;
#ifdef HAKA_BIGENDIAN
	static const bool bigendian = true;
#else
	static const bool bigendian = false;
#endif

	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build(&buffer);

	/* Check number conversion on a single chunk */
	vbuffer_sub_create(&sub, &buffer, 2, 1);
	ck_check_error;

	/* Same endianness */
	ck_assert(vbuffer_setnumber(&sub, bigendian, 0));
	ck_assert(vbuffer_setbits(&sub, offset, bits, bigendian, number));
	ck_assert_int_eq((int)vbuffer_asbits(&sub, offset, bits, bigendian), number);
	ck_assert_int_eq(vbuffer_asnumber(&sub, bigendian), number);

	/* Different endianness */
	ck_assert(vbuffer_setnumber(&sub, !bigendian, 0));
	ck_assert(vbuffer_setbits(&sub, offset, bits, !bigendian, number));
	ck_assert_int_eq((int)vbuffer_asbits(&sub, offset, bits, !bigendian), number);
	ck_assert_int_eq(vbuffer_asnumber(&sub, !bigendian), number<<4);

	vbuffer_sub_clear(&sub);
	ck_check_error;

	vbuffer_release(&buffer);
	vbuffer_sub_clear(&sub);
	ck_check_error;
}
END_TEST

START_TEST(test_string)
{
	char str[100];
	const char *string2 = "01234567890123456789";
	struct vbuffer buffer = vbuffer_init;
	struct vbuffer_sub sub;
	vbuffer_test_build2(&buffer);

	/* Check known string */
	vbuffer_sub_create(&sub, &buffer, insert_offset, strlen(string));
	ck_check_error;
	ck_assert(vbuffer_asstring(&sub, str, 100));
	ck_assert_str_eq(str, string);

	/* Check string set on single chunk */
	ck_assert(vbuffer_setfixedstring(&sub, string2, strlen(string2)));
	ck_assert(vbuffer_asstring(&sub, str, 100));
	str[strlen(string2)] = 0;
	ck_assert_str_eq(str, string2);

	ck_assert(vbuffer_setstring(&sub, string2, strlen(string2)));
	ck_assert_int_eq(vbuffer_sub_size(&sub), strlen(string2));
	ck_assert(vbuffer_asstring(&sub, str, 100));
	ck_assert_str_eq(str, string2);

	vbuffer_sub_clear(&sub);
	ck_check_error;

	/* Check string set over chunk boundary */
	vbuffer_sub_create(&sub, &buffer, insert_offset-10, strlen(string));
	ck_check_error;

	ck_assert(vbuffer_setfixedstring(&sub, string2, strlen(string2)));
	ck_assert(vbuffer_asstring(&sub, str, 100));
	str[strlen(string2)] = 0;
	ck_assert_str_eq(str, string2);

	ck_assert(vbuffer_setstring(&sub, string2, strlen(string2)));
	ck_assert_int_eq(vbuffer_sub_size(&sub), strlen(string2));
	ck_assert(vbuffer_asstring(&sub, str, 100));
	ck_assert_str_eq(str, string2);

	vbuffer_sub_clear(&sub);
	ck_check_error;

	vbuffer_release(&buffer);
	vbuffer_sub_clear(&sub);
	ck_check_error;
}
END_TEST

int main(int argc, char *argv[])
{
	int number_failed;

	Suite *suite = suite_create("vbuffer_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, test_create);
	tcase_add_test(tcase, test_iterator_available);
	tcase_add_test(tcase, test_insert);
	tcase_add_test(tcase, test_erase);
	tcase_add_test(tcase, test_extract);
	tcase_add_test(tcase, test_mmap);
	tcase_add_test(tcase, test_select);
	tcase_add_test(tcase, test_flatten);
	tcase_add_test(tcase, test_compact);
	tcase_add_test(tcase, test_number);
	tcase_add_test(tcase, test_bits);
	tcase_add_test(tcase, test_bits_endian);
	tcase_add_test(tcase, test_string);
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

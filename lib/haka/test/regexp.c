/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <haka/error.h>
#include <haka/regexp_module.h>
#include <haka/vbuffer.h>

#define ck_check_error     if (check_error()) { ck_abort_msg("Error: %ls", clear_error()); return; }

static void haka_initialized_with_good_path(void)
{
	const char *haka_path_s = getenv("HAKA_PATH");
	static const char *HAKA_CORE_PATH = "/share/haka/core/*";
	static const char *HAKA_MODULE_PATH = "/share/haka/modules/*";

	size_t path_len;
	char *path;

	path_len = 2*strlen(haka_path_s) + strlen(HAKA_CORE_PATH) + 1 +
		strlen(HAKA_MODULE_PATH) + 1;

	path = malloc(path_len);
	if (!path) {
		fprintf(stderr, "memory allocation error\n");
		exit(1);
	}

	snprintf(path, path_len, "%s%s;%s%s", haka_path_s, HAKA_CORE_PATH,
			haka_path_s, HAKA_MODULE_PATH);

	module_set_path(path);
}

START_TEST(regexp_module_load_should_be_successful)
{
	// Given
	haka_initialized_with_good_path();
	clear_error();

	// When
	struct regexp_module *module = regexp_module_load("regexp/pcre", NULL);

	// Then
	ck_check_error;
	ck_assert_msg(module != NULL, "regexp module expected to load, but got NULL module");
}
END_TEST

START_TEST(regexp_module_load_should_return_null_if_module_is_not_MODULE_REGEXP)
{
	// Given
	haka_initialized_with_good_path();
	clear_error();

	// When
	struct regexp_module *module = regexp_module_load("protocol/ipv4", NULL);

	// Then
	ck_assert(module == NULL);
	const wchar_t *error = clear_error();
	ck_assert_msg(wcscmp(error, L"Module protocol/ipv4 is not of type MODULE_REGEXP") == 0,
			"Was expecting 'Module protocol/ipv4 is not of type MODULE_REGEXP', but found '%ls'", error);
}
END_TEST

static struct regexp_module *some_regexp_module(void)
{
	haka_initialized_with_good_path();
	return regexp_module_load("regexp/pcre", NULL);
}

START_TEST(regexp_compile_should_be_successful)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	struct regexp *re = rem->compile(".*");

	// Then
	ck_check_error;
	ck_assert_msg(re != NULL, "compile expected to be successful on valid regexp '.*', but found NULL regexp");
}
END_TEST

START_TEST(regexp_compile_should_should_fail_with_module_error)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	struct regexp *re = rem->compile("?");

	// Then
	ck_assert(re == NULL);
	const wchar_t *error = clear_error();
	ck_assert_msg(wcsncmp(error, L"PCRE compilation failed with error '", 36) == 0,
			"Was expecting 'PCRE compilation failed with error '...', but found '%ls'", error);
}
END_TEST

START_TEST(regexp_exec_should_match_when_string_match)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	int ret = rem->exec(re, "toto", 4);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "exec expected to match, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_exec_should_not_match_when_string_does_not_match)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("aaa");
	clear_error();

	// When
	int ret = rem->exec(re, "abc", 3);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "exec expected to not match, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_free_should_not_fail)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	rem->free(re);

	// Then
	ck_check_error;
}
END_TEST

START_TEST(regexp_match_should_be_successful_when_string_match)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	int ret = rem->match(".*", "toto", 4);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_match_should_not_match_when_string_does_not_match)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	int ret = rem->match("aaa", "abc", 3);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "match expected not to match, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_feed_should_not_fail_when_feed_twice)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	rem->feed(re, "aaa", 3);
	rem->feed(re, "bbb", 3);

	// Then
	ck_check_error;
}
END_TEST

START_TEST(regexp_feed_should_match)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	ret = rem->feed(re, "aaa", 3);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "feed expected to match, but found ret = %d", ret);

}
END_TEST

START_TEST(regexp_feed_should_match_accross_two_string)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("ab");
	clear_error();

	// When
	ret = rem->feed(re, "aaa", 3);
	ret = rem->feed(re, "bbb", 3);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "feed expected to match accross two string, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_feed_should_not_fail_if_no_match)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abc");
	clear_error();

	// When
	ret = rem->feed(re, "aaa", 3);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "feed expected not to fail if no match, but found ret = %d", ret);
}
END_TEST

struct vbuffer *some_vbuffer(char *str, ...)
{
	va_list ap;
	struct vbuffer *vbuf, *last_vbuf, *head_vbuf;
	size_t len, last_len;
	void *iter = NULL;
	uint8 *ptr;

	last_len = len = strlen(str);
	head_vbuf = last_vbuf = vbuf = vbuffer_create_new(len);
	ptr = vbuffer_mmap(vbuf, &iter, &len, true);
	memcpy(ptr, str, len);
	vbuffer_clearmodified(vbuf);

	va_start(ap, str);
	while ((str = va_arg(ap, char *)) != NULL) {
		iter = NULL;
		len = strlen(str);
		vbuf = vbuffer_create_new(len);
		ptr = vbuffer_mmap(vbuf, &iter, &len, true);
		memcpy(ptr, str, len);
		vbuffer_clearmodified(vbuf);
		vbuffer_insert(last_vbuf, last_len, vbuf, false);
		last_vbuf = vbuf;
		last_len = len;
	}
	va_end(ap);

	return head_vbuf;
}

START_TEST(regexp_vbexec_should_match_on_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abc");
	struct vbuffer *vb = some_vbuffer("abc", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on vbuffer, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_vbexec_should_match_on_multiple_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("ab");
	struct vbuffer *vb = some_vbuffer("aaa", "bbb", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on multiple vbuffer, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_vbexec_should_not_change_vbuffer_flags)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abc");
	struct vbuffer *vb = some_vbuffer("aaa", NULL);
	clear_error();

	// When
	rem->vbexec(re, vb);

	// Then
	ck_check_error;
	ck_assert_msg(!vbuffer_ismodified(vb), "vbexec expected to not modify vbuffer flags, but found vbuffer is in modified state");
}
END_TEST

START_TEST(regexp_vbfeed_should_match_on_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	struct vbuffer *vb = some_vbuffer("aaa", NULL);
	clear_error();

	// When
	ret = rem->vbfeed(re, vb);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbfeed expected to match on vbuffer, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_vbfeed_should_match_on_multiple_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abbbcccd");
	struct vbuffer *vb1 = some_vbuffer("aaa", "bbb", NULL);
	struct vbuffer *vb2 = some_vbuffer("ccc", "ddd", NULL);
	clear_error();

	// When
	ret = rem->vbfeed(re, vb1);
	ret = rem->vbfeed(re, vb2);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on multiple vbuffer, but found ret = %d", ret);

}
END_TEST

START_TEST(regexp_vbmatch_should_match_on_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("aaa", NULL);
	clear_error();

	// When
	ret = rem->vbmatch(".*", vb);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match on vbuffer, but found ret = %d", ret);
}
END_TEST

START_TEST(regexp_vbmatch_should_match_on_multiple_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("aaa", "bbb", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("ab", vb);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match on multiple vbuffer, but found ret = %d", ret);
}
END_TEST

Suite* regexp_suite(void)
{
	Suite *suite = suite_create("regexp_suite");
	TCase *tcase = tcase_create("regexp_module");
	tcase_add_test(tcase, regexp_module_load_should_be_successful);
	tcase_add_test(tcase, regexp_module_load_should_return_null_if_module_is_not_MODULE_REGEXP);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_function");
	tcase_add_test(tcase, regexp_compile_should_be_successful);
	tcase_add_test(tcase, regexp_compile_should_should_fail_with_module_error);
	tcase_add_test(tcase, regexp_exec_should_match_when_string_match);
	tcase_add_test(tcase, regexp_exec_should_not_match_when_string_does_not_match);
	tcase_add_test(tcase, regexp_free_should_not_fail);
	tcase_add_test(tcase, regexp_match_should_be_successful_when_string_match);
	tcase_add_test(tcase, regexp_match_should_not_match_when_string_does_not_match);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_feed");
	tcase_add_test(tcase, regexp_feed_should_not_fail_when_feed_twice);
	tcase_add_test(tcase, regexp_feed_should_match);
	tcase_add_test(tcase, regexp_feed_should_match_accross_two_string);
	tcase_add_test(tcase, regexp_feed_should_not_fail_if_no_match);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_vbuffer");
	tcase_add_test(tcase, regexp_vbexec_should_match_on_vbuffer);
	tcase_add_test(tcase, regexp_vbexec_should_match_on_multiple_vbuffer);
	tcase_add_test(tcase, regexp_vbexec_should_not_change_vbuffer_flags);
	tcase_add_test(tcase, regexp_vbfeed_should_match_on_vbuffer);
	tcase_add_test(tcase, regexp_vbfeed_should_match_on_multiple_vbuffer);
	tcase_add_test(tcase, regexp_vbmatch_should_match_on_vbuffer);
	tcase_add_test(tcase, regexp_vbmatch_should_match_on_multiple_vbuffer);
	suite_add_tcase(suite, tcase);

	return suite;
}

int main(int argc, char *argv[])
{
	int number_failed;

	SRunner *runner = srunner_create(regexp_suite());
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}

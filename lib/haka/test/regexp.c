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

#define MODULE_NAME_LEN 254
static char MODULE[MODULE_NAME_LEN];

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

	free(path);
}

START_TEST(regexp_module_load_should_be_successful)
{
	// Given
	haka_initialized_with_good_path();
	clear_error();

	// When
	struct regexp_module *module = regexp_module_load(MODULE, NULL);

	// Then
	ck_check_error;
	ck_assert_msg(module != NULL, "regexp module expected to load, but got NULL module");

	// Finally
	regexp_module_release(module);
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
	/* regexp_module_load is sensitive about remaining error
	 * we have to clear all error before calling it */
	clear_error();
	return regexp_module_load(MODULE, NULL);
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

	// Finally
	rem->release_regexp(re);
	regexp_module_release(rem);
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

	// Finally
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_exec_should_match_when_string_match)
{
	int ret;
	struct regexp_result result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	ret = rem->exec(re, "toto", 4, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "exec expected to match, but found ret = %d", ret);

	// Finally
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_exec_should_not_match_when_string_does_not_match)
{
	int ret;
	struct regexp_result result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("aaa");
	clear_error();

	// When
	ret = rem->exec(re, "abc", 3, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "exec expected to not match, but found ret = %d", ret);

	// Finally
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_exec_should_return_results_on_match)
{
	int ret;
	struct regexp_result result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("bar");
	clear_error();

	// When
	ret = rem->exec(re, "foo bar foo", 11, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "exec expected to match, but found ret = %d", ret);
	ck_assert_msg(result.offset == 4, "exec expected to result with offset = 4, but found offset = %d", result.offset);
	ck_assert_msg(result.size == 3, "exec expected to result with size = 3, but found size = %d", result.size);

	// Finally
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_exec_should_not_fail_without_results)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("bar");
	clear_error();

	// When
	ret = rem->exec(re, "foo bar foo", 11, NULL);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "exec expected to match, but found ret = %d", ret);

	// Finally
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_release_should_not_fail)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	rem->release_regexp(re);

	// Then
	ck_check_error;

	// Finally
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_match_should_be_successful_when_string_match)
{
	int ret;
	struct regexp_result result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	ret = rem->match(".*", "toto", 4, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match, but found ret = %d", ret);

	// Finally
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_match_should_not_match_when_string_does_not_match)
{
	int ret;
	struct regexp_result result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	ret = rem->match("aaa", "abc", 3, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "match expected not to match, but found ret = %d", ret);

	// Finally
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_match_should_return_results_on_match)
{
	int ret;
	struct regexp_result result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	ret = rem->match("bar", "foo bar foo", 11, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match, but found ret = %d", ret);
	ck_assert_msg(result.offset == 4, "match expected to result with offset = 4, but found offset = %d", result.offset);
	ck_assert_msg(result.size == 3, "match expected to result with size = 3, but found size = %d", result.size);

	// Finally
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_match_should_not_fail_without_results)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	clear_error();

	// When
	ret = rem->match("bar", "foo bar foo", 11, NULL);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match, but found ret = %d", ret);

	// Finally
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_get_sink_should_return_regexp_sink)
{
	struct regexp_sink *sink;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	sink = rem->get_sink(re);

	// Then
	ck_assert_msg(sink != NULL, "get_sink expected to return a regexp_sink, but found NULL");

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_feed_should_not_fail_when_feed_twice)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	struct regexp_sink *sink = rem->get_sink(re);
	clear_error();

	// When
	rem->feed(sink, "aaa", 3, false);
	rem->feed(sink, "bbb", 3, true);

	// Then
	ck_check_error;

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_feed_should_match)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	struct regexp_sink *sink = rem->get_sink(re);
	clear_error();

	// When
	ret = rem->feed(sink, "aaa", 3, true);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "feed expected to match, but found ret = %d", ret);

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_feed_should_match_accross_two_string)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("ab");
	struct regexp_sink *sink = rem->get_sink(re);
	clear_error();

	// When
	ret = rem->feed(sink, "aaa", 3, false);
	ret = rem->feed(sink, "bbb", 3, true);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "feed expected to match accross two string, but found ret = %d", ret);

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_feed_should_not_fail_if_no_match)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abc");
	struct regexp_sink *sink = rem->get_sink(re);
	clear_error();

	// When
	ret = rem->feed(sink, "aaa", 3, true);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "feed expected not to fail if no match, but found ret = %d", ret);

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_feed_should_return_results_on_match)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("bar");
	struct regexp_sink *sink = rem->get_sink(re);
	clear_error();

	// When
	ret = rem->feed(sink, "foo b", 5, false);
	ret = rem->feed(sink, "ar foo", 6, false);
	ret = rem->feed(sink, "fail", 4, true);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "feed expected to match, but found ret = %d", ret);
	ck_assert_msg(sink->result.offset == 4, "feed expected result with offset = 4, but found offset = %d", sink->result.offset);
	ck_assert_msg(sink->result.size == 3, "feed expected result with size = 3, but found size = %d", sink->result.size);

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	regexp_module_release(rem);
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
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abc");
	struct vbuffer *vb = some_vbuffer("abc", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on vbuffer, but found ret = %d", ret);

	// Finally
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbexec_should_match_on_multiple_vbuffer)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("ab");
	struct vbuffer *vb = some_vbuffer("aaa", "bbb", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on multiple vbuffer, but found ret = %d", ret);

	// Finally
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
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
	rem->vbexec(re, vb, NULL);

	// Then
	ck_check_error;
	ck_assert_msg(!vbuffer_ismodified(vb), "vbexec expected to not modify vbuffer flags, but found vbuffer is in modified state");

	// Finally
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbexec_should_return_results_on_match_on_single_vbuffer)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("bar");
	struct vbuffer *vb = some_vbuffer("foo bar foo", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on single vbuffer, but found ret = %d", ret);
	ck_assert_msg(result.offset == 4, "vbexec expected to result with offset = 4, but found offset = %d", result.offset);
	ck_assert_msg(result.size == 3, "vbexec expected to result with size = 3, but found size = %d", result.size);

	// Finally
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbexec_should_return_results_on_match_on_multiple_vbuffer)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("bar");
	struct vbuffer *vb = some_vbuffer("foo b", "ar foo", "fail", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on multiple vbuffer, but found ret = %d", ret);
	ck_assert_msg(result.offset == 4, "vbexec expected to result with offset = 4, but found offset = %d", result.offset);
	ck_assert_msg(result.size == 3, "vbexec expected to result with size = 3, but found size = %d", result.size);

	// Finally
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbexec_should_not_fail_without_results)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("bar");
	struct vbuffer *vb = some_vbuffer("foo bar foo", NULL);
	clear_error();

	// When
	ret = rem->vbexec(re, vb, NULL);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbexec expected to match on multiple vbuffer, but found ret = %d", ret);

	// Finally
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbfeed_should_match_on_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	struct regexp_sink *sink = rem->get_sink(re);
	struct vbuffer *vb = some_vbuffer("aaa", NULL);
	clear_error();

	// When
	ret = rem->vbfeed(sink, vb, true);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbfeed expected to match on vbuffer, but found ret = %d", ret);

	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbfeed_should_match_on_multiple_vbuffer)
{
	int ret;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile("abbbcccd");
	struct regexp_sink *sink = rem->get_sink(re);
	struct vbuffer *vb1 = some_vbuffer("aaa", "bbb", NULL);
	struct vbuffer *vb2 = some_vbuffer("ccc", "ddd", NULL);
	clear_error();

	// When
	ret = rem->vbfeed(sink, vb1, false);
	ret = rem->vbfeed(sink, vb2, true);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "vbfeed expected to match on multiple vbuffer, but found ret = %d", ret);


	// Finally
	rem->free_regexp_sink(sink);
	rem->release_regexp(re);
	vbuffer_free(vb1);
	vbuffer_free(vb2);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbfeed_should_return_results_on_match)
{
}
END_TEST

START_TEST(regexp_vbmatch_should_match_on_vbuffer)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("aaa", NULL);
	clear_error();

	// When
	ret = rem->vbmatch(".*", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match on vbuffer, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbmatch_should_match_on_multiple_vbuffer)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("aaa", "bbb", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("ab", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "match expected to match on multiple vbuffer, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(regexp_vbmatch_should_return_results_on_match)
{
}
END_TEST

START_TEST(regexp_vbmatch_should_not_fail_without_results)
{
}
END_TEST

START_TEST(nonreg_regexp_should_not_match_after_start_of_line_if_pattern_start_with_circum)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("aaa", "abc", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("^abc", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "regexp expected to not match after start of line, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);

}
END_TEST

START_TEST(nonreg_regexp_should_match_end_of_line)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("foo", "abc", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("abc$", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "regexp expected to match end of line, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);

}
END_TEST

START_TEST(nonreg_regexp_should_not_match_end_of_line_before_end)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("aaa", "abc", "foo", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("abc$", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret == 0, "regexp expected not to match end of line, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);

}
END_TEST

START_TEST(nonreg_regexp_should_match_even_with_empty_string)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("a", "", "", "bc", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("abc", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "regexp expected to match, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);

}
END_TEST

START_TEST(nonreg_regexp_should_match_even_when_ending_with_empty_string)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("abc", "", "", "", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("abc$", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "regexp expected to match, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

START_TEST(nonreg_regexp_should_match_even_with_nul_byte)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	// A vbuffer of 4 bytes with "abc\0"
	size_t len = 4;
	struct vbuffer *vb = vbuffer_create_new(len);
	void *iter = NULL;
	uint8 *ptr = vbuffer_mmap(vb, &iter, &len, true);
	memcpy(ptr, "abc", len);
	vbuffer_clearmodified(vb);
	clear_error();

	// When
	ret = rem->vbmatch("abc", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "regexp expected to match, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);

}
END_TEST

START_TEST(nonreg_regexp_should_match_even_with_a_partial_failed)
{
	int ret;
	struct regexp_vbresult result;

	// Given
	struct regexp_module *rem = some_regexp_module();
	struct vbuffer *vb = some_vbuffer("fo", "bar", NULL);
	clear_error();

	// When
	ret = rem->vbmatch("foo|bar", vb, &result);

	// Then
	ck_check_error;
	ck_assert_msg(ret > 0, "regexp expected to match, but found ret = %d", ret);

	// Finally
	vbuffer_free(vb);
	regexp_module_release(rem);
}
END_TEST

Suite* regexp_suite(void)
{
	Suite *suite = suite_create("regexp_suite");
	TCase *tcase = tcase_create("regexp_module");
	tcase_add_test(tcase, regexp_module_load_should_be_successful);
	tcase_add_test(tcase, regexp_module_load_should_return_null_if_module_is_not_MODULE_REGEXP);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_compile");
	tcase_add_test(tcase, regexp_compile_should_be_successful);
	tcase_add_test(tcase, regexp_compile_should_should_fail_with_module_error);
	tcase_add_test(tcase, regexp_release_should_not_fail);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_exec");
	tcase_add_test(tcase, regexp_exec_should_match_when_string_match);
	tcase_add_test(tcase, regexp_exec_should_not_match_when_string_does_not_match);
	tcase_add_test(tcase, regexp_exec_should_return_results_on_match);
	tcase_add_test(tcase, regexp_exec_should_not_fail_without_results);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_match");
	tcase_add_test(tcase, regexp_match_should_be_successful_when_string_match);
	tcase_add_test(tcase, regexp_match_should_not_match_when_string_does_not_match);
	tcase_add_test(tcase, regexp_match_should_return_results_on_match);
	tcase_add_test(tcase, regexp_match_should_not_fail_without_results);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_feed");
	tcase_add_test(tcase, regexp_get_sink_should_return_regexp_sink);
	tcase_add_test(tcase, regexp_feed_should_not_fail_when_feed_twice);
	tcase_add_test(tcase, regexp_feed_should_match);
	tcase_add_test(tcase, regexp_feed_should_match_accross_two_string);
	tcase_add_test(tcase, regexp_feed_should_not_fail_if_no_match);
	tcase_add_test(tcase, regexp_feed_should_return_results_on_match);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_vbexec");
	tcase_add_test(tcase, regexp_vbexec_should_match_on_vbuffer);
	tcase_add_test(tcase, regexp_vbexec_should_match_on_multiple_vbuffer);
	tcase_add_test(tcase, regexp_vbexec_should_not_change_vbuffer_flags);
	tcase_add_test(tcase, regexp_vbexec_should_return_results_on_match_on_single_vbuffer);
	tcase_add_test(tcase, regexp_vbexec_should_return_results_on_match_on_multiple_vbuffer);
	tcase_add_test(tcase, regexp_vbexec_should_not_fail_without_results);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_vbmatch");
	tcase_add_test(tcase, regexp_vbmatch_should_match_on_vbuffer);
	tcase_add_test(tcase, regexp_vbmatch_should_match_on_multiple_vbuffer);
	tcase_add_test(tcase, regexp_vbmatch_should_return_results_on_match);
	tcase_add_test(tcase, regexp_vbmatch_should_not_fail_without_results);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_vbfeed");
	tcase_add_test(tcase, regexp_vbfeed_should_match_on_vbuffer);
	tcase_add_test(tcase, regexp_vbfeed_should_match_on_multiple_vbuffer);
	tcase_add_test(tcase, regexp_vbfeed_should_return_results_on_match);
	suite_add_tcase(suite, tcase);

	tcase = tcase_create("regexp_nonreg");
	tcase_add_test(tcase, nonreg_regexp_should_not_match_after_start_of_line_if_pattern_start_with_circum);
	tcase_add_test(tcase, nonreg_regexp_should_match_end_of_line);
	tcase_add_test(tcase, nonreg_regexp_should_not_match_end_of_line_before_end);
	tcase_add_test(tcase, nonreg_regexp_should_match_even_with_empty_string);
	tcase_add_test(tcase, nonreg_regexp_should_match_even_when_ending_with_empty_string);
	tcase_add_test(tcase, nonreg_regexp_should_match_even_with_nul_byte);
	tcase_add_test(tcase, nonreg_regexp_should_match_even_with_a_partial_failed);
	suite_add_tcase(suite, tcase);

	return suite;
}

int main(int argc, char *argv[])
{
	int number_failed;
	snprintf(MODULE, MODULE_NAME_LEN, "regexp/%s", getenv("HAKA_MODULE"));

	SRunner *runner = srunner_create(regexp_suite());
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}

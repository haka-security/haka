/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <check.h>
#include <stdlib.h>
#include <haka/error.h>
#include <haka/regexp_module.h>

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
	ck_assert(module != NULL);
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

	// When
	struct regexp *re = rem->compile(".*");

	// Then
	ck_assert(re != NULL);
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

START_TEST(regexp_exec_should_be_successful_when_string_match)
{
	// Given
	struct regexp_module *rem = some_regexp_module();
	struct regexp *re = rem->compile(".*");
	clear_error();

	// When
	int ret = rem->exec(re, "toto", 4);

	// Then
	ck_check_error;
	ck_assert(ret > 0);
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
	ck_assert_msg(ret == 0, "regexp exec expected to return '0', but found '%d'", ret);
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
	ck_assert(ret > 0);
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
	ck_assert(ret == 0);
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
	tcase_add_test(tcase, regexp_exec_should_be_successful_when_string_match);
	tcase_add_test(tcase, regexp_exec_should_not_match_when_string_does_not_match);
	tcase_add_test(tcase, regexp_free_should_not_fail);
	tcase_add_test(tcase, regexp_match_should_be_successful_when_string_match);
	tcase_add_test(tcase, regexp_match_should_not_match_when_string_does_not_match);
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

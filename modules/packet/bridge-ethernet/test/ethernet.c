/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <check.h>
#include <haka/config.h>
#include <haka/module.h>
#include <haka/parameters.h>
#include <haka/error.h>
#include <haka/types.h>

#define ck_check_error     if (check_error()) { ck_abort_msg("Error: %ls", clear_error()); return; }
#define ck_check_error_not if (!check_error()) { ck_abort_msg("Error: should have failed."); return; }

START_TEST(module_load_should_be_successful)
{
	// Given
	struct parameters *params = parameters_create();
	parameters_set_string(params, "interfaces","eth0");
	module_set_default_path();
	clear_error();

	// When
	struct module *module = module_load("packet/bridge-ethernet", params);

	// Then
	ck_check_error;
	ck_assert_msg(module != NULL, "module expected to load, but got NULL module");

	// Finally
	module_release(module);
	parameters_free(params);
}
END_TEST

START_TEST(module_load_should_fail_with_missing_interfaces_parameter)
{
	// Given
	struct parameters *params = parameters_create();
	module_set_default_path();
	clear_error();

	// When
	struct module *module = module_load("packet/bridge-ethernet", params);

	// Then
	ck_check_error_not;
	ck_assert_msg(module == NULL, "module load expected to fail");

	// Finally
	if (module) {
		module_release(module);
	}
	parameters_free(params);
}
END_TEST

int main(int argc, char *argv[])
{
	int number_failed;

	Suite *suite = suite_create("ethernet_suite");
	TCase *tcase = tcase_create("case");

	tcase_add_test(tcase, module_load_should_be_successful);
	tcase_add_test(tcase, module_load_should_fail_with_missing_interfaces_parameter);

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

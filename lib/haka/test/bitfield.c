/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include <haka/config.h>
#include <haka/container/bitfield.h>


BITFIELD_STATIC(11, bitfield11);
BITFIELD_STATIC(94, bitfield94);

struct bitfield11 bf11 = BITFIELD_STATIC_INIT(11);
struct bitfield94 bf94 = BITFIELD_STATIC_INIT(94);

START_TEST(test_set_get_11)
{
	bitfield_set(&bf11.bitfield, 9, true);
	ck_assert(bitfield_get(&bf11.bitfield, 9));

	bitfield_set(&bf11.bitfield, 9, false);
	ck_assert(!bitfield_get(&bf11.bitfield, 9));
}
END_TEST

START_TEST(test_set_get_94)
{
	bitfield_set(&bf94.bitfield, 32, true);
	ck_assert(bitfield_get(&bf94.bitfield, 32));

	bitfield_set(&bf94.bitfield, 32, false);
	ck_assert(!bitfield_get(&bf94.bitfield, 32));
}
END_TEST

START_TEST(test_set_get_94_2)
{
	bitfield_set(&bf94.bitfield, 77, true);
	ck_assert(bitfield_get(&bf94.bitfield, 77));

	bitfield_set(&bf94.bitfield, 77, false);
	ck_assert(!bitfield_get(&bf94.bitfield, 77));
}
END_TEST

int main(int argc, char *argv[])
{
	int number_failed;

	Suite *suite = suite_create("bitfield");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, test_set_get_11);
	tcase_add_test(tcase, test_set_get_94);
	tcase_add_test(tcase, test_set_get_94_2);
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

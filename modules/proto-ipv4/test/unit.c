
#include <stdio.h>
#include <check.h>
#include <ipv4.h>


START_TEST(ipv4_addr_check)
{
	char str[24];

	ck_assert_int_eq(ipv4_addr_from_string("192.12.1.9"), -1072955127);
	ck_assert_int_eq(ipv4_addr_from_bytes(192, 12, 1, 9), -1072955127);
	ipv4_addr_to_string(-1072955127, str, 24);
	ck_assert_str_eq(str, "192.12.1.9");
}
END_TEST

Suite* ipv4_suite(void)
{
	Suite *suite = suite_create("ipv4_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, ipv4_addr_check);
	suite_add_tcase(suite, tcase);
	return suite;
}

int main (int argc, char *argv[])
{
	int number_failed;
	Suite *suite = ipv4_suite();
	SRunner *runner = srunner_create(suite);
	srunner_run_all(runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);
	return number_failed;
}

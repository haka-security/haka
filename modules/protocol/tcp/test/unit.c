
#include <stdio.h>
#include <check.h>
#include <haka/tcp.h>
#include <haka/ipv4.h>


START_TEST(tcp_stream_check)
{
}
END_TEST

Suite* tcp_suite(void)
{
	Suite *suite = suite_create("tcp_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, tcp_stream_check);
	suite_add_tcase(suite, tcase);
	return suite;
}

int main (int argc, char *argv[])
{
	int number_failed;
	Suite *suite = tcp_suite();
	SRunner *runner = srunner_create(suite);
	srunner_run_all(runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);
	return number_failed;
}

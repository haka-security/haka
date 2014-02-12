/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <check.h>
#include <wchar.h>
#include <haka/ipv4.h>

START_TEST(ipv4_addr_check_from_string)
{
	ck_assert_int_eq(ipv4_addr_from_string("192.12.1.9"), 0xC00C0109);
}
END_TEST

START_TEST(ipv4_addr_check_from_badstring)
{
	ck_assert_int_eq(ipv4_addr_from_string(".192.12.1.9"), 0);
	ck_assert(check_error());
	clear_error();
}
END_TEST

START_TEST(ipv4_addr_check_from_bytes)
{
	ck_assert_int_eq(ipv4_addr_from_bytes(192, 12, 1, 9), 0xC00C0109);
}
END_TEST

START_TEST(ipv4_addr_check_to_string)
{
	char str[IPV4_ADDR_STRING_MAXLEN+1];
	ipv4_addr_to_string(0xC00C0109, str, IPV4_ADDR_STRING_MAXLEN+1);
	ck_assert_str_eq(str, "192.12.1.9");
}
END_TEST

START_TEST(ipv4_badnetwork1_check)
{
	ipv4network network;
	network = ipv4_network_from_string("192.168.1.24/33");
	ck_assert_int_eq(network.net, 0);
	ck_assert_int_eq(network.mask, 0);
	ck_assert(check_error());
	clear_error();
}
END_TEST

START_TEST(ipv4_badnetwork2_check)
{
	ipv4network network;
	network = ipv4_network_from_string("1.9.2.168.1.0/24");
	ck_assert_int_eq(network.net, 0);
	ck_assert_int_eq(network.mask, 0);
	ck_assert(check_error());
	clear_error();
}
END_TEST

START_TEST(ipv4_badnetwork3_check)
{
	ipv4network network;
	network = ipv4_network_from_string("192.168.1.24");
	ck_assert_int_eq(network.net, 0);
	ck_assert_int_eq(network.mask, 0);
	ck_assert(check_error());
	clear_error();
}
END_TEST

START_TEST(ipv4_badnetwork4_check)
{
	ipv4network network;
	network = ipv4_network_from_string("192.168.192.2400/12");
	ck_assert_int_eq(network.net, 0);
	ck_assert_int_eq(network.mask, 0);
	ck_assert(check_error());
	clear_error();
}
END_TEST

START_TEST(ipv4_badnetwork5_check)
{
	ipv4network network;
	network = ipv4_network_from_string("192.168.1.2/24");
	ck_assert_int_eq(network.net, 0xC0A80100);
	ck_assert_int_eq(network.mask, 24);
	ck_assert(check_error());
	clear_error();
}
END_TEST

START_TEST(ipv4_network_check)
{
	char str[32];
	ipv4network network;
	network = ipv4_network_from_string("192.168.1.0/24");
	ck_assert_int_eq(network.net, 0xC0A80100);
	ck_assert_int_eq(network.mask, 24);
	network.net = 0xAC100000;
	network.mask = 16;
	ipv4_network_to_string(network, str, IPV4_NETWORK_STRING_MAXLEN+1);
	ck_assert_str_eq(str, "172.16.0.0/16");
}
END_TEST

int main (int argc, char *argv[])
{
	int number_failed;

	Suite *suite = suite_create("ipv4_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, ipv4_addr_check_from_string);
	tcase_add_test(tcase, ipv4_addr_check_from_badstring);
	tcase_add_test(tcase, ipv4_addr_check_from_bytes);
	tcase_add_test(tcase, ipv4_addr_check_to_string);
	tcase_add_test(tcase, ipv4_network_check);
	tcase_add_test(tcase, ipv4_badnetwork1_check);
	tcase_add_test(tcase, ipv4_badnetwork2_check);
	tcase_add_test(tcase, ipv4_badnetwork3_check);
	tcase_add_test(tcase, ipv4_badnetwork4_check);
	tcase_add_test(tcase, ipv4_badnetwork5_check);
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

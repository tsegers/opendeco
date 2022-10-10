#include "minunit.h"

#include "../src/deco.h"

MU_TEST(test_bar_to_msw)
{
    mu_assert_double_eq(10, bar_to_msw(1));
}

MU_TEST(test_msw_to_bar)
{
    mu_assert_double_eq(1, msw_to_bar(10));
}

MU_TEST(test_abs_gauge)
{
    mu_assert_double_eq(1, abs_depth(gauge_depth(1)));
}

MU_TEST(test_gas)
{
    gas_t foo = gas_new(21, 35, MOD_AUTO);
    gas_t bar = gas_new(21, 0, MOD_AUTO);
    gas_t baz = gas_new(21, 35, MOD_AUTO);
    gas_t qux = gas_new(21, 35, 99);

    mu_assert_int_eq(21, gas_o2(&foo));
    mu_assert_int_eq(35, gas_he(&foo));
    mu_assert_int_eq(44, gas_n2(&foo));

    mu_check(gas_equal(&foo, &foo));
    mu_check(!gas_equal(&foo, &bar));
    mu_check(gas_equal(&foo, &baz));
    mu_check(!gas_equal(&foo, &qux));

    mu_assert_double_eq(abs_depth(msw_to_bar(51.6)), gas_mod(&foo));
    mu_assert_double_eq(abs_depth(msw_to_bar(30)), gas_mod(&bar));
    mu_assert_double_eq(99, gas_mod(&qux));
}

void testsuite_deco_setup(void)
{
}

void testsuite_deco_teardown(void)
{
}

MU_TEST_SUITE(testsuite_deco)
{
    MU_SUITE_CONFIGURE(&testsuite_deco_setup, &testsuite_deco_teardown);

    MU_RUN_TEST(test_bar_to_msw);
    MU_RUN_TEST(test_msw_to_bar);
    MU_RUN_TEST(test_abs_gauge);
    MU_RUN_TEST(test_gas);
}

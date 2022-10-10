#include "minunit.h"

MU_TEST_SUITE(testsuite_deco);

int main(int argc, const char *argv[])
{
    MU_RUN_SUITE(testsuite_deco, NULL);
    MU_REPORT();

    return MU_EXIT_CODE;
}

/*
 * Copyright (c) 2012 David Siñuela Pastor, siu.4coders@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef MINUNIT_MINUNIT_H
#define MINUNIT_MINUNIT_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#include <Windows.h>
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#define __func__ __FUNCTION__
#endif

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))

/* Change POSIX C SOURCE version for pure c99 compilers */
#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <string.h>
#include <sys/resource.h>
#include <sys/time.h> /* gethrtime(), gettimeofday() */
#include <sys/times.h>
#include <time.h> /* clock_gettime(), time() */
#include <unistd.h> /* POSIX flags */

#if defined(__MACH__) && defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#if __GNUC__ >= 5 && !defined(__STDC_VERSION__)
#define __func__ __extension__ __FUNCTION__
#endif

#else
#error "Unable to define timers for an unknown OS."
#endif

#include <math.h>
#include <stdio.h>

/*  Maximum length of last message */
#define MINUNIT_MESSAGE_LEN 1024
/*  Accuracy with which floats are compared */
#define MINUNIT_EPSILON 1E-3

/*  Misc. counters */
extern int minunit_run;
extern int minunit_suites;
extern int minunit_fail;
extern int minunit_status;

/*  Timers */
extern double minunit_real_timer;
extern double minunit_proc_timer;

/*  Last message */
extern char minunit_last_message[MINUNIT_MESSAGE_LEN];

/*  Test setup and teardown function pointers */
extern void (*minunit_setup)(void);
extern void (*minunit_teardown)(void);

/*  Definitions */
#define MU_TEST(method_name) static void method_name(void)
#define MU_TEST_SUITE(suite_name) void suite_name(void)

#define MU__SAFE_BLOCK(block) \
    do {                      \
        block                 \
    } while (0)

/*  Run test suite and unset setup and teardown functions */
#define MU_RUN_SUITE(suiteFn, suiteName)                                            \
    MU__SAFE_BLOCK(                                                                 \
        if (suiteName != NULL) {                                                    \
            printf("Running \"%s\" suite\n", (char *) suiteName);                   \
            fflush(stdout);                                                         \
        };                                                                          \
        minunit_suites++; suiteFn(); minunit_setup = NULL; minunit_teardown = NULL; \
        if (suiteName != NULL) { printf("\n"); })

/*  Configure setup and teardown functions */
#define MU_SUITE_CONFIGURE(setup_fun, teardown_fun) \
    MU__SAFE_BLOCK(minunit_setup = setup_fun; minunit_teardown = teardown_fun;)

/*  Test runner */
#define MU_RUN_TEST(test)                                                                                    \
    MU__SAFE_BLOCK(                                                                                          \
        printf("."); if (minunit_real_timer == 0 && minunit_proc_timer == 0) {                               \
            minunit_real_timer = mu_timer_real();                                                            \
            minunit_proc_timer = mu_timer_cpu();                                                             \
        };                                                                                                   \
        minunit_status = 0; minunit_run++; if (minunit_setup) { (*minunit_setup)(); }; if (minunit_status) { \
            minunit_fail++;                                                                                  \
            printf("F");                                                                                     \
            printf("\n%s\n", minunit_last_message);                                                          \
        } else {                                                                                             \
            test();                                                                                          \
            if (minunit_status) {                                                                            \
                minunit_fail++;                                                                              \
                printf("F");                                                                                 \
                printf("\n%s\n", minunit_last_message);                                                      \
            };                                                                                               \
        };                                                                                                   \
        fflush(stdout); if (minunit_teardown)(*minunit_teardown)();)

/*  Report */
#define MU_REPORT()                                                                                               \
    MU__SAFE_BLOCK(                                                                                               \
        double minunit_end_real_timer; double minunit_end_proc_timer;                                             \
        printf("\n\n%d tests from %d test suites ran, %d failures\n", minunit_run, minunit_suites, minunit_fail); \
        minunit_end_real_timer = mu_timer_real(); minunit_end_proc_timer = mu_timer_cpu();                        \
        printf("\nFinished in %.8f seconds (real) %.8f seconds (proc)\n\n",                                       \
               minunit_end_real_timer - minunit_real_timer, minunit_end_proc_timer - minunit_proc_timer);)
#define MU_EXIT_CODE minunit_fail

/*  Assertions */
#define mu_check(test)                                                                                               \
    MU__SAFE_BLOCK(if (!(test)) {                                                                                    \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 #test);                                                                                             \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define mu_fail(message)                                                                                              \
    MU__SAFE_BLOCK(snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, \
                            __LINE__, message);                                                                       \
                   minunit_status = 1; return;)

#define FAIL(message)                                                                                                 \
    MU__SAFE_BLOCK(snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, \
                            __LINE__, message);                                                                       \
                   minunit_status = 1; return;)

#define ASSERT_EQ(inParam1, inParam2, message)                                                                       \
    MU__SAFE_BLOCK(if (!(inParam1 == inParam2)) {                                                                    \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define ASSERT_NE(inParam1, inParam2, message)                                                                       \
    MU__SAFE_BLOCK(if (!(inParam1 != inParam2)) {                                                                    \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define ASSERT_LE(inParam1, inParam2, message)                                                                       \
    MU__SAFE_BLOCK(if (!(inParam1 <= inParam2)) {                                                                    \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define ASSERT_LT(inParam1, inParam2, message)                                                                       \
    MU__SAFE_BLOCK(if (!(inParam1 < inParam2)) {                                                                     \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define ASSERT_GE(inParam1, inParam2, message)                                                                       \
    MU__SAFE_BLOCK(if (!(inParam1 >= inParam2)) {                                                                    \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define ASSERT_GT(inParam1, inParam2, message)                                                                       \
    MU__SAFE_BLOCK(if (!(inParam1 > inParam2)) {                                                                     \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

// Verifies that the difference between inParam1 and inParam2 does not exceed the absolute error
// bound inAbsErr.
#define ASSERT_NEAR(inParam1, inParam2, inAbsErr, message)                                                           \
    MU__SAFE_BLOCK(if (!(inAbsErr > fabs(1.0 * inParam1 - inParam2))) {                                              \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define mu_assert(test, message)                                                                                     \
    MU__SAFE_BLOCK(if (!(test)) {                                                                                    \
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: %s", __func__, __FILE__, __LINE__, \
                 message);                                                                                           \
        minunit_status = 1;                                                                                          \
        return;                                                                                                      \
    })

#define mu_assert_int_eq(expected, result)                                                                     \
    MU__SAFE_BLOCK(int minunit_tmp_e; int minunit_tmp_r; minunit_tmp_e = (expected); minunit_tmp_r = (result); \
                   if (minunit_tmp_e != minunit_tmp_r) {                                                       \
                       snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN,                                     \
                                "%s failed:\n\t%s:%d: %d expected but was %d", __func__, __FILE__, __LINE__,   \
                                minunit_tmp_e, minunit_tmp_r);                                                 \
                       minunit_status = 1;                                                                     \
                       return;                                                                                 \
                   })

#define mu_assert_double_eq(expected, result)                                                                        \
    MU__SAFE_BLOCK(double minunit_tmp_e; double minunit_tmp_r; minunit_tmp_e = (expected); minunit_tmp_r = (result); \
                   if (fabs(minunit_tmp_e - minunit_tmp_r) > MINUNIT_EPSILON) {                                      \
                       int minunit_significant_figures = 1 - log10(MINUNIT_EPSILON);                                 \
                       snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN,                                           \
                                "%s failed:\n\t%s:%d: %.*g expected but was %.*g", __func__, __FILE__, __LINE__,     \
                                minunit_significant_figures, minunit_tmp_e, minunit_significant_figures,             \
                                minunit_tmp_r);                                                                      \
                       minunit_status = 1;                                                                           \
                       return;                                                                                       \
                   })

#define ASSERT_STREQ(expected, result)                                                                             \
    MU__SAFE_BLOCK(                                                                                                \
        const char *minunit_tmp_e = expected; const char *minunit_tmp_r = result; if (!minunit_tmp_e) {            \
            minunit_tmp_e = "<null pointer>";                                                                      \
        } if (!minunit_tmp_r) { minunit_tmp_r = "<null pointer>"; } if (strcmp(minunit_tmp_e, minunit_tmp_r)) {    \
            snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: '%s' expected but was '%s'", \
                     __func__, __FILE__, __LINE__, minunit_tmp_e, minunit_tmp_r);                                  \
            minunit_status = 1;                                                                                    \
            return;                                                                                                \
        })

#define mu_assert_string_eq(expected, result)                                                                      \
    MU__SAFE_BLOCK(                                                                                                \
        const char *minunit_tmp_e = expected; const char *minunit_tmp_r = result; if (!minunit_tmp_e) {            \
            minunit_tmp_e = "<null pointer>";                                                                      \
        } if (!minunit_tmp_r) { minunit_tmp_r = "<null pointer>"; } if (strcmp(minunit_tmp_e, minunit_tmp_r)) {    \
            snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\t%s:%d: '%s' expected but was '%s'", \
                     __func__, __FILE__, __LINE__, minunit_tmp_e, minunit_tmp_r);                                  \
            minunit_status = 1;                                                                                    \
            return;                                                                                                \
        })

/*
 * The following two functions were written by David Robert Nadeau
 * from http://NadeauSoftware.com/ and distributed under the
 * Creative Commons Attribution 3.0 Unported License
 */

/**
 * Returns the real time, in seconds, or -1.0 if an error occurred.
 *
 * Time is measured since an arbitrary and OS-dependent start time.
 * The returned real time is only useful for computing an elapsed time
 * between two calls to this function.
 */
double mu_timer_real(void);

/**
 * Returns the amount of CPU time used by the current process,
 * in seconds, or -1.0 if an error occurred.
 */
double mu_timer_cpu(void);

#ifdef __cplusplus
}
#endif

#endif /* MINUNIT_MINUNIT_H */

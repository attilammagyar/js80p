/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/// \cond HIDDEN
#ifndef JS80P__TESTS__TEST_CPP
#define JS80P__TESTS__TEST_CPP
/// \endcond

/**
 * \file test.cpp
 * \brief A minimalistic C++ unit test framework.
 *
 * Example:
 *
 * \code{.cpp}
 * #include "test.cpp"
 *
 *
 * TEST(booleans, {
 *     assert_true(true);
 *     assert_true(true, "Custom message");
 *     assert_true(true, "Custom %s message", "parametric");
 *     assert_false(1 == 2, "Custom %s message", "parametric");
 * })
 *
 *
 * TEST(ints, {
 *     assert_eq(1, 1);
 *     assert_neq(1, 2);
 *     assert_lt(1, 2);
 *     assert_lte(1, 2);
 *     assert_gt(2, 1, "Custom message");
 *     assert_gte(1, 3 - 1, "Custom %s message", "parametric");
 * })
 *
 *
 * TEST(doubles, {
 *     assert_eq(1.0, 1.0);
 *     assert_neq(1.0, 2.0);
 *     assert_lt(1.0, 2.0);
 *     assert_lte(1.0, 2.0);
 *     assert_gt(2.0, 1.0, "Custom message");
 *     assert_gte(1.0, 1.0, "Custom message");
 *     assert_eq(1.0, 1.0 + 0.2, 0.1, "Custom %s message", "parametric");
 * })
 *
 *
 * char const* some_func()
 * {
 *     return NULL;
 * }
 *
 *
 * TEST(c_strings, {
 *     char* b = NULL;
 *
 *     assert_eq("foo", "foo");
 *     assert_neq("foo", "bar");
 *     assert_eq(NULL, b);
 *     assert_lt("aaa", "bbb");
 *     assert_lte("aaa", "aaa");
 *     assert_gt("bbb", "aaa", "Custom message");
 *     assert_gte("aaa", some_func(), "Custom %s message", "parametric");
 * })
 *
 *
 * TEST(arrays, {
 *     constexpr int length = 100;
 *     int a_int[length];
 *     int b_int[length];
 *     int c_int[length];
 *     double a_dbl[length];
 *     double b_dbl[length];
 *     double c_dbl[length];
 *
 *     for (int i = 0; i != length; ++i) {
 *         a_int[i] = b_int[i] = i;
 *         c_int[i] = -1;
 *         a_dbl[i] = b_dbl[i] = (double)i;
 *         c_dbl[i] = -1.0;
 *     }
 *
 *     b_dbl[50] += 0.1;
 *
 *     assert_eq(a_int, b_int, length);
 *     assert_neq(a_int, c_int, length);
 *     assert_eq(a_int, b_int, length, "Custom %s message", "parametric");
 *     assert_eq(a_dbl, b_dbl, length, 0.2);
 *     assert_neq(a_dbl, c_dbl, length, 0.01);
 *     assert_neq(a_dbl, b_dbl, length, 0.2, "Custom %s message", "parametric");
 * })
 *
 *
 * TEST(double_arrays_close, {
 *     constexpr int length = 10;
 *     double a_dbl[length] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
 *     double b_dbl[length] = {0.9, 1.1, 1.0, 1.1, 0.9, 1.0, 1.5, 0.3, 1.1, 0.9};
 *
 *     assert_close(a_dbl, b_dbl, length, 0.2);
 *     assert_close(a_dbl, b_dbl, length, 0.01);
 * })
 * \endcode
 *
 * Output:
 *
 * \code{.unparsed}
 * Running tests from tests/test_example.cpp
 *  running (tests/test_example.cpp:4 booleans)
 *    pass (tests/test_example.cpp:4 booleans)
 *  running (tests/test_example.cpp:12 ints)
 *
 *  FAIL (tests/test_example.cpp:18 ints): failed to assert that a >= b
 *   a=1 (0x1) // 1
 *   b=2 (0x2) // 3 - 1
 *   message=Custom parametric message
 *
 *  running (tests/test_example.cpp:22 doubles)
 *
 *  FAIL (tests/test_example.cpp:29 doubles): failed to assert that a == b
 *   a=1.000000000 (1.000000000000e+00) // 1.0
 *   b=1.200000000 (1.200000000000e+00) // 1.0 + 0.2
 *   diff=-0.200000000 (-2.000000000000e-01)
 *   tolerance=0.100000000 (1.000000000000e-01)
 *   message=Custom parametric message
 *
 *  running (tests/test_example.cpp:39 c_strings)
 *
 *  FAIL (tests/test_example.cpp:48 c_strings): failed to assert that a >= b
 *   a=aaa // "aaa"
 *   b=<NULL> // some_func()
 *   message=Custom parametric message
 *
 *  running (tests/test_example.cpp:52 arrays)
 *
 *  FAIL (tests/test_example.cpp:75 arrays): failed to assert that a != b
 *   a=a_dbl
 *   b=b_dbl
 *   tolerance=0.200000000 (2.000000000000e-01)
 *   first_mismatch=-1
 *   message=Custom parametric message
 *
 *
 *              i:	a[i]	b[i]	abs(a[i] - b[i])
 *              0:	0.000000000 (0.000000000000e+00)	0.000000000 (0.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              1:	1.000000000 (1.000000000000e+00)	1.000000000 (1.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              2:	2.000000000 (2.000000000000e+00)	2.000000000 (2.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              3:	3.000000000 (3.000000000000e+00)	3.000000000 (3.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              4:	4.000000000 (4.000000000000e+00)	4.000000000 (4.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              5:	5.000000000 (5.000000000000e+00)	5.000000000 (5.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              6:	6.000000000 (6.000000000000e+00)	6.000000000 (6.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              7:	7.000000000 (7.000000000000e+00)	7.000000000 (7.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              8:	8.000000000 (8.000000000000e+00)	8.000000000 (8.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              9:	9.000000000 (9.000000000000e+00)	9.000000000 (9.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *             10:	10.000000000 (1.000000000000e+01)	10.000000000 (1.000000000000e+01)	0.000000000 (0.000000000000e+00)
 *             11:	11.000000000 (1.100000000000e+01)	11.000000000 (1.100000000000e+01)	0.000000000 (0.000000000000e+00)
 *             12:	12.000000000 (1.200000000000e+01)	12.000000000 (1.200000000000e+01)	0.000000000 (0.000000000000e+00)
 *             13:	13.000000000 (1.300000000000e+01)	13.000000000 (1.300000000000e+01)	0.000000000 (0.000000000000e+00)
 *             14:	14.000000000 (1.400000000000e+01)	14.000000000 (1.400000000000e+01)	0.000000000 (0.000000000000e+00)
 *             15:	15.000000000 (1.500000000000e+01)	15.000000000 (1.500000000000e+01)	0.000000000 (0.000000000000e+00)
 *             16:	16.000000000 (1.600000000000e+01)	16.000000000 (1.600000000000e+01)	0.000000000 (0.000000000000e+00)
 *             17:	17.000000000 (1.700000000000e+01)	17.000000000 (1.700000000000e+01)	0.000000000 (0.000000000000e+00)
 *             18:	18.000000000 (1.800000000000e+01)	18.000000000 (1.800000000000e+01)	0.000000000 (0.000000000000e+00)
 *             19:	19.000000000 (1.900000000000e+01)	19.000000000 (1.900000000000e+01)	0.000000000 (0.000000000000e+00)
 *            ...
 *
 *  running (tests/test_example.cpp:79 double_arrays_close)
 *
 *  FAIL (tests/test_example.cpp:79 double_arrays_close): failed to assert that a is close to b
 *   a=a_dbl
 *   b=b_dbl
 *   tolerance=0.010000000 (1.000000000000e-02)
 *   avg_diff=0.180000000 (1.800000000000e-01)
 *   max_mismatch=7
 *
 *
 *              i:	a[i]	b[i]	abs(a[i] - b[i])
 *              0:	1.000000000 (1.000000000000e+00)	0.900000000 (9.000000000000e-01)	0.100000000 (1.000000000000e-01)
 *              1:	1.000000000 (1.000000000000e+00)	1.100000000 (1.100000000000e+00)	0.100000000 (1.000000000000e-01)
 *              2:	1.000000000 (1.000000000000e+00)	1.000000000 (1.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              3:	1.000000000 (1.000000000000e+00)	1.100000000 (1.100000000000e+00)	0.100000000 (1.000000000000e-01)
 *              4:	1.000000000 (1.000000000000e+00)	0.900000000 (9.000000000000e-01)	0.100000000 (1.000000000000e-01)
 *              5:	1.000000000 (1.000000000000e+00)	1.000000000 (1.000000000000e+00)	0.000000000 (0.000000000000e+00)
 *              6:	1.000000000 (1.000000000000e+00)	1.500000000 (1.500000000000e+00)	0.500000000 (5.000000000000e-01)
 *   -->        7:	1.000000000 (1.000000000000e+00)	0.300000000 (3.000000000000e-01)	0.700000000 (7.000000000000e-01)
 *              8:	1.000000000 (1.000000000000e+00)	1.100000000 (1.100000000000e+00)	0.100000000 (1.000000000000e-01)
 *              9:	1.000000000 (1.000000000000e+00)	0.900000000 (9.000000000000e-01)	0.100000000 (1.000000000000e-01)
 *
 *
 * Summary for tests/test_example.cpp: 1 passed, 5 failed, 32 assertions
 * \endcode
 */

/**
 * \def TEST(name, body)
 * \hideinitializer
 * \brief Define a test function with the given name and function body.
 * \param name  Name of the test. Must be syntactically valid as a class name.
 * \param body  Block of code to be executed.
 */
#define TEST(name, body, ...)                                               \
void test_ ## name()                                                        \
{                                                                           \
    body, ## __VA_ARGS__                                                    \
}                                                                           \
                                                                            \
class _TEST_CONCAT_ID(_TestClass_ ## name, __LINE__) : public _Test         \
{                                                                           \
    public:                                                                 \
        _TEST_CONCAT_ID(_TestClass_ ## name, __LINE__)()                    \
            : _Test(#name, __FILE__, __LINE__)                              \
        {                                                                   \
        }                                                                   \
                                                                            \
        virtual void run(int const argc, char const* const* argv) override  \
        {                                                                   \
            initialize(argc, argv);                                         \
            test_ ## name();                                                \
            finalize();                                                     \
        }                                                                   \
};                                                                          \
_TEST_CONCAT_ID(_TestClass_ ## name, __LINE__)                              \
    _TEST_CONCAT_ID(_test_inst_ ## name, __LINE__);

/**
 * \def assert_true(condition, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if the given condition is false.
 * \param condition     (bool) Condition to be checked.
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_true(condition, ...) do {                                    \
    if (                                                                    \
        !_TestAssert::true_(                                                \
            __FILE__, __LINE__, #condition, condition, ## __VA_ARGS__       \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_false(condition, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if the given condition is true.
 * \param condition     (bool) Condition to be checked.
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_false(condition, ...) do {                                   \
    if (                                                                    \
        !_TestAssert::false_(                                               \
            __FILE__, __LINE__, #condition, condition, ## __VA_ARGS__       \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_eq(a, b, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if the given parameters are not equal. C-strings are
 *        compared with \c strcmp() .
 * \param a, b          (int|int[]|double|double[]|char*|std::string) Values to
 *                      be compared.
 * \param length        [optional] Array length when \c a[] and \c b[] are
 *                      arrays.
 * \param tolerance     (double) [optional] When \c a and \c b are of the type
 *                      \c double and the difference between them is not greater
 *                      than this value, then they are considered equal and the
 *                      assertion succeeds.
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_eq(a, b, ...) do {                                           \
    if (                                                                    \
        !_TestAssert::eq(                                                   \
            __FILE__, __LINE__, #a, #b, a, b, ## __VA_ARGS__                \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_neq(a, b, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if the given parameters are equal. C-strings are
 *        compared with \c strcmp() .
 * \param a, b          (int|int[]|double|double[]|char*|std::string) Values to
 *                      be compared.
 * \param length        [optional] Array length when \c a[] and \c b[] are
 *                      arrays.
 * \param tolerance     (double) [optional] When \c a and \c b are of the type
 *                      \c double and the difference between them is not greater
 *                      than this value, then they are considered equal and the
 *                      assertion fails.
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_neq(a, b, ...) do {                                          \
    if (                                                                    \
        !_TestAssert::neq(                                                  \
            __FILE__, __LINE__, #a, #b, a, b, ## __VA_ARGS__                \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_lt(a, b, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if \c a is not less than \c b. C-strings are compared
 *        with \c strcmp() .
 * \param a, b          (int|double|char*|std::string) Values to be compared
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_lt(a, b, ...) do {                                           \
    if (                                                                    \
        !_TestAssert::lt(                                                   \
            __FILE__, __LINE__, #a, #b, a, b, ## __VA_ARGS__                \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_lte(a, b, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if \c a is not less than or equal to \c b. C-strings are
 *        compared with \c strcmp() .
 * \param a, b          (int|double|char*|std::string) Values to be compared
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_lte(a, b, ...) do {                                          \
    if (                                                                    \
        !_TestAssert::lte(                                                  \
            __FILE__, __LINE__, #a, #b, a, b, ## __VA_ARGS__                \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_gt(a, b, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if \c a is not greater than \c b. C-strings are compared
 *        with \c strcmp() .
 * \param a, b          (int|double|char*|std::string) Values to be compared
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_gt(a, b, ...) do {                                           \
    if (                                                                    \
        !_TestAssert::gt(                                                   \
            __FILE__, __LINE__, #a, #b, a, b, ## __VA_ARGS__                \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_gte(a, b, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if \c a is not greater than or equal to \c b. C-strings
 *        are compared with \c strcmp() .
 * \param a, b          (int|double|char*|std::string) Values to be compared
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_gte(a, b, ...) do {                                          \
    if (                                                                    \
        !_TestAssert::gte(                                                  \
            __FILE__, __LINE__, #a, #b, a, b, ## __VA_ARGS__                \
        )                                                                   \
    ) {                                                                     \
        return;                                                             \
    }                                                                       \
} while (false)


/**
 * \def assert_close(a, b, length, tolerance, message_fmt, ...)
 * \hideinitializer
 * \brief Fail the test if the average difference of the elements of the two
 *        arrays exceed the given threshold.
 *
 * \param a, b          double[] Arrays to be compared.
 * \param length        Array length.
 * \param tolerance     (double) When the average difference between the
 *                      elements of \c a and \c b is not greater than this
 *                      value, then they are considered equal and the assertion
 *                      succeeds.
 * \param message_fmt   (char*) [optional] Custom failure message printf()-style
 *                      format string.
 * \param ...           [optional] Format string parameters.
 */
#define assert_close(a, b, length, tolerance, ...) do {                         \
    if (                                                                        \
        !_TestAssert::close(                                                    \
            __FILE__, __LINE__, #a, #b, a, b, length, tolerance, ## __VA_ARGS__ \
        )                                                                       \
    ) {                                                                         \
        return;                                                                 \
    }                                                                           \
} while (false)


/**
 * \brief Define this macro during compilation to control the maximum number of
 *        array elements printed when an assertion fails. Defaults to 20.
 */
#ifndef TEST_MAX_ARRAY_PRINT
#define TEST_MAX_ARRAY_PRINT 20
#endif

/// \cond HIDDEN


#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>


#define _TEST_CONCAT(a, b) a ## b
#define _TEST_EXPAND_CONCAT(a, b) _TEST_CONCAT(a, b)
#define _TEST_CONCAT_ID(a, b) _TEST_EXPAND_CONCAT(a, b)


#define _TEST_STR_OR(str, null_value) (str ? str : null_value)
#define _TEST_NULL_PTR_STR "<NULL>"


int _test_result = 0;
int _test_started = 0;
int _test_assertions = 0;
int _test_failed = 0;
bool _test_current_test_passed = true;
char const* _test_name = NULL;
char const* _test_file = NULL;
int _test_line = -1;

std::vector<std::string> TEST_ARGV;


class _Test;
typedef std::vector<_Test*> _Tests;
_Tests _tests;


class _Test
{
    public:
        _Test(
                char const* const name,
                char const* const file,
                int const line
        ) : name(name),
            file(file),
            line(line)
        {
            _tests.push_back(this);
        }

        void initialize(int const argc, char const* const* argv)
        {
            _test_name = name;
            _test_file = file;
            _test_line = line;
            _test_current_test_passed = true;
            ++_test_started;

            TEST_ARGV.clear();

            for (int i = 0; i < argc; ++i) {
                TEST_ARGV.push_back(argv[i]);
            }

            if (_test_started == 1) {
                fprintf(
                    stderr,
                    "\nRunning tests from %s\n",
                    _TEST_STR_OR(_test_file, "UNKNOWN")
                );
            }

            fprintf(
                stderr,
                " running (%s:%d %s)\n",
                _TEST_STR_OR(_test_file, "UNKNOWN"),
                _test_line,
                _TEST_STR_OR(_test_name, "UNKNOWN")
            );
        }

        virtual void run(int const argc, char const* const* argv)
        {
        }

        void finalize()
        {
            if (_test_current_test_passed) {
                fprintf(
                    stderr,
                    "   pass (%s:%d %s)\n",
                    _TEST_STR_OR(_test_file, "UNKNOWN"),
                    _test_line,
                    _TEST_STR_OR(_test_name, "UNKNOWN")
                );
            } else {
                ++_test_failed;
            }

            _test_name = NULL;
            _test_line = -1;
        }

        char const* const name;
        char const* const file;
        int const line;
};


int main(int argc, char const* argv[])
{
    if (_tests.size() < 1) {
        fprintf(
            stderr,
            "\nFAIL: No tests found in %s\n\n",
            _TEST_STR_OR(_test_file, argv[0])
        );

        return 2;
    }

    if (argc > 1) {
        bool found = false;

        for (_Tests::const_iterator it = _tests.begin(); it != _tests.end(); ++it) {
            _Test* test = *it;
            std::string const name(test->name);

            if (name == argv[1]) {
                found = true;
                test->run(argc, argv);
            }
        }

        if (!found) {
            fprintf(
                stderr,
                "Test not found in %s: \"%s\"\n",
                _TEST_STR_OR(_test_file, argv[0]),
                argv[1]
            );
        }
    } else {
        for (_Tests::const_iterator it = _tests.begin(); it != _tests.end(); ++it) {
            _Test* test = *it;
            test->run(argc, argv);
        }
    }

    fprintf(
        stderr,
        "\nSummary for %s: %d passed, %d failed, %d assertion%s\n\n",
        _TEST_STR_OR(_test_file, argv[0]),
        _test_started - _test_failed,
        _test_failed,
        _test_assertions,
        _test_assertions == 1 ? "" : "s"
    );

    return _test_result;
}


#define _TEST_INT_F "%d (0x%x)"
#define _TEST_LLINT_F "%lld (0x%llx)"
#define _TEST_PTR_F "%p"
#define _TEST_DBL_F "%.9f (%.12e)"
#define _TEST_CSTR_F "%s"


#define _TEST_PASS() do { ++_test_assertions; } while (false)


#define _TEST_FAIL(file, line, fail_message, ...) do {                      \
    fprintf(                                                                \
        stderr,                                                             \
        "\n FAIL (%s:%d %s): failed to assert that " fail_message "\n",     \
        _TEST_STR_OR(file, "UNKNOWN"),                                      \
        line,                                                               \
        _TEST_STR_OR(_test_name, "UNKNOWN"),                                \
        ## __VA_ARGS__                                                      \
    );                                                                      \
    if (message != NULL) {                                                  \
        va_list args;                                                       \
        va_start(args, message);                                            \
        fprintf(stderr, "  message=");                                      \
        vfprintf(stderr, message, args);                                    \
        fprintf(stderr, "\n");                                              \
        va_end(args);                                                       \
    }                                                                       \
    fprintf(stderr, "\n");                                                  \
    _test_result = 1;                                                       \
    _test_current_test_passed = false;                                      \
    ++_test_assertions;                                                     \
} while (false)


#define _TEST_PRINT_ARRAYS(a, b, length, first_mismatch, fmt, abs_fn) do {  \
    fprintf(stderr, "\n             i:\ta[i]\tb[i]\tabs(a[i] - b[i])\n");   \
    int start = std::max(                                                   \
        0,                                                                  \
        first_mismatch - std::max(1, TEST_MAX_ARRAY_PRINT / 2)              \
    );                                                                      \
    int end = std::min(start + std::max(3, TEST_MAX_ARRAY_PRINT), length);  \
    if (start > 0) {                                                        \
        fprintf(stderr, "           ...\n");                                \
    }                                                                       \
    for (int i = start; i != end; ++i) {                                    \
        fprintf(                                                            \
            stderr,                                                         \
            "  %s%9d:\t" fmt "\t" fmt "\t" fmt "\n",                        \
            (i == first_mismatch ? "-->" : "   "),                          \
            i,                                                              \
            a[i],                                                           \
            a[i],                                                           \
            b[i],                                                           \
            b[i],                                                           \
            abs_fn(a[i] - b[i]),                                            \
            abs_fn(a[i] - b[i])                                             \
        );                                                                  \
    }                                                                       \
    if (end < length) {                                                     \
        fprintf(stderr, "           ...\n");                                \
    }                                                                       \
    fprintf(stderr, "\n");                                                  \
} while (false)


#define _TEST_ARGS char const* const file, int const line
#define _TEST_VARGS char const* const message = NULL, ...


class _TestAssert
{
    public:
        static bool true_(
                _TEST_ARGS,
                char const* condition_str,
                bool const condition,
                _TEST_VARGS
        ) {
            if (condition) {
                _TEST_PASS();

                return true;
            } else {
                _TEST_FAIL(
                    file,
                    line,
                    "condition is true\n  condition=false // %s",
                    condition_str
                );

                return false;
            }
        }

        static bool false_(
                _TEST_ARGS,
                char const* condition_str,
                bool const condition,
                _TEST_VARGS
        ) {
            if (!condition) {
                _TEST_PASS();

                return true;
            } else {
                _TEST_FAIL(
                    file,
                    line,
                    "condition is false\n  condition=true // %s",
                    condition_str
                );

                return false;
            }
        }


#define _CONV_ID(x, _type) (x)
#define _CONV_REPEAT(x, _type) (x), (x)
#define _CONV_REPEAT_AS_UNSIGNED(x, _type) (x), ((unsigned _type)(x))
#define _CONV_STR_TO_CSTR(x, _type) (x).c_str()


#define _ASSERT_A_OP_B_METHOD(_name, _op, _type, _type_f, _conv)            \
static bool _name(                                                          \
        _TEST_ARGS,                                                         \
        char const* a_src,                                                  \
        char const* b_src,                                                  \
        _type const a,                                                      \
        _type const b,                                                      \
        _TEST_VARGS                                                         \
) {                                                                         \
    if (a _op b) {                                                          \
        _TEST_PASS();                                                       \
        return true;                                                        \
    } else {                                                                \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a " #_op " b\n  a=" _type_f " // %s\n  b=" _type_f " // %s",   \
            _conv(a, _type), a_src,                                         \
            _conv(b, _type), b_src                                          \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
}

        _ASSERT_A_OP_B_METHOD(eq, ==, int, _TEST_INT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(eq, ==, long long int, _TEST_LLINT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(eq, ==, void const*, _TEST_PTR_F, _CONV_ID)
        _ASSERT_A_OP_B_METHOD(eq, ==, std::string, _TEST_CSTR_F, _CONV_STR_TO_CSTR)

        _ASSERT_A_OP_B_METHOD(neq, !=, int, _TEST_INT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(neq, !=, long long int, _TEST_LLINT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(neq, !=, void const*, _TEST_PTR_F, _CONV_ID)
        _ASSERT_A_OP_B_METHOD(neq, !=, std::string, _TEST_CSTR_F, _CONV_STR_TO_CSTR)

        _ASSERT_A_OP_B_METHOD(lt, <, int, _TEST_INT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(lt, <, long long int, _TEST_LLINT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(lt, <, void const*, _TEST_PTR_F, _CONV_ID)
        _ASSERT_A_OP_B_METHOD(lt, <, float, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(lt, <, double, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(lt, <, std::string, _TEST_CSTR_F, _CONV_STR_TO_CSTR)

        _ASSERT_A_OP_B_METHOD(lte, <=, int, _TEST_INT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(lte, <=, long long int, _TEST_LLINT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(lte, <=, void const*, _TEST_PTR_F, _CONV_ID)
        _ASSERT_A_OP_B_METHOD(lte, <=, float, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(lte, <=, double, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(lte, <=, std::string, _TEST_CSTR_F, _CONV_STR_TO_CSTR)

        _ASSERT_A_OP_B_METHOD(gt, >, int, _TEST_INT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(gt, >, long long int, _TEST_LLINT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(gt, >, void const*, _TEST_PTR_F, _CONV_ID)
        _ASSERT_A_OP_B_METHOD(gt, >, float, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(gt, >, double, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(gt, >, std::string, _TEST_CSTR_F, _CONV_STR_TO_CSTR)

        _ASSERT_A_OP_B_METHOD(gte, >=, int, _TEST_INT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(gte, >=, long long int, _TEST_LLINT_F, _CONV_REPEAT_AS_UNSIGNED)
        _ASSERT_A_OP_B_METHOD(gte, >=, void const*, _TEST_PTR_F, _CONV_ID)
        _ASSERT_A_OP_B_METHOD(gte, >=, float, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(gte, >=, double, _TEST_DBL_F, _CONV_REPEAT)
        _ASSERT_A_OP_B_METHOD(gte, >=, std::string, _TEST_CSTR_F, _CONV_STR_TO_CSTR)


#define _ASSERT_A_OP_B_WITH_TOLERANCE_METHOD(_name, _cmp, _op, _type, _zero)  \
static bool _name(                                                            \
        _TEST_ARGS,                                                           \
        char const* a_src,                                                    \
        char const* b_src,                                                    \
        _type const a,                                                        \
        _type const b,                                                        \
        _type const tolerance = _zero,                                        \
        _TEST_VARGS                                                           \
) {                                                                           \
    _type const diff = a - b;                                                 \
    if (std::fabs(diff) _cmp tolerance) {                                     \
        _TEST_PASS();                                                         \
        return true;                                                          \
    } else {                                                                  \
        _TEST_FAIL(                                                           \
            file,                                                             \
            line,                                                             \
            "a " #_op " b\n  a=" _TEST_DBL_F " // %s\n"                       \
                "  b=" _TEST_DBL_F " // %s\n"                                 \
                "  diff=" _TEST_DBL_F "\n"                                    \
                "  tolerance=" _TEST_DBL_F,                                   \
            a, a, a_src,                                                      \
            b, b, b_src,                                                      \
            diff, diff,                                                       \
            tolerance, tolerance                                              \
        );                                                                    \
        return false;                                                         \
    }                                                                         \
}

        _ASSERT_A_OP_B_WITH_TOLERANCE_METHOD(eq, <=, ==, float, 0.0f)
        _ASSERT_A_OP_B_WITH_TOLERANCE_METHOD(eq, <=, ==, double, 0.0)

        _ASSERT_A_OP_B_WITH_TOLERANCE_METHOD(neq, >, !=, float, 0.0f)
        _ASSERT_A_OP_B_WITH_TOLERANCE_METHOD(neq, >, !=, double, 0.0)


#define _ASSERT_ARRAY_OP_METHOD(_name, _op, _type, _type_f, _abs_fn)        \
static bool _name(                                                          \
        _TEST_ARGS,                                                         \
        char const* a_src,                                                  \
        char const* b_src,                                                  \
        _type const* const a,                                               \
        _type const* const b,                                               \
        int const length,                                                   \
        _TEST_VARGS                                                         \
) {                                                                         \
    if (a == NULL && b == NULL) {                                           \
        _TEST_PASS();                                                       \
        return true;                                                        \
    }                                                                       \
    if (a == NULL || b == NULL) {                                           \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a " #_op " b\n  a=%s (%p)\n  b=%s (%p)",                       \
            a_src,                                                          \
            (void*)a,                                                       \
            b_src,                                                          \
            (void*)b                                                        \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
    int first_mismatch = -1;                                                \
    for (int i = 0; i != length; ++i) {                                     \
        if (a[i] != b[i]) {                                                 \
            first_mismatch = i;                                             \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    if (first_mismatch _op -1) {                                            \
        _TEST_PASS();                                                       \
        return true;                                                        \
    } else {                                                                \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a " #_op " b\n  a=%s\n  b=%s\n  first_mismatch=%d",            \
            a_src,                                                          \
            b_src,                                                          \
            first_mismatch                                                  \
        );                                                                  \
        _TEST_PRINT_ARRAYS(                                                 \
            a, b, length, first_mismatch, _type_f, _abs_fn                  \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
}

        _ASSERT_ARRAY_OP_METHOD(eq, ==, int, _TEST_INT_F, std::abs)
        _ASSERT_ARRAY_OP_METHOD(eq, ==, long long int, _TEST_LLINT_F, std::abs)
        _ASSERT_ARRAY_OP_METHOD(eq, ==, float, _TEST_DBL_F, std::fabs)
        _ASSERT_ARRAY_OP_METHOD(eq, ==, double, _TEST_DBL_F, std::fabs)

        _ASSERT_ARRAY_OP_METHOD(neq, !=, int, _TEST_INT_F, std::abs)
        _ASSERT_ARRAY_OP_METHOD(neq, !=, long long int, _TEST_LLINT_F, std::abs)
        _ASSERT_ARRAY_OP_METHOD(neq, !=, float, _TEST_DBL_F, std::fabs)
        _ASSERT_ARRAY_OP_METHOD(neq, !=, double, _TEST_DBL_F, std::fabs)


#define _ASSERT_FLOAT_ARRAY_OP_METHOD(_name, _op, _type)                    \
static bool _name(                                                          \
        _TEST_ARGS,                                                         \
        char const* a_src,                                                  \
        char const* b_src,                                                  \
        _type const* const a,                                               \
        _type const* const b,                                               \
        int const length,                                                   \
        _type const tolerance,                                              \
        _TEST_VARGS                                                         \
) {                                                                         \
    if (a == NULL && b == NULL) {                                           \
        _TEST_PASS();                                                       \
        return true;                                                        \
    }                                                                       \
    if (a == NULL || b == NULL) {                                           \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a " #_op " b\n  a=%s (%p)\n  b=%s (%p)",                       \
            a_src,                                                          \
            (void*)a,                                                       \
            b_src,                                                          \
            (void*)b                                                        \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
    int first_mismatch = -1;                                                \
    for (int i = 0; i != length; ++i) {                                     \
        if (std::fabs(a[i] - b[i]) > tolerance) {                           \
            first_mismatch = i;                                             \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    if (first_mismatch _op -1) {                                            \
        _TEST_PASS();                                                       \
        return true;                                                        \
    } else {                                                                \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a " #_op " b\n  a=%s\n  b=%s\n"                                \
                "  tolerance=" _TEST_DBL_F "\n  first_mismatch=%d",         \
            a_src,                                                          \
            b_src,                                                          \
            tolerance,                                                      \
            tolerance,                                                      \
            first_mismatch                                                  \
        );                                                                  \
        _TEST_PRINT_ARRAYS(                                                 \
            a, b, length, first_mismatch, _TEST_DBL_F, std::fabs            \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
}

        _ASSERT_FLOAT_ARRAY_OP_METHOD(eq, ==, float)
        _ASSERT_FLOAT_ARRAY_OP_METHOD(eq, ==, double)

        _ASSERT_FLOAT_ARRAY_OP_METHOD(neq, !=, float)
        _ASSERT_FLOAT_ARRAY_OP_METHOD(neq, !=, double)


#define _ASSERT_FLOAT_ARRAY_CLOSE_METHOD(_type, _zero)                      \
static bool close(                                                          \
        _TEST_ARGS,                                                         \
        char const* a_src,                                                  \
        char const* b_src,                                                  \
        _type const* const a,                                               \
        _type const* const b,                                               \
        int const length,                                                   \
        _type const tolerance,                                              \
        _TEST_VARGS                                                         \
) {                                                                         \
    if (length < 1) {                                                       \
        _TEST_PASS();                                                       \
        return true;                                                        \
    }                                                                       \
    _type diff_sum = _zero;                                                 \
    _type max_mismatch = _zero;                                             \
    _type avg_diff;                                                         \
    int max_mismatch_index = -1;                                            \
    for (int i = 0; i != length; ++i) {                                     \
        _type diff = std::fabs(a[i] - b[i]);                                \
        diff_sum += diff;                                                   \
        if (diff > max_mismatch) {                                          \
            max_mismatch_index = i;                                         \
            max_mismatch = diff;                                            \
        }                                                                   \
    }                                                                       \
    avg_diff = diff_sum / (_type)length;                                    \
    if (avg_diff <= tolerance) {                                            \
        _TEST_PASS();                                                       \
        return true;                                                        \
    } else {                                                                \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a is close to b\n  a=%s\n  b=%s\n"                             \
                "  tolerance=" _TEST_DBL_F "\n"                             \
                "  avg_diff=" _TEST_DBL_F "\n"                              \
                "  max_mismatch=%d",                                        \
            a_src,                                                          \
            b_src,                                                          \
            tolerance,                                                      \
            tolerance,                                                      \
            avg_diff,                                                       \
            avg_diff,                                                       \
            max_mismatch_index                                              \
        );                                                                  \
        _TEST_PRINT_ARRAYS(                                                 \
            a, b, length, max_mismatch_index, _TEST_DBL_F, std::fabs        \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
}

        _ASSERT_FLOAT_ARRAY_CLOSE_METHOD(float, 0.0f)
        _ASSERT_FLOAT_ARRAY_CLOSE_METHOD(double, 0.0)


#define _ASSERT_CSTR_A_OP_B_METHOD(_name, _op)                              \
static bool _name(                                                          \
        _TEST_ARGS,                                                         \
        char const* a_src,                                                  \
        char const* b_src,                                                  \
        char const* a,                                                      \
        char const* b,                                                      \
        _TEST_VARGS                                                         \
) {                                                                         \
    if (                                                                    \
        (a == NULL && b == NULL)                                            \
        || (a != NULL && b != NULL && strcmp(a, b) _op 0)                   \
    ) {                                                                     \
        _TEST_PASS();                                                       \
        return true;                                                        \
    } else {                                                                \
        _TEST_FAIL(                                                         \
            file,                                                           \
            line,                                                           \
            "a " #_op " b\n  a=%s // %s\n  b=%s // %s",                     \
            _TEST_STR_OR(a, _TEST_NULL_PTR_STR),                            \
            a_src,                                                          \
            _TEST_STR_OR(b, _TEST_NULL_PTR_STR),                            \
            b_src                                                           \
        );                                                                  \
        return false;                                                       \
    }                                                                       \
}

        _ASSERT_CSTR_A_OP_B_METHOD(eq, ==)
        _ASSERT_CSTR_A_OP_B_METHOD(neq, !=)
        _ASSERT_CSTR_A_OP_B_METHOD(lt, <)
        _ASSERT_CSTR_A_OP_B_METHOD(lte, <=)
        _ASSERT_CSTR_A_OP_B_METHOD(gt, >)
        _ASSERT_CSTR_A_OP_B_METHOD(gte, >=)
};

/// \endcond
#endif

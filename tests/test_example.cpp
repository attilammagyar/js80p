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

#include "test.cpp"


TEST(booleans, {
    assert_true(true);
    assert_true(true, "Custom message");
    assert_true(true, "Custom %s message", "parametric");
    assert_false(1 == 2, "Custom %s message", "parametric");
})


TEST(ints, {
    assert_eq(1, 1);
    assert_neq(1, 2);
    assert_lt(1, 2);
    assert_lte(1, 2);
    assert_gt(2, 1, "Custom message");
    assert_gte(1, 3 - 1, "Custom %s message", "parametric");
})


TEST(doubles, {
    assert_eq(1.0, 1.0);
    assert_neq(1.0, 2.0);
    assert_lt(1.0, 2.0);
    assert_lte(1.0, 2.0);
    assert_gt(2.0, 1.0, "Custom message");
    assert_gte(1.0, 1.0, "Custom message");
    assert_eq(1.0, 1.0 + 0.2, 0.1, "Custom %s message", "parametric");
})


char const* some_func()
{
    return NULL;
}


TEST(c_strings, {
    char* b = NULL;

    assert_eq("foo", "foo");
    assert_neq("foo", "bar");
    assert_eq(NULL, b);
    assert_lt("aaa", "bbb");
    assert_lte("aaa", "aaa");
    assert_gt("bbb", "aaa", "Custom message");
    assert_gte("aaa", some_func(), "Custom %s message", "parametric");
})


TEST(arrays, {
    constexpr int length = 100;
    int a_int[length];
    int b_int[length];
    int c_int[length];
    double a_dbl[length];
    double b_dbl[length];
    double c_dbl[length];

    for (int i = 0; i != length; ++i) {
        a_int[i] = b_int[i] = i;
        c_int[i] = -1;
        a_dbl[i] = b_dbl[i] = (double)i;
        c_dbl[i] = -1.0;
    }

    b_dbl[50] += 0.1;

    assert_eq(a_int, b_int, length);
    assert_neq(a_int, c_int, length);
    assert_eq(a_int, b_int, length, "Custom %s message", "parametric");
    assert_eq(a_dbl, b_dbl, length, 0.2);
    assert_neq(a_dbl, c_dbl, length, 0.01);
    assert_neq(a_dbl, b_dbl, length, 0.2, "Custom %s message", "parametric");
})


TEST(double_arrays_close, {
    constexpr int length = 10;
    double a_dbl[length] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    double b_dbl[length] = {0.9, 1.1, 1.0, 1.1, 0.9, 1.0, 1.5, 0.3, 1.1, 0.9};

    assert_close(a_dbl, b_dbl, length, 0.2);
    assert_close(a_dbl, b_dbl, length, 0.01);
})

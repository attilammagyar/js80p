/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
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

#include <cmath>
#include <cstdio>
#include <cstring>

#include "js80p.hpp"

#include "dsp/math.cpp"


using namespace JS80P;


constexpr Number MINUS_LN_OF_10 = -Math::LN_OF_10;

constexpr int MAX_FUNCTIONS = 32;


char const* func_names[MAX_FUNCTIONS] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

char const** next_func_name = &func_names[0];


typedef bool (*RunIfRequestedFunc)(char const* const func_name, int const n);

RunIfRequestedFunc run_if_requested_functions[MAX_FUNCTIONS] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

RunIfRequestedFunc* next_func = &run_if_requested_functions[0];


class Function
{
    public:
        Function(Number const min, Number const max) : min(min), max(max)
        {
        }

        Number operator()(Number const x, Number const y) const
        {
            return 0.0;
        }

        Number get_min() const
        {
            return min;
        }

        Number get_max() const
        {
            return max;
        }

    private:
        Number min;
        Number max;
};


template<class FuncClass>
Number run_many_times(FuncClass const& func, Number const n)
{
    Number const n_inv = 1.0 / n;
    Number const min = func.get_min();
    Number const max = func.get_max();
    Number const delta = (max - min) * n_inv;
    Number sum = 0.0;

    for (Number x = min; x < max; x += delta) {
        Number const y = max - x;
        sum += func(x, y);
    }

    return sum * n_inv;
};


template<class FuncClass>
bool run_if_requested(char const* const func_name, int const n)
{
    if (0 != strcmp(func_name, FuncClass::NAME)) {
        return false;
    }

    Number const result = run_many_times<FuncClass>(FuncClass::func, n);

    fprintf(stderr, "%s\t%f\n", FuncClass::NAME, result);

    return true;
}


#define __NEW_GROUP(line)                                                   \
class NewGroup_ ## line                                                     \
{                                                                           \
    public:                                                                 \
        NewGroup_ ## line () {                                              \
            *(next_func_name++) = "";                                       \
        }                                                                   \
};                                                                          \
                                                                            \
NewGroup_ ## line new_group_ ## line;

#define _NEW_GROUP(line) __NEW_GROUP(line)
#define NEW_GROUP() _NEW_GROUP(__LINE__)


#define DEFINE_FUNC(class_name, name, expr, min, max)                       \
class class_name : public Function                                          \
{                                                                           \
    public:                                                                 \
        static constexpr char const* NAME = (name);                         \
                                                                            \
        static class_name const func;                                       \
                                                                            \
        class_name() : Function(min, max)                                   \
        {                                                                   \
            *(next_func_name++) = (name);                                   \
            *(next_func++) = &run_if_requested<class_name>;                 \
        }                                                                   \
                                                                            \
        Number operator()(Number const x, Number const y) const             \
        {                                                                   \
            return (expr);                                                  \
        }                                                                   \
};                                                                          \
                                                                            \
class_name const class_name::func;


constexpr Number FOUR_PI = 2.0 * Math::PI_DOUBLE;

DEFINE_FUNC(MathSin, "Math::sin(x)", Math::sin(x), -FOUR_PI, FOUR_PI);
DEFINE_FUNC(StdSin, "std::sin(x)", std::sin(x), -FOUR_PI, FOUR_PI);


NEW_GROUP();

DEFINE_FUNC(MathCos, "Math::cos(x)", Math::cos(x), -FOUR_PI, FOUR_PI);
DEFINE_FUNC(StdCos, "std::cos(x)", std::cos(x), -FOUR_PI, FOUR_PI);


NEW_GROUP();

DEFINE_FUNC(MathExp, "Math::exp(x)", Math::exp(x), Math::EXP_MIN, Math::EXP_MAX);
DEFINE_FUNC(StdExp, "std::exp(x)", std::exp(x), Math::EXP_MIN, Math::EXP_MAX);


NEW_GROUP();

constexpr Number POW_10_MIN = Math::POW_10_MIN;
constexpr Number POW_10_MAX = Math::POW_10_MAX;

DEFINE_FUNC(MathPow10, "Math::pow_10(x)", Math::pow_10(x), POW_10_MIN, POW_10_MAX);
DEFINE_FUNC(StdExpLog10, "std::exp(std::log(10)*x)", std::exp(Math::LN_OF_10 * x), POW_10_MIN, POW_10_MAX);
DEFINE_FUNC(StdPow10, "std::pow(10,x)", std::pow(10.0, x), POW_10_MIN, POW_10_MAX);


NEW_GROUP();

constexpr Number POW_10_INV_MIN = Math::POW_10_INV_MIN;
constexpr Number POW_10_INV_MAX = Math::POW_10_INV_MAX;

DEFINE_FUNC(MathPow10Inv, "Math::pow_10_inv(x)", Math::pow_10_inv(x), POW_10_INV_MIN, POW_10_INV_MAX);
DEFINE_FUNC(StdExpLog10Inv, "1/std::exp(std::log(10)*x)", 1.0 / std::exp(Math::LN_OF_10 * x), POW_10_INV_MIN, POW_10_INV_MAX);
DEFINE_FUNC(StdPow10Inv, "1/std::pow(10,x)", 1.0 / std::pow(10.0, x), POW_10_INV_MIN, POW_10_INV_MAX);
DEFINE_FUNC(StdExpMinusLog10, "std::exp(-std::log(10)*x)", std::exp(MINUS_LN_OF_10 * x), POW_10_INV_MIN, POW_10_INV_MAX);
DEFINE_FUNC(StdPow10Minus, "std::pow(10,-x)", std::pow(10.0, -x), POW_10_INV_MIN, POW_10_INV_MAX);


NEW_GROUP();

/* We need to compare these with each containing a multiplication as well. */
constexpr Number POW_2_SCALE = 1.0 / 100.0;
constexpr Number EXP_2_SCALE = POW_2_SCALE * Math::LN_OF_2;

DEFINE_FUNC(StdExpLog2, "std::exp(std::log(2)*0.01*x)", std::exp(EXP_2_SCALE * x), POW_10_MIN, POW_10_MAX);
DEFINE_FUNC(StdPow2, "std::pow(2,0.01*x)", std::pow(2.0, POW_2_SCALE * x), POW_10_MIN, POW_10_MAX);


NEW_GROUP();

/*
Shelving filters need both A = 10 ^ (G / 40) and sqrt(A) - but which is faster:
taking the square root of the already calculated A, or to calculate
10 ^ (G / 80) from scratch?
*/
DEFINE_FUNC(MathPow10Scaled, "Math::pow_10(0.0125*x)", Math::pow_10(0.0125 * x), 0.0, 20.0);
DEFINE_FUNC(StdSqrt, "std::sqrt(x)", std::sqrt(x), 0.0, 20.0);


NEW_GROUP();

DEFINE_FUNC(MathCombine, "Math::combine(x,0.5,y)", Math::combine(x, 0.5, y), 0.0, 1.0);
DEFINE_FUNC(SimpleCombine, "x*0.5+(1.0-x)*y", x * 0.5 + (1.0 - x) * y, 0.0, 1.0);


NEW_GROUP();

DEFINE_FUNC(StdPow24000, "std::pow(24000,x)", std::pow(24000.0, x), 0.0, 1.0);
DEFINE_FUNC(
    MathLookupLogBiquadFilterFreq,
    "Math::lookup(Math::log_biquad_filter_freq_table(),x)",
    Math::lookup(
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        x * Math::LOG_BIQUAD_FILTER_FREQ_SCALE
    ),
    0.0,
    1.0
);


void usage(char const* name)
{
    char const** next_func_name = &(func_names[0]);

    fprintf(stderr, "Usage: %s func N\n", name);
    fprintf(stderr, "\n");
    fprintf(stderr, "    func   function name to test\n");
    fprintf(stderr, "    N      positive integer, number of times to call the function\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Valid options for function name:\n");
    fprintf(stderr, "\n");

    while (NULL != *next_func_name) {
        fprintf(stderr, "    %s\n", *next_func_name);
        ++next_func_name;
    }
}


int main(int const argc, char const* argv[])
{
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    int const n = atoi(argv[2]);
    char const* const func_name = argv[1];

    if (n < 1) {
        fprintf(
            stderr,
            "ERROR: number of test runs must be a positive integer, got: %d (interpreted from \"%s\")\n\n",
            n,
            argv[2]
        );
        return 2;
    }

    RunIfRequestedFunc* next_func = &run_if_requested_functions[0];
    bool matched = false;

    while (NULL != *next_func && !matched) {
        matched = (*next_func)(func_name, n);
        ++next_func;
    }

    if (!matched) {
        fprintf(stderr, "ERROR: unknown function name: \"%s\"\n\n", func_name);
        return 3;
    }

    return 0;
}

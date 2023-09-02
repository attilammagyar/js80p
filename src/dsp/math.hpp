/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2022  Attila M. Magyar
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

#ifndef JS80P__DSP__MATH_HPP
#define JS80P__DSP__MATH_HPP

#include <cmath>
#include <algorithm>
#include <vector>

#include "js80p.hpp"


namespace JS80P
{

/**
 * \brief Faster versions of frequently used math.h functions, using table
 *        lookup with linear interpolation.
 */
class Math
{
    public:
        static constexpr Number PI = 3.14159265358979323846264338327950288419716939937510;
        static constexpr Number PI_DOUBLE = 2.0 * PI;
        static constexpr Number PI_HALF = PI / 2.0;
        static constexpr Number PI_QUARTER = PI / 4.0;
        static constexpr Number PI_SQR = PI * PI;

        static constexpr Number SQRT_OF_2 = std::sqrt(2.0);

        static constexpr Number LN_OF_2 = std::log(2.0);
        static constexpr Number LN_OF_10 = std::log(10.0);

        static constexpr Number POW_10_MIN = (
            Constants::BIQUAD_FILTER_GAIN_MIN * Constants::BIQUAD_FILTER_GAIN_SCALE
        ); ///< \warning This limit is not enforced. Values outside the limit may be imprecise.

        static constexpr Number POW_10_MAX = (
            Constants::BIQUAD_FILTER_GAIN_MAX * Constants::BIQUAD_FILTER_GAIN_SCALE
        ); ///< \warning This limit is not enforced. Values outside the limit may be imprecise.

        static constexpr Number POW_10_INV_MIN = (
            Constants::BIQUAD_FILTER_Q_MIN * Constants::BIQUAD_FILTER_Q_SCALE
        ); ///< \warning This limit is not enforced. Values outside the limit may be imprecise.

        static constexpr Number POW_10_INV_MAX = (
            Constants::BIQUAD_FILTER_Q_MAX * Constants::BIQUAD_FILTER_Q_SCALE
        ); ///< \warning This limit is not enforced. Values outside the limit may be imprecise.

        static constexpr Number EXP_MIN = (
            std::min(LN_OF_10 * POW_10_MIN, -1.0 * LN_OF_10 * POW_10_INV_MAX)
        ); ///< \warning This limit is not enforced. Values outside the limit may be imprecise.

        static constexpr Number EXP_MAX = (
            std::max(LN_OF_10 * POW_10_MAX, -1.0 * LN_OF_10 * POW_10_INV_MIN)
        ); ///< \warning This limit is not enforced. Values outside the limit may be imprecise.

        static constexpr int SIN_TABLE_SIZE = 0x1000;
        static constexpr int SIN_TABLE_MASK = SIN_TABLE_SIZE - 1;

        static constexpr int LOG_BIQUAD_FILTER_FREQ_TABLE_SIZE = 0x1000;

        static constexpr int LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX = (
            LOG_BIQUAD_FILTER_FREQ_TABLE_SIZE - 1
        );

        static constexpr Number LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV = (
            1.0 / (Number)LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX
        );

        static constexpr Number LOG_BIQUAD_FILTER_FREQ_SCALE = (
            (Number)LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX
        );

        /**
         * \warning Negative numbers close to multiples of PI are not handled
         *          very well with regards to precision.
         */
        static Number sin(Number const x) noexcept;

        /**
         * \warning Negative numbers close to multiples of PI are not handled
         *          very well with regards to precision.
         */
        static Number cos(Number const x) noexcept;

        static Number exp(Number const x) noexcept;
        static Number pow_10(Number const x) noexcept;
        static Number pow_10_inv(Number const x) noexcept;

        static Number const* log_biquad_filter_freq_table() noexcept;

        /**
         * \brief Calcualte the exact biquad filter frequency value using a
         *        logarithmic scale for a given ratio between 0.0 and 1.0.
         *        Intended for testing purposes.
         */
        static Number ratio_to_exact_log_biquad_filter_frequency(Number ratio) noexcept;

        static Frequency detune(
            Frequency const frequency,
            Number const cents
        ) noexcept;

        /**
         * \brief Compute a_weight * a + (1.0 - a_weight) * b
         */
        static Number combine(
            Number const a_weight,
            Number const a,
            Number const b
        ) noexcept;

        /**
         * \brief Apply a steep, tanh() based distortion to the given value
         *        between 0.0 and 1.0.
         */
        static Number distort(Number const level, Number const number) noexcept;

        /**
         * \brief Same as \c Math::distort(), but input and output are between
         *        -0.5 and 0.5.
         */
        static Number distort_centered_lfo(Number const level, Number const number) noexcept;

        /**
         * \brief Return a pseudo random number between 0.0 and 1.0, based on
         *        the given number between 0.0 and 1.0. The return value is
         *        deterministic, the same input number will always generate the
         *        same result.
         */
        static Number randomize(Number const level, Number const number) noexcept;

        /**
         * \brief Same as \c Math::randomize(), but input and output are between
         *        -0.5 and 0.5.
         */
        static Number randomize_centered_lfo(Number const level, Number const number) noexcept;

        /**
         * \brief Look up the given floating point, non-negative \c index in the
         *        given table, with linear interpolation. If \c index is greater
         *        than or equal to \c max_index, then the last element of the
         *        table is returned.
         */
        static Number lookup(
            Number const* const table,
            int const max_index,
            Number const index
        ) noexcept;

        /**
         * \brief Look up the given floating point \c index in the given table,
         *        with linear interpolation. If the \c index is negative, or it
         *        is greater than or equal to the specified \c table_size, then
         *        it wraps around.
         */
        static Number lookup_periodic(
            Number const* table,
            int const table_size,
            Number const index
        ) noexcept;

        /**
         * \brief Same as \c lookup_periodic() but for tables that have a size
         *        that is a power of 2.
         */
        static Number lookup_periodic_2(
            Number const* table,
            int const table_size,
            int const table_mask,
            Number const index
        ) noexcept;

        class Statistics;

        static void compute_statistics(
            std::vector<Number> const& numbers,
            Statistics& statistics
        ) noexcept;

        class Statistics {
            public:
                Number min;
                Number max;
                Number median;
                Number mean;
                Number standard_deviation;
                bool is_valid;
        };

    private:
        static constexpr int RANDOMS = 0x0200;
        static constexpr int RANDOMS_MAX_INDEX = RANDOMS - 1;
        static constexpr Number RANDOM_SCALE = (Number)RANDOMS;

        static constexpr int DISTORTION_TABLE_SIZE = 0x0800;
        static constexpr int DISTORTION_TABLE_MAX_INDEX = DISTORTION_TABLE_SIZE - 1;
        static constexpr Number DISTORTION_SCALE = (Number)DISTORTION_TABLE_SIZE;

        static constexpr Number SINE_SCALE = (Number)SIN_TABLE_SIZE / PI_DOUBLE;

        static constexpr int EXP_ITERATIONS = 8;

        static constexpr Number EXP_SCALE = 1.0 / (Number)(1 << EXP_ITERATIONS);
        static constexpr Number POW_10_SCALE = LN_OF_10 * EXP_SCALE;
        static constexpr Number POW_10_INV_SCALE = -1.0 * POW_10_SCALE;

        static constexpr Number DETUNE_CENTS_TO_POWER_OF_2_SCALE = 1.0 / 1200.0;

        static Math const math;

        static Number iterate_exp(Number const x, Number const scale) noexcept;

        Math() noexcept;

        void init_sines() noexcept;
        void init_randoms() noexcept;
        void init_distortion() noexcept;
        void init_log_biquad_filter_freq() noexcept;

        Number sin_impl(Number const x) const noexcept;

        Number sines[SIN_TABLE_SIZE];
        Number randoms[RANDOMS];
        Number randoms_centered_lfo[RANDOMS];
        Number distortion[DISTORTION_TABLE_SIZE];
        Number distortion_centered_lfo[DISTORTION_TABLE_SIZE];
        Number log_biquad_filter_freq[LOG_BIQUAD_FILTER_FREQ_TABLE_SIZE];
};

}

#endif

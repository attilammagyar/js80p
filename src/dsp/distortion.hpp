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

#ifndef JS80P__DSP__DISTORTION_HPP
#define JS80P__DSP__DISTORTION_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/filter.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P { namespace Distortion
{

constexpr Byte TYPE_TANH_3          = 0;
constexpr Byte TYPE_TANH_5          = 1;
constexpr Byte TYPE_TANH_10         = 2;

constexpr Byte TYPE_HARMONIC_13     = 3;
constexpr Byte TYPE_HARMONIC_15     = 4;
constexpr Byte TYPE_HARMONIC_135    = 5;
constexpr Byte TYPE_HARMONIC_SQR    = 6;
constexpr Byte TYPE_HARMONIC_TRI    = 7;

constexpr Byte TYPE_BIT_CRUSH_1     = 8;
constexpr Byte TYPE_BIT_CRUSH_2     = 9;
constexpr Byte TYPE_BIT_CRUSH_3     = 10;
constexpr Byte TYPE_BIT_CRUSH_4     = 11;
constexpr Byte TYPE_BIT_CRUSH_4_6   = 12;
constexpr Byte TYPE_BIT_CRUSH_5     = 13;
constexpr Byte TYPE_BIT_CRUSH_5_6   = 14;
constexpr Byte TYPE_BIT_CRUSH_6     = 15;
constexpr Byte TYPE_BIT_CRUSH_6_6   = 16;
constexpr Byte TYPE_BIT_CRUSH_7     = 17;
constexpr Byte TYPE_BIT_CRUSH_7_6   = 18;
constexpr Byte TYPE_BIT_CRUSH_8     = 19;
constexpr Byte TYPE_BIT_CRUSH_8_6   = 20;
constexpr Byte TYPE_BIT_CRUSH_9     = 21;

constexpr Byte TYPE_DELAY_FEEDBACK  = 22;

constexpr Byte TYPES                = 23;


class TypeParam : public ByteParam
{
    public:
        TypeParam(std::string const& name, Byte const default_type) noexcept;
};


typedef Sample Table[0x2000];


/**
 * \brief Lookup table for the Distortion effect.
 *
 * The tables hold values of various shaping functions which
 * - map values between -3.0 and 3.0 to the [-1.0, 1.0] interval,
 * - and for which f(x) = -f(-x) for all real valued x,
 * - and which are monotonically increasing,
 * - and which map the [-1.0, 1.0] interval to the [-1.0 + A, 1.0 - A] interval
 *   for some sufficiently small or zero A constant.
 *
 * Due to the symmetry, the tables actually only contain values for the
 * [0.0, 3.0] interval.
 *
 * The f tables contain the values for the shaping functions, and the F0 tables
 * hold their respective antiderivatives.
 *
 * \sa Distortion
 */
class Tables
{
    public:
        static constexpr int SIZE = 0x2000;
        static constexpr int MAX_INDEX = SIZE - 1;

        static constexpr Sample INPUT_MAX = 3.0;
        static constexpr Sample INPUT_MIN = - INPUT_MAX;

        Tables();

        Table const& get_f_table(Byte const type) const noexcept;
        Table const& get_F0_table(Byte const type) const noexcept;

    private:
        static constexpr Sample SIZE_INV = 1.0 / (Sample)SIZE;

        void initialize_tanh_tables(
            Byte const type,
            Number const steepness
        ) noexcept;

        template<class H>
        void initialize_spline_tables(
                Byte const type,
                H const& h,
                Number const alpha,
                Number const gamma,
                Number const A,
                Number const B,
                Number const C,
                Number const D,
                Number const cf
        ) noexcept;

        void initialize_harmonic_tables(
            Byte const type,
            Number const w1,
            Number const w3,
            Number const w5,
            Number const alpha,
            Number const gamma,
            Number const A,
            Number const B,
            Number const C,
            Number const D,
            Number const cf,
            Number const ch
        ) noexcept;

        void initialize_bit_crush_tables(
            Byte const type,
            Integer const k,
            Number const alpha,
            Number const gamma,
            Number const A,
            Number const B,
            Number const C,
            Number const D,
            Number const cf,
            Number const ch
        ) noexcept;

        void initialize_delay_feedback_tables() noexcept;

        Table f_tables[TYPES];
        Table F0_tables[TYPES];
};


/**
 * \brief Antialiased waveshaper based distortion, using Antiderivative
 *        Antialiasing (ADAA). See:
 *        <a href="https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf">
 *        Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution
 *        (Parker, J., Zavalishin, V., & Bivic, E.L. - 2016)</a>.
 */
template<class InputSignalProducerClass>
class Distortion : public Filter<InputSignalProducerClass>
{
    friend class JS80P::SignalProducer;

    public:

        Distortion(
            std::string const& name,
            TypeParam const& type,
            InputSignalProducerClass& input,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        Distortion(
            std::string const& name,
            TypeParam const& type,
            InputSignalProducerClass& input,
            FloatParamS& level_leader,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        ~Distortion();

        virtual void reset() noexcept override;

        FloatParamS level;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

    private:
        static constexpr int MAX_INDEX = Tables::MAX_INDEX;

        static constexpr Sample INPUT_MAX = Tables::INPUT_MAX;
        static constexpr Sample INPUT_MIN = Tables::INPUT_MIN;
        static constexpr Sample INPUT_MAX_INV = 1.0 / INPUT_MAX;
        static constexpr Sample TABLE_SIZE_FLOAT = (Sample)Tables::SIZE;
        static constexpr Sample SCALE = TABLE_SIZE_FLOAT * INPUT_MAX_INV;

        void initialize_instance() noexcept;

        Sample distort(
            Table const& f_table,
            Table const& F0_table,
            Sample const input_sample,
            Sample& previous_input_sample,
            Sample& F0_previous_input_sample
        ) noexcept;

        Sample f(Table const& f_table, Sample const x) const noexcept;
        Sample F0(Table const& F0_table, Sample const x) const noexcept;
        Sample lookup(Table const& table, Sample const x) const noexcept;

        TypeParam const& type;

        Sample const* level_buffer;
        Sample* previous_input_sample;
        Sample* F0_previous_input_sample;
        Number level_value;
        Byte previous_type;
        Byte current_type;
};

} }

#endif


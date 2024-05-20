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

#ifndef JS80P__DSP__WAVEFOLDER_HPP
#define JS80P__DSP__WAVEFOLDER_HPP

#include "js80p.hpp"

#include "dsp/filter.hpp"
#include "dsp/math.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

/**
 * \brief Antialiased waveshaper based wavefolder, using Antiderivative
 *        Antialiasing (ADAA). See:
 *        <a href="https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf">
 *        Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution
 *        (Parker, J., Zavalishin, V., & Bivic, E.L. - 2016)</a>. The shaping
 *        function is an approximation of a triangle wave which has a wavelength
 *        of 4.0, and which is positioned so that f(0.0) = 0.0, and the
 *        projection of the [-1.0, 1.0] interval is approximately itself.
 */
template<class InputSignalProducerClass>
class Wavefolder : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        explicit Wavefolder(InputSignalProducerClass& input) noexcept;

        Wavefolder(
            InputSignalProducerClass& input,
            FloatParamS& folding_leader,
            Byte const& voice_status,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        ~Wavefolder();

        virtual void reset() noexcept override;

        FloatParamS folding;

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
        static constexpr Sample TRANSITION_INV = 1.0 / Constants::FOLD_TRANSITION;
        static constexpr Sample TRANSITION_DELTA = 1.0 - Constants::FOLD_TRANSITION;

        static constexpr Sample TRIANGLE_SCALE = 8.0 / Math::PI_SQR;

        static constexpr Sample S0 = TRIANGLE_SCALE;
        static constexpr Sample S1 = Math::PI_HALF;
        static constexpr Sample S2 = TRIANGLE_SCALE / 9.0;
        static constexpr Sample S3 = Math::PI_HALF * 3.0;
        static constexpr Sample S4 = TRIANGLE_SCALE / 25.0;
        static constexpr Sample S5 = Math::PI_HALF * 5.0;
        static constexpr Sample S6 = TRIANGLE_SCALE * 2.0 / Math::PI;
        static constexpr Sample S7 = TRIANGLE_SCALE / (27.0 * Math::PI);
        static constexpr Sample S8 = TRIANGLE_SCALE / (125.0 * Math::PI);

        /*
        The table contains a whole period of the triangle wave function's
        antiderivative for the [-2.0, 2.0] interval. Folding occurs because the
        input (which is supposed to go from -1.0 to 1.0) is scaled up by
        1 + folding_level, so when the periodic-triangle-wave shaping function
        is applied, the scaled up input spans multiple wave periods.

        The triangle wave is aligned so that it projects the [-1.0, 1.0]
        interval onto itself. Since the wave is bandlimited, this projection is
        imperfect, so the first 10% of the folding level parameter is used for
        smoothly transitioning from the "bypass" state to the "no folding yet
        but the triangle wave already has some small influence" state.
        */
        static constexpr int TABLE_SIZE = 0x1000;
        static constexpr int TABLE_INDEX_MASK = TABLE_SIZE - 1;
        static constexpr Number TABLE_SIZE_FLOAT = (Number)TABLE_SIZE;
        static constexpr Number TABLE_SIZE_FLOAT_INV = 1.0 / TABLE_SIZE_FLOAT;
        static constexpr Number WAVE_LENGTH = Math::PI_DOUBLE / S1;
        static constexpr Number WAVE_LENGTH_HALF = WAVE_LENGTH / 2.0;
        static constexpr Number TABLE_SCALE = TABLE_SIZE_FLOAT / WAVE_LENGTH;
        static constexpr Number TABLE_OFFSET = TABLE_SIZE_FLOAT / WAVE_LENGTH_HALF;

        // static Sample f_table[TABLE_SIZE];
        static Sample F0_table[TABLE_SIZE];
        static bool is_initialized;

        static void initialize_class() noexcept;

        void initialize_instance() noexcept;

        Sample fold(
            Sample const folding,
            Sample const input_sample,
            Sample& previous_input_sample,
            Sample& F0_previous_input_sample,
            Sample& previous_output_sample
        ) noexcept;

        // Sample f(Sample const x) const noexcept;
        Sample F0(Sample const x) const noexcept;

        Sample const* folding_buffer;
        Sample* previous_input_sample;
        Sample* F0_previous_input_sample;
        Sample* previous_output_sample;
        Number folding_value;
};

}

#endif

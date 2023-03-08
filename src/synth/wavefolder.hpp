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

#ifndef JS80P__SYNTH__WAVEFOLDER_HPP
#define JS80P__SYNTH__WAVEFOLDER_HPP

#include "js80p.hpp"

#include "synth/filter.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

/**
 * \brief Antialiased waveshaper based wave folder. See:
 *        <a href="https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf">
 *        Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution
 *        (Parker, J., Zavalishin, V., & Bivic, E.L. - 2016)</a>.
 */
template<class InputSignalProducerClass>
class Wavefolder : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        Wavefolder(InputSignalProducerClass& input);
        Wavefolder(InputSignalProducerClass& input, FloatParam& folding_leader);
        ~Wavefolder();

        FloatParam folding;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        );

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

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
        The trigonometric functions in the Math class handle positive numbers
        better, so we shift everything by a few periods.
        */
        static constexpr Sample TRIG_OFFSET = (
            Math::PI_DOUBLE * std::ceil(Constants::FOLD_MAX * S5)
        );

        void initialize_instance();

        Sample fold(
            Sample const folding,
            Sample const input_sample,
            Sample& previous_input_sample,
            Sample& F0_previous_input_sample,
            Sample& previous_output_sample
        );

        Sample f(Sample const x) const;
        Sample F0(Sample const x) const;

        Sample const* folding_buffer;
        Sample* previous_input_sample;
        Sample* F0_previous_input_sample;
        Sample* previous_output_sample;
        Number folding_value;
};

}

#endif

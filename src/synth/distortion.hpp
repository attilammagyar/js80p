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

#ifndef JS80P__SYNTH__DISTORTION_HPP
#define JS80P__SYNTH__DISTORTION_HPP

#include <string>

#include "js80p.hpp"

#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

/**
 * \brief Antialiased waveshaper based distortion. See:
 *        <a href="https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf">
 *        Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution
 *        (Parker, J., Zavalishin, V., & Bivic, E.L. - 2016)</a>.
 */
template<class InputSignalProducerClass>
class Distortion : public Filter<InputSignalProducerClass>
{
    public:
        Distortion(
            std::string const name,
            Number const steepness,
            InputSignalProducerClass& input
        );

        ~Distortion();

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

        FloatParam level;

    private:
        static constexpr int TABLE_SIZE = 0x0800;
        static constexpr int MAX_INDEX = TABLE_SIZE - 1;

        static constexpr Sample INPUT_MAX = 3.0;
        static constexpr Sample INPUT_MIN = -3.0;
        static constexpr Sample INPUT_MAX_INV = 1.0 / INPUT_MAX;
        static constexpr Sample TABLE_SIZE_FLOAT = (Sample)TABLE_SIZE;
        static constexpr Sample SCALE = TABLE_SIZE_FLOAT * INPUT_MAX_INV;

        Sample distort(
            Sample const input_sample,
            Sample& previous_input_sample,
            Sample& F0_previous_input_sample
        );

        Sample f(Sample const x) const;
        Sample F0(Sample const x) const;

        Sample lookup(Sample const* const table, Sample const x) const;

        Sample f_table[TABLE_SIZE];
        Sample F0_table[TABLE_SIZE];

        Sample const* level_buffer;
        Sample* previous_input_sample;
        Sample* F0_previous_input_sample;
        Number level_value;
};

}

#endif


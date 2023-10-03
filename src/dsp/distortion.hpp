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

#ifndef JS80P__DSP__DISTORTION_HPP
#define JS80P__DSP__DISTORTION_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/filter.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P { namespace Distortion
{

enum Type {
    SOFT = 0,
    HEAVY = 1,

    NUMBER_OF_TYPES = 2,
};


typedef Sample Table[0x2000];


class Tables
{
    public:
        static constexpr int SIZE = 0x2000;
        static constexpr int MAX_INDEX = SIZE - 1;

        static constexpr Sample INPUT_MAX = 3.0;
        static constexpr Sample INPUT_MIN = - INPUT_MAX;

        Tables();

        Table const& get_f_table(Type const type) const noexcept;
        Table const& get_F0_table(Type const type) const noexcept;

    private:
        static constexpr Sample SIZE_INV = 1.0 / (Sample)SIZE;

        void fill_tables(Type const type, Number const steepness) noexcept;

        Table f_tables[Type::NUMBER_OF_TYPES];
        Table F0_tables[Type::NUMBER_OF_TYPES];
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
            std::string const name,
            Type const type,
            InputSignalProducerClass& input
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

        Sample distort(
            Sample const input_sample,
            Sample& previous_input_sample,
            Sample& F0_previous_input_sample
        ) noexcept;

        Sample f(Sample const x) const noexcept;
        Sample F0(Sample const x) const noexcept;

        Sample lookup(Table const& table, Sample const x) const noexcept;

        Table const& f_table;
        Table const& F0_table;

        Sample const* level_buffer;
        Sample* previous_input_sample;
        Sample* F0_previous_input_sample;
        Number level_value;
};

} }

#endif


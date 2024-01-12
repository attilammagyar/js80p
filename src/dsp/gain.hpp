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

#ifndef JS80P__DSP__GAIN_HPP
#define JS80P__DSP__GAIN_HPP

#include "js80p.hpp"

#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Gain : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        Gain(
            InputSignalProducerClass& input,
            FloatParamS& gain,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        void find_input_peak(
            Integer const round,
            Integer const sample_count,
            Sample& peak,
            Integer& peak_index
        ) const noexcept;

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
        Sample const* gain_buffer;

        FloatParamS& gain;
};

}

#endif

/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__FILTER_CPP
#define JS80P__DSP__FILTER_CPP

#include "dsp/filter.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Filter<InputSignalProducerClass>::Filter(
        InputSignalProducerClass& input,
        Integer const number_of_children,
        Integer const channels,
        SignalProducer* const buffer_owner
) noexcept
    : SignalProducer(
        channels > 0 ? channels : input.get_channels(),
        number_of_children,
        0,
        buffer_owner
    ),
    input(input),
    input_buffer(NULL)
{
}


template<class InputSignalProducerClass>
Sample const* const* Filter<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    JS80P_ASSERT(input.get_channels() == this->get_channels());

    input_buffer = SignalProducer::produce<InputSignalProducerClass>(
        input, round, sample_count
    );

    return input_buffer;
}


template<class InputSignalProducerClass>
Sample const* const* Filter<InputSignalProducerClass>::input_was_silent(
        Integer const round
) noexcept {
    mark_round_as_silent(round);

    return input_buffer;
}

}

#endif

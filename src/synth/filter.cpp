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

#ifndef JS80P__SYNTH__FILTER_CPP
#define JS80P__SYNTH__FILTER_CPP

#include "synth/filter.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Filter<InputSignalProducerClass>::Filter(
        InputSignalProducerClass& input,
        Integer const number_of_children,
        Integer const channels
) noexcept
    : SignalProducer(channels > 0 ? channels : input.get_channels(), number_of_children),
    input(input),
    input_buffer(NULL)
{
}


template<class InputSignalProducerClass>
Sample const* const* Filter<InputSignalProducerClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    input_buffer = SignalProducer::produce<InputSignalProducerClass>(
        &input, round, sample_count
    );

    return input_buffer;
}

}

#endif

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

#ifndef JS80P__DSP__FILTER_HPP
#define JS80P__DSP__FILTER_HPP

#include "js80p.hpp"

#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Filter;


typedef Filter<SignalProducer> SimpleFilter;


template<class InputSignalProducerClass>
class Filter : public SignalProducer
{
    friend class SignalProducer;

    public:
        Filter(
            InputSignalProducerClass& input,
            Integer const number_of_children = 0,
            Integer const channels = 0,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* input_was_silent(Integer const round) noexcept;

        InputSignalProducerClass& input;
        Sample const* const* input_buffer;

};

}

#endif

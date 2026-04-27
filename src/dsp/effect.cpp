/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
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

#ifndef JS80P__DSP__EFFECT_CPP
#define JS80P__DSP__EFFECT_CPP

#include "dsp/effect.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Effect<InputSignalProducerClass>::Effect(
        std::string const& name,
        InputSignalProducerClass& input,
        Integer const number_of_children,
        SignalProducer* const buffer_owner
) : Filter<InputSignalProducerClass>(
        input,
        number_of_children + 2,
        input.get_channels(),
        buffer_owner
    ),
    dry(name + "DRY", 0.0, 1.0, 1.0),
    wet(name + "WET", 0.0, 1.0, 0.0)
{
    this->register_child(dry);
    this->register_child(wet);
}


template<class InputSignalProducerClass>
Sample const* const* Effect<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    dry_buffer = FloatParamS::produce_if_not_constant(dry, round, sample_count);
    wet_buffer = FloatParamS::produce_if_not_constant(wet, round, sample_count);

    is_dry = wet_buffer == NULL && wet.get_value() < 0.000001;

    if (is_dry && dry_buffer == NULL && dry.get_value() > 0.99999) {
        return this->input_buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Effect<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const end_sample_index,
        Sample** const buffer
) noexcept {
    if (is_dry) {
        if (dry_buffer == NULL) {
            render<true, ParamValueWrapper, ParamValueWrapper>(
                round,
                first_sample_index,
                end_sample_index,
                buffer,
                ParamValueWrapper(dry.get_value()),
                ParamValueWrapper(0.0)
            );
        } else {
            render<true, ParamValueBufferWrapper, ParamValueWrapper>(
                round,
                first_sample_index,
                end_sample_index,
                buffer,
                ParamValueBufferWrapper(dry_buffer),
                ParamValueWrapper(0.0)
            );
        }

        return;
    }

    if (wet_buffer == NULL) {
        if (dry_buffer == NULL) {
            render<false, ParamValueWrapper, ParamValueWrapper>(
                round,
                first_sample_index,
                end_sample_index,
                buffer,
                ParamValueWrapper(dry.get_value()),
                ParamValueWrapper(wet.get_value())
            );
        } else {
            render<false, ParamValueBufferWrapper, ParamValueWrapper>(
                round,
                first_sample_index,
                end_sample_index,
                buffer,
                ParamValueBufferWrapper(dry_buffer),
                ParamValueWrapper(wet.get_value())
            );
        }
    } else {
        if (dry_buffer == NULL) {
            render<false, ParamValueWrapper, ParamValueBufferWrapper>(
                round,
                first_sample_index,
                end_sample_index,
                buffer,
                ParamValueWrapper(dry.get_value()),
                ParamValueBufferWrapper(wet_buffer)
            );
        } else {
            render<false, ParamValueBufferWrapper, ParamValueBufferWrapper>(
                round,
                first_sample_index,
                end_sample_index,
                buffer,
                ParamValueBufferWrapper(dry_buffer),
                ParamValueBufferWrapper(wet_buffer)
            );
        }
    }
}


template<class InputSignalProducerClass>
template<bool is_dry, class DryBufferClass, class WetBufferClass>
void Effect<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const end_sample_index,
        Sample** const buffer,
        DryBufferClass const& dry,
        WetBufferClass const& wet
) const noexcept {
    Integer const channels = this->channels;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* const input_channel = this->input_buffer[c];
        Sample* const out_channel = buffer[c];

        for (Integer i = first_sample_index; i != end_sample_index; ++i) {
            if constexpr (is_dry) {
                out_channel[i] = dry[i] * input_channel[i];
            } else {
                out_channel[i] = dry[i] * input_channel[i] + wet[i] * out_channel[i];
            }
        }
    }
}

}

#endif

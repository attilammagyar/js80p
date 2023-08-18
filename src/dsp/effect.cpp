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

#ifndef JS80P__DSP__EFFECT_CPP
#define JS80P__DSP__EFFECT_CPP

#include "dsp/effect.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Effect<InputSignalProducerClass>::Effect(
        std::string const name,
        InputSignalProducerClass& input,
        Integer const number_of_children
) : Filter<InputSignalProducerClass>(
        input,
        number_of_children + 2,
        input.get_channels()
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
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    if (is_dry) {
        if (dry_buffer == NULL) {
            Sample const dry_level = dry.get_value();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = dry_level * this->input_buffer[c][i];
                }
            }
        } else {
            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = dry_buffer[i] * this->input_buffer[c][i];
                }
            }
        }

        return;
    }

    if (wet_buffer == NULL) {
        Sample const wet_level = wet.get_value();

        if (dry_buffer == NULL) {
            Sample const dry_level = dry.get_value();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_level * this->input_buffer[c][i]
                        + wet_level * buffer[c][i]
                    );
                }
            }
        } else {
            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_buffer[i] * this->input_buffer[c][i]
                        + wet_level * buffer[c][i]
                    );
                }
            }
        }
    } else {
        if (dry_buffer == NULL) {
            Sample const dry_level = dry.get_value();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_level * this->input_buffer[c][i]
                        + wet_buffer[i] * buffer[c][i]
                    );
                }
            }
        } else {
            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_buffer[i] * this->input_buffer[c][i]
                        + wet_buffer[i] * buffer[c][i]
                    );
                }
            }
        }
    }
}

}

#endif

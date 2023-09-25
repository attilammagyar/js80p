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

#ifndef JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_CPP
#define JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_CPP

#include <cmath>

#include "dsp/math.hpp"
#include "dsp/side_chain_compressable_effect.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
SideChainCompressableEffect<InputSignalProducerClass>::SideChainCompressableEffect(
        std::string const name,
        InputSignalProducerClass& input,
        Integer const number_of_children
) : Effect<InputSignalProducerClass>(
        name,
        input,
        number_of_children + 5
    ),
    side_chain_compression_threshold(name + "STH", -120.0, 0.0, -30.0),
    side_chain_compression_attack_time(name + "SAT", 0.001, 3.0, 0.02),
    side_chain_compression_release_time(name + "SRL", 0.001, 3.0, 0.20),
    side_chain_compression_gain_reduction(name + "SG", -120.0, 0.0, 0.0),
    previous_action(Action::BYPASS)
{
    this->register_child(side_chain_compression_threshold);
    this->register_child(side_chain_compression_attack_time);
    this->register_child(side_chain_compression_release_time);
    this->register_child(side_chain_compression_gain_reduction);
    this->register_child(gain);
}


template<class InputSignalProducerClass>
Sample const* const* SideChainCompressableEffect<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* buffer = (
        Effect<InputSignalProducerClass>::initialize_rendering(
            round, sample_count
        )
    );

    if (buffer != NULL) {
        FloatParamS::produce_if_not_constant(gain, round, sample_count);

        return buffer;
    }

    Number const attack_time_value = side_chain_compression_attack_time.get_value();
    Number const release_time_value = side_chain_compression_release_time.get_value();
    Number const gain_reduction_value = side_chain_compression_gain_reduction.get_value();

    is_bypassing = this->is_dry || std::fabs(gain_reduction_value) < 0.000001;

    if (!is_bypassing) {
        Action const next_action = decide_next_action(sample_count);

        if (next_action == Action::COMPRESS) {
            if (previous_action == Action::BYPASS) {
                gain.cancel_events_at(0.0);
                gain.schedule_linear_ramp(
                    attack_time_value, Math::db_to_linear(gain_reduction_value)
                );
            }
        } else if (previous_action == Action::COMPRESS) {
            gain.cancel_events_at(0.0);
            gain.schedule_linear_ramp(release_time_value, 1.0);
        }

        previous_action = next_action;
    }

    gain_buffer = FloatParamS::produce_if_not_constant(gain, round, sample_count);

    return NULL;
}


template<class InputSignalProducerClass>
typename SideChainCompressableEffect<InputSignalProducerClass>::Action SideChainCompressableEffect<InputSignalProducerClass>::decide_next_action(
        Integer const sample_count
) const noexcept {
    Integer const channels = this->channels;
    Number const threshold = Math::db_to_linear(
        side_chain_compression_threshold.get_value()
    );
    Sample const* const* input_buffer = this->input_buffer;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* input_channel = input_buffer[c];

        for (Integer i = 0; i != sample_count; ++i) {
            if (std::fabs(input_channel[i]) > threshold) {
                return Action::COMPRESS;
            }
        }
    }

    return Action::BYPASS;
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (is_bypassing) {
        Effect<InputSignalProducerClass>::render(
            round, first_sample_index, last_sample_index, buffer
        );

        return;
    }

    Sample const* dry_buffer = this->dry_buffer;
    Sample const* wet_buffer = this->wet_buffer;
    Sample const* gain_buffer = this->gain_buffer;

    Integer const channels = this->channels;

    if (wet_buffer == NULL) {
        Sample const wet_level = this->wet.get_value();

        if (dry_buffer == NULL) {
            Sample const dry_level = this->dry.get_value();

            if (gain_buffer == NULL) {
                Sample const gain_value = wet_level * gain.get_value();

                for (Integer c = 0; c != channels; ++c) {
                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[c][i] = (
                            dry_level * this->input_buffer[c][i] + gain_value * buffer[c][i]
                        );
                    }
                }
            } else {
                for (Integer c = 0; c != channels; ++c) {
                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        buffer[c][i] = (
                            dry_level * this->input_buffer[c][i]
                            + wet_level * gain_buffer[i] * buffer[c][i]
                        );
                    }
                }
            }
        } else if (gain_buffer == NULL) {
            Sample const gain_value = wet_level * gain.get_value();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_buffer[i] * this->input_buffer[c][i]
                        + gain_value * buffer[c][i]
                    );
                }
            }
        } else {
            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_buffer[i] * this->input_buffer[c][i]
                        + wet_level * gain_buffer[i] * buffer[c][i]
                    );
                }
            }
        }
    } else if (dry_buffer == NULL) {
        Sample const dry_level = this->dry.get_value();

        if (gain_buffer == NULL) {
            Sample const gain_value = gain.get_value();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_level * this->input_buffer[c][i]
                        + wet_buffer[i] * gain_value * buffer[c][i]
                    );
                }
            }
        } else {
            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = (
                        dry_level * this->input_buffer[c][i]
                        + wet_buffer[i] * gain_buffer[i] * buffer[c][i]
                    );
                }
            }
        }
    } else if (gain_buffer == NULL) {
        Sample const gain_value = gain.get_value();

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = (
                    dry_buffer[i] * this->input_buffer[c][i]
                    + wet_buffer[i] * gain_value * buffer[c][i]
                );
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = (
                    dry_buffer[i] * this->input_buffer[c][i]
                    + wet_buffer[i] * gain_buffer[i] * buffer[c][i]
                );
            }
        }
    }
}

}

#endif


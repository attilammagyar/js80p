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

#ifndef JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_CPP
#define JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_CPP

#include <algorithm>

#include "dsp/math.hpp"
#include "dsp/side_chain_compressable_effect.hpp"


namespace JS80P
{

CompressionModeParam::CompressionModeParam(
        std::string const& name
) noexcept
    : ByteParam(
        name,
        (Byte)CompressionMode::COMPRESSION_MODE_COMPRESSOR,
        (Byte)CompressionMode::COMPRESSION_MODE_EXPANDER,
        (Byte)CompressionMode::COMPRESSION_MODE_COMPRESSOR
    )
{
}


template<class InputSignalProducerClass>
SideChainCompressableEffect<InputSignalProducerClass>::SideChainCompressableEffect(
        std::string const& name,
        InputSignalProducerClass& input,
        Integer const number_of_children,
        SignalProducer* const wet_buffer_owner
) : Effect<InputSignalProducerClass>(
        name,
        input,
        number_of_children + 6,
        wet_buffer_owner
    ),
    side_chain_compression_threshold(name + "CTH", -120.0, 0.0, -18.0),
    side_chain_compression_attack_time(name + "CAT", 0.001, 3.0, 0.02),
    side_chain_compression_release_time(name + "CRL", 0.001, 3.0, 0.20),
    side_chain_compression_ratio(name + "CR", 1.0, 120.0, NO_OP_RATIO),
    side_chain_compression_mode(name + "CM"),
    gain(name + "G", 0.0, BYPASS_GAIN, BYPASS_GAIN),
    previous_action(Action::BYPASS_OR_RELEASE),
    previous_mode(CompressionMode::COMPRESSION_MODE_COMPRESSOR),
    is_bypassing(false)
{
    this->register_child(side_chain_compression_threshold);
    this->register_child(side_chain_compression_attack_time);
    this->register_child(side_chain_compression_release_time);
    this->register_child(side_chain_compression_ratio);
    this->register_child(side_chain_compression_mode);
    this->register_child(gain);
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::reset() noexcept
{
    Effect<InputSignalProducerClass>::reset();
    clear_state();
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::clear_state() noexcept
{
    gain.cancel_events_at(0.0);
    gain.set_value(BYPASS_GAIN);
    previous_action = Action::BYPASS_OR_RELEASE;
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
        fast_bypass();

        return buffer;
    }

    copy_input(sample_count);

    Number const ratio_value = side_chain_compression_ratio.get_value();

    is_bypassing = this->is_dry || Math::is_close(ratio_value, NO_OP_RATIO);

    if (is_bypassing) {
        fast_bypass();

        return NULL;
    }

    Byte const new_mode = side_chain_compression_mode.get_value();

    if (new_mode != previous_mode) {
        clear_state();
        previous_mode = new_mode;
    }

    Number const threshold_db = side_chain_compression_threshold.get_value();

    Sample peak;
    Integer peak_index;

    SignalProducer::find_peak(
        this->input_buffer, this->channels, sample_count, peak, peak_index
    );
    peak_tracker.update(peak, peak_index, sample_count, this->sampling_period);
    peak = peak_tracker.get_peak();

    Number const diff_db = Math::linear_to_db(peak) - threshold_db;

    if ((CompressionMode)new_mode == CompressionMode::COMPRESSION_MODE_COMPRESSOR) {
        if (diff_db > 0.0) {
            compress(
                peak,
                threshold_db + diff_db / ratio_value,
                BYPASS_GAIN,
                side_chain_compression_attack_time
            );
        } else if (previous_action == Action::COMPRESS) {
            release(side_chain_compression_release_time);
        } else if (Math::is_close(gain.get_value(), BYPASS_GAIN)) {
            fast_bypass();
        }
    } else {
        if (diff_db < 0.0) {
            /* Gate closes, hence using the release time. */
            compress(
                peak,
                std::max(Math::DB_MIN, threshold_db + diff_db * ratio_value),
                0.0,
                side_chain_compression_release_time
            );
        } else if (previous_action == Action::COMPRESS) {
            /* Gate opens, hence using the attack time. */
            release(side_chain_compression_attack_time);
        } else if (Math::is_close(gain.get_value(), BYPASS_GAIN)) {
            fast_bypass();
        }
    }

    gain_buffer = FloatParamS::produce_if_not_constant(gain, round, sample_count);

    return NULL;
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::fast_bypass() noexcept
{
    clear_state();
    is_bypassing = true;
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::copy_input(
        Integer const sample_count
) noexcept {
    if (
            (void*)this->buffer == (void*)this->input_buffer
            || this->input_buffer == NULL
            || (void*)this->get_buffer_owner() != (void*)this
    ) {
        return;
    }

    for (Integer c = 0; c != this->channels; ++c) {
        Sample const* const in_channel = this->input_buffer[c];
        Sample* const out_channel = this->buffer[c];

        for (Integer i = 0; i != sample_count; ++i) {
            out_channel[i] = in_channel[i];
        }
    }
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::compress(
        Sample const peak,
        Number const target_peak_db,
        Number const zero_peak_target,
        FloatParamB const& time_param
) noexcept {
    Sample const target_peak = Math::db_to_linear(target_peak_db);
    Number const new_target_gain = (
        peak > 0.000001
            ? std::min(BYPASS_GAIN, (Number)(target_peak / peak))
            : zero_peak_target
    );

    if (Math::is_close(gain.get_value(), new_target_gain, 0.005)) {
        gain.cancel_events_at(0.0);
    } else {
        schedule_gain_ramp(new_target_gain, time_param);
    }

    previous_action = Action::COMPRESS;
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::schedule_gain_ramp(
        Number const target_gain,
        FloatParamB const& time_param
) noexcept {
    gain.cancel_events_at(0.0);
    gain.schedule_linear_ramp((Seconds)time_param.get_value(), target_gain);
}


template<class InputSignalProducerClass>
void SideChainCompressableEffect<InputSignalProducerClass>::release(
        FloatParamB const& time_param
) noexcept {
    schedule_gain_ramp(BYPASS_GAIN, time_param);
    previous_action = Action::BYPASS_OR_RELEASE;
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


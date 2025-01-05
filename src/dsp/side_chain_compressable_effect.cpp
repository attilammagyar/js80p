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

template<class InputSignalProducerClass, CompressionCurve curve>
SideChainCompressableEffect<InputSignalProducerClass, curve>::SideChainCompressableEffect(
        std::string const& name,
        InputSignalProducerClass& input,
        Integer const number_of_children,
        SignalProducer* const wet_buffer_owner
) : Effect<InputSignalProducerClass>(
        name,
        input,
        number_of_children + 5,
        wet_buffer_owner
    ),
    side_chain_compression_threshold(name + "CTH", -120.0, 0.0, -18.0),
    side_chain_compression_attack_time(name + "CAT", 0.001, 3.0, 0.02),
    side_chain_compression_release_time(name + "CRL", 0.001, 3.0, 0.20),
    side_chain_compression_ratio(name + "CR", 1.0, 120.0, NO_OP_RATIO),
    gain(name + "G", 0.0, 1.0, BYPASS_GAIN),
    target_gain(BYPASS_GAIN),
    previous_action(Action::BYPASS_OR_RELEASE)
{
    this->register_child(side_chain_compression_threshold);
    this->register_child(side_chain_compression_attack_time);
    this->register_child(side_chain_compression_release_time);
    this->register_child(side_chain_compression_ratio);
    this->register_child(gain);
}


template<class InputSignalProducerClass, CompressionCurve curve>
Sample const* const* SideChainCompressableEffect<InputSignalProducerClass, curve>::initialize_rendering(
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
        fast_bypass();

        return buffer;
    }

    copy_input(sample_count);

    Number const ratio_value = side_chain_compression_ratio.get_value();

    is_bypassing = this->is_dry || Math::is_close(ratio_value, NO_OP_RATIO);

    if (is_bypassing) {
        fast_bypass();
    } else {
        Number const threshold_db = side_chain_compression_threshold.get_value();

        Sample peak;
        Integer peak_index;

        SignalProducer::find_peak(
            this->input_buffer, this->channels, sample_count, peak, peak_index
        );
        peak_tracker.update(peak, peak_index, sample_count, this->sampling_period);
        peak = peak_tracker.get_peak();

        Number const diff_db = Math::linear_to_db(peak) - threshold_db;

        if (diff_db > 0.0) {
            compress(peak, threshold_db, diff_db, ratio_value);
        } else if (previous_action == Action::COMPRESS) {
            release();
        } else if (Math::is_close(gain.get_value(), BYPASS_GAIN)) {
            fast_bypass();
        }
    }

    gain_buffer = FloatParamS::produce_if_not_constant(gain, round, sample_count);

    return NULL;
}


template<class InputSignalProducerClass, CompressionCurve curve>
void SideChainCompressableEffect<InputSignalProducerClass, curve>::fast_bypass() noexcept {
    gain.cancel_events_at(0.0);
    gain.set_value(BYPASS_GAIN);
    target_gain = BYPASS_GAIN;
    previous_action = Action::BYPASS_OR_RELEASE;
    is_bypassing = true;
}


template<class InputSignalProducerClass, CompressionCurve curve>
void SideChainCompressableEffect<InputSignalProducerClass, curve>::copy_input(
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


template<class InputSignalProducerClass, CompressionCurve curve>
void SideChainCompressableEffect<InputSignalProducerClass, curve>::compress(
        Sample const peak,
        Number const threshold_db,
        Number const diff_db,
        Number const ratio_value
) noexcept {
    Number const new_peak = Math::db_to_linear(threshold_db + diff_db / ratio_value);
    Number const new_target_gain = (
        peak > 0.000001 ? std::min(BYPASS_GAIN, new_peak / peak) : BYPASS_GAIN
    );

    if constexpr (curve == CompressionCurve::COMPRESSION_CURVE_SMOOTH) {
        if (!gain.is_ramping() || !Math::is_close(target_gain, new_target_gain, 0.01)) {
            schedule_gain_ramp(new_target_gain, side_chain_compression_attack_time);
        }
    } else {
        if (Math::is_close(gain.get_value(), new_target_gain, 0.005)) {
            gain.cancel_events_at(0.0);
            target_gain = new_target_gain;
        } else {
            schedule_gain_ramp(new_target_gain, side_chain_compression_attack_time);
        }
    }

    previous_action = Action::COMPRESS;
}


template<class InputSignalProducerClass, CompressionCurve curve>
void SideChainCompressableEffect<InputSignalProducerClass, curve>::schedule_gain_ramp(
        Number const target_gain,
        FloatParamB const& time_param
) noexcept {
    gain.cancel_events_at(0.0);

    if constexpr (curve == CompressionCurve::COMPRESSION_CURVE_SMOOTH) {
        gain.schedule_curved_ramp(
            (Seconds)time_param.get_value(),
            target_gain,
            Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SMOOTH
        );
    } else {
        gain.schedule_linear_ramp((Seconds)time_param.get_value(), target_gain);
    }

    this->target_gain = target_gain;
}


template<class InputSignalProducerClass, CompressionCurve curve>
void SideChainCompressableEffect<InputSignalProducerClass, curve>::release() noexcept
{
    schedule_gain_ramp(BYPASS_GAIN, side_chain_compression_release_time);
    previous_action = Action::BYPASS_OR_RELEASE;
}


template<class InputSignalProducerClass, CompressionCurve curve>
void SideChainCompressableEffect<InputSignalProducerClass, curve>::render(
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


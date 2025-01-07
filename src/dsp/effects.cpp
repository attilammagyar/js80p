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

#ifndef JS80P__DSP__EFFECTS_CPP
#define JS80P__DSP__EFFECTS_CPP

#include "dsp/effects.hpp"


namespace JS80P { namespace Effects
{

template<class InputSignalProducerClass>
Effects<InputSignalProducerClass>::Effects(
        std::string const& name,
        InputSignalProducerClass& input,
        BiquadFilterSharedBuffers& echo_filter_shared_buffers,
        BiquadFilterSharedBuffers& reverb_filter_shared_buffers
) : Filter< Volume3<InputSignalProducerClass> >(
        volume_3,
        24 + (Integer)TapeParams::SIGNAL_PRODUCERS,
        input.get_channels()
    ),
    volume_1_gain(name + "V1V", 0.0, 2.0, 1.0),
    volume_2_gain(name + "V2V", 0.0, 1.0, 1.0),
    volume_3_gain(name + "V3V", 0.0, 1.0, 1.0),
    /*
    The distortion type used to be fixed, and the two effects were called
    Overdrive and Distortion. Level parameter names are kept for
    backward-compatibility, and new param names use the old scheme for
    consistency.
    */
    distortion_1_type(name + "OT", Distortion::TYPE_TANH_3),
    distortion_2_type(name + "DT", Distortion::TYPE_TANH_10),
    tape_at_end(name + "TEND", ToggleParam::OFF),
    tape_params(name + "T", tape_at_end),
    volume_1(input, volume_1_gain),
    distortion_1(name + "O", distortion_1_type, volume_1, &volume_1),
    distortion_2(name + "D", distortion_2_type, distortion_1, &volume_1),
    filter_1_type(name + "F1TYP"),
    filter_2_type(name + "F2TYP"),
    filter_1_freq_log_scale(name + "F1LOG", ToggleParam::OFF),
    filter_1_q_log_scale(name + "F1QLG", ToggleParam::OFF),
    filter_2_freq_log_scale(name + "F2LOG", ToggleParam::OFF),
    filter_2_q_log_scale(name + "F2QLG", ToggleParam::OFF),
    filter_1(
        name + "F1",
        distortion_2,
        filter_1_type,
        filter_1_freq_log_scale,
        filter_1_q_log_scale,
        &volume_1
    ),
    filter_2(
        name + "F2",
        filter_1,
        filter_2_type,
        filter_2_freq_log_scale,
        filter_2_q_log_scale,
        &volume_1
    ),
    volume_2(filter_2, volume_2_gain),
    tape_1(name + "T1", tape_params, volume_2),
    chorus(name + "C", tape_1),
    echo(name + "E", chorus, echo_filter_shared_buffers),
    reverb(name + "R", echo, reverb_filter_shared_buffers),
    tape_2(name + "T2", tape_params, reverb),
    volume_3(tape_2, volume_3_gain)
{
    size_t i = 0;
    SignalProducer* sp = tape_params.get_signal_producer(i++);

    while (sp != NULL) {
        this->register_child(*sp);
        sp = tape_params.get_signal_producer(i++);
    }

    this->register_child(volume_1_gain);
    this->register_child(volume_2_gain);
    this->register_child(volume_3_gain);
    this->register_child(distortion_1_type);
    this->register_child(distortion_2_type);
    this->register_child(tape_at_end);
    this->register_child(volume_1);
    this->register_child(distortion_1);
    this->register_child(distortion_2);
    this->register_child(filter_1_type);
    this->register_child(filter_2_type);
    this->register_child(filter_1_freq_log_scale);
    this->register_child(filter_1_q_log_scale);
    this->register_child(filter_2_freq_log_scale);
    this->register_child(filter_2_q_log_scale);
    this->register_child(filter_1);
    this->register_child(filter_2);
    this->register_child(volume_2);
    this->register_child(tape_1);
    this->register_child(chorus);
    this->register_child(echo);
    this->register_child(reverb);
    this->register_child(tape_2);
    this->register_child(volume_3);
}

} }

#endif

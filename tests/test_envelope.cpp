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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "synth/envelope.cpp"
#include "synth/midi_controller.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"


using namespace JS80P;


TEST(an_envelope_is_a_collection_of_float_params, {
    Envelope envelope("E1");

    assert_eq("E1AMT", envelope.amount.get_name());
    assert_eq("E1INI", envelope.initial_value.get_name());
    assert_eq("E1DEL", envelope.delay_time.get_name());
    assert_eq("E1ATK", envelope.attack_time.get_name());
    assert_eq("E1PK", envelope.peak_value.get_name());
    assert_eq("E1HLD", envelope.hold_time.get_name());
    assert_eq("E1DEC", envelope.decay_time.get_name());
    assert_eq("E1SUS", envelope.sustain_value.get_name());
    assert_eq("E1REL", envelope.release_time.get_name());
    assert_eq("E1FIN", envelope.final_value.get_name());
})


TEST(when_basic_properties_change_then_all_params_are_notified, {
    constexpr Integer block_size = 12345;
    constexpr Frequency sample_rate = 48000.0;
    Envelope envelope("E1");

    envelope.set_block_size(block_size);
    envelope.set_sample_rate(sample_rate);

    assert_eq(sample_rate, envelope.amount.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.initial_value.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.delay_time.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.attack_time.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.peak_value.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.hold_time.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.decay_time.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.sustain_value.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.release_time.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(sample_rate, envelope.final_value.get_sample_rate(), DOUBLE_DELTA);

    assert_eq((int)block_size, (int)envelope.amount.get_block_size());
    assert_eq((int)block_size, (int)envelope.initial_value.get_block_size());
    assert_eq((int)block_size, (int)envelope.delay_time.get_block_size());
    assert_eq((int)block_size, (int)envelope.attack_time.get_block_size());
    assert_eq((int)block_size, (int)envelope.peak_value.get_block_size());
    assert_eq((int)block_size, (int)envelope.hold_time.get_block_size());
    assert_eq((int)block_size, (int)envelope.decay_time.get_block_size());
    assert_eq((int)block_size, (int)envelope.sustain_value.get_block_size());
    assert_eq((int)block_size, (int)envelope.release_time.get_block_size());
    assert_eq((int)block_size, (int)envelope.final_value.get_block_size());
})

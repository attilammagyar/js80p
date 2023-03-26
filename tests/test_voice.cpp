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

#include <cmath>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "synth/biquad_filter.cpp"
#include "synth/delay.cpp"
#include "synth/envelope.cpp"
#include "synth/filter.cpp"
#include "synth/flexible_controller.cpp"
#include "synth/lfo.cpp"
#include "synth/math.cpp"
#include "synth/midi_controller.cpp"
#include "synth/oscillator.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"
#include "synth/voice.cpp"
#include "synth/wavefolder.cpp"
#include "synth/wavetable.cpp"


using namespace JS80P;


typedef Voice<SignalProducer> SimpleVoice;

constexpr Integer NOTE_MAX = 5;
constexpr Number FREQUENCIES[NOTE_MAX + 1] = {
    100.0, 200.0, 400.0, 800.0, 1600.0
};


TEST(rendering_is_independent_of_chunk_size, {
    constexpr Frequency sample_rate = 44100.0;

    SimpleVoice::Params params("");
    SimpleVoice voice_1(FREQUENCIES, NOTE_MAX, params);
    SimpleVoice voice_2(FREQUENCIES, NOTE_MAX, params);

    params.waveform.set_value(SimpleOscillator::SINE);
    params.amplitude.set_value(1.0);
    params.volume.set_value(1.0);
    params.width.set_value(0.0);

    voice_1.set_sample_rate(sample_rate);
    voice_2.set_sample_rate(sample_rate);

    voice_1.note_on(0.12, 1, 0.5, 1);
    voice_1.note_off(0.12 + 1.0, 1, 0.5);

    voice_2.note_on(0.12, 1, 0.5, 1);
    voice_2.note_off(0.12 + 1.0, 1, 0.5);

    assert_rendering_is_independent_from_chunk_size<SimpleVoice>(
        voice_1, voice_2
    );
})


TEST(portamento, {
    constexpr Frequency sample_rate = 44100.0;
    constexpr Integer block_size = 8196;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = block_size * rounds;
    constexpr Seconds portamento_length = 1.0;
    constexpr Number portamento_depth = -200.0;
    constexpr Seconds note_start = ((Seconds)(block_size - 2)) / sample_rate;

    Buffer expected_output(sample_count, SimpleVoice::CHANNELS);
    Buffer actual_output(sample_count, SimpleVoice::CHANNELS);
    SumOfSines expected(
        std::sin(Math::PI / 4.0),
        200.0,
        0.0,
        0.0,
        0.0,
        0.0,
        SimpleVoice::CHANNELS,
        0.00009
    );
    SimpleVoice::Params params("");
    Envelope envelope("");
    SimpleVoice voice(FREQUENCIES, NOTE_MAX, params);

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    params.portamento_length.set_sample_rate(sample_rate);
    params.portamento_length.set_block_size(block_size);

    params.portamento_depth.set_sample_rate(sample_rate);
    params.portamento_depth.set_block_size(block_size);

    params.fine_detune.set_sample_rate(sample_rate);
    params.fine_detune.set_block_size(block_size);
    params.fine_detune.set_envelope(&envelope);

    envelope.amount.set_block_size(block_size);
    envelope.initial_value.set_block_size(block_size);
    envelope.delay_time.set_block_size(block_size);
    envelope.attack_time.set_block_size(block_size);
    envelope.peak_value.set_block_size(block_size);
    envelope.hold_time.set_block_size(block_size);
    envelope.decay_time.set_block_size(block_size);
    envelope.sustain_value.set_block_size(block_size);
    envelope.release_time.set_block_size(block_size);
    envelope.final_value.set_block_size(block_size);

    envelope.amount.set_sample_rate(sample_rate);
    envelope.initial_value.set_sample_rate(sample_rate);
    envelope.delay_time.set_sample_rate(sample_rate);
    envelope.attack_time.set_sample_rate(sample_rate);
    envelope.peak_value.set_sample_rate(sample_rate);
    envelope.hold_time.set_sample_rate(sample_rate);
    envelope.decay_time.set_sample_rate(sample_rate);
    envelope.sustain_value.set_sample_rate(sample_rate);
    envelope.release_time.set_sample_rate(sample_rate);
    envelope.final_value.set_sample_rate(sample_rate);

    envelope.attack_time.set_value(portamento_length);
    envelope.initial_value.set_value(
        params.fine_detune.value_to_ratio(-portamento_depth)
    );
    envelope.peak_value.set_value(params.fine_detune.value_to_ratio(0.0));

    params.waveform.set_value(SimpleOscillator::SINE);
    params.amplitude.set_value(1.0);
    params.volume.set_value(1.0);
    params.width.set_value(0.0);
    params.portamento_length.set_value(portamento_length);
    params.portamento_depth.set_value(portamento_depth);

    voice.set_sample_rate(sample_rate);
    voice.set_block_size(block_size);

    voice.note_on(note_start, 1, 1.0, 1);

    SignalProducer::produce<SimpleVoice>(&voice, 999999, block_size);

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    render_rounds<SimpleVoice>(voice, actual_output, rounds);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], block_size, 0.03
    );
})

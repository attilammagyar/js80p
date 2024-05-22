/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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
#include <functional>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/biquad_filter.cpp"
#include "dsp/delay.cpp"
#include "dsp/distortion.cpp"
#include "dsp/envelope.cpp"
#include "dsp/filter.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavefolder.cpp"
#include "dsp/wavetable.cpp"

#include "voice.cpp"


using namespace JS80P;


typedef Voice<SignalProducer> SimpleVoice;

constexpr FrequencyTable FREQUENCIES = {
    {100.0, 200.0, 400.0, 800.0, 1600.0},
    {300.0, 600.0, 1200.0, 2400.0, 4800.0},
};

constexpr PerChannelFrequencyTable PER_CHANNEL_FREQUENCIES = {
    {100.0, 200.0, 400.0, 800.0, 1600.0},
    {100.0, 200.0, 400.0, 800.0, 1600.0},
    {75.0, 150.0, 300.0, 600.0, 1200.0},
};


TEST(turning_off_with_wrong_note_or_note_id_keeps_the_voice_on, {
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    voice.note_on(0.12, 42, 1, 0, 0.5, 1, true);

    voice.note_off(0.12 + 1.0, 123, 1, 0.5);
    assert_false(voice.is_off_after(2.0));

    voice.note_off(0.12 + 1.0, 42, 2, 0.5);
    assert_false(voice.is_off_after(2.0));

    voice.note_off(0.12 + 1.0, 42, 1, 0.5);
    assert_true(voice.is_off_after(2.0));
})


TEST(rendering_is_independent_of_chunk_size, {
    constexpr Frequency sample_rate = 44100.0;

    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    SimpleVoice::Params params("");
    SimpleVoice voice_1(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );
    SimpleVoice voice_2(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    params.waveform.set_value(SimpleOscillator::SINE);
    params.amplitude.set_value(1.0);
    params.volume.set_value(1.0);
    params.width.set_value(0.0);

    voice_1.set_sample_rate(sample_rate);
    voice_2.set_sample_rate(sample_rate);

    voice_1.note_on(0.12, 42, 1, 0, 0.5, 1, true);
    voice_1.note_off(0.12 + 1.0, 42, 1, 0.5);

    voice_2.note_on(0.12, 123, 1, 0, 0.5, 1, true);
    voice_2.note_off(0.12 + 1.0, 123, 1, 0.5);

    assert_rendering_is_independent_from_chunk_size<SimpleVoice>(
        voice_1, voice_2
    );
})


void set_up_voice(
        SimpleVoice& voice,
        SimpleVoice::Params& params,
        Integer const block_size,
        Frequency const sample_rate
) {
    voice.set_sample_rate(sample_rate);
    voice.set_block_size(block_size);

    params.tuning.set_block_size(block_size);
    params.oscillator_inaccuracy.set_block_size(block_size);
    params.oscillator_instability.set_block_size(block_size);

    params.waveform.set_block_size(block_size);
    params.amplitude.set_block_size(block_size);
    params.velocity_sensitivity.set_block_size(block_size);
    params.folding.set_block_size(block_size);
    params.portamento_length.set_block_size(block_size);
    params.portamento_depth.set_block_size(block_size);
    params.detune.set_block_size(block_size);
    params.fine_detune.set_block_size(block_size);
    params.width.set_block_size(block_size);
    params.panning.set_block_size(block_size);
    params.volume.set_block_size(block_size);

    params.harmonic_0.set_block_size(block_size);
    params.harmonic_1.set_block_size(block_size);
    params.harmonic_2.set_block_size(block_size);
    params.harmonic_3.set_block_size(block_size);
    params.harmonic_4.set_block_size(block_size);
    params.harmonic_5.set_block_size(block_size);
    params.harmonic_6.set_block_size(block_size);
    params.harmonic_7.set_block_size(block_size);
    params.harmonic_8.set_block_size(block_size);
    params.harmonic_9.set_block_size(block_size);

    params.filter_1_type.set_block_size(block_size);
    params.filter_1_freq_log_scale.set_block_size(block_size);
    params.filter_1_q_log_scale.set_block_size(block_size);
    params.filter_1_frequency.set_block_size(block_size);
    params.filter_1_q.set_block_size(block_size);
    params.filter_1_gain.set_block_size(block_size);
    params.filter_1_freq_inaccuracy.set_block_size(block_size);
    params.filter_1_q_inaccuracy.set_block_size(block_size);

    params.filter_2_type.set_block_size(block_size);
    params.filter_2_freq_log_scale.set_block_size(block_size);
    params.filter_2_q_log_scale.set_block_size(block_size);
    params.filter_2_frequency.set_block_size(block_size);
    params.filter_2_q.set_block_size(block_size);
    params.filter_2_gain.set_block_size(block_size);
    params.filter_2_freq_inaccuracy.set_block_size(block_size);
    params.filter_2_q_inaccuracy.set_block_size(block_size);

    params.subharmonic_amplitude.set_block_size(block_size);

    params.amplitude.set_value(1.0);
    params.volume.set_value(1.0);
    params.width.set_value(0.0);

    params.portamento_length.set_sample_rate(sample_rate);
    params.portamento_length.set_block_size(block_size);

    params.portamento_depth.set_sample_rate(sample_rate);
    params.portamento_depth.set_block_size(block_size);

    params.fine_detune.set_sample_rate(sample_rate);
    params.fine_detune.set_block_size(block_size);
}


void set_up_envelope(
        Envelope& envelope,
        Integer const block_size,
        Frequency const sample_rate
) {
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
}


TEST(portamento, {
    constexpr Frequency sample_rate = 44100.0;
    constexpr Integer block_size = 8192;
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
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    Envelope envelope("");
    Envelope* envelopes[Constants::ENVELOPES] = {
        &envelope, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL,
    };
    SimpleVoice::Params params("", envelopes);
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    set_up_voice(voice, params, block_size, sample_rate);
    set_up_envelope(envelope, block_size, sample_rate);

    params.fine_detune.set_envelope(&envelope);

    envelope.attack_time.set_value(portamento_length);
    envelope.initial_value.set_value(
        params.fine_detune.value_to_ratio(-portamento_depth)
    );
    envelope.peak_value.set_value(params.fine_detune.value_to_ratio(0.0));

    params.waveform.set_value(SimpleOscillator::SINE);
    params.portamento_length.set_value(portamento_length);
    params.portamento_depth.set_value(portamento_depth);

    voice.note_on(note_start, 123, 1, 0, 1.0, 1, true);

    SignalProducer::produce<SimpleVoice>(voice, 999999, block_size);

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    render_rounds<SimpleVoice>(voice, actual_output, rounds);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], sample_count, 0.03
    );
})


void test_turning_off_voice(std::function<void (SimpleVoice&)> reset)
{
    constexpr Frequency sample_rate = 44100.0;
    constexpr Integer block_size = 8192;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = block_size * rounds;

    Buffer expected_output(sample_count, SimpleVoice::CHANNELS);
    Buffer actual_output(sample_count, SimpleVoice::CHANNELS);
    Constant expected(0.0, SimpleVoice::CHANNELS);
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    params.waveform.set_value(SimpleOscillator::SINE);
    params.amplitude.set_value(1.0);
    params.volume.set_value(1.0);

    voice.set_sample_rate(sample_rate);
    voice.set_block_size(block_size);

    voice.note_on(0.0, 123, 2, 3, 1.0, 1, true);

    assert_eq(123, (int)voice.get_note_id());
    assert_eq(2, (int)voice.get_note());
    assert_eq(3, (int)voice.get_channel());

    SignalProducer::produce<SimpleVoice>(voice, 999999, block_size);

    reset(voice);

    render_rounds<Constant>(expected, expected_output, rounds);
    render_rounds<SimpleVoice>(voice, actual_output, rounds);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], sample_count, DOUBLE_DELTA
    );
    assert_false(voice.is_on());
    assert_true(voice.is_off_after(0.0));
}


TEST(voice_can_be_turned_off_immediately, {
    test_turning_off_voice([](SimpleVoice& voice) { voice.reset(); });
    test_turning_off_voice([](SimpleVoice& voice) { voice.cancel_note(); });
})


enum NoteDecay {
    NEVER = 0,
    AT_BEGINNING_OF_SUSTAIN = 1,
    BEFORE_SUSTAIN = 2,
};


void test_decay_before_note_off(
        SimpleVoice& voice,
        Envelope* const envelope,
        NoteDecay const expected_decay,
        char const* const test_name
) {
    constexpr Seconds delay_time = 0.01;
    constexpr Seconds attack_time = 0.01;
    constexpr Seconds hold_time = 0.1;
    constexpr Seconds decay_time = 0.01;

    Seconds const note_start = (
        (Number)(voice.get_block_size() * 2) / voice.get_sample_rate()
    );
    Seconds const sustain_start = (
        note_start + delay_time + attack_time + hold_time + decay_time
    );

    Integer rendered_samples = 0;
    Integer round = 0;

    Integer const sustain_start_samples = (
        (Integer)std::ceil(sustain_start * voice.get_sample_rate())
    );

    if (envelope != NULL) {
        envelope->update_mode.set_value(Envelope::UPDATE_MODE_STATIC);
        envelope->amount.set_value(1.0);
        envelope->initial_value.set_value(0.0);
        envelope->delay_time.set_value(delay_time);
        envelope->attack_time.set_value(attack_time);
        envelope->hold_time.set_value(hold_time);
        envelope->decay_time.set_value(decay_time);
        envelope->release_time.set_value(envelope->release_time.get_max_value());
        envelope->final_value.set_value(0.0);
    }

    voice.reset();
    voice.note_on(note_start, 42, 1, 0, 1.0, 1, true);

    while (rendered_samples < sustain_start_samples) {
        assert_eq(
            expected_decay == NoteDecay::BEFORE_SUSTAIN,
            voice.has_decayed_before_note_off(),
            "test=\"%s\", rendered_samples=%d, round=%d",
            test_name,
            (int)rendered_samples,
            (int)round
        );

        SignalProducer::produce<SimpleVoice>(voice, round);

        rendered_samples += voice.get_block_size();
        ++round;
    }

    assert_eq(
        expected_decay != NoteDecay::NEVER,
        voice.has_decayed_before_note_off(),
        "test=\"%s\"",
        test_name
    );
}


TEST(can_tell_if_note_decayed_before_note_off, {
    Envelope envelope("E");
    Envelope* envelopes[Constants::ENVELOPES] = {
        &envelope, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL,
    };
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    MidiController midi_controller;
    Macro macro("M");
    LFO lfo("L");

    SimpleVoice::Params params_with_envelope("VE", envelopes);
    SimpleVoice::Params params_with_zero_amplitudes("VAS", envelopes);
    SimpleVoice::Params params_with_zero_volume("VV", envelopes);
    SimpleVoice::Params params_with_zero_subharmonic_amplitude("VS", envelopes);
    SimpleVoice::Params params_with_zero_amplitude("VA", envelopes);
    SimpleVoice::Params params_with_midi_controller("VMI", envelopes);
    SimpleVoice::Params params_with_macro("VMA", envelopes);
    SimpleVoice::Params params_with_lfo("VL", envelopes);

    SimpleVoice voice_with_envelope(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_envelope
    );

    SimpleVoice voice_with_zero_amplitudes(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_zero_amplitudes
    );

    SimpleVoice voice_with_zero_volume(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_zero_volume
    );

    SimpleVoice voice_with_zero_amplitude(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_zero_amplitude
    );

    SimpleVoice voice_with_zero_subharmonic_amplitude(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_zero_subharmonic_amplitude
    );

    SimpleVoice voice_with_midi_controller(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_midi_controller
    );

    SimpleVoice voice_with_macro(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_macro
    );

    SimpleVoice voice_with_lfo(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_with_lfo
    );

    params_with_envelope.amplitude.set_value(1.0);
    params_with_envelope.volume.set_value(1.0);

    params_with_envelope.amplitude.set_envelope(&envelope);
    params_with_envelope.subharmonic_amplitude.set_envelope(&envelope);

    params_with_zero_amplitudes.amplitude.set_value(0.0);
    params_with_zero_amplitudes.subharmonic_amplitude.set_value(0.0);
    params_with_zero_amplitudes.volume.set_value(1.0);

    params_with_zero_volume.amplitude.set_value(1.0);
    params_with_zero_volume.subharmonic_amplitude.set_value(1.0);
    params_with_zero_volume.volume.set_value(0.0);

    params_with_zero_subharmonic_amplitude.amplitude.set_value(1.0);
    params_with_zero_subharmonic_amplitude.subharmonic_amplitude.set_value(0.0);
    params_with_zero_subharmonic_amplitude.volume.set_value(1.0);

    params_with_zero_amplitude.amplitude.set_value(0.0);
    params_with_zero_amplitude.subharmonic_amplitude.set_value(1.0);
    params_with_zero_amplitude.volume.set_value(1.0);

    midi_controller.change(0.0, 0.0);
    params_with_midi_controller.volume.set_midi_controller(&midi_controller);

    macro.input.set_value(0.0);
    params_with_macro.volume.set_macro(&macro);

    params_with_lfo.volume.set_lfo(&lfo);

    envelope.peak_value.set_value(1.0);
    envelope.sustain_value.set_value(0.0);
    test_decay_before_note_off(
        voice_with_envelope,
        &envelope,
        NoteDecay::AT_BEGINNING_OF_SUSTAIN,
        "when sustain value is 0.0, then the note should decay when starting sustaining"
    );

    envelope.peak_value.set_value(0.0);
    envelope.sustain_value.set_value(0.5);
    test_decay_before_note_off(
        voice_with_envelope,
        &envelope,
        NoteDecay::NEVER,
        "when sustain value is greater than 0.0, then the note should never decay"
    );

    test_decay_before_note_off(
        voice_with_zero_amplitudes,
        NULL,
        NoteDecay::BEFORE_SUSTAIN,
        "when amplitudes are constant 0.0, then the note should decay instantly"
    );

    test_decay_before_note_off(
        voice_with_zero_volume,
        NULL,
        NoteDecay::BEFORE_SUSTAIN,
        "when volume is constant 0.0, then the note should decay instantly"
    );

    test_decay_before_note_off(
        voice_with_zero_amplitude,
        NULL,
        NoteDecay::NEVER,
        "when only the amplitude is 0.0, then the note should never decay"

    );

    test_decay_before_note_off(
        voice_with_zero_subharmonic_amplitude,
        NULL,
        NoteDecay::NEVER,
        "when only the subharmonic amplitude is 0.0, then the note should never decay"
    );

    test_decay_before_note_off(
        voice_with_midi_controller,
        NULL,
        NoteDecay::NEVER,
        "when a voice is controlled by MIDI CC then it should never decay"
    );

    test_decay_before_note_off(
        voice_with_macro,
        NULL,
        NoteDecay::NEVER,
        "when a voice is controlled by a macro then it should never decay"
    );

    test_decay_before_note_off(
        voice_with_lfo,
        NULL,
        NoteDecay::NEVER,
        "when a voice is controlled by an LFO then it should never decay"
    );
})


TEST(can_glide_smoothly_to_a_new_note, {
    constexpr Frequency sample_rate = 44100.0;
    constexpr Integer block_size = 8192;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = block_size * rounds;
    constexpr Seconds note_start = 0.0;
    constexpr Seconds glide_start = 0.05;
    constexpr Seconds glide_duration = 0.05;

    Buffer expected_output(sample_count, SimpleVoice::CHANNELS);
    Buffer actual_output(sample_count, SimpleVoice::CHANNELS);
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    Envelope envelope("E");
    Envelope* envelopes[Constants::ENVELOPES] = {
        &envelope, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL,
    };
    SimpleVoice::Params params_ref("R", envelopes);
    SimpleVoice::Params params("P", envelopes);
    SimpleVoice reference(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params_ref
    );
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    set_up_voice(voice, params, block_size, sample_rate);
    set_up_voice(reference, params_ref, block_size, sample_rate);
    set_up_envelope(envelope, block_size, sample_rate);

    params_ref.volume.set_envelope(&envelope);

    envelope.amount.set_value(1.0);
    envelope.initial_value.set_value(0.5);
    envelope.delay_time.set_value(0.0);
    envelope.attack_time.set_value(glide_duration);
    envelope.peak_value.set_value(0.5);
    envelope.hold_time.set_value(0.0);
    envelope.decay_time.set_value(0.0);
    envelope.sustain_value.set_value(0.5);
    envelope.release_time.set_value(0.0);
    envelope.final_value.set_value(0.5);

    params.waveform.set_value(SimpleOscillator::SINE);
    params_ref.waveform.set_value(SimpleOscillator::SINE);

    params.portamento_length.set_value(glide_duration);
    params_ref.portamento_length.set_value(glide_duration);

    reference.note_on(note_start, 123, 0, 0, 1.0, 0, true);
    reference.note_off(glide_start, 123, 0, 1.0);

    envelope.peak_value.set_value(1.0);
    envelope.sustain_value.set_value(1.0);
    envelope.final_value.set_value(1.0);

    reference.note_on(glide_start, 42, 1, 0, 1.0, 0, true);

    voice.note_on(note_start, 123, 0, 0, 0.5, 0, true);
    voice.glide_to(glide_start, 42, 1, 0, 1.0, 123, true);

    render_rounds<SimpleVoice>(reference, expected_output, rounds);
    render_rounds<SimpleVoice>(voice, actual_output, rounds);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], sample_count, 0.03
    );
})


TEST(tuning_can_be_changed, {
    constexpr Frequency sample_rate = 44100.0;
    constexpr Integer block_size = 8192;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = block_size * rounds;

    Buffer expected_output(sample_count, SimpleVoice::CHANNELS);
    Buffer actual_output(sample_count, SimpleVoice::CHANNELS);
    SumOfSines expected(
        std::sin(Math::PI / 4.0),
        1200.0,
        0.0,
        0.0,
        0.0,
        0.0,
        SimpleVoice::CHANNELS
    );
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.2);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    set_up_voice(voice, params, block_size, sample_rate);

    params.tuning.set_value(SimpleVoice::TUNING_432HZ_12TET);
    params.oscillator_inaccuracy.set_value(1);
    params.oscillator_instability.set_value(1);
    voice.note_on(0.0, 123, 2, 0, 1.0, 2, true);

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    render_rounds<SimpleVoice>(voice, actual_output, rounds);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], sample_count, 0.02
    );
})


TEST(when_using_mts_esp_tuning_then_note_frequency_is_selected_based_on_the_channel, {
    constexpr Frequency sample_rate = 44100.0;
    constexpr Integer block_size = 8192;
    constexpr Integer rounds = 1;
    constexpr Integer sample_count = block_size * rounds;

    Buffer expected_output(sample_count, SimpleVoice::CHANNELS);
    Buffer actual_output(sample_count, SimpleVoice::CHANNELS);
    SumOfSines expected(
        std::sin(Math::PI / 4.0),
        150.0,
        0.0,
        0.0,
        0.0,
        0.0,
        SimpleVoice::CHANNELS
    );
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    set_up_voice(voice, params, block_size, sample_rate);

    params.tuning.set_value(SimpleVoice::TUNING_MTS_ESP_NOTE_ON);
    voice.note_on(0.0, 123, 1, 2, 1.0, 1, true);

    render_rounds<SumOfSines>(expected, expected_output, rounds);
    render_rounds<SimpleVoice>(voice, actual_output, rounds);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], sample_count, 0.001
    );
})


TEST(when_using_continuous_mts_esp_tuning_then_frequency_can_be_updated_before_each_round, {
    constexpr Frequency sample_rate = 30000.0;
    constexpr Integer block_size = 3000;
    constexpr Seconds portamento_length = 2.0 * ((Seconds)block_size / sample_rate);
    constexpr Number portamento_depth = -1200.0;
    constexpr Number tolerance = 0.001;
    constexpr Frequency orig_freq = 300.0;
    constexpr Frequency new_freq = 500.0;

    PerChannelFrequencyTable per_channel_frequencies = {
        {100.0, 200.0, 400.0, 800.0, 1600.0},
        {100.0, 200.0, 400.0, 800.0, 1600.0},
        {75.0, 150.0, orig_freq, 600.0, 1200.0},
    };

    Sample const* const* expected_output;
    Sample const* const* actual_output;
    SimpleOscillator::WaveformParam expected_waveform("WF");
    SimpleOscillator expected(expected_waveform);
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.5);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        per_channel_frequencies,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);
    expected.start(0.0);

    expected_waveform.set_sample_rate(sample_rate);
    expected_waveform.set_block_size(block_size);
    expected_waveform.set_value(SimpleOscillator::SINE);

    expected.amplitude.set_value(std::sin(Math::PI / 4.0));
    expected.frequency.set_value(Math::detune(orig_freq, portamento_depth));
    expected.frequency.schedule_linear_ramp(
        portamento_length / 2.0,
        (orig_freq + Math::detune(orig_freq, portamento_depth)) / 2.0
    );
    expected.frequency.schedule_linear_ramp(portamento_length / 2.0, new_freq);

    set_up_voice(voice, params, block_size, sample_rate);

    params.portamento_length.set_value(portamento_length);
    params.portamento_depth.set_value(portamento_depth);

    params.tuning.set_value(SimpleVoice::TUNING_MTS_ESP_CONTINUOUS);
    voice.note_on(0.0, 123, 2, 2, 1.0, 2, true);
    voice.update_note_frequency_for_continuous_mts_esp<true, true>(1);

    expected_output = SignalProducer::produce<SimpleOscillator>(expected, 1);
    actual_output = SignalProducer::produce<SimpleVoice>(voice, 1);

    assert_close(
        expected_output[0],
        actual_output[0],
        block_size,
        tolerance,
        "round=1, channel=0"
    );
    assert_close(
        expected_output[0],
        actual_output[1],
        block_size,
        tolerance,
        "round=1, channel=1"
    );

    per_channel_frequencies[2][2] = new_freq;

    voice.update_note_frequency_for_continuous_mts_esp<true, true>(2);

    expected_output = SignalProducer::produce<SimpleOscillator>(expected, 2);
    actual_output = SignalProducer::produce<SimpleVoice>(voice, 2);

    assert_close(
        expected_output[0],
        actual_output[0],
        block_size,
        tolerance,
        "round=2, channel=0"
    );
    assert_close(
        expected_output[0],
        actual_output[1],
        block_size,
        tolerance,
        "round=2, channel=1"
    );
})


TEST(when_synced_and_drifting_then_synced_inaccuracy_is_updated_once_per_round, {
    OscillatorInaccuracy synced_oscillator_inaccuracy(0.123);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    params.oscillator_inaccuracy.set_value(1);
    params.oscillator_instability.set_value(1);
    voice.note_on(0.0, 42, 1, 0, 0.5, 1, true);

    voice.update_unstable_note_frequency<true>(1);
    SignalProducer::produce<SimpleVoice>(voice, 1);
    Number const inaccuracy_in_round_1 = synced_oscillator_inaccuracy.get_inaccuracy();

    voice.update_unstable_note_frequency<true>(2);
    SignalProducer::produce<SimpleVoice>(voice, 2);
    Number const inaccuracy_in_round_2 = synced_oscillator_inaccuracy.get_inaccuracy();

    synced_oscillator_inaccuracy.update(2);
    assert_lt(0.01, std::fabs(inaccuracy_in_round_1 - inaccuracy_in_round_2));
    assert_eq(inaccuracy_in_round_2, synced_oscillator_inaccuracy.get_inaccuracy());

    synced_oscillator_inaccuracy.update(2);
    assert_eq(inaccuracy_in_round_2, synced_oscillator_inaccuracy.get_inaccuracy());
})


TEST(when_voice_is_reset_then_synced_inaccuracy_is_also_reset, {
    constexpr Number seed = 0.123;

    OscillatorInaccuracy synced_oscillator_inaccuracy(seed);
    SimpleVoice::Params params("");
    SimpleVoice voice(
        FREQUENCIES,
        PER_CHANNEL_FREQUENCIES,
        synced_oscillator_inaccuracy,
        0.0,
        params
    );

    params.oscillator_inaccuracy.set_value(1);
    params.oscillator_instability.set_value(1);
    voice.note_on(0.12, 42, 1, 0, 0.5, 1, true);

    voice.update_unstable_note_frequency<true>(1);
    SignalProducer::produce<SimpleVoice>(voice, 1);

    voice.update_unstable_note_frequency<true>(2);
    SignalProducer::produce<SimpleVoice>(voice, 2);

    voice.reset();

    assert_eq(seed, synced_oscillator_inaccuracy.get_inaccuracy(), DOUBLE_DELTA);
})

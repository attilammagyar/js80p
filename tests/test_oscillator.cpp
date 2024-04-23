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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/envelope.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


constexpr Frequency SAMPLE_RATE = 22050.0;
constexpr Frequency NYQUIST_FREQUENCY = SAMPLE_RATE / 2.0;
constexpr Seconds ALMOST_IMMEDIATELY = 0.15 / (Seconds)SAMPLE_RATE;

SimpleOscillator::WaveformParam wavetable_cache_waveform("WAV");
SimpleOscillator wavetable_cache(wavetable_cache_waveform);

typedef Oscillator<SignalProducer, true> SimpleLFO;


class NonBandLimitedReferenceWaveform
{
    public:
        explicit NonBandLimitedReferenceWaveform(Frequency const frequency)
            : frequency(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const = 0;

    protected:
        Frequency const frequency;
};


class ReferenceZero : public NonBandLimitedReferenceWaveform
{
    public:
        explicit ReferenceZero(Frequency const frequency)
            : NonBandLimitedReferenceWaveform(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            return 0.0;
        }
};


class ReferenceSine : public NonBandLimitedReferenceWaveform
{
    public:
        explicit ReferenceSine(
                Frequency const frequency_1,
                Frequency const frequency_2 = 0.0,
                Seconds const chirp_duration = 0.0,
                Number const offset = 0.0
        )
            : NonBandLimitedReferenceWaveform(frequency_1),
            chirp_rate(
                chirp_duration > 0.0
                    ? (
                        (Number)(frequency_2 - frequency_1)
                        / (Number)chirp_duration
                    ) : 0.0
            ),
            offset(offset)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            return offset + (Sample)std::sin(
                Math::PI_DOUBLE * (
                    (chirp_rate / 2.0) * (Number)(time * time)
                    + (Number)frequency * (Number)time
                )
            );
        }

    private:
        Number const chirp_rate;
        Number const offset;
};


class ReferenceSawtooth : public NonBandLimitedReferenceWaveform
{
    public:
        explicit ReferenceSawtooth(Frequency const frequency)
            : NonBandLimitedReferenceWaveform(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            Sample const period = 1.0 / (Sample)frequency;
            Sample const d = (Sample)time / period;

            return 2.0 * (d - std::floor(0.5 + d));
        }
};


class ReferenceInverseSawtooth : public ReferenceSawtooth
{
    public:
        explicit ReferenceInverseSawtooth(Frequency const frequency)
            : ReferenceSawtooth(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            return -ReferenceSawtooth::generate_sample(time);
        }
};


class ReferenceTriangle : public NonBandLimitedReferenceWaveform
{
    public:
        explicit ReferenceTriangle(Frequency const frequency)
            : NonBandLimitedReferenceWaveform(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            Sample const period = 1.0 / (Sample)frequency;
            Sample const half_period = period * 0.5;

            /* Shifting by a quarter period so that the wave starts with 0.0 */
            Sample const phase = std::fmod((Sample)time + period * 0.25, period);

            return 2.0 * (
                phase < half_period
                    ? phase / half_period
                    : 1.0 - (phase - half_period) / half_period
            ) - 1.0;
        }
};


class ReferenceSquare : public NonBandLimitedReferenceWaveform
{
    public:
        explicit ReferenceSquare(Frequency const frequency)
            : NonBandLimitedReferenceWaveform(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            Sample const period = 1.0 / (Sample)frequency;
            Sample const half_period = period * 0.5;

            return std::fmod((Sample)time, period) < half_period ? 0.8 : -0.8;
        }
};


class ReferenceSawtoothWithDisappearingPartial : public NonBandLimitedReferenceWaveform
{
    public:
        explicit ReferenceSawtoothWithDisappearingPartial(Frequency const frequency)
            : NonBandLimitedReferenceWaveform(frequency)
        {
        }

        virtual Sample generate_sample(Seconds const time) const override
        {
            Sample const fundamental = std::sin(
                (Sample)Math::PI_DOUBLE * (Sample)frequency * (Sample)time
            );
            Sample const partial = std::sin(
                (Sample)Math::PI_DOUBLE * 2.0 * (Sample)frequency * (Sample)time
            );
            Sample const fundamental_amplitude = 2.0 / (Sample)Math::PI;
            Sample const partial_amplitude = 0.5 * (-1.0 / (Sample)Math::PI);

            return (
                fundamental_amplitude * fundamental
                + partial_amplitude * partial
            );
        }
};


template<class OscillatorClass = SimpleOscillator>
void assert_oscillator_output_is_close_to_reference(
        NonBandLimitedReferenceWaveform const& reference,
        OscillatorClass& oscillator,
        Frequency const sample_rate,
        Integer const block_size,
        Integer const rounds,
        Number const tolerance,
        Byte const waveform = OscillatorClass::SINE
) {
    Integer const sample_count = rounds * block_size;
    Sample expected_samples[sample_count];
    Buffer rendered_samples(sample_count);

    oscillator.start(0.0);

    for (Integer i = 0; i != sample_count; ++i) {
        Seconds const time = (Seconds)i / (Seconds)sample_rate;
        expected_samples[i] = reference.generate_sample(time);
    }

    render_rounds<OscillatorClass>(
        oscillator, rendered_samples, rounds, block_size
    );

    assert_close(
        expected_samples,
        rendered_samples.samples[0],
        sample_count,
        tolerance,
        "waveform=%d",
        (int)waveform
    );
}


template<class ReferenceWaveformClass>
void test_basic_waveform(
        Byte const waveform,
        Number const tolerance,
        Frequency const sample_rate = SAMPLE_RATE,
        Frequency const frequency = 100.0,
        Integer const block_size = 128,
        Integer const rounds = 5
) {
    ReferenceWaveformClass reference(frequency);
    SimpleOscillator::WaveformParam waveform_param("");
    SimpleOscillator oscillator(waveform_param);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(sample_rate);
    oscillator.waveform.set_value(waveform);
    oscillator.frequency.set_value(frequency);

    assert_oscillator_output_is_close_to_reference(
        reference, oscillator, sample_rate, block_size, rounds, tolerance, waveform
    );
}


TEST(basic_waveforms, {
    test_basic_waveform<ReferenceSine>(SimpleOscillator::SINE, 0.01);
    test_basic_waveform<ReferenceSawtooth>(
        SimpleOscillator::SAWTOOTH, 0.08
    );
    test_basic_waveform<ReferenceInverseSawtooth>(
        SimpleOscillator::INVERSE_SAWTOOTH, 0.08
    );
    test_basic_waveform<ReferenceTriangle>(
        SimpleOscillator::TRIANGLE, 0.001
    );
    test_basic_waveform<ReferenceSquare>(
        SimpleOscillator::SQUARE, 0.05
    );
})


TEST(low_frequency_oscillator_applies_dc_offset_to_oscillate_between_0_and_2, {
    constexpr Frequency frequency = 100.0;
    constexpr Integer block_size = 128;
    constexpr Integer rounds = 5;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Sample amplitude = 0.75;
    ReferenceSine reference(frequency, 0.0, 0.0, 1.0);
    SimpleLFO::WaveformParam waveform_param("");
    SimpleLFO oscillator(waveform_param);
    Sample expected_samples[sample_count];
    Buffer rendered_samples(sample_count);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleLFO::SINE);
    oscillator.frequency.set_value(frequency);
    oscillator.amplitude.set_value(amplitude);
    oscillator.start(0.0);

    for (Integer i = 0; i != sample_count; ++i) {
        Seconds const time = (Seconds)i / (Seconds)SAMPLE_RATE;
        expected_samples[i] = amplitude * reference.generate_sample(time);
    }

    render_rounds<SimpleLFO>(
        oscillator, rendered_samples, rounds, block_size
    );

    assert_close(
        expected_samples, rendered_samples.samples[0], sample_count, 0.001
    );
})


TEST(custom_waveform_is_updated_before_each_rendering_round, {
    constexpr Frequency sample_rate = 22050.0;
    constexpr Integer block_size = 2048;

    SumOfSines expected_1(0.5, 440.0, -0.5, 440.0 * 2.0, 0.0, 440.0 * 9.0, 1, 0.0);
    SumOfSines expected_2(0.5, 440.0, 0.3, 440.0 * 2.0, 0.2, 440.0 * 9.0, 1, (Number)block_size / sample_rate);

    FloatParamS amplitude("", 0.0, 1.0, 1.0);
    FloatParamS dummy_float_param("", 0.0, 1.0, 0.0);
    ToggleParam dummy_toggle_param("", ToggleParam::OFF);
    FloatParamB harmonic_0("", -1.0, 1.0, 0.0);
    FloatParamB harmonic_1("", -1.0, 1.0, 0.0);
    FloatParamB harmonic_8("", -1.0, 1.0, 0.0);
    FloatParamB harmonic_rest("", -1.0, 1.0, 0.0);

    SimpleOscillator::WaveformParam waveform_param("");
    SimpleOscillator oscillator(
        waveform_param,
        amplitude,
        dummy_float_param,
        dummy_float_param,
        dummy_float_param,
        dummy_toggle_param,
        harmonic_0,
        harmonic_1,
        harmonic_rest,
        harmonic_rest,
        harmonic_rest,
        harmonic_rest,
        harmonic_rest,
        harmonic_rest,
        harmonic_8,
        harmonic_rest
    );

    Buffer actual_output(block_size);
    Buffer expected_output(block_size);

    amplitude.set_sample_rate(sample_rate);
    amplitude.set_block_size(block_size);

    dummy_float_param.set_sample_rate(sample_rate);
    dummy_float_param.set_block_size(block_size);

    harmonic_0.set_sample_rate(sample_rate);
    harmonic_0.set_block_size(block_size);

    harmonic_1.set_sample_rate(sample_rate);
    harmonic_1.set_block_size(block_size);

    harmonic_8.set_sample_rate(sample_rate);
    harmonic_8.set_block_size(block_size);

    harmonic_rest.set_sample_rate(sample_rate);
    harmonic_rest.set_block_size(block_size);

    expected_1.set_sample_rate(sample_rate);
    expected_1.set_block_size(block_size);

    expected_2.set_sample_rate(sample_rate);
    expected_2.set_block_size(block_size);

    waveform_param.set_sample_rate(sample_rate);
    waveform_param.set_block_size(block_size);
    waveform_param.set_value(SimpleOscillator::CUSTOM);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(sample_rate);

    harmonic_0.set_value(0.5);
    harmonic_1.set_value(-0.5);

    oscillator.frequency.set_value(440.0);
    oscillator.start(0.0);

    harmonic_1.schedule_value(0.001, 0.3);
    harmonic_8.schedule_value(0.001, 0.2);

    render_rounds<SimpleOscillator>(oscillator, actual_output, 1, block_size, 1);
    render_rounds<SumOfSines>(expected_1, expected_output, 1, block_size, 1);

    assert_close(
        expected_output.samples[0],
        actual_output.samples[0],
        block_size,
        0.01,
        "round=1"
    );

    render_rounds<SimpleOscillator>(oscillator, actual_output, 1, block_size, 2);
    render_rounds<SumOfSines>(expected_2, expected_output, 1, block_size, 2);

    assert_close(
        expected_output.samples[0],
        actual_output.samples[0],
        block_size,
        0.01,
        "round=2"
    );
});


TEST(sine_chirp_from_100hz_to_400hz, {
    constexpr Frequency start_frequency = 100.0;
    constexpr Frequency end_frequency = 400.0;
    constexpr Integer block_size = 128;
    constexpr Integer rounds = 5;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Seconds duration = (Seconds)sample_count / (Seconds)SAMPLE_RATE;
    ReferenceSine reference_sine(start_frequency, end_frequency, duration);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value(start_frequency);
    oscillator.frequency.schedule_linear_ramp(duration, (Number)end_frequency);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.02
    );
})


TEST(oscillator_can_be_started_and_stopped_between_samples, {
    constexpr Sample frequency = 1.0;
    constexpr Frequency sample_rate = 6.0;
    constexpr Integer block_size = 6;
    constexpr Sample sample_period = 1.0 / (Seconds)sample_rate;
    constexpr Sample time_offset = 0.5 * sample_period;
    constexpr Sample pi_double = (Sample)Math::PI_DOUBLE;
    Sample const* const* block;
    Sample expected_samples[2][block_size] = {
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {
            /* sin(2 * pi * frequency * time) */
            std::sin(pi_double * frequency * (0.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (1.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (2.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (3.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (4.0 * sample_period + time_offset)),
            0.0,
        },
    };

    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(sample_rate);
    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value((Number)frequency);
    oscillator.start((Seconds)(1.0 - time_offset));
    oscillator.stop((Seconds)(2.0 - time_offset - sample_period));

    block = SignalProducer::produce<SimpleOscillator>(oscillator, 1, block_size);
    assert_eq(expected_samples[0], block[0], block_size, DOUBLE_DELTA);

    block = SignalProducer::produce<SimpleOscillator>(oscillator, 2, block_size);
    assert_eq(expected_samples[1], block[0], block_size, DOUBLE_DELTA);
})


TEST(repeated_start_and_stop_calls_are_ignored, {
    constexpr Sample frequency = 1.0;
    constexpr Frequency sample_rate = 6.0;
    constexpr Integer block_size = 6;
    constexpr Sample sample_period = 1.0 / (Seconds)sample_rate;
    constexpr Sample time_offset = 0.5 * sample_period;
    constexpr Sample pi_double = (Sample)Math::PI_DOUBLE;
    Sample const* const* block;
    Sample expected_samples[2][block_size] = {
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {
            /* sin(2 * pi * frequency * time) */
            std::sin(pi_double * frequency * (0.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (1.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (2.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (3.0 * sample_period + time_offset)),
            std::sin(pi_double * frequency * (4.0 * sample_period + time_offset)),
            0.0,
        },
    };

    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(sample_rate);
    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value((Number)frequency);
    oscillator.start(1.0 - time_offset);
    oscillator.start(0.42);
    oscillator.stop(2.0 - time_offset - sample_period);
    oscillator.stop(1.23);

    block = SignalProducer::produce<SimpleOscillator>(oscillator, 1, block_size);
    assert_eq(expected_samples[0], block[0], block_size, DOUBLE_DELTA);

    block = SignalProducer::produce<SimpleOscillator>(oscillator, 2, block_size);
    assert_eq(expected_samples[1], block[0], block_size, DOUBLE_DELTA);
})


TEST(harmonics_above_the_nyquist_frequency_disappear, {
    test_basic_waveform<ReferenceSine>(
        SimpleOscillator::SAWTOOTH,
        0.09,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY - 1.0
    );
})


TEST(frequency_may_be_very_low, {
    test_basic_waveform<ReferenceSawtooth>(
        SimpleOscillator::SAWTOOTH, 0.08, SAMPLE_RATE, 1.0, 1024, 10
    );
})


void assert_amplitude_and_frequency_automation_are_independent_of_each_other(
        bool const automate_amplitude,
        bool const automate_frequency
) {
    constexpr Number const_amplitudes[] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    constexpr Number changing_amplitudes[] = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    Number const* const amplitudes = (
        automate_amplitude ? changing_amplitudes : const_amplitudes
    );
    constexpr Sample frequency = 1.0;
    constexpr Frequency sample_rate = 6.0;
    constexpr Integer block_size = 6;
    constexpr Sample sample_period = 1.0 / (Seconds)sample_rate;
    constexpr Sample pi_double = (Sample)Math::PI_DOUBLE;
    Sample const* const* block;
    Sample expected_samples[block_size] = {
        /* sin(2 * pi * frequency * time) */
        amplitudes[0] * std::sin(pi_double * frequency * (0.0 * sample_period)),
        amplitudes[1] * std::sin(pi_double * frequency * (1.0 * sample_period)),
        amplitudes[2] * std::sin(pi_double * frequency * (2.0 * sample_period)),
        amplitudes[3] * std::sin(pi_double * frequency * (3.0 * sample_period)),
        amplitudes[4] * std::sin(pi_double * frequency * (4.0 * sample_period)),
        amplitudes[5] * std::sin(pi_double * frequency * (5.0 * sample_period)),
    };

    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(sample_rate);
    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value((Number)frequency);

    if (automate_frequency) {
        oscillator.frequency.schedule_value(
            0.7, (Number)frequency + DOUBLE_DELTA
        );
    }

    oscillator.amplitude.set_value(0.5);

    if (automate_amplitude) {
        oscillator.amplitude.schedule_linear_ramp(5.0 / 6.0, 1.0);
    }

    oscillator.start(0.0);

    block = SignalProducer::produce<SimpleOscillator>(oscillator, 1, block_size);
    assert_eq(expected_samples, block[0], block_size, DOUBLE_DELTA);
}


TEST(amplitude_and_frequency_may_be_automated_independently_of_each_other, {
    assert_amplitude_and_frequency_automation_are_independent_of_each_other(false, false);
    assert_amplitude_and_frequency_automation_are_independent_of_each_other(true, false);
    assert_amplitude_and_frequency_automation_are_independent_of_each_other(false, true);
    assert_amplitude_and_frequency_automation_are_independent_of_each_other(true, true);
})


void schedule_100hz_tuning(
        SimpleOscillator& oscillator,
        Integer const block_size,
        Integer const rounds
) {
    Seconds const time_offset = (
        oscillator.sample_count_to_relative_time_offset(rounds * block_size) / 2.0
    );
    Seconds const one_block = oscillator.sample_count_to_relative_time_offset(block_size);

    oscillator.frequency.schedule_value(time_offset, 50.0);
    oscillator.detune.schedule_value(time_offset, 1600.0);
    oscillator.fine_detune.schedule_value(time_offset, -400.0);

    oscillator.detune.schedule_value(time_offset + one_block, 900.0);
    oscillator.fine_detune.schedule_value(time_offset + one_block, 300.0);
}


void assert_completed(SimpleOscillator& oscillator)
{
    char const* message = (
        "Oscillator failed to complete the timeline of its parameters"
    );

    assert_eq(50.0, oscillator.frequency.get_value(), DOUBLE_DELTA, message);
    assert_eq(900.0, oscillator.detune.get_value(), DOUBLE_DELTA, message);
    assert_eq(300.0, oscillator.fine_detune.get_value(), DOUBLE_DELTA, message);
}


/* Both frequency, detune, and fine_detune are constants. */
TEST(sine_wave_25hz_detuned_and_fine_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(25.0);
    oscillator.detune.set_value(2200.0);
    oscillator.fine_detune.set_value(200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


/* Detune and fine detune are constants. */
TEST(sine_wave_scheduled_to_be_25hz_and_detuned_and_fine_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 1000;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(20.0);
    oscillator.frequency.schedule_value(ALMOST_IMMEDIATELY, 25.0);
    oscillator.detune.set_value(2200.0);
    oscillator.fine_detune.set_value(200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


/* Frequency and fine detune are constants. */
TEST(sine_wave_25hz_fine_detuned_and_scheduled_to_be_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(25.0);
    oscillator.detune.set_value(1200.0);
    oscillator.detune.schedule_value(ALMOST_IMMEDIATELY, 2200.0);
    oscillator.fine_detune.set_value(200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


/* Fine detune is constant. */
TEST(sine_wave_scheduled_to_be_25hz_and_fine_detuned_and_scheduled_to_be_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(20.0);
    oscillator.frequency.schedule_value(ALMOST_IMMEDIATELY, 25.0);
    oscillator.detune.set_value(1200.0);
    oscillator.detune.schedule_value(ALMOST_IMMEDIATELY, 2200.0);
    oscillator.fine_detune.set_value(200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.011
    );
    assert_completed(oscillator);
})


/* Frequency and detune are constants. */
TEST(sine_wave_25hz_detuned_and_scheduled_to_be_fine_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(25.0);
    oscillator.detune.set_value(2200.0);
    oscillator.fine_detune.set_value(-400.0);
    oscillator.fine_detune.schedule_value(ALMOST_IMMEDIATELY, 200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


/* Detune is constant. */
TEST(sine_wave_scheduled_to_be_25hz_and_detuned_and_scheduled_to_be_fine_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(20.0);
    oscillator.frequency.schedule_value(ALMOST_IMMEDIATELY, 25.0);
    oscillator.detune.set_value(2200.0);
    oscillator.fine_detune.set_value(-400.0);
    oscillator.fine_detune.schedule_value(ALMOST_IMMEDIATELY, 200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


/* Frequency is constant. */
TEST(sine_wave_25hz_scheduled_to_be_detuned_and_fine_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(25.0);
    oscillator.detune.set_value(2000.0);
    oscillator.detune.schedule_value(ALMOST_IMMEDIATELY, 2200.0);
    oscillator.fine_detune.set_value(-200.0);
    oscillator.fine_detune.schedule_value(ALMOST_IMMEDIATELY, 200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


/* All frequency related params are changing. */
TEST(sine_wave_scheduled_to_be_25hz_and_scheduled_to_be_detuned_and_fine_detuned_two_octaves_up_should_make_a_100hz_wave, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(20.0);
    oscillator.frequency.schedule_value(ALMOST_IMMEDIATELY, 25.0);
    oscillator.detune.set_value(2000.0);
    oscillator.detune.schedule_value(ALMOST_IMMEDIATELY, 2200.0);
    oscillator.fine_detune.set_value(-200.0);
    oscillator.fine_detune.schedule_value(ALMOST_IMMEDIATELY, 200.0);

    schedule_100hz_tuning(oscillator, block_size, rounds);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );
    assert_completed(oscillator);
})


TEST(fine_detune_range_can_be_increased, {
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    ReferenceSine reference_sine(100.0);
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator(waveform);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);

    oscillator.frequency.set_value(20.0);
    oscillator.frequency.schedule_value(ALMOST_IMMEDIATELY, 25.0);
    oscillator.detune.set_value(2000.0);
    oscillator.detune.schedule_value(ALMOST_IMMEDIATELY, 2200.0);
    oscillator.fine_detune.set_value(-50.0);
    oscillator.fine_detune.schedule_value(ALMOST_IMMEDIATELY, 50.0);

    oscillator.fine_detune_x4.set_value(ToggleParam::ON);

    assert_oscillator_output_is_close_to_reference(
        reference_sine, oscillator, SAMPLE_RATE, block_size, rounds, 0.01
    );

    oscillator.fine_detune_x4.set_value(ToggleParam::OFF);
})


TEST(when_frequency_is_above_the_nyquist_frequency_then_oscillator_is_silent, {
    test_basic_waveform<ReferenceZero>(
        SimpleOscillator::SINE,
        0.01,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY + 0.001
    );
    test_basic_waveform<ReferenceZero>(
        SimpleOscillator::SAWTOOTH,
        0.01,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY + 0.001
    );
    test_basic_waveform<ReferenceZero>(
        SimpleOscillator::INVERSE_SAWTOOTH,
        0.01,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY + 0.001
    );
    test_basic_waveform<ReferenceZero>(
        SimpleOscillator::TRIANGLE,
        0.01,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY + 0.001
    );
    test_basic_waveform<ReferenceZero>(
        SimpleOscillator::SQUARE,
        0.01,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY + 0.001
    );
    test_basic_waveform<ReferenceZero>(
        SimpleOscillator::CUSTOM,
        0.01,
        SAMPLE_RATE,
        NYQUIST_FREQUENCY + 0.001
    );
})


TEST(while_frequency_goes_close_to_the_nyquist_frequency_harmonics_disappear_gradually, {
    test_basic_waveform<ReferenceSawtoothWithDisappearingPartial>(
        SimpleOscillator::SAWTOOTH,
        0.12,
        SAMPLE_RATE,
        0.75 * NYQUIST_FREQUENCY,
        256,
        1
    );
})


void set_up_chunk_size_independent_test(
        SimpleOscillator& oscillator,
        Frequency const sample_rate
) {
    oscillator.set_sample_rate(sample_rate);
    oscillator.start(0.01);
    oscillator.amplitude.set_value(0.0);
    oscillator.amplitude.schedule_linear_ramp(0.15, 1.0);
    oscillator.frequency.set_value(110.0);
    oscillator.frequency.schedule_linear_ramp(0.25, 220.0);
    oscillator.waveform.set_value(SimpleOscillator::SAWTOOTH);
}


TEST(oscillator_rendering_is_independent_of_chunk_size, {
    constexpr Frequency sample_rate = 44100.0;
    SimpleOscillator::WaveformParam waveform("");
    SimpleOscillator oscillator_1(waveform);
    SimpleOscillator oscillator_2(waveform);

    set_up_chunk_size_independent_test(oscillator_1, sample_rate);
    set_up_chunk_size_independent_test(oscillator_2, sample_rate);

    assert_rendering_is_independent_from_chunk_size<SimpleOscillator>(
        oscillator_1, oscillator_2, 0.0002
    );
})


TEST(amplitude_modulation_creates_two_sidebands, {
    /* https://www.soundonsound.com/techniques/amplitude-modulation */

    typedef SimpleOscillator Modulator;
    typedef Oscillator<Modulator> Carrier;

    constexpr Frequency sample_rate = 22050.0;
    constexpr Frequency freq_carrier = 1000.0;
    constexpr Frequency freq_modulator = 200.0;
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    constexpr Integer sample_count = rounds * block_size;

    SumOfSines expected(
        0.6, freq_carrier,
        0.5 * 0.6 * 0.7, freq_carrier - freq_modulator,
        0.5 * 0.6 * 0.7, freq_carrier + freq_modulator,
        1,
        -0.001
    );

    FloatParamS dummy_param("", 0.0, 1.0, 0.0);
    FloatParamS modulation_level("", 0.0, 1.0, 0.7);

    Modulator::WaveformParam modulator_waveform("");
    Modulator modulator(modulator_waveform);

    Carrier::WaveformParam carrier_waveform("");
    Carrier carrier(
        carrier_waveform,
        modulator,
        modulation_level,
        dummy_param,
        dummy_param
    );

    Buffer carrier_output(sample_count);
    Buffer expected_output(sample_count);

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    modulation_level.set_sample_rate(sample_rate);
    modulation_level.set_block_size(block_size);

    modulator.set_sample_rate(sample_rate);
    modulator.set_block_size(block_size);
    modulator.waveform.set_value(Modulator::SINE);
    modulator.frequency.set_value(freq_modulator);
    modulator.start(0.0);

    carrier.set_sample_rate(sample_rate);
    carrier.set_block_size(block_size);
    carrier.waveform.set_value(Carrier::SINE);
    carrier.frequency.set_value(freq_carrier);
    carrier.amplitude.set_value(0.2);
    carrier.amplitude.schedule_value(0.00001, 0.6);
    carrier.start(0.0);

    render_rounds<Carrier>(carrier, carrier_output, rounds, block_size);
    render_rounds<SumOfSines>(expected, expected_output, rounds, block_size);

    assert_close(
        expected_output.samples[0],
        carrier_output.samples[0],
        sample_count,
        0.06
    );
})


TEST(frequency_may_be_modulated, {
    /*
    https://www.soundonsound.com/techniques/introduction-frequency-modulation
    */

    typedef Constant Modulator;
    typedef Oscillator<Modulator> Carrier;

    constexpr Frequency sample_rate = 22050.0;
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    constexpr Integer sample_count = rounds * block_size;

    SumOfSines expected(1.0, 1000.0, 0.0, 0.0, 0.0, 0.0, 1, -0.000025);

    FloatParamS dummy_param("", 0.0, 1.0, 0.0);
    FloatParamS modulation_level("", 0.0, 1.0, 0.5);

    Modulator modulator(500.0);

    Carrier::WaveformParam carrier_waveform("");
    Carrier carrier(
        carrier_waveform,
        modulator,
        dummy_param,
        modulation_level,
        dummy_param
    );

    Buffer carrier_output(sample_count);
    Buffer expected_output(sample_count);

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    modulation_level.set_sample_rate(sample_rate);
    modulation_level.set_block_size(block_size);

    modulator.set_sample_rate(sample_rate);
    modulator.set_block_size(block_size);

    carrier.set_sample_rate(sample_rate);
    carrier.set_block_size(block_size);
    carrier.waveform.set_value(Carrier::SINE);
    carrier.frequency.set_value(0.0);
    carrier.frequency.schedule_value(0.000001, 750.0);
    carrier.start(0.0);

    render_rounds<Carrier>(carrier, carrier_output, rounds, block_size);
    render_rounds<SumOfSines>(expected, expected_output, rounds, block_size);

    assert_close(
        expected_output.samples[0],
        carrier_output.samples[0],
        sample_count,
        0.05
    );
})


TEST(phase_can_be_controlled, {
    constexpr Number period_length = 100.0;
    constexpr Number phase = 120.0 / 360.0;
    constexpr Frequency frequency = SAMPLE_RATE / period_length;
    constexpr Integer block_size = 256;
    constexpr Integer rounds = 10;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Number block_length = (Number)block_size / SAMPLE_RATE;
    SumOfSines expected(
        1.0, frequency,
        0.0, 0.0,
        0.0, 0.0,
        1,
        -(period_length - period_length * phase) / SAMPLE_RATE
    );
    SimpleOscillator::WaveformParam waveform_param("");
    SimpleOscillator oscillator(waveform_param);
    Buffer rendered_samples(sample_count);
    Buffer expected_samples(sample_count);

    expected.set_block_size(block_size);
    expected.set_sample_rate(SAMPLE_RATE);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);

    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value(frequency);
    oscillator.frequency.schedule_value(block_length + 0.000001, frequency);
    oscillator.amplitude.set_value(1.0);
    oscillator.amplitude.schedule_value(block_length * 2.0 + 0.000001, 1.0);
    oscillator.start(0.0);

    oscillator.phase.set_value(phase);
    oscillator.phase.schedule_value(0.000001, phase + 0.000001);
    oscillator.phase.schedule_value(block_length + 0.000001, phase - 0.000001);

    render_rounds<SumOfSines>(expected, expected_samples, rounds, block_size);
    render_rounds<SimpleOscillator>(
        oscillator, rendered_samples, rounds, block_size
    );

    assert_close(
        expected_samples.samples[0],
        rendered_samples.samples[0],
        sample_count,
        0.001
    );
})


TEST(can_skip_a_round_without_rendering, {
    constexpr Integer block_size = 2048;
    constexpr Frequency frequency = 440.0;
    Constant zero(0.0);
    SumOfSines sine(1.0, frequency, 0.0, 0.0, 0.0, 0.0, 1);
    SimpleOscillator::WaveformParam waveform_param("");
    SimpleOscillator oscillator(waveform_param);
    Buffer rendered_samples(block_size);
    Buffer expected_samples(block_size);

    zero.set_block_size(block_size);
    zero.set_sample_rate(SAMPLE_RATE);

    sine.set_block_size(block_size);
    sine.set_sample_rate(SAMPLE_RATE);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value(frequency);
    oscillator.start(0.0);

    oscillator.skip_round(1, block_size);
    oscillator.skip_round(1, block_size);
    render_rounds<Constant>(zero, expected_samples, 1, block_size, 1);
    render_rounds<SimpleOscillator>(
        oscillator, rendered_samples, 1, block_size, 1
    );
    assert_eq(
        expected_samples.samples[0],
        rendered_samples.samples[0],
        block_size,
        0.001,
        "round=1"
    );

    render_rounds<SumOfSines>(sine, expected_samples, 1, block_size, 2);
    render_rounds<SimpleOscillator>(
        oscillator, rendered_samples, 1, block_size, 2
    );
    assert_eq(
        expected_samples.samples[0],
        rendered_samples.samples[0],
        block_size,
        0.001,
        "round=2"
    );
})


TEST(resetting_the_oscillator_turns_it_off, {
    constexpr Frequency frequency = 100.0;
    constexpr Integer block_size = 128;
    constexpr Integer rounds = 5;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Sample amplitude = 0.75;
    ReferenceSine reference(frequency);
    SimpleOscillator::WaveformParam waveform_param("");
    SimpleOscillator oscillator(waveform_param);
    Sample expected_samples[sample_count];
    Buffer rendered_samples(sample_count);

    assert_false(oscillator.is_on());

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.waveform.set_value(SimpleOscillator::SINE);
    oscillator.frequency.set_value(frequency);
    oscillator.amplitude.set_value(amplitude);
    oscillator.start(0.0);

    for (Integer i = 0; i != sample_count; ++i) {
        expected_samples[i] = 0.0;
    }

    render_rounds<SimpleOscillator>(
        oscillator, rendered_samples, rounds, block_size
    );

    assert_true(oscillator.is_on());

    oscillator.reset();

    render_rounds<SimpleOscillator>(
        oscillator, rendered_samples, rounds, block_size
    );

    assert_false(oscillator.is_on());

    assert_close(
        expected_samples, rendered_samples.samples[0], sample_count, DOUBLE_DELTA
    );
})


TEST(when_oscillator_is_tempo_synced_then_frequency_is_interpreted_in_terms_of_beats_instead_of_seconds, {
    constexpr Frequency frequency = 100.0;
    constexpr Number scale = 3.0;
    constexpr Number bpm = scale * 60.0;
    constexpr Frequency expeced_frequency = scale * frequency;
    constexpr Integer block_size = 128;

    ReferenceSine reference(expeced_frequency, 0.0, 0.0, 1.0);
    SimpleLFO::WaveformParam waveform_param("");
    ToggleParam tempo_sync("SYN", ToggleParam::ON);
    FloatParamS amplitude_leader("AMP", 0.0, 1.0, 1.0);
    FloatParamS frequency_leader("FREQ", 0.01, 10000.0, 500.0);
    FloatParamS dummy_param("", 0.0, 1.0, 0.0);
    ToggleParam dummy_toggle("", ToggleParam::OFF);
    SimpleLFO oscillator(
        waveform_param,
        amplitude_leader,
        frequency_leader,
        dummy_param,
        tempo_sync,
        dummy_toggle
    );

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(SAMPLE_RATE);
    oscillator.set_bpm(bpm);
    oscillator.waveform.set_value(SimpleLFO::SINE);
    frequency_leader.set_value(frequency);

    assert_oscillator_output_is_close_to_reference<SimpleLFO>(
        reference, oscillator, SAMPLE_RATE, block_size, 5, 0.000001, SimpleOscillator::SINE
    );
});


TEST(amplitude_of_subharmonic_is_independent_from_main_amplitude, {
    constexpr Frequency sample_rate = 22050.0;
    constexpr Integer block_size = 2048;
    constexpr Integer rounds = 3;
    constexpr Integer buffer_size = rounds * block_size;

    SumOfSines expected(0.7, 220.0, 0.3, 440.0, 0.0, 0.0, 1, 0.0);

    SimpleOscillator::WaveformParam waveform_param("");
    SimpleOscillator oscillator(waveform_param);

    Buffer actual_output(buffer_size);
    Buffer expected_output(buffer_size);

    expected.set_sample_rate(sample_rate);
    expected.set_block_size(block_size);

    waveform_param.set_sample_rate(sample_rate);
    waveform_param.set_block_size(block_size);
    waveform_param.set_value(SimpleOscillator::SINE);

    oscillator.set_block_size(block_size);
    oscillator.set_sample_rate(sample_rate);
    oscillator.amplitude.set_value(0.7);
    oscillator.subharmonic_amplitude.set_value(0.3);
    oscillator.frequency.set_value(440.0);
    oscillator.start(0.0);

    oscillator.amplitude.schedule_value(0.0001, 0.3);
    oscillator.subharmonic_amplitude.schedule_value(0.0001, 0.7);

    render_rounds<SimpleOscillator>(oscillator, actual_output, rounds, block_size);
    render_rounds<SumOfSines>(expected, expected_output, rounds, block_size);

    assert_close(
        expected_output.samples[0], actual_output.samples[0], buffer_size, 0.0001
    );
})

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

#ifndef JS80P__TESTS__UTILS_CPP
#define JS80P__TESTS__UTILS_CPP

#include "js80p.hpp"

#include "synth/math.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"


namespace JS80P
{

constexpr float FLOAT_DELTA = 0.000001f;
constexpr double DOUBLE_DELTA = 0.000001;


class Buffer
{
    public:
        Buffer(Integer const size, Integer const channels = 1)
            : size(size),
            channels(channels)
        {
            samples = new Sample*[channels];

            for (Integer channel = 0; channel != channels; ++channel) {
                samples[channel] = new Sample[size];

                for (Integer i = 0; i != size; ++i) {
                    samples[channel][i] = 0.0;
                }
            }
        }

        ~Buffer()
        {
            for (Integer channel = 0; channel != channels; ++channel) {
                delete[] samples[channel];
            }

            delete[] samples;
        }

        Sample** samples;
        Integer const size;
        Integer const channels;
};


class Constant : public SignalProducer
{
    friend class SignalProducer;

    public:
        Constant(Sample const value) : SignalProducer(1, 0), value(value)
        {
        }

    protected:
        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) {
            Integer const channels = get_channels();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = value;
                }
            }
        }

    private:
        Sample const value;
};


class SumOfSines : public SignalProducer
{
    friend class SignalProducer;

    public:
        SumOfSines(
                Number const amplitude_1 = 0.0,
                Frequency const frequency_1 = 0.0,
                Number const amplitude_2 = 0.0,
                Frequency const frequency_2 = 0.0,
                Number const amplitude_3 = 0.0,
                Frequency const frequency_3 = 0.0,
                Integer const channels = 1,
                Number const delay = 0.0
        ) : SignalProducer(channels, 0),
            amplitude_1((Sample)amplitude_1),
            amplitude_2((Sample)amplitude_2),
            amplitude_3((Sample)amplitude_3),
            frequency_1_times_pi_double(
                (Sample)frequency_1 * (Sample)Math::PI_DOUBLE
            ),
            frequency_2_times_pi_double(
                (Sample)frequency_2 * (Sample)Math::PI_DOUBLE
            ),
            frequency_3_times_pi_double(
                (Sample)frequency_3 * (Sample)Math::PI_DOUBLE
            ),
            delay((Sample)delay),
            rendered_samples(0)
        {
        }

        void reset()
        {
            current_time = 0.0;
            rendered_samples = 0;
            cached_round = -1;
            events.drop(0);
        }

    protected:
        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) {
            Integer const channels = get_channels();
            Sample time = (
                (Sample)rendered_samples * (Sample)sampling_period + delay
            );

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const sample = (Sample)(
                    amplitude_1 * Math::sin(
                        frequency_1_times_pi_double * time
                    )
                    + amplitude_2 * Math::sin(
                        frequency_2_times_pi_double * time
                    )
                    + amplitude_3 * Math::sin(
                        frequency_3_times_pi_double * time
                    )
                );

                for (Integer c = 0; c != channels; ++c) {
                    buffer[c][i] = sample;
                }

                time += (Sample)sampling_period;
            }

            rendered_samples += last_sample_index - first_sample_index;
        }

    private:
        Sample const amplitude_1;
        Sample const amplitude_2;
        Sample const amplitude_3;
        Sample const frequency_1_times_pi_double;
        Sample const frequency_2_times_pi_double;
        Sample const frequency_3_times_pi_double;
        Sample const delay;

        Integer rendered_samples;
};


template<class SignalProducerClass>
void render_rounds(
        SignalProducerClass& signal_producer,
        Buffer& buffer,
        Integer const rounds,
        Integer const chunk_size = 0,
        Integer const first_round = 1
) {
    Integer const channels = signal_producer.get_channels();
    Integer const size = (
        0 < chunk_size ? chunk_size : signal_producer.get_block_size()
    );

    for (Integer i = 0; i != rounds; ++i) {
        Sample const* const* block = (
            SignalProducer::produce<SignalProducerClass>(
                &signal_producer, i + first_round, size
            )
        );

        for (Integer c = 0; c != channels; ++c) {
            for (Integer b = 0; b != size; ++b) {
                buffer.samples[c][i * size + b] = block[c][b];
            }
        }
    }
}


/**
 * \brief Assert that two identical signal chains render the same output,
 *        regardless of rendering chunk and block size.
 *
 * \warning The two signal chains must be identical, but they must not share any
 *          \c SignalProducer instances with each other, even if it would seem
 *          rational to do so (e.g. to use the same \c FloatParam leader
 *          instance for both chains). The reason is that the two signal chains
 *          are rendered independently from each other, so their rendering
 *          rounds are not synchronized to each other.
 */
template<class SignalProducerClass>
void assert_rendering_is_independent_from_chunk_size(
        SignalProducerClass& signal_producer_1,
        SignalProducerClass& signal_producer_2,
        Number const tolerance = 0.001,
        char const* message = NULL
) {
    constexpr Integer block_size = 500;
    constexpr Integer rounds_1 = 360;
    constexpr Integer buffer_size = block_size * rounds_1;
    constexpr Integer short_round_size = 150;
    constexpr Integer rounds_2 = buffer_size / short_round_size;
    Integer const channels = signal_producer_1.get_channels();

    Buffer buffer_1(buffer_size, channels);
    Buffer buffer_2(buffer_size, channels);

    signal_producer_1.set_block_size(block_size);
    signal_producer_2.set_block_size(block_size * 10);

    render_rounds<SignalProducerClass>(signal_producer_1, buffer_1, rounds_1, block_size);
    render_rounds<SignalProducerClass>(signal_producer_2, buffer_2, rounds_2, short_round_size);

    for (Integer c = 0; c != channels; ++c) {
        assert_eq(buffer_1.samples[c], buffer_2.samples[c], buffer_size, tolerance, message);
    }
}


void assert_statistics(
        Number const expected_validity,
        Number const expected_min,
        Number const expected_median,
        Number const expected_max,
        Number const expected_mean,
        Number const expected_standard_deviation,
        Math::Statistics const& statistics,
        Number const tolerance = DOUBLE_DELTA
) {
    if (!expected_validity) {
        assert_false(statistics.is_valid);

        return;
    }

    assert_true(statistics.is_valid);
    assert_eq(expected_min, statistics.min, tolerance);
    assert_eq(expected_median, statistics.median, tolerance);
    assert_eq(expected_max, statistics.max, tolerance);
    assert_eq(expected_mean, statistics.mean, tolerance);
    assert_eq(
        expected_standard_deviation, statistics.standard_deviation, tolerance
    );
}

}

#endif

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

#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"


using namespace JS80P;


TEST(basic_properties, {
    constexpr Integer block_size = 12345;
    constexpr Frequency sample_rate = 48000.0;
    SignalProducer signal_producer(3);

    signal_producer.set_block_size(block_size);
    signal_producer.set_sample_rate(sample_rate);

    assert_eq((int)block_size, (int)signal_producer.get_block_size());
    assert_eq(sample_rate, signal_producer.get_sample_rate(), DOUBLE_DELTA);
    assert_eq(3, (int)signal_producer.get_channels());
})


class CompositeSignalProducer : public SignalProducer
{
    public:
        CompositeSignalProducer()
            : SignalProducer(1, 1),
            child(1, 0)
        {
            register_child(child);
        }

        SignalProducer child;
};


TEST(changes_of_basic_properties_are_propagated_to_children, {
    constexpr Integer block_size = 12345;
    constexpr Frequency sample_rate = 48000.0;
    CompositeSignalProducer composite_signal_producer;

    composite_signal_producer.set_block_size(block_size);
    composite_signal_producer.set_sample_rate(sample_rate);

    assert_eq((int)block_size, (int)composite_signal_producer.child.get_block_size());
    assert_eq(sample_rate, composite_signal_producer.child.get_sample_rate(), DOUBLE_DELTA);
})


TEST(zero_channels_signal_producer_does_not_produce_anything, {
    constexpr Integer block_size = 123;
    SignalProducer signal_producer(0);
    Sample const* const* rendered_samples;

    signal_producer.set_block_size(block_size);
    signal_producer.set_sample_rate(22050.0);

    rendered_samples = SignalProducer::produce<SignalProducer>(
        &signal_producer, 1, block_size
    );

    assert_eq(NULL, rendered_samples);
})


class PublicSignalProducer : public SignalProducer
{
    public:
        PublicSignalProducer()
            : SignalProducer(1)
        {
        }

        Seconds get_sampling_period() const
        {
            return sampling_period;
        }

        Frequency get_nyquist_frequency() const
        {
            return nyquist_frequency;
        }
};


TEST(sampling_period_is_the_reciprocal_of_the_sample_rate, {
    PublicSignalProducer signal_producer;
    signal_producer.set_sample_rate(5.0);
    assert_eq(0.2, signal_producer.get_sampling_period(), DOUBLE_DELTA);
    assert_eq(2.5, signal_producer.get_nyquist_frequency(), DOUBLE_DELTA);
})


TEST(allocates_memory_for_the_given_channels_and_block_size, {
    constexpr Integer channels = 10;
    constexpr Integer block_size = 1024;
    Sample expected_samples[block_size];
    SignalProducer signal_producer(channels);
    Sample const* const* rendered_samples;

    signal_producer.set_block_size(block_size);
    rendered_samples = (
        SignalProducer::produce<SignalProducer>(&signal_producer, 2, block_size)
    );

    assert_eq((int)block_size, (int)signal_producer.get_block_size());

    for (Integer i = 0; i != block_size; ++i) {
        expected_samples[i] = 0.0;
    }

    for (Integer c = 0; c != channels; ++c) {
        assert_eq(
            expected_samples, rendered_samples[c], block_size, "channel=%d", c
        );
    }
})


TEST(can_convert_sample_number_to_time_offset, {
    SignalProducer signal_producer(1);
    signal_producer.set_sample_rate(4);

    assert_eq(0.0, signal_producer.sample_count_to_time_offset(0), DOUBLE_DELTA);
    assert_eq(0.25, signal_producer.sample_count_to_time_offset(1), DOUBLE_DELTA);
    assert_eq(0.5, signal_producer.sample_count_to_time_offset(2), DOUBLE_DELTA);
    assert_eq(0.75, signal_producer.sample_count_to_time_offset(3), DOUBLE_DELTA);
    assert_eq(1.0, signal_producer.sample_count_to_time_offset(4), DOUBLE_DELTA);
    assert_eq(1.25, signal_producer.sample_count_to_time_offset(5), DOUBLE_DELTA);
})


class CachingTestSignalProducer : public SignalProducer
{
    public:
        CachingTestSignalProducer()
            : SignalProducer(1)
        {
        }

        virtual void set_block_size(Integer const new_block_size) override
        {
            SignalProducer::set_block_size(new_block_size);
            SignalProducer::render(0, 0, block_size, buffer);
        }

        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) {
            buffer[0][0] += 1.0;
        }
};


TEST(rendering_is_done_only_once_per_round, {
    Sample const* const* rendered_samples;
    CachingTestSignalProducer signal_producer;
    signal_producer.set_block_size(2);

    SignalProducer::produce<CachingTestSignalProducer>(&signal_producer, 1, 2);
    SignalProducer::produce<CachingTestSignalProducer>(&signal_producer, 2, 2);
    rendered_samples = (
        SignalProducer::produce<CachingTestSignalProducer>(&signal_producer, 2, 2)
    );

    assert_eq(2.0, rendered_samples[0][0], DOUBLE_DELTA);
})


class DelegatingSignalProducer : public SignalProducer
{
    public:
        DelegatingSignalProducer(
                Sample const value,
                DelegatingSignalProducer* delegate = NULL
        ) : SignalProducer(1),
            delegate(delegate),
            initialize_rendering_calls(0),
            render_calls(0),
            value(value)
        {
        }

        Sample const* const* initialize_rendering(
                Integer const round,
                Integer const sample_count
        ) {
            ++initialize_rendering_calls;

            if (delegate != NULL) {
                return SignalProducer::produce<DelegatingSignalProducer>(
                    delegate, round, sample_count
                );
            }

            return NULL;
        }

        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) {
            ++render_calls;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = value;
            }
        }

        DelegatingSignalProducer* delegate;
        Integer initialize_rendering_calls;
        Integer render_calls;
        Sample value;
};


TEST(can_query_last_rendered_block, {
    Sample const* const* rendered_samples;
    Integer sample_count = 42;
    DelegatingSignalProducer delegate(1.0);
    DelegatingSignalProducer delegator(2.0, &delegate);

    delegate.get_last_rendered_block(sample_count);
    assert_eq(0, (int)sample_count);

    delegator.get_last_rendered_block(sample_count);
    assert_eq(0, (int)sample_count);

    delegate.set_block_size(10);
    delegator.set_block_size(10);

    rendered_samples = SignalProducer::produce<DelegatingSignalProducer>(&delegate, 1, 5);
    assert_eq(
        (void*)rendered_samples,
        (void*)delegate.get_last_rendered_block(sample_count)
    );
    assert_eq(5, (int)sample_count);

    sample_count = 42;
    rendered_samples = SignalProducer::produce<DelegatingSignalProducer>(
        &delegator, 1, 5
    );
    assert_eq(
        (void*)rendered_samples,
        (void*)delegator.get_last_rendered_block(sample_count)
    );
    assert_eq(5, (int)sample_count);

    delegate.set_block_size(20);
    delegate.get_last_rendered_block(sample_count);
    assert_eq(0, (int)sample_count);

    sample_count = 42;
    delegator.set_block_size(20);
    delegator.get_last_rendered_block(sample_count);
    assert_eq(0, (int)sample_count);
})


TEST(a_signal_producer_may_delegate_rendering_to_another_during_initialization, {
    constexpr Integer block_size = 2;
    constexpr Sample expected_samples[] = {1.0, 1.0};
    DelegatingSignalProducer delegate(1.0);
    DelegatingSignalProducer delegator(2.0, &delegate);
    Sample const* const* delegate_output;
    Sample const* const* delegator_output;

    delegate.set_block_size(block_size);
    delegator.set_block_size(block_size);

    delegate_output = SignalProducer::produce<DelegatingSignalProducer>(
        &delegate, 1, block_size
    );
    delegator_output = SignalProducer::produce<DelegatingSignalProducer>(
        &delegator, 1, block_size
    );

    assert_eq(expected_samples, delegate_output[0], block_size, DOUBLE_DELTA);
    assert_eq(expected_samples, delegator_output[0], block_size, DOUBLE_DELTA);
    assert_eq((void*)delegate_output, (void*)delegator_output);

    SignalProducer::produce<DelegatingSignalProducer>(
        &delegator, 1, block_size
    );

    assert_eq(1, (int)delegate.initialize_rendering_calls);
    assert_eq(1, (int)delegator.initialize_rendering_calls);
    assert_eq(1, (int)delegate.render_calls);
    assert_eq(0, (int)delegator.render_calls);
})


class RendererWithCircularDependecy : public SignalProducer
{
    public:
        RendererWithCircularDependecy()
            : SignalProducer(1)
        {
        }

        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) {
            Sample const* const* other_buffer = (
                SignalProducer::produce<RendererWithCircularDependecy>(dependency, round)
            );

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = other_buffer[0][i] + 1.0;
            }
        }

        RendererWithCircularDependecy* dependency;
};


TEST(cyclic_dependencies_in_rendering_are_broken_up_by_delaying_one_of_the_signal_producers, {
    constexpr Integer block_size = 3;
    constexpr Sample expected_samples_1[] = {3.0, 3.0, 3.0};
    constexpr Sample expected_samples_2[] = {2.0, 2.0, 2.0};
    constexpr Sample expected_samples_3[] = {1.0, 1.0, 1.0};
    RendererWithCircularDependecy signal_producer_1;
    RendererWithCircularDependecy signal_producer_2;
    RendererWithCircularDependecy signal_producer_3;
    Sample const* const* samples_1;
    Sample const* const* samples_2;
    Sample const* const* samples_3;

    signal_producer_1.set_block_size(block_size);
    signal_producer_2.set_block_size(block_size);
    signal_producer_3.set_block_size(block_size);

    signal_producer_1.set_sample_rate(22050.0);
    signal_producer_2.set_sample_rate(22050.0);
    signal_producer_3.set_sample_rate(22050.0);

    signal_producer_1.dependency = &signal_producer_2;
    signal_producer_2.dependency = &signal_producer_3;
    signal_producer_3.dependency = &signal_producer_1;

    samples_1 = SignalProducer::produce<RendererWithCircularDependecy>(
        &signal_producer_1, 1
    );
    samples_2 = SignalProducer::produce<RendererWithCircularDependecy>(
        &signal_producer_2, 1
    );
    samples_3 = SignalProducer::produce<RendererWithCircularDependecy>(
        &signal_producer_3, 1
    );

    assert_eq(expected_samples_1, samples_1[0], block_size);
    assert_eq(expected_samples_2, samples_2[0], block_size);
    assert_eq(expected_samples_3, samples_3[0], block_size);
})


class PreparerWithCircularDependecy : public SignalProducer
{
    public:
        PreparerWithCircularDependecy()
            : SignalProducer(1),
            value(0)
        {
        }

        Sample const* const* initialize_rendering(
                Integer const round,
                Integer const sample_count
        ) {
            SignalProducer::produce<PreparerWithCircularDependecy>(
                dependency, round
            );

            value = dependency->value + 1;

            return NULL;
        }

        PreparerWithCircularDependecy* dependency;
        int value;
};


TEST(cyclic_dependencies_in_rendering_initialization_are_broken_up_by_delaying_one_of_the_signal_producers, {
    constexpr Integer block_size = 3;
    PreparerWithCircularDependecy signal_producer_1;
    PreparerWithCircularDependecy signal_producer_2;
    PreparerWithCircularDependecy signal_producer_3;

    signal_producer_1.set_block_size(block_size);
    signal_producer_2.set_block_size(block_size);
    signal_producer_3.set_block_size(block_size);

    signal_producer_1.set_sample_rate(22050.0);
    signal_producer_2.set_sample_rate(22050.0);
    signal_producer_3.set_sample_rate(22050.0);

    signal_producer_1.dependency = &signal_producer_2;
    signal_producer_2.dependency = &signal_producer_3;
    signal_producer_3.dependency = &signal_producer_1;

    SignalProducer::produce<PreparerWithCircularDependecy>(&signal_producer_1, 1);
    SignalProducer::produce<PreparerWithCircularDependecy>(&signal_producer_2, 1);
    SignalProducer::produce<PreparerWithCircularDependecy>(&signal_producer_3, 1);

    assert_eq(3, signal_producer_1.value);
    assert_eq(2, signal_producer_2.value);
    assert_eq(1, signal_producer_3.value);
})


class EventTestSignalProducer : public SignalProducer
{
    public:
        static constexpr Event::Type SET_VALUE = 1;

        EventTestSignalProducer()
            : SignalProducer(1),
            render_calls(0),
            value(0.0)
        {
        }

        void schedule(Seconds const time_offset, Number const param)
        {
            SignalProducer::schedule(SET_VALUE, time_offset, 0, param, param);
        }

        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) {
            ++render_calls;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = value;
            }
        }

        void handle_event(Event const& event)
        {
            if (event.type != SignalProducer::EVT_CANCEL) {
                value = event.number_param_1;
            }
        }

        int render_calls;

    private:
        Number value;
};


typedef struct
{
    Seconds const time_offset;
    Number const param;
} TestEvent;


void test_event_handling(
        EventTestSignalProducer& signal_producer,
        Integer const block_size,
        Integer const rounds,
        TestEvent const events[],
        Integer const events_length,
        Seconds const cancellations[],
        Integer const cancellations_length,
        Sample const expected_samples[]
) {
    Integer const sample_count = rounds * block_size;
    Sample const* const* block;
    Buffer buffer(sample_count);
    Integer next_sample_index = 0;

    signal_producer.set_block_size(block_size);

    assert_false(signal_producer.has_events_after(0.0));

    for (Integer i = 0; i != events_length; ++i) {
        signal_producer.schedule(events[i].time_offset, events[i].param);
        assert_false(signal_producer.has_events_after(events[i].time_offset));
        assert_true(
            signal_producer.has_events_after(events[i].time_offset - DOUBLE_DELTA)
        );
        assert_eq(
            events[i].time_offset,
            signal_producer.get_last_event_time_offset(),
            DOUBLE_DELTA
        );
    }

    for (Integer i = 0; i != cancellations_length; ++i) {
        signal_producer.cancel_events(cancellations[i]);
        assert_false(signal_producer.has_events_after(cancellations[i]));
        assert_true(signal_producer.has_events_after(0.0));
        assert_true(
            signal_producer.has_events_after(cancellations[i] - DOUBLE_DELTA)
        );
        assert_eq(
            cancellations[i],
            signal_producer.get_last_event_time_offset(),
            DOUBLE_DELTA
        );
    }

    for (Integer round = 0; round != rounds; ++round) {
        block = SignalProducer::produce<EventTestSignalProducer>(
            &signal_producer, round, block_size
        );

        for (Integer i = 0; i != block_size; ++i) {
            buffer.samples[0][next_sample_index++] = block[0][i];
        }
    }

    assert_eq(expected_samples, buffer.samples[0], sample_count);
}


TEST(events_may_be_scheduled_multiple_rounds_in_the_future, {
    constexpr Integer block_size = 3;
    constexpr Integer rounds = 6;
    constexpr TestEvent events[] = {
        {0.3, 1.0},
        {1.1, 2.0},
        {1.3, 3.0},
    };
    constexpr Sample expected_samples[] = {
        0.0, 0.0, 0.0,
        1.0, 1.0, 1.0,
        1.0, 1.0, 1.0,
        1.0, 1.0, 2.0,
        2.0, 3.0, 3.0,
        3.0, 3.0, 3.0,
    };
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(10.0);

    test_event_handling(
        signal_producer, block_size, rounds, events, 3, NULL, 0, expected_samples
    );
})


TEST(multiple_events_can_occur_in_a_single_round, {
    constexpr Integer block_size = 10;
    constexpr Integer rounds = 1;
    constexpr TestEvent events[] = {
        {0.2, 1.0},
        {0.5, 2.0},
        {0.7, 3.0},
    };
    constexpr Sample expected_samples[] = {
        0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 3.0
    };
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(10.0);

    test_event_handling(
        signal_producer, block_size, rounds, events, 3, NULL, 0, expected_samples
    );
})


TEST(multiple_events_can_occur_at_the_same_time_offset, {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 1;
    constexpr TestEvent events[] = {
        {0.2, 1.0},
        {0.2, 2.0},
        {0.2, 3.0},
    };
    constexpr Sample expected_samples[] = {0.0, 0.0, 3.0, 3.0, 3.0};
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(10.0);

    test_event_handling(
        signal_producer, block_size, rounds, events, 3, NULL, 0, expected_samples
    );
    assert_eq(
        2,
        signal_producer.render_calls,
        "EventTestSignalProducer::render() got called unnecessarily many times"
    );
})


TEST(an_event_may_occur_between_samples, {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 2;
    constexpr TestEvent events[] = {
        {0.15, 1.0},
        {0.45, 2.0},
        {0.55, 3.0},
        {0.75, 4.0},
        {0.899, 5.0},
        {0.901, 6.0},
    };
    constexpr Sample expected_samples[] = {
        0.0, 0.0, 1.0, 1.0, 1.0,
        2.0, 3.0, 3.0, 4.0, 5.0,
    };
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(10.0);

    test_event_handling(
        signal_producer, block_size, rounds, events, 6, NULL, 0, expected_samples
    );
})


TEST(events_may_be_cancelled_following_a_given_point_in_time, {
    constexpr Integer block_size = 5;
    constexpr Integer rounds = 1;
    constexpr TestEvent events[] = {
        {0.1, 1.0},
        {0.2, 2.0},
        {0.3, 3.0},
    };
    constexpr Seconds cancellations[] = {0.2};
    constexpr Sample expected_samples[] = {0.0, 1.0, 1.0, 1.0, 1.0};
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(10.0);

    test_event_handling(
        signal_producer, block_size, rounds, events, 3, cancellations, 1, expected_samples
    );
})


TEST(events_may_be_scheduled_near_the_boundaries_of_a_block, {
    constexpr Integer block_size = 2;
    constexpr Integer rounds = 3;
    constexpr TestEvent events[] = {
        {0.0, 1.0},
        {0.5, 2.0},
        {0.99, 3.0},
        {1.49, 4.0},
        {1.51, 5.0},
        {2.01, 6.0},
    };
    constexpr Sample expected_samples[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(2.0);

    test_event_handling(
        signal_producer, block_size, rounds, events, 6, NULL, 0, expected_samples
    );
})


TEST(event_scheduling_is_relative_to_current_time, {
    constexpr Integer block_size = 3;
    constexpr Integer rounds = 5;
    constexpr Integer sample_count = rounds * block_size;
    constexpr Sample expected_samples[] = {
        1.0, 1.0, 2.0,
        3.0, 3.0, 4.0,
        4.0, 4.0, 4.0,
        4.0, 5.0, 5.0,
        5.0, 5.0, 5.0,
    };
    Integer next_sample_index = 0;
    Integer round = 0;
    Buffer rendered(sample_count);
    EventTestSignalProducer signal_producer;
    signal_producer.set_sample_rate(2.0);
    Sample const* const* block;

    signal_producer.schedule(0.0, 1.0);
    signal_producer.schedule(1.0, 2.0);
    signal_producer.schedule(1000.0, 1000.0);
    block = SignalProducer::produce<EventTestSignalProducer>(
        &signal_producer, round++, block_size
    );

    for (Integer i = 0; i != block_size; ++i) {
        rendered.samples[0][next_sample_index++] = block[0][i];
    }

    assert_eq(998.5, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);
    signal_producer.cancel_events(0.0);
    assert_eq(0.0, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);
    signal_producer.schedule(0.0, 3.0);
    assert_eq(0.0, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);
    signal_producer.schedule(1.0, 4.0);
    assert_eq(1.0, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);
    signal_producer.schedule(1000.0, 1000.0);
    assert_eq(1000.0, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);

    block = SignalProducer::produce<EventTestSignalProducer>(
        &signal_producer, round++, block_size
    );

    for (Integer i = 0; i != block_size; ++i) {
        rendered.samples[0][next_sample_index++] = block[0][i];
    }

    assert_eq(998.5, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);

    signal_producer.cancel_events(1.0);
    signal_producer.schedule(2.0, 5.0);
    signal_producer.schedule(1000.0, 1000.0);
    block = SignalProducer::produce<EventTestSignalProducer>(
        &signal_producer, round++, block_size
    );

    for (Integer i = 0; i != block_size; ++i) {
        rendered.samples[0][next_sample_index++] = block[0][i];
    }

    signal_producer.cancel_events(2.0);
    signal_producer.schedule(2.0, 6.0);
    signal_producer.schedule(1000.0, 1000.0);
    block = SignalProducer::produce<EventTestSignalProducer>(
        &signal_producer, round++, block_size
    );

    for (Integer i = 0; i != block_size; ++i) {
        rendered.samples[0][next_sample_index++] = block[0][i];
    }

    signal_producer.cancel_events(0.5);
    block = SignalProducer::produce<EventTestSignalProducer>(
        &signal_producer, round++, block_size
    );

    for (Integer i = 0; i != block_size; ++i) {
        rendered.samples[0][next_sample_index++] = block[0][i];
    }

    assert_eq(0.0, signal_producer.get_last_event_time_offset(), DOUBLE_DELTA);

    assert_eq(expected_samples, rendered.samples[0], sample_count);
})

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

#ifndef JS80P__SYNTH__SIGNAL_PRODUCER_HPP
#define JS80P__SYNTH__SIGNAL_PRODUCER_HPP

#include <vector>

#include "js80p.hpp"

#include "synth/queue.hpp"


namespace JS80P
{

/**
 * \brief Base class for everything which can generate audio signals.
 */
class SignalProducer
{
    public:
        class Event
        {
            public:
                typedef Byte Type;

                Event(Type const type);
                Event(Event const& event);
                Event(
                    Type const type,
                    Seconds const time_offset,
                    Integer const int_param = 0,
                    Number const number_param_1 = 0.0,
                    Number const number_param_2 = 0.0
                );

                Event& operator=(Event const& event);

                Seconds time_offset;
                Integer int_param;
                Number number_param_1;
                Number number_param_2;
                Type type;
        };

        static constexpr Integer DEFAULT_BLOCK_SIZE = 128;
        static constexpr Frequency DEFAULT_SAMPLE_RATE = 44100.0;

        static constexpr Event::Type EVT_CANCEL = 0;

        /**
         * \brief Orchestrate rendering signals and handling events.
         *
         * \note An instance method template calling virtual
         *       \c initialize_rendering(), \c render() and \c handle_event()
         *       methods would probably be simpler, but a static method template
         *       which is parametrized by the \c SignalProducer descendant class
         *       grants the compiler more freedom with inlining.
         *
         * \warning It's the caller's responsibility to ensure that
         *          \c sample_count is not greater than the current block size.
         *
         * \param signal_producer   The SignalProducer which is to render its
         *                          signal.
         *
         * \param round             Identifies the rendering round - a
         *                          \c SignalProducer's output may be needed by
         *                          multiple other \c SignalProducer objects,
         *                          but it is only rendered once per rendering
         *                          round.
         *
         * \param sample_count      How many samples to render per channel.
         *
         * \param external_buffer   Produce the output into this buffer insted
         *                          of the \c SignalProducer's own.
         *
         * \return                  The rendered buffer.
         */
        template<class SignalProducerClass>
        static Sample const* const* produce(
            SignalProducerClass* signal_producer,
            Integer const round,
            Integer const sample_count = -1
        );

        SignalProducer(
            Integer const channels, Integer const number_of_children = 0
        );
        virtual ~SignalProducer();

        Integer get_channels() const;

        virtual void set_sample_rate(Frequency const new_sample_rate);
        Frequency get_sample_rate() const;

        virtual void set_block_size(Integer const new_block_size);
        Integer get_block_size() const;

        Sample const* const* get_last_rendered_block(Integer& sample_count) const;

        Seconds sample_count_to_time_offset(Integer const sample_count) const;

        void schedule(
            Event::Type const type,
            Seconds const time_offset,
            Integer const int_param = 0,
            Number const number_param_1 = 0.0,
            Number const number_param_2 = 0.0
        );
        void cancel_events(Seconds const time_offset);
        bool has_events_after(Seconds const time_offset) const;
        Seconds get_last_event_time_offset() const;

        /**
         * \brief Implement preparations for sample rendering in this method,
         *        e.g. render necessary inputs and param buffers here. Return
         *        NULL if rendering samples is needed, return an already
         *        rendered buffer if calling \c render() is unnecessary (e.g.
         *        when a filter wants to return its input unaffected).
         *        Do not call this method directly!
         *
         * \note It would be nice to have \c protected access for
         *       \c initialize_rendering(), \c render() and \c handle_event(),
         *       and declare \c SignalProducer as a friend class in descendants,
         *       but I could not find a friend declaration that would be
         *       acceptable for GCC 3.4.2.
         *
         * \warning Do not call this method directly, use
         *          \c SignalProducer::produce<SignalProducerClass>()
         *          instead.
         * }
         */
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        );

        /**
         * \brief Implement sample rendering in this method. Do not call this
         *        method directly!
         *
         * \note It would be nice to have \c protected access for
         *       \c initialize_rendering(), \c render() and \c handle_event(),
         *       and declare \c SignalProducer as a friend class in descendants,
         *       but I could not find a friend declaration that would be
         *       acceptable for GCC 3.4.2.
         *
         * \warning Do not call this method directly, use
         *          \c SignalProducer::produce<SignalProducerClass>()
         *          instead.
         * }
         */
        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        /**
         * \brief Implement handling events in this method. Do not call this
         *        method directly!
         *
         * \note It would be nice to have \c protected access for
         *       \c initialize_rendering(), \c render() and \c handle_event(),
         *       and declare \c SignalProducer as a friend class in descendants,
         *       but I could not find a friend declaration that would be
         *       acceptable for GCC 3.4.2.
         *
         * \warning Do not call this method directly, use
         *          \c SignalProducer::produce<SignalProducerClass>()
         *          instead.
         * }
         */
        void handle_event(Event const& event);

    protected:
        Sample** reallocate_buffer(Sample** old_buffer) const;
        Sample** allocate_buffer() const;
        Sample** free_buffer(Sample** old_buffer) const;

        void render_silence(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        bool has_upcoming_events(Integer const sample_count) const;
        bool is_time_offset_before_sample_count(
            Seconds const time_offset,
            Integer const sample_count
        ) const;
        Integer sample_count_or_block_size(
            Integer const sample_count = -1
        ) const;
        void register_child(SignalProducer& signal_producer);

        Integer const channels;

        Queue<Event> events;
        Sample** buffer;
        Integer last_sample_count;
        Integer block_size;
        Frequency sample_rate;
        Frequency nyquist_frequency;
        Seconds sampling_period;
        Seconds current_time;
        Integer cached_round;

    private:
        typedef std::vector<SignalProducer*> Children;

        template<class SignalProducerClass>
        static void handle_events(
            SignalProducerClass* signal_producer,
            Integer const current_sample_index,
            Integer const sample_count,
            Integer& next_stop
        );

        Sample const* const* cached_buffer;
        Children children;
};

}

#endif

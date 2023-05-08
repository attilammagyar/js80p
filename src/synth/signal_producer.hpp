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

                Event(Type const type) noexcept;
                Event(Event const& event) noexcept;
                Event(
                    Type const type,
                    Seconds const time_offset,
                    Integer const int_param = 0,
                    Number const number_param_1 = 0.0,
                    Number const number_param_2 = 0.0
                ) noexcept;

                Event& operator=(Event const& event) noexcept;

                Seconds time_offset;
                Integer int_param;
                Number number_param_1;
                Number number_param_2;
                Type type;
        };

        static constexpr Integer DEFAULT_BLOCK_SIZE = 128;
        static constexpr Frequency DEFAULT_SAMPLE_RATE = 44100.0;

        /*
        Default to 60, so that 1 beat = 1 second, so when no BPM info is
        available, then toggling tempo-sync becomes no-op.
        */
        static constexpr Number DEFAULT_BPM = 60.0;

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
         * \param signal_producer   The \c SignalProducer which is about to
         *                          render a signal.
         *
         * \param round             Identifies the rendering round - a
         *                          \c SignalProducer's output may be needed by
         *                          multiple other \c SignalProducer objects,
         *                          but it is only rendered once per rendering
         *                          round.
         *
         * \param sample_count      How many samples to render per channel.
         *
         * \return                  The rendered buffer.
         */
        template<class SignalProducerClass>
        static Sample const* const* produce(
            SignalProducerClass* signal_producer,
            Integer const round,
            Integer const sample_count = -1
        ) noexcept;

        SignalProducer(
            Integer const channels,
            Integer const number_of_children = 0
        ) noexcept;

        virtual ~SignalProducer() noexcept;

        Integer get_channels() const noexcept;

        virtual void set_sample_rate(Frequency const new_sample_rate) noexcept;
        Frequency get_sample_rate() const noexcept;

        virtual void set_block_size(Integer const new_block_size) noexcept;
        Integer get_block_size() const noexcept;

        virtual void reset() noexcept;

        void set_bpm(Number const new_bpm) noexcept;
        Number get_bpm() const noexcept;


        Sample const* const* get_last_rendered_block(
            Integer& sample_count
        ) const noexcept;

        Seconds sample_count_to_time_offset(
            Integer const sample_count
        ) const noexcept;

        void schedule(
            Event::Type const type,
            Seconds const time_offset,
            Integer const int_param = 0,
            Number const number_param_1 = 0.0,
            Number const number_param_2 = 0.0
        ) noexcept;
        void cancel_events(Seconds const time_offset) noexcept;
        bool has_events_after(Seconds const time_offset) const noexcept;
        Seconds get_last_event_time_offset() const noexcept;

    protected:
        /**
         * \brief Implement preparations for sample rendering in this method,
         *        e.g. render necessary inputs and parameter buffers here.
         *        Return NULL if rendering samples is needed, return an already
         *        rendered buffer if calling \c render() is unnecessary (e.g.
         *        when a filter wants to return its input unaffected).
         */
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        /**
         * \brief Implement sample rendering in this method.
         */
        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        /**
         * \brief Implement handling events in this method.
         */
        void handle_event(Event const& event) noexcept;

        Sample** reallocate_buffer(Sample** old_buffer) const noexcept;
        Sample** allocate_buffer() const noexcept;
        Sample** free_buffer(Sample** old_buffer) const noexcept;

        void render_silence(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        bool has_upcoming_events(Integer const sample_count) const noexcept;
        bool is_time_offset_before_sample_count(
            Seconds const time_offset,
            Integer const sample_count
        ) const noexcept;
        Integer sample_count_or_block_size(
            Integer const sample_count = -1
        ) const noexcept;
        void register_child(SignalProducer& signal_producer) noexcept;

        Integer const channels;

        Queue<Event> events;
        Sample** buffer;
        Integer last_sample_count;
        Integer block_size;
        Frequency sample_rate;
        Seconds sampling_period;
        Frequency nyquist_frequency;
        Number bpm;
        Seconds current_time;
        Integer cached_round;
        Sample const* const* cached_buffer;

    private:
        typedef std::vector<SignalProducer*> Children;

        template<class SignalProducerClass>
        static void handle_events(
            SignalProducerClass* signal_producer,
            Integer const current_sample_index,
            Integer const sample_count,
            Integer& next_stop
        ) noexcept;

        Children children;
};

}

#endif

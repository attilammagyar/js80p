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

#ifndef JS80P__GUI__GUI_HPP
#define JS80P__GUI__GUI_HPP

#include "js80p.hpp"
#include "synth.hpp"


namespace JS80P
{

class GUI
{
    public:
        typedef void* Window; /// \brief GUI platform dependent parent window identifier
        typedef void* Application; /// \brief GUI platform dependent application handle

        class Controller
        {
            public:
                Controller(
                    int const index,
                    Synth::ControllerId const id,
                    char const* const long_name,
                    char const* const short_name
                );

                char const* const long_name;
                char const* const short_name;
                int const index;
                Synth::ControllerId const id;
        };

        static constexpr long int WIDTH = 980;
        static constexpr long int HEIGHT = 600;

        static constexpr int NO_CTLS = 0;
        static constexpr int ALL_CTLS = 97;
        static constexpr int LFO_CTLS = ALL_CTLS - Synth::ENVELOPES;
        static constexpr int MIDI_CTLS = ALL_CTLS - Synth::ENVELOPES - Synth::LFOS;

        static char const* const WAVEFORMS[];
        static int const WAVEFORMS_COUNT;

        static char const* const BIQUAD_FILTER_TYPES[];
        static int const BIQUAD_FILTER_TYPES_COUNT;

        static char const* const PARAMS[];

        static Controller const CONTROLLERS[];

        static GUI* create_instance(
            Application application,
            Window parent_window,
            Synth& synth
        );

        static Controller const* get_controller(Synth::ControllerId controller_id);

        virtual ~GUI() {}

        virtual void show() = 0;

    private:
        static void initialize_controllers_by_id();

        static Controller const* controllers_by_id[Synth::ControllerId::MAX_CONTROLLER_ID];
        static bool controllers_by_id_initialized;
};

}

#endif

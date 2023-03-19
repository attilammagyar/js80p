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

#include <cstddef>
#include <vector>

#include "js80p.hpp"
#include "synth.hpp"


namespace JS80P
{

class Background;
class ControllerSelector;
class ExportPatchButton;
class ExternallyCreatedWindow;
class ImportPatchButton;
class ParamEditor;
class TabBody;
class TabSelector;
class Widget;


class GUI
{
    public:
        typedef void* Window; ///< \brief GUI platform dependent window identifier.
        typedef void* PlatformData; ///< \brief GUI platform dependent data (e.g. HINSTANCE on Windows).
        typedef void* Bitmap; ///< \breif GUI platform dependent bitmap.

        typedef std::vector<Widget*> Widgets;

        typedef std::vector<ParamEditor*> ParamEditors;

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

        class Object
        {
            public:
                enum Type {
                    CLICKABLE = 0,
                    BITMAP = 1,
                    CLICKABLE_BITMAP = 2,
                };

                enum TextAlignment {
                    LEFT = 0,
                    CENTER = 1,
                };

                enum FontWeight {
                    NORMAL = 0,
                    BOLD = 1,
                };

                virtual void click();

                virtual ~Object();

            protected:
                Object();
                Object(int const left, int const top, int const width, int const height);

                virtual bool timer();
                virtual bool paint();
                virtual bool double_click();
                virtual bool mouse_down(int const x, int const y);
                virtual bool mouse_up(int const x, int const y);
                virtual bool mouse_move(int const x, int const y, bool const modifier);
                virtual bool mouse_leave(int const x, int const y);
                virtual bool mouse_wheel(Number const delta);

                int left;
                int top;
                int width;
                int height;

                bool is_clicking;
        };

        static constexpr long int WIDTH = 980;
        static constexpr long int HEIGHT = 600;

        static constexpr int NO_CTLS = 0;
        static constexpr int ALL_CTLS = 97;
        static constexpr int LFO_CTLS = ALL_CTLS - Synth::ENVELOPES;
        static constexpr int FLEX_CTLS = (
            ALL_CTLS - Synth::ENVELOPES - Synth::LFOS
        );
        static constexpr int MIDI_CTLS = (
            ALL_CTLS
            - Synth::ENVELOPES - Synth::LFOS - Synth::FLEXIBLE_CONTROLLERS
        );

        static char const* const WAVEFORMS[];
        static int const WAVEFORMS_COUNT;

        static char const* const BIQUAD_FILTER_TYPES[];
        static int const BIQUAD_FILTER_TYPES_COUNT;

        static char const* const PARAMS[];

        static Controller const CONTROLLERS[];

        static Controller const* get_controller(Synth::ControllerId const controller_id);

        static void refresh_param_editors(ParamEditors editors);
        static void refresh_controlled_param_editors(ParamEditors editors);

        static void param_ratio_to_str(
            Synth& synth,
            Synth::ParamId const param_id,
            Number const ratio,
            Number const scale,
            char const* const format,
            char const* const* const options,
            int const number_of_options,
            char* const buffer,
            size_t const buffer_size
        );

        static Number clamp_ratio(Number const ratio);

        GUI(
            PlatformData platform_data,
            Window parent_window,
            Synth& synth
        );

        ~GUI();

        void show();

    private:
        static void initialize_controllers_by_id();

        static void param_ratio_to_str_float(
            Synth& synth,
            Synth::ParamId const param_id,
            Number const ratio,
            Number const scale,
            char const* const format,
            char* const buffer,
            size_t const buffer_size
        );
        static void param_ratio_to_str_int(
            Synth& synth,
            Synth::ParamId const param_id,
            Number const ratio,
            char const* const* const options,
            int const number_of_options,
            char* const buffer,
            size_t const buffer_size
        );

        static Controller const* controllers_by_id[Synth::ControllerId::MAX_CONTROLLER_ID];
        static bool controllers_by_id_initialized;

        void build_about_body();
        void build_controllers_body();
        void build_effects_body();
        void build_envelopes_body();
        void build_lfos_body();
        void build_synth_body();

        Bitmap about_bitmap;
        Bitmap controllers_bitmap;
        Bitmap effects_bitmap;
        Bitmap envelopes_bitmap;
        Bitmap lfos_bitmap;
        Bitmap synth_bitmap;

        ControllerSelector* controller_selector;
        Background* background;
        TabBody* about_body;
        TabBody* controllers_body;
        TabBody* effects_body;
        TabBody* envelopes_body;
        TabBody* lfos_body;
        TabBody* synth_body;

        Synth& synth;
        JS80P::GUI::PlatformData platform_data;
        ExternallyCreatedWindow* parent_window;
};

}

#endif

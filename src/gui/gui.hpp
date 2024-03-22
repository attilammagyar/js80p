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
class ParamStateImages;
class KnobParamEditor;
class StatusLine;
class TabBody;
class TabSelector;
class DiscreteParamEditor;
class ToggleSwitchParamEditor;
class TuningSelector;
class Widget;
class WidgetBase;


class GUI
{
    public:
        typedef void* PlatformWidget; ///< \brief GUI platform dependent widget type.
        typedef void* PlatformData; ///< \brief GUI platform dependent data (e.g. HINSTANCE on Windows).
        typedef void* Image; ///< \brief GUI platform dependent image handle.

        typedef std::vector<WidgetBase*> Widgets;

        typedef std::vector<KnobParamEditor*> KnobParamEditors;
        typedef std::vector<ToggleSwitchParamEditor*> ToggleSwitchParamEditors;
        typedef std::vector<DiscreteParamEditor*> DiscreteParamEditors;

        typedef unsigned int Color;
        typedef unsigned char ColorComponent;

        enum ControllerCapability {
            NONE                = 0,
            MIDI_CONTROLLER     = 1 << 0,
            MACRO               = 1 << 1,
            LFO                 = 1 << 2,
            ENVELOPE            = 1 << 3,
            CHANNEL_PRESSURE    = 1 << 4,
        };

        class Controller
        {
            public:
                Controller(
                    int const index,
                    ControllerCapability const required_capability,
                    Synth::ControllerId const id,
                    char const* const long_name,
                    char const* const short_name
                );

                char const* const long_name;
                char const* const short_name;
                ControllerCapability const required_capability;
                int const index;
                Synth::ControllerId const id;
        };

        static constexpr long int WIDTH = 980;
        static constexpr long int HEIGHT = 600;

        static constexpr Frequency REFRESH_RATE = 18.0;
        static constexpr Seconds REFRESH_RATE_SECONDS = 1.0 / REFRESH_RATE;

        static constexpr int CONTROLLERS_COUNT = 121;

        static char const* const MODES[];
        static int const MODES_COUNT;

        static char const* const TUNINGS[];
        static int const TUNINGS_COUNT;

        static char const* const OSCILLATOR_INACCURACY_LEVELS[OscillatorInaccuracy::MAX_LEVEL + 1];
        static int const OSCILLATOR_INACCURACY_LEVELS_COUNT;

        static char const* const WAVEFORMS[];
        static int const WAVEFORMS_COUNT;

        static char const* const BIQUAD_FILTER_TYPES[];
        static int const BIQUAD_FILTER_TYPES_COUNT;

        static char const* const CHORUS_TYPES[];
        static int const CHORUS_TYPES_COUNT;

        static char const* const REVERB_TYPES[];
        static int const REVERB_TYPES_COUNT;

        static char const* const LFO_AMOUNT_ENVELOPES[];
        static int const LFO_AMOUNT_ENVELOPES_COUNT;

        static char const* const PARAMS[Synth::ParamId::PARAM_ID_COUNT];

        static Controller const CONTROLLERS[];

        static Controller const* get_controller(Synth::ControllerId const controller_id);

        static constexpr Color rgb(
            ColorComponent const red,
            ColorComponent const green,
            ColorComponent const blue
        );
        static constexpr ColorComponent red(Color const color);
        static constexpr ColorComponent green(Color const color);
        static constexpr ColorComponent blue(Color const color);

        static Color const TEXT_COLOR;
        static Color const TEXT_BACKGROUND;
        static Color const TEXT_HIGHLIGHT_COLOR;
        static Color const TEXT_HIGHLIGHT_BACKGROUND;
        static Color const STATUS_LINE_BACKGROUND;
        static Color const TOGGLE_OFF_COLOR;
        static Color const TOGGLE_ON_COLOR;

        static Color const CTL_COLOR_NONE_TEXT;
        static Color const CTL_COLOR_NONE_BG;
        static Color const CTL_COLOR_MIDI_CC_TEXT;
        static Color const CTL_COLOR_MIDI_CC_BG;
        static Color const CTL_COLOR_MIDI_SPECIAL_TEXT;
        static Color const CTL_COLOR_MIDI_SPECIAL_BG;
        static Color const CTL_COLOR_MIDI_LEARN_TEXT;
        static Color const CTL_COLOR_MIDI_LEARN_BG;
        static Color const CTL_COLOR_AFTERTOUCH_TEXT;
        static Color const CTL_COLOR_AFTERTOUCH_BG;
        static Color const CTL_COLOR_MACRO_TEXT;
        static Color const CTL_COLOR_MACRO_BG;
        static Color const CTL_COLOR_LFO_TEXT;
        static Color const CTL_COLOR_LFO_BG;
        static Color const CTL_COLOR_ENVELOPE_TEXT;
        static Color const CTL_COLOR_ENVELOPE_BG;

        static void param_ratio_to_str(
            Synth const& synth,
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

        static Color controller_id_to_text_color(Synth::ControllerId const controller_id);

        static Color controller_id_to_bg_color(Synth::ControllerId const controller_id);

        GUI(
            char const* sdk_version,
            PlatformData platform_data,
            PlatformWidget parent_window,
            Synth& synth,
            bool const show_vst_logo
        );

        ~GUI();

        void show();
        void idle();

        void set_status_line(char const* text);
        void redraw_status_line();

        bool is_mts_esp_connected() const;

        PlatformData get_platform_data() const;

    private:
        static void initialize_controllers_by_id();

        static void param_ratio_to_str_float(
            Synth const& synth,
            Synth::ParamId const param_id,
            Number const ratio,
            Number const scale,
            char const* const format,
            char* const buffer,
            size_t const buffer_size
        );
        static void param_ratio_to_str_int(
            Synth const& synth,
            Synth::ParamId const param_id,
            Number const ratio,
            char const* const* const options,
            int const number_of_options,
            char* const buffer,
            size_t const buffer_size
        );

        static Controller const* controllers_by_id[Synth::ControllerId::CONTROLLER_ID_COUNT];
        static bool controllers_by_id_initialized;

        void initialize();
        void destroy();

        void build_about_body(char const* sdk_version);
        void build_macros_1_body(ParamStateImages const* knob_states);
        void build_macros_2_body(ParamStateImages const* knob_states);
        void build_effects_body(ParamStateImages const* knob_states);

        void build_envelopes_1_body(
            ParamStateImages const* knob_states,
            ParamStateImages const* screw_states
        );

        void build_envelopes_2_body(
            ParamStateImages const* knob_states,
            ParamStateImages const* screw_states
        );

        void build_lfos_body(ParamStateImages const* knob_states);

        void build_synth_body(
            ParamStateImages const* knob_states,
            ParamStateImages const* screw_states
        );

        bool const show_vst_logo;

        Widget* dummy_widget;

        Image about_image;
        Image macros_1_image;
        Image macros_2_image;
        Image effects_image;
        Image envelopes_1_image;
        Image envelopes_2_image;
        Image lfos_image;
        Image synth_image;
        Image vst_logo_image;

        ParamStateImages const* knob_states;
        ParamStateImages const* screw_states;
        ControllerSelector* controller_selector;
        Background* background;
        TabBody* about_body;
        TabBody* macros_1_body;
        TabBody* macros_2_body;
        TabBody* effects_body;
        TabBody* envelopes_1_body;
        TabBody* envelopes_2_body;
        TabBody* lfos_body;
        TabBody* synth_body;
        StatusLine* status_line;

        Synth& synth;
        JS80P::GUI::PlatformData platform_data;
        ExternallyCreatedWindow* parent_window;
};


/**
 * \brief Base class for the platform-dependent \c Widget class.
 */
class WidgetBase
{
    public:
        enum Type {
            BACKGROUND = 1 << 0,
            CONTROLLER = 1 << 1,
            CONTROLLER_SELECTOR = 1 << 2,
            EXPORT_PATCH_BUTTON = 1 << 3,
            EXTERNALLY_CREATED_WINDOW = 1 << 4,
            IMPORT_PATCH_BUTTON = 1 << 5,
            KNOB = 1 << 6,
            KNOB_PARAM_EDITOR = 1 << 7,
            TAB_BODY = 1 << 8,
            TAB_SELECTOR = 1 << 9,
            ABOUT_TEXT = 1 << 10,
            STATUS_LINE = 1 << 11,
            TOGGLE_SWITCH = 1 << 12,
            DISCRETE_PARAM_EDITOR = 1 << 13,
        };

        enum TextAlignment {
            LEFT = 0,
            CENTER = 1,
            RIGHT = 2,
        };

        enum FontWeight {
            NORMAL = 0,
            BOLD = 1,
        };

        explicit WidgetBase(char const* const text);
        virtual ~WidgetBase();

        virtual int get_left() const;
        virtual int get_top() const;
        virtual WidgetBase* get_parent() const;

        virtual void set_text(char const* text);
        virtual char const* get_text() const;

        virtual GUI::Image load_image(
            GUI::PlatformData platform_data,
            char const* name
        );

        virtual GUI::Image copy_image_region(
            GUI::Image source,
            int const left,
            int const top,
            int const width,
            int const height
        );

        virtual void delete_image(GUI::Image image);

        virtual void show();
        virtual void hide();
        virtual void focus();
        virtual void bring_to_top();
        virtual void redraw();
        virtual WidgetBase* own(WidgetBase* widget);

        virtual GUI::Image set_image(GUI::Image image);
        virtual GUI::Image get_image() const;

        virtual GUI::PlatformWidget get_platform_widget();

        virtual void click();

    protected:
        WidgetBase(
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            Type const type
        );

        WidgetBase(
            GUI::PlatformData platform_data,
            GUI::PlatformWidget platform_widget,
            Type const type
        );

        virtual void destroy_children();

        virtual void set_up(GUI::PlatformData platform_data, WidgetBase* parent);

        virtual void set_gui(GUI& gui);

        /**
         * \brief Event handler for painting the widget on the screen.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool paint();

        /**
         * \brief Handle the double click event.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool double_click();

        /**
         * \brief Event handler to run when the (left) mouse button is
         *        pressed down.
         *
         * \param   x           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \param   y           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool mouse_down(int const x, int const y);

        /**
         * \brief Event handler to run when the (left) mouse button is released.
         *
         * \param   x           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \param   y           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool mouse_up(int const x, int const y);

        /**
         * \brief Event handler to run when the mouse cursor moves over the widget.
         *
         * \param   x           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \param   y           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \param   modifier    Tells if the modifier key (e.g. "Control" on PC)
         *                      is held when the event occurred.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool mouse_move(int const x, int const y, bool const modifier);

        /**
         * \brief Event handler to run when the mouse cursor leaves the widget's area.
         *
         * \param   x           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \param   y           Horizontal coordinate of the cursor relative to
         *                      the top left corner of the widget.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool mouse_leave(int const x, int const y);

        /**
         * \brief Event handler to run when the mouse wheel is scrolled while
         *        the cursor is over the widget.
         *
         * \param   delta       A floating point number between roughly -1.0 and
         *                      1.0, indicating the direction and relative speed
         *                      of the scrolling.
         *
         * \param   modifier    Tells if the modifier key (e.g. "Control" on PC)
         *                      is held when the event occurred.
         *
         * \return              Return \c true if the event was handled,
         *                      \c false if the platform's default event handler
         *                      needs to be run.
         */
        virtual bool mouse_wheel(Number const delta, bool const modifier);

        virtual void fill_rectangle(
            int const left,
            int const top,
            int const width,
            int const height,
            GUI::Color const color
        );

        virtual void draw_text(
            char const* const text,
            int const font_size_px,
            int const left,
            int const top,
            int const width,
            int const height,
            GUI::Color const color,
            GUI::Color const background,
            FontWeight const font_weight = FontWeight::NORMAL,
            int const padding = 0,
            TextAlignment const alignment = TextAlignment::CENTER
        );

        virtual void draw_image(
            GUI::Image image,
            int const left,
            int const top,
            int const width,
            int const height
        );

        Type const type;

        GUI::Widgets children;
        GUI::PlatformWidget platform_widget;
        GUI::PlatformData platform_data;
        GUI::Image image;
        GUI* gui;
        WidgetBase* parent;
        char const* text;

        int left;
        int top;
        int width;
        int height;

        bool is_clicking;
};

}

#endif

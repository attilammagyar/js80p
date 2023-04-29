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
class WidgetBase;


class GUI
{
    public:
        typedef void* PlatformWidget; ///< \brief GUI platform dependent widget type.
        typedef void* PlatformData; ///< \brief GUI platform dependent data (e.g. HINSTANCE on Windows).
        typedef void* Image; ///< \breif GUI platform dependent image handle.

        typedef std::vector<WidgetBase*> Widgets;

        typedef std::vector<ParamEditor*> ParamEditors;

        typedef unsigned int Color;
        typedef unsigned char ColorComponent;

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

        static constexpr Frequency REFRESH_RATE = 18.0;
        static constexpr Seconds REFRESH_RATE_SECONDS = 1.0 / REFRESH_RATE;

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

        static char const* const MODES[];
        static int const MODES_COUNT;

        static char const* const WAVEFORMS[];
        static int const WAVEFORMS_COUNT;

        static char const* const BIQUAD_FILTER_TYPES[];
        static int const BIQUAD_FILTER_TYPES_COUNT;

        static char const* const PARAMS[];

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

        static void refresh_param_editors(ParamEditors editors);
        static void refresh_controlled_param_editors(ParamEditors editors);

        static void param_ratio_to_str(
            Synth* synth,
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
            PlatformWidget parent_window,
            Synth* synth,
            bool const show_vst_logo
        );

        ~GUI();

        void show();
        void idle();

        PlatformData get_platform_data() const;

    private:
        static void initialize_controllers_by_id();

        static void param_ratio_to_str_float(
            Synth* synth,
            Synth::ParamId const param_id,
            Number const ratio,
            Number const scale,
            char const* const format,
            char* const buffer,
            size_t const buffer_size
        );
        static void param_ratio_to_str_int(
            Synth* synth,
            Synth::ParamId const param_id,
            Number const ratio,
            char const* const* const options,
            int const number_of_options,
            char* const buffer,
            size_t const buffer_size
        );

        static Controller const* controllers_by_id[Synth::ControllerId::MAX_CONTROLLER_ID];
        static bool controllers_by_id_initialized;

        void initialize();
        void destroy();

        void build_about_body();
        void build_controllers_body();
        void build_effects_body();
        void build_envelopes_body();
        void build_lfos_body();
        void build_synth_body();

        bool const show_vst_logo;

        Widget* dummy_widget;

        Image about_image;
        Image controllers_image;
        Image effects_image;
        Image envelopes_image;
        Image lfos_image;
        Image synth_image;
        Image vst_logo_image;

        ControllerSelector* controller_selector;
        Background* background;
        TabBody* about_body;
        TabBody* controllers_body;
        TabBody* effects_body;
        TabBody* envelopes_body;
        TabBody* lfos_body;
        TabBody* synth_body;

        Synth* synth;
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
            BACKGROUND = 1,
            CONTROLLER = 2,
            CONTROLLER_SELECTOR = 4,
            EXPORT_PATCH_BUTTON = 8,
            EXTERNALLY_CREATED_WINDOW = 16,
            IMPORT_PATCH_BUTTON = 32,
            KNOB = 64,
            PARAM_EDITOR = 128,
            TAB_BODY = 256,
            TAB_SELECTOR = 512,
            ABOUT_TEXT = 1024,
        };

        enum TextAlignment {
            LEFT = 0,
            CENTER = 1,
        };

        enum FontWeight {
            NORMAL = 0,
            BOLD = 1,
        };

        WidgetBase(char const* const text);
        virtual ~WidgetBase();

        virtual int get_left() const;
        virtual int get_top() const;
        virtual char const* get_text() const;
        virtual WidgetBase* get_parent() const;

        virtual GUI::Image load_image(
            GUI::PlatformData platform_data,
            char const* name
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

        virtual GUI::Image copy_image_region(
            GUI::Image source,
            int const left,
            int const top,
            int const width,
            int const height
        );

        Type const type;
        char const* const text;

        GUI::Widgets children;
        GUI::PlatformWidget platform_widget;
        GUI::PlatformData platform_data;
        GUI::Image image;
        WidgetBase* parent;

        int left;
        int top;
        int width;
        int height;

        bool is_clicking;
};

}

#endif

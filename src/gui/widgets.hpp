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

#ifndef JS80P__GUI__WIDGETS_HPP
#define JS80P__GUI__WIDGETS_HPP

#include <cmath>
#include <string>
#include <vector>

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P
{

class ExternallyCreatedWindow : public Widget
{
    public:
        ExternallyCreatedWindow(
            GUI::PlatformData platform_data,
            GUI::PlatformWidget window
        );
        virtual ~ExternallyCreatedWindow();
};


class TransparentWidget : public Widget
{
    public:
        TransparentWidget(
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            Type const type
        );

    protected:
        virtual bool paint() override;
};


class ImportPatchButton : public TransparentWidget
{
    public:
        ImportPatchButton(
            GUI& gui,
            int const left,
            int const top,
            int const width,
            int const height,
            Synth* synth,
            TabBody* synth_gui_body
        );

        void import_patch(char const* buffer, Integer const size) const;

    protected:
        virtual void click() override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        Synth* synth;
        TabBody* synth_gui_body;
};


class ExportPatchButton : public TransparentWidget
{
    public:
        ExportPatchButton(
            GUI& gui,
            int const left,
            int const top,
            int const width,
            int const height,
            Synth* synth
        );

    protected:
        virtual void click() override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        Synth* synth;
};


class ParamEditor;


class TabBody : public TransparentWidget
{
    public:
        static constexpr int LEFT = 0;
        static constexpr int TOP = 30;
        static constexpr int WIDTH = GUI::WIDTH;
        static constexpr int HEIGHT = GUI::HEIGHT - TOP;

        TabBody(char const* const text);

        using TransparentWidget::own;

        ParamEditor* own(ParamEditor* param_editor);
        ToggleSwitch* own(ToggleSwitch* param_editor);

        void stop_editing();

        void refresh_controlled_param_editors();
        void refresh_param_editors();
        void refresh_toggle_switches();

    private:
        GUI::ParamEditors param_editors;
        GUI::ToggleSwitches toggle_switches;
};


class Background : public Widget
{
    public:
        Background();
        ~Background();

        void replace_body(TabBody* new_body);
        void hide_body();
        void show_body();
        void refresh();

    private:
        static constexpr Integer FULL_REFRESH_TICKS = 3;

        TabBody* body;
        Integer next_full_refresh;
};


class TabSelector : public TransparentWidget
{
    public:
        static constexpr int LEFT = 3;
        static constexpr int TOP = 2;
        static constexpr int WIDTH = 114;
        static constexpr int HEIGHT = 28;

        TabSelector(
            Background* background,
            GUI::Image image,
            TabBody* tab_body,
            char const* const text,
            int const left
        );

    protected:
        virtual void click() override;

    private:
        Background* background;
        TabBody* tab_body;

        GUI::Image image;
};


class ControllerSelector : public Widget
{
    public:
        static constexpr int LEFT = 0;
        static constexpr int TOP = 0;
        static constexpr int WIDTH = GUI::WIDTH;
        static constexpr int HEIGHT = GUI::HEIGHT;
        static constexpr int TITLE_HEIGHT = 30;

        ControllerSelector(Background& background, Synth* synth);

        void show(
            Synth::ParamId const param_id,
            int const controller_choices,
            ParamEditor* param_editor
        );

        virtual void hide();

        void handle_selection_change(Synth::ControllerId const new_controller_id);

    protected:
        virtual void set_up(
            GUI::PlatformData platform_data,
            WidgetBase* parent
        ) override;

        virtual bool paint() override;

    private:
        static constexpr int TITLE_SIZE = 128;

        class Controller : public Widget
        {
            public:
                static constexpr int WIDTH = 240;
                static constexpr int HEIGHT = 21;

                Controller(
                    ControllerSelector& controller_selector,
                    GUI::ControllerCapability const required_capability,
                    char const* const text,
                    int const left,
                    int const top,
                    Synth::ControllerId const controller_id
                );

                void select();
                void unselect();

                GUI::ControllerCapability const required_capability;

            protected:
                virtual bool paint() override;
                virtual bool mouse_up(int const x, int const y) override;
                virtual bool mouse_move(int const x, int const y, bool const modifier) override;
                virtual bool mouse_leave(int const x, int const y) override;

            private:
                Synth::ControllerId const controller_id;

                ControllerSelector& controller_selector;
                bool is_selected;
                bool is_mouse_over;
        };

        char title[TITLE_SIZE];
        Background& background;
        Synth* synth;
        ParamEditor* param_editor;
        Controller* controllers[GUI::CONTROLLERS_COUNT];
        Synth::ParamId param_id;
        Synth::ControllerId selected_controller_id;
};


class ParamEditorKnobStates
{
    public:
        static constexpr int COUNT = 128;

        ParamEditorKnobStates(
            WidgetBase* widget,
            GUI::Image free_image,
            GUI::Image controlled_image,
            GUI::Image none_image
        );

        ~ParamEditorKnobStates();

        WidgetBase* widget;

        GUI::Image free_image;
        GUI::Image controlled_image;
        GUI::Image none_image;

        GUI::Image free_images[COUNT];
        GUI::Image controlled_images[COUNT];
};


class ParamEditor : public TransparentWidget
{
    public:
        static constexpr int WIDTH = 58;
        static constexpr int HEIGHT = 100;
        static constexpr int KNOB_WIDTH = 48;
        static constexpr int KNOB_HEIGHT = 48;

        ParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            ControllerSelector& controller_selector,
            Synth* synth,
            Synth::ParamId const param_id,
            int const controller_choices,
            char const* format,
            double const scale,
            ParamEditorKnobStates* knob_states
        );

        ParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            ControllerSelector& controller_selector,
            Synth* synth,
            Synth::ParamId const param_id,
            int const controller_choices,
            char const* const* const options,
            int const number_of_options,
            ParamEditorKnobStates* knob_states
        );

        virtual void set_up(
            GUI::PlatformData platform_data,
            WidgetBase* parent
        ) override;

        bool has_controller() const;

        void adjust_ratio(Number const ratio);
        void handle_ratio_change(Number const new_ratio);
        void handle_controller_change(Synth::ControllerId const new_controller_id);

        void refresh();
        void update_editor(
            Number const new_ratio,
            Synth::ControllerId const new_controller_id
        );
        void update_editor(Number const new_ratio);
        void update_editor(Synth::ControllerId const new_controller_id);
        void update_editor();

        void reset_default();

        void stop_editing();

        Synth::ParamId const param_id;

    protected:
        virtual bool paint() override;
        virtual bool mouse_up(int const x, int const y) override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        static constexpr Number KNOB_STATES_LAST_INDEX = (
            (Number)(ParamEditorKnobStates::COUNT - 1)
        );
        static constexpr int TEXT_MAX_LENGTH = 16;

        class Knob : public Widget
        {
            public:
                static constexpr int WIDTH = KNOB_WIDTH;
                static constexpr int HEIGHT = KNOB_HEIGHT;

                static constexpr Number MOUSE_WHEEL_COARSE_SCALE = 1.0 / 200.0;

                static constexpr Number MOUSE_WHEEL_FINE_SCALE = (
                    MOUSE_WHEEL_COARSE_SCALE / 50.0
                );

                static constexpr Number MOUSE_MOVE_COARSE_SCALE = (
                    1.0 / ((Number)HEIGHT * 5.0)
                );
                static constexpr Number MOUSE_MOVE_FINE_SCALE = (
                    MOUSE_MOVE_COARSE_SCALE / 50.0
                );

                Knob(
                    ParamEditor& editor,
                    GUI& gui,
                    char const* const text,
                    int const left,
                    int const top,
                    Number const steps,
                    ParamEditorKnobStates* knob_states
                );
                virtual ~Knob();

                virtual void set_up(
                    GUI::PlatformData platform_data,
                    WidgetBase* parent
                ) override;

                void update(Number const ratio);
                void update();
                void make_free();
                void make_controlled(Synth::ControllerId const controller_id);

                bool is_editing() const;
                void start_editing();
                void stop_editing();

            protected:
                virtual bool double_click() override;
                virtual bool mouse_down(int const x, int const y) override;
                virtual bool mouse_up(int const x, int const y) override;
                virtual bool mouse_move(int const x, int const y, bool const modifier) override;
                virtual bool mouse_leave(int const x, int const y) override;
                virtual bool mouse_wheel(Number const delta, bool const modifier) override;

            private:
                Number const steps;

                ParamEditor& editor;
                ParamEditorKnobStates* knob_states;
                GUI::Image knob_state;
                Number prev_x;
                Number prev_y;
                Number ratio;
                Number mouse_move_delta;
                bool is_controlled;
                bool is_controller_polyphonic;
                bool is_editing_;
        };

        void update_value_str();
        void update_controller_str();

        char const* const format;
        double const scale;

        char const* const* const options;
        int const number_of_options;
        int const value_font_size;
        int const controller_choices;

        ControllerSelector& controller_selector;
        ParamEditorKnobStates* knob_states;
        Synth* synth;
        Number default_ratio;
        Number ratio;
        Knob* knob;
        char value_str[TEXT_MAX_LENGTH];
        char controller_str[TEXT_MAX_LENGTH];
        Synth::ControllerId controller_id;
        bool has_controller_;
};


class AboutText : public Widget
{
    public:
        static constexpr int LEFT = 10;
        static constexpr int TOP = 10;
        static constexpr int WIDTH = 960;
        static constexpr int HEIGHT = 546;

        static constexpr int LOGO_WIDTH = 320;
        static constexpr int LOGO_HEIGHT = 299;

        static constexpr int FONT_SIZE = 14;
        static constexpr int TEXT_TOP = 23;
        static constexpr int LINE_HEIGHT = 25;
        static constexpr int EMPTY_LINE_HEIGHT = 12;
        static constexpr int PADDING = 10;

        static constexpr char const* TEXT = (
            "JS80P (version " JS80P_TO_STRING(JS80P_VERSION_STR) ")\n"
            "A MIDI driven, performance oriented, versatile synthesizer\n"
            "Copyright (C) 2023 Attila M. Magyar\n"
            "https://attilammagyar.github.io/js80p\n"
            "\n"
            "License: GNU General Public License Version 3\n"
            "https://www.gnu.org/licenses/gpl-3.0.en.html\n"
            "\n"
            "\n"
            "Usage\n"
            "\n"
            "Move the cursor over a knob, and use the mouse wheel\n"
            "for adjusting its value, or start dragging it.\n"
            "\n"
            "Hold down the \"Control\" key while adjusting a knob\n"
            "for fine grained adjustments.\n"
            "\n"
            "Double click on a knob to reset it to its default value.\n"
            "\n"
            "Click on the area below a knob to assign a controller to it.\n"
            "\n"
            "It is recommended to use a small buffer size for lower latency,\n"
            "for example, 3-6 milliseconds, or 128 or 256 samples\n"
            "at 44.1 kHz sample rate.\n"
        );

        AboutText(GUI::Image logo);

    protected:
        virtual bool paint() override;

    private:
        GUI::Image logo;
        std::vector<std::string> lines;
};


class StatusLine : public TransparentWidget
{
    public:
        static constexpr int LEFT = 690;
        static constexpr int TOP = 0;
        static constexpr int WIDTH = 290;
        static constexpr int HEIGHT = 24;

        StatusLine();

        virtual void set_text(char const* text) override;

    protected:
        virtual bool paint() override;
};


class ToggleSwitch : public TransparentWidget
{
    public:
        static constexpr int HEIGHT = 24;

        ToggleSwitch(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const box_left,
            Synth* synth,
            Synth::ParamId const param_id
        );

        virtual void set_up(
            GUI::PlatformData platform_data,
            WidgetBase* parent
        ) override;

        void refresh();

        Synth::ParamId const param_id;

    protected:
        virtual bool paint() override;
        virtual bool mouse_up(int const x, int const y) override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        bool is_editing() const;
        void start_editing();
        void stop_editing();

        int const box_left;

        Synth* synth;

        Number default_ratio;
        Number ratio;
        bool is_editing_;
};

}

#endif

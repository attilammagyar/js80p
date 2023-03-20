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

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P
{

class ExternallyCreatedWindow : public Widget
{
    public:
        ExternallyCreatedWindow(GUI::PlatformData platform_data, GUI::Window window);
        virtual ~ExternallyCreatedWindow();
};


class TransparentWidget : public Widget
{
    public:
        TransparentWidget(
            char const* const label,
            int const left,
            int const top,
            int const width,
            int const height
        );

    protected:
        virtual bool paint() override;
};


class ImportPatchButton : public TransparentWidget
{
    public:
        ImportPatchButton(
            int const left,
            int const top,
            int const width,
            int const height,
            Synth& synth,
            TabBody* synth_gui_body
        );

    protected:
        virtual void click() override;

    private:
        char buffer[Serializer::MAX_SIZE];
        Synth& synth;
        TabBody* synth_gui_body;
};


class ExportPatchButton : public TransparentWidget
{
    public:
        ExportPatchButton(
            int const left,
            int const top,
            int const width,
            int const height,
            Synth& synth
        );

    protected:
        virtual void click() override;

    private:
        Synth& synth;
};


class ParamEditor;


class TabBody : public TransparentWidget
{
    public:
        static constexpr int LEFT = 0;
        static constexpr int TOP = 30;
        static constexpr int WIDTH = GUI::WIDTH;
        static constexpr int HEIGHT = GUI::HEIGHT - TOP;

        TabBody(char const* const label);

        ParamEditor* own(ParamEditor* param_editor);

        void refresh_controlled_param_editors();
        void refresh_param_editors();

    private:
        GUI::ParamEditors param_editors;
};


class Background : public Widget
{
    public:
        static constexpr Frequency REFRESH_RATE = 18.0;
        static constexpr Frequency FULL_REFRESH_RATE = 3.0;

        Background();
        ~Background();

        void replace_body(TabBody* new_body);
        void hide_body();
        void show_body();

    protected:
        virtual void set_up(GUI::PlatformData platform_data, Widget* parent) override;

        virtual bool timer() override;

    private:
        static constexpr Integer FULL_REFRESH_TICKS = (
            (Integer)std::ceil(REFRESH_RATE / FULL_REFRESH_RATE)
        );

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
            GUI::Bitmap bitmap,
            TabBody* tab_body,
            char const* const label,
            int const left
        );

    protected:
        virtual void click() override;

    private:
        Background* background;
        TabBody* tab_body;

        GUI::Bitmap bitmap;
};


class ControllerSelector : public Widget
{
    public:
        static constexpr int LEFT = 0;
        static constexpr int TOP = 0;
        static constexpr int WIDTH = GUI::WIDTH;
        static constexpr int HEIGHT = GUI::HEIGHT;
        static constexpr int TITLE_HEIGHT = 30;

        ControllerSelector(Background& background, Synth& synth);

        void show(
            Synth::ParamId const param_id,
            int const controller_choices,
            ParamEditor* param_editor
        );

        virtual void hide();

        void handle_selection_change(Synth::ControllerId const new_controller_id);

    protected:
        virtual void set_up(GUI::PlatformData platform_data, Widget* parent) override;

        virtual bool paint() override;

    private:
        static constexpr int TITLE_SIZE = 128;

        class Controller : public Widget
        {
            public:
                static constexpr int WIDTH = 240;
                static constexpr int HEIGHT = 22;

                Controller(
                    ControllerSelector& controller_selector,
                    char const* const label,
                    int const left,
                    int const top,
                    Synth::ControllerId const controller_id
                );

                void select();
                void unselect();

            protected:
                virtual bool paint() override;
                virtual bool mouse_up(int const x, int const y) override;
                virtual bool mouse_move(int const x, int const y, bool const modifier) override;
                virtual bool mouse_leave(int const x, int const y) override;

            private:
                char const* const label;
                Synth::ControllerId const controller_id;
                ControllerSelector& controller_selector;
                bool is_selected;
                bool is_mouse_over;
        };

        char title[TITLE_SIZE];
        Background& background;
        Synth& synth;
        ParamEditor* param_editor;
        Controller* controllers[GUI::ALL_CTLS];
        Synth::ParamId param_id;
        Synth::ControllerId selected_controller_id;
};


class ParamEditor : public TransparentWidget
{
    public:
        static constexpr int WIDTH = 58;
        static constexpr int HEIGHT = 100;

        static void initialize_knob_states(
            GUI::Bitmap active,
            GUI::Bitmap inactive
        );

        static void free_knob_states();

        ParamEditor(
            char const* const label,
            int const left,
            int const top,
            ControllerSelector& controller_selector,
            Synth& synth,
            Synth::ParamId const param_id,
            int const controller_choices,
            char const* format,
            double const scale
        );

        ParamEditor(
            char const* const label,
            int const left,
            int const top,
            ControllerSelector& controller_selector,
            Synth& synth,
            Synth::ParamId const param_id,
            int const controller_choices,
            char const* const* const options,
            int const number_of_options
        );

        virtual void set_up(GUI::PlatformData platform_data, Widget* parent) override;

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

    protected:
        virtual bool paint() override;
        virtual bool mouse_up(int const x, int const y) override;

    private:
        static constexpr int KNOB_STATES_COUNT = 128;
        static constexpr Number KNOB_STATES_LAST_INDEX = (Number)(KNOB_STATES_COUNT - 1);
        static constexpr int TEXT_MAX_LENGTH = 16;

        static bool knob_states_initialization_complete;

        static GUI::Bitmap knob_states_active_bitmap;
        static GUI::Bitmap knob_states_inactive_bitmap;

        static GUI::Bitmap knob_states_active[KNOB_STATES_COUNT];
        static GUI::Bitmap knob_states_inactive[KNOB_STATES_COUNT];

        class Knob : public Widget
        {
            public:
                static constexpr int WIDTH = 48;
                static constexpr int HEIGHT = 48;

                static constexpr Number MOUSE_MOVE_COARSE_SCALE = (
                    1.0 / ((Number)HEIGHT * 3.0)
                );
                static constexpr Number MOUSE_MOVE_FINE_SCALE = (
                    MOUSE_MOVE_COARSE_SCALE / 20.0
                );

                Knob(
                    ParamEditor& editor,
                    char const* const label,
                    int const left,
                    int const top,
                    Number const steps
                );
                virtual ~Knob();

                virtual void set_up(GUI::PlatformData platform_data, Widget* parent) override;

                void update(Number const ratio);
                void update();
                void activate();
                void deactivate();

            protected:
                virtual bool double_click() override;
                virtual bool mouse_down(int const x, int const y) override;
                virtual bool mouse_up(int const x, int const y) override;
                virtual bool mouse_move(int const x, int const y, bool const modifier) override;
                virtual bool mouse_wheel(Number const delta) override;

            private:
                Number const steps;

                ParamEditor& editor;
                GUI::Bitmap knob_state;
                Number prev_x;
                Number prev_y;
                Number ratio;
                bool is_inactive;
        };

        void complete_knob_state_initialization();

        void update_value_str();
        void update_controller_str();

        Synth::ParamId const param_id;
        char const* const format;
        char const* const label;
        double const scale;

        char const* const* const options;
        int const number_of_options;
        int const value_font_size;
        int const controller_choices;

        ControllerSelector& controller_selector;
        Synth& synth;
        Number default_ratio;
        Number ratio;
        Knob* knob;
        int skip_refresh_calls;
        char value_str[TEXT_MAX_LENGTH];
        char controller_str[TEXT_MAX_LENGTH];
        Synth::ControllerId controller_id;
        bool has_controller_;
};

}

#endif
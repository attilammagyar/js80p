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

#ifndef JS80P__GUI__WIDGETS_HPP
#define JS80P__GUI__WIDGETS_HPP

#include <cstddef>
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
            Synth& synth,
            TabBody* synth_gui_body
        );

        void import_patch(char const* buffer, Integer const size) const;

    protected:
        virtual void click() override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        Synth& synth;
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
            Synth& synth
        );

    protected:
        virtual void click() override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        Synth& synth;
};


class TabBody : public TransparentWidget
{
    public:
        static constexpr int LEFT = 0;
        static constexpr int TOP = 30;
        static constexpr int WIDTH = GUI::WIDTH;
        static constexpr int HEIGHT = GUI::HEIGHT - TOP;

        explicit TabBody(char const* const text);

        using TransparentWidget::own;

        KnobParamEditor* own(KnobParamEditor* knob_param_editor);
        ToggleSwitchParamEditor* own(ToggleSwitchParamEditor* toggle_switch_param_editor);
        DiscreteParamEditor* own(DiscreteParamEditor* discrete_param_editor);

        void stop_editing();

        void refresh_controlled_knob_param_editors();
        void refresh_all_params();

    private:
        GUI::KnobParamEditors knob_param_editors;
        GUI::ToggleSwitchParamEditors toggle_switch_param_editors;
        GUI::DiscreteParamEditors discrete_param_editors;
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
        static constexpr int WIDTH = 72;
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

        ControllerSelector(Background& background, Synth& synth);

        void select_controller(
            Synth::ParamId const param_id,
            int const controller_choices,
            KnobParamEditor* knob_param_editor
        );

        virtual void hide() override;

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
                static constexpr int HEIGHT = 18;

                Controller(
                    ControllerSelector& controller_selector,
                    GUI::ControllerCapability const required_capability,
                    char const* const text,
                    int const left,
                    int const top,
                    int const width,
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
        Synth& synth;
        KnobParamEditor* knob_param_editor;
        Controller* controllers[GUI::CONTROLLERS_COUNT];
        Synth::ParamId param_id;
        Synth::ControllerId selected_controller_id;
};


class ParamStateImages
{
    public:
        ParamStateImages(
            WidgetBase* widget,
            GUI::Image free_image,
            GUI::Image controlled_image,
            GUI::Image synced_image,
            GUI::Image none_image,
            size_t const count,
            int const width,
            int const height
        );

        ~ParamStateImages();

        size_t ratio_to_index(Number const ratio) const;

        size_t const count;
        int const width;
        int const height;

        WidgetBase* widget;

        GUI::Image free_image;
        GUI::Image controlled_image;
        GUI::Image synced_image;
        GUI::Image none_image;

        GUI::Image* free_images;
        GUI::Image* controlled_images;
        GUI::Image* synced_images;

    private:
        GUI::Image* split_image(GUI::Image image) const;
        GUI::Image* free_images_(GUI::Image* images) const;

        size_t const last_index;
        Number const last_index_float;
};


class KnobParamEditor : public TransparentWidget
{
    public:
        KnobParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            int const knob_top,
            ControllerSelector& controller_selector,
            Synth& synth,
            Synth::ParamId const param_id,
            int const controller_choices,
            char const* format,
            double const scale,
            ParamStateImages const* knob_states,
            Synth::ParamId const scale_x4_toggle_param_id = Synth::ParamId::INVALID_PARAM_ID
        );

        KnobParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            int const knob_top,
            ControllerSelector& controller_selector,
            Synth& synth,
            Synth::ParamId const param_id,
            int const controller_choices,
            char const* const* const options,
            size_t const number_of_options,
            ParamStateImages const* knob_states,
            Synth::ParamId const scale_x4_toggle_param_id = Synth::ParamId::INVALID_PARAM_ID
        );

        void set_sync_param_id(Synth::ParamId const param_id);

        bool has_controller() const;

        void adjust_ratio(Number const ratio);
        void handle_ratio_change(Number const new_ratio);
        void handle_controller_change(Synth::ControllerId const new_controller_id);

        void refresh();

        void update_editor(
            Number const new_ratio,
            Synth::ControllerId const new_controller_id,
            bool const new_is_scaled_x4
        );

        void update_editor(Number const new_ratio);
        void update_editor(Synth::ControllerId const new_controller_id);
        void update_editor();

        void reset_default();

        void stop_editing();

        Synth::ParamId const param_id;
        Synth::ParamId const scale_x4_toggle_param_id;
        bool const is_continuous;

    protected:
        virtual void set_up(
            GUI::PlatformData platform_data,
            WidgetBase* parent
        ) override;

        virtual bool paint() override;
        virtual bool mouse_up(int const x, int const y) override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        static constexpr size_t TEXT_MAX_LENGTH = 16;
        static constexpr size_t TITLE_MAX_LENGTH = 64;

        static constexpr int VALUE_TEXT_HEIGHT = 20;
        static constexpr int CONTROLLER_TEXT_HEIGHT = 16;
        static constexpr int TEXTS_HEIGHT = VALUE_TEXT_HEIGHT + CONTROLLER_TEXT_HEIGHT;

        class Knob : public Widget
        {
            public:
                static constexpr Number MOUSE_WHEEL_COARSE_SCALE = 1.0 / 200.0;

                static constexpr Number MOUSE_WHEEL_FINE_SCALE = (
                    MOUSE_WHEEL_COARSE_SCALE / 50.0
                );

                static constexpr Number MOUSE_MOVE_COARSE_SCALE = 1.0 / 240.0;

                static constexpr Number MOUSE_MOVE_FINE_SCALE = (
                    MOUSE_MOVE_COARSE_SCALE / 50.0
                );

                Knob(
                    KnobParamEditor& editor,
                    GUI& gui,
                    char const* const text,
                    int const left,
                    int const top,
                    Number const steps,
                    ParamStateImages const* knob_states
                );

                virtual ~Knob();

                void set_sync_param_id(Synth::ParamId const param_id);

                void update(Number const ratio);
                void update();
                bool update_sync_status();
                void make_free();
                void make_controlled(Synth::ControllerId const controller_id);

                bool is_editing() const;
                void start_editing();
                void stop_editing();

            protected:
                virtual void set_up(
                    GUI::PlatformData platform_data,
                    WidgetBase* parent
                ) override;

                virtual bool double_click() override;
                virtual bool mouse_down(int const x, int const y) override;
                virtual bool mouse_up(int const x, int const y) override;
                virtual bool mouse_move(int const x, int const y, bool const modifier) override;
                virtual bool mouse_leave(int const x, int const y) override;
                virtual bool mouse_wheel(Number const delta, bool const modifier) override;

            private:
                Number const steps;

                ParamStateImages const* const knob_states;

                KnobParamEditor& editor;
                GUI::Image knob_state;
                Number prev_x;
                Number prev_y;
                Number ratio;
                Number mouse_move_delta;
                Synth::ParamId sync_param_id;
                bool is_controlled;
                bool is_controller_polyphonic;
                bool is_editing_;
                bool is_synced;
        };

        void update_value_str();
        void update_controller_str();

        bool should_be_scaled_x4() const;

        char const* const format;
        double const scale;

        Number const discrete_step_size;

        ParamStateImages const* knob_states;

        char const* const* const options;
        size_t const number_of_options;
        int const value_font_size;
        int const controller_choices;

        int const knob_top;
        bool const has_room_for_texts;
        bool const can_scale_x4;

        ControllerSelector& controller_selector;
        Synth& synth;
        Number ratio;
        Knob* knob;
        char value_str[TEXT_MAX_LENGTH];
        char controller_str[TEXT_MAX_LENGTH];
        char title[TITLE_MAX_LENGTH];
        Synth::ControllerId controller_id;
        bool has_controller_;
        bool is_scaled_x4;
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
        static constexpr int TEXT_TOP = 10;
        static constexpr int LINE_HEIGHT = 25;
        static constexpr int EMPTY_LINE_HEIGHT = 12;
        static constexpr int PADDING = 10;

        static constexpr char const* NAME = "JS80P";

        static constexpr char const* VERSION = (
            JS80P_TO_STRING(JS80P_VERSION_STR) ", "
            JS80P_TO_STRING(JS80P_TARGET_PLATFORM) ", "
            JS80P_TO_STRING(JS80P_INSTRUCTION_SET)
        );

        static constexpr char const* TEXT = (
            "A MIDI driven, performance oriented, versatile synthesizer\n"
            "Copyright (C) 2023, 2024 Attila M. Magyar and contributors\n"
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
            "A buffer size of around 6 ms (256 samples at 44.1 kHz sample\n"
            "rate) usually gives good performance and low latency.\n"
        );

        AboutText(char const* sdk_version, GUI::Image logo);

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


class ToggleSwitchParamEditor: public TransparentWidget
{
    public:
        ToggleSwitchParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            int const box_left,
            Synth& synth,
            Synth::ParamId const param_id
        );

        void refresh();

        bool is_on() const;

        Synth::ParamId const param_id;

    protected:
        virtual void set_up(
            GUI::PlatformData platform_data,
            WidgetBase* parent
        ) override;

        virtual bool paint() override;
        virtual bool mouse_up(int const x, int const y) override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;

    private:
        static constexpr size_t TITLE_MAX_LENGTH = 64;

        void update_title();
        bool is_editing() const;
        void start_editing();
        void stop_editing();

        int const box_left;

        Synth& synth;

        char title[TITLE_MAX_LENGTH];
        Number default_ratio;
        Number ratio;
        bool is_editing_;
};


class DiscreteParamEditor : public TransparentWidget
{
    public:
        DiscreteParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            int const value_left,
            int const value_width,
            Synth& synth,
            Synth::ParamId const param_id,
            char const* const* const options,
            size_t const number_of_options
        );

        DiscreteParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            int const value_left,
            int const value_width,
            Synth& synth,
            Synth::ParamId const param_id,
            ParamStateImages const* state_images
        );

        virtual void refresh();

        Synth::ParamId const param_id;

    protected:
        static constexpr size_t TEXT_MAX_LENGTH = 24;
        static constexpr size_t TITLE_MAX_LENGTH = 64;

        virtual void set_up(GUI::PlatformData platform_data, WidgetBase* parent) override;

        virtual bool paint() override;
        virtual bool mouse_up(int const x, int const y) override;
        virtual bool mouse_move(int const x, int const y, bool const modifier) override;
        virtual bool mouse_leave(int const x, int const y) override;
        virtual bool mouse_wheel(Number const delta, bool const modifier) override;

        void set_ratio(Number const new_ratio);

        virtual void update();
        void update_value_str(Byte const value);

        void update_title();

        bool is_editing() const;

        Synth& synth;
        char value_str[TEXT_MAX_LENGTH];
        char title[TITLE_MAX_LENGTH];
        Number ratio;

    private:
        DiscreteParamEditor(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            int const value_left,
            int const value_width,
            Synth& synth,
            Synth::ParamId const param_id,
            char const* const* const options,
            size_t const number_of_options,
            ParamStateImages const* state_images
        );

        void start_editing();
        void stop_editing();

        Number const step_size;

        ParamStateImages const* const state_images;

        char const* const* const options;
        size_t const number_of_options;

        int const value_left;
        int const value_width;

        bool is_editing_;
};


class TuningSelector : public DiscreteParamEditor
{
    public:
        static constexpr int WIDTH = 93;
        static constexpr int HEIGHT = 23;

        TuningSelector(
            GUI& gui,
            char const* const text,
            int const left,
            int const top,
            Synth& synth,
            Synth::ParamId const param_id
        );

        virtual void refresh() override;

    protected:
        virtual void update() override;

    private:
        bool is_mts_esp_connected;
};

}

#endif

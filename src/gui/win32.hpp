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

#ifndef JS80P__GUI__WIN32_HPP
#define JS80P__GUI__WIN32_HPP

#include <cmath>
#include <string>
#include <vector>

#include <windows.h>
#include <windowsx.h>

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P
{

class Widget
{
    public:
        class Text
        {
            public:
                Text();
                Text(std::string const text);
                ~Text();

                void set(std::string const text);

                char const* c_str() const;
                WCHAR const* c_wstr() const;

                LPCTSTR get() const;

            private:
                void free_wtext();

                std::string text;
                WCHAR* wtext;
        };

        static GUI::Bitmap load_bitmap(GUI::PlatformData platform_data, char const* name);
        static void delete_bitmap(GUI::Bitmap bitmap);

        static LRESULT process_message(
            HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam
        );

        virtual ~Widget();

        virtual void show();
        virtual void hide();
        virtual void click();

        void redraw();
        Widget* own(Widget* widget);
        GUI::Bitmap set_bitmap(GUI::Bitmap bitmap);

    protected:
        Widget();
        Widget(
            char const* const label,
            DWORD const dwStyle,
            int const left,
            int const top,
            int const width,
            int const height
        );
        Widget(GUI::PlatformData platform_data, GUI::Window window);

        virtual void set_up(GUI::PlatformData platform_data, Widget* parent);

        virtual LRESULT timer(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT paint(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT lbuttondblclk(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT lbuttondown(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT mousemove(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT mouseleave(UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual LRESULT mousewheel(UINT uMsg, WPARAM wParam, LPARAM lParam);

        LRESULT call_original_window_procedure(
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam
        );

        void fill_rectangle(
            HDC hdc,
            int const left,
            int const top,
            int const width,
            int const height,
            COLORREF const color
        );

        void draw_text(
            HDC hdc,
            Text const& text,
            int const font_size_px,
            int const left,
            int const top,
            int const width,
            int const height,
            COLORREF const color,
            COLORREF const background,
            int const weight = FW_NORMAL,
            int const padding = 0,
            UINT const format = DT_SINGLELINE | DT_CENTER | DT_VCENTER
        );

        GUI::Bitmap copy_bitmap_region(
            GUI::Bitmap source,
            int const left,
            int const top,
            int const width,
            int const height
        );

        GUI::Window window;
        GUI::PlatformData platform_data;
        GUI::Bitmap bitmap;
        Widget* parent;

        Text class_name;
        Text label;
        DWORD dwStyle;
        int left;
        int top;
        int width;
        int height;
        bool is_hidden;
        bool is_clicking;

    private:
        typedef std::vector<Widget*> Children;

        Children children;

        WNDPROC original_window_procedure;
};


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
        virtual LRESULT paint(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
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
        static constexpr Frequency FULL_REFRESH_RATE = 2.0;

        Background();
        ~Background();

        void replace_body(TabBody* new_body);
        void hide_body();
        void show_body();

    protected:
        virtual void set_up(GUI::PlatformData platform_data, Widget* parent) override;

        virtual LRESULT timer(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    private:
        static constexpr UINT_PTR TIMER_ID = 1;
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


class ImportPatchButton : public TransparentWidget
{
    public:
        static std::string const FILTER_STR;

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

        virtual void hide() override;

        void handle_selection_change(Synth::ControllerId const new_controller_id);

    protected:
        virtual void set_up(GUI::PlatformData platform_data, Widget* parent) override;

        virtual LRESULT paint(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    private:
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
                virtual LRESULT paint(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT mousemove(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT mouseleave(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

            private:
                Synth::ControllerId const controller_id;
                ControllerSelector& controller_selector;
                bool is_selected;
                bool is_mouse_over;
        };

        Text title;
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

        static GUI::Bitmap knob_states;
        static GUI::Bitmap knob_states_inactive;

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
        virtual LRESULT paint(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
        virtual LRESULT lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    private:
        class Knob : public Widget
        {
            public:
                static constexpr int WIDTH = 48;
                static constexpr int HEIGHT = 48;

                static constexpr Number KNOB_STATES_LAST_INDEX = 127.0;

                static constexpr Number MOUSE_MOVE_COARSE_SCALE = (
                    1.0 / ((Number)HEIGHT * 3.0)
                );
                static constexpr Number MOUSE_MOVE_FINE_SCALE = (
                    MOUSE_MOVE_COARSE_SCALE / 20.0
                );

                static constexpr Number MOUSE_WHEEL_COARSE_SCALE = (
                    1.0 / ((Number)WHEEL_DELTA * 100.0)
                );
                static constexpr Number MOUSE_WHEEL_FINE_SCALE = (
                    MOUSE_WHEEL_COARSE_SCALE / 25.0
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
                void capture_mouse();
                void release_captured_mouse();

                virtual LRESULT lbuttondblclk(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT lbuttondown(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT lbuttonup(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT mousemove(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT mouseleave(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
                virtual LRESULT mousewheel(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

            private:
                Number const steps;

                ParamEditor& editor;
                GUI::Bitmap knob_state;
                Number prev_x;
                Number prev_y;
                Number ratio;
                bool is_inactive;
                bool is_clicking;
                bool is_mouse_captured;
        };

        void update_value_str();
        void update_controller_str();

        Synth::ParamId const param_id;
        char const* format;
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
        Text value_str;
        Text controller_str;
        Synth::ControllerId controller_id;
        bool has_controller_;
};

}

#endif

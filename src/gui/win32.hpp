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

class Widget : public GUI::Object
{
    public:
        typedef COLORREF Color;

        static GUI::Bitmap load_bitmap(GUI::PlatformData platform_data, char const* name);
        static void delete_bitmap(GUI::Bitmap bitmap);

        static Color rgb(
            unsigned char const red,
            unsigned char const green,
            unsigned char const blue
        );

        static LRESULT process_message(
            HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam
        );

        virtual ~Widget();

        void show();
        void hide();

        void focus();
        void bring_to_top();
        void redraw();
        Widget* own(Widget* widget);
        GUI::Bitmap set_bitmap(GUI::Bitmap bitmap);

    protected:
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

        Widget();
        Widget(
            char const* const label,
            int const left,
            int const top,
            int const width,
            int const height,
            Type const type = Type::CLICKABLE
        );
        Widget(GUI::PlatformData platform_data, GUI::Window window);

        virtual bool paint() override;

        virtual void set_up(GUI::PlatformData platform_data, Widget* parent);

        void start_timer(Frequency const frequency);

        void fill_rectangle(
            int const left,
            int const top,
            int const width,
            int const height,
            Color const color
        );

        void draw_text(
            char const* const text,
            int const font_size_px,
            int const left,
            int const top,
            int const width,
            int const height,
            Color const color,
            Color const background,
            FontWeight const font_weight = FontWeight::NORMAL,
            int const padding = 0,
            TextAlignment const alignment = TextAlignment::CENTER
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
        HDC hdc;

    private:
        static constexpr UINT_PTR TIMER_ID = 1;

        static constexpr Number MOUSE_WHEEL_COARSE_SCALE = (
            1.0 / ((Number)WHEEL_DELTA * 100.0)
        );
        static constexpr Number MOUSE_WHEEL_FINE_SCALE = (
            MOUSE_WHEEL_COARSE_SCALE / 25.0
        );

        LRESULT call_original_window_procedure(
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam
        );

        void capture_mouse();
        void release_captured_mouse();

        void destroy_window();

        GUI::Widgets children;
        Text class_name;
        Text label;
        DWORD dwStyle;
        WNDPROC original_window_procedure;
        bool is_mouse_captured;
        bool is_timer_started;
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

}

#include "widgets.hpp"

#endif

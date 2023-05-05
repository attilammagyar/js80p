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
#include <commctrl.h>

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P
{

class Widget : public WidgetBase
{
    public:
        static COLORREF to_colorref(GUI::Color const color);

        static LRESULT process_message(
            HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam
        );

        Widget(char const* const text);
        virtual ~Widget();

        virtual void set_text(char const* text) override;

        virtual GUI::Image load_image(
            GUI::PlatformData platform_data,
            char const* name
        ) override;

        virtual void delete_image(GUI::Image image) override;

        virtual void show() override;
        virtual void hide() override;

        virtual void focus() override;
        virtual void bring_to_top() override;
        virtual void redraw() override;

    protected:
        class Text
        {
            public:
                Text();
                Text(std::string const text);
                ~Text();

                void set(std::string const text);

                char* c_str() const;
                WCHAR* c_wstr() const;

                LPCTSTR get_const() const;
                LPTSTR get() const;

            private:
                void free_buffers();

                std::string text;
                WCHAR* wtext;
                char* ctext;
        };

        static std::string const FILTER_STR;

        Widget(
            char const* const text,
            int const left,
            int const top,
            int const width,
            int const height,
            Type const type
        );
        Widget(
            GUI::PlatformData platform_data,
            GUI::PlatformWidget platform_widget,
            Type const type
        );

        virtual void set_up(
            GUI::PlatformData platform_data,
            WidgetBase* parent
        ) override;

        virtual void fill_rectangle(
            int const left,
            int const top,
            int const width,
            int const height,
            GUI::Color const color
        ) override;

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
        ) override;

        virtual void draw_image(
            GUI::Image image,
            int const left,
            int const top,
            int const width,
            int const height
        ) override;

        virtual GUI::Image copy_image_region(
            GUI::Image source,
            int const left,
            int const top,
            int const width,
            int const height
        ) override;

        HDC hdc;

    private:
        static constexpr Number MOUSE_WHEEL_SCALE = 1.0 / (Number)WHEEL_DELTA;

        static UINT_PTR next_timer_id;

        LRESULT call_original_window_procedure(
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam
        );

        void capture_mouse();
        void release_captured_mouse();

        GUI::PlatformWidget get_platform_widget() const;

        Text class_name;
        Text text_text;
        DWORD dwStyle;
        WNDPROC original_window_procedure;
        UINT_PTR timer_id;
        bool is_mouse_captured;
        bool is_timer_started;
};

}

#include "gui/widgets.hpp"

#endif

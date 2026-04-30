/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2026  Attila M. Magyar
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

#ifndef JS80P__GUI_MACOS_HPP
#define JS80P__GUI_MACOS_HPP

#include <cstddef>

#include "js80p.hpp"

#include "gui/gui.hpp"


namespace JS80P {

class Widget : public WidgetBase
{
    public:
        static bool notify_paint(Widget* widget);
        static bool notify_double_click(Widget* widget);
        static bool notify_mouse_down(Widget* widget, int const x, int const y);
        static bool notify_mouse_up(Widget* widget, int const x, int const y);

        static bool notify_mouse_move(
            Widget* widget,
            int const x,
            int const y,
            bool const modifier
        );

        static bool notify_mouse_leave(
            Widget* widget,
            int const x,
            int const y
        );

        static bool notify_mouse_wheel(
            Widget* widget,
            Number const delta,
            bool const modifier
        );

        static bool notify_timer_tick(Widget* widget);

        explicit Widget(char const* const text);

        virtual ~Widget();

        virtual void set_scale(Number const new_scale) override;

        virtual GUI::Image load_image(
            GUI::PlatformData platform_data,
            char const* const name
        ) override;

        GUI::Image copy_image_region(
            GUI::Image source,
            int const left,
            int const top,
            int const width,
            int const height
        ) override;

        GUI::Image downscale_image(
            GUI::Image source,
            int const old_width,
            int const old_height,
            int const new_width,
            int const new_height
        ) override;

        virtual void delete_image(GUI::Image image) override;

        virtual uint64_t monotonic_clock_ms() override;

        virtual void show() override;
        virtual void hide() override;

        virtual void focus() override;
        virtual void bring_to_top() override;
        virtual void redraw() override;

    protected:
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
            WidgetBase* const parent
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

    private:
        bool timer_tick();
};

}


extern "C" {
    JS80P::GUI::PlatformWidget js80p_create_platform_widget(
        JS80P::Widget* cpp_widget,
        int const left,
        int const top,
        int const width,
        int const height,
        JS80P::GUI::PlatformWidget parent,
        JS80P::WidgetBase::Type const type
    );

    void js80p_destroy_platform_widget(
        JS80P::GUI::PlatformWidget platform_widget
    );

    void js80p_widget_resize(
        JS80P::GUI::PlatformWidget platform_widget,
        int const left,
        int const top,
        int const width,
        int const height
    );

    void js80p_widget_show(JS80P::GUI::PlatformWidget platform_widget);
    void js80p_widget_hide(JS80P::GUI::PlatformWidget platform_widget);
    void js80p_widget_focus(JS80P::GUI::PlatformWidget platform_widget);
    void js80p_widget_bring_to_top(JS80P::GUI::PlatformWidget platform_widget);
    void js80p_widget_redraw(JS80P::GUI::PlatformWidget platform_widget);

    void js80p_widget_fill_rectangle(
        int const left,
        int const top,
        int const width,
        int const height,
        JS80P::GUI::Color const color
    );

    void js80p_widget_draw_text(
        JS80P::GUI::PlatformWidget platform_widget,
        char const* const text,
        int const font_size_px,
        int const left,
        int const top,
        int const width,
        int const height,
        JS80P::GUI::Color const color,
        JS80P::WidgetBase::FontWeight const font_weight,
        JS80P::WidgetBase::TextAlignment const alignment
    );

    JS80P::GUI::Image js80p_widget_load_image(char const* const name);

    JS80P::GUI::Image js80p_widget_copy_image_region(
        JS80P::GUI::Image source,
        int const left,
        int const top,
        int const width,
        int const height
    );

    JS80P::GUI::Image js80p_widget_downscale_image(
        JS80P::GUI::Image source,
        int const old_width,
        int const old_height,
        int const new_width,
        int const new_height
    );

    void js80p_widget_delete_image(JS80P::GUI::Image image);

    void js80p_widget_draw_image(
        JS80P::GUI::Image image,
        int const left,
        int const top,
        int const width,
        int const height
    );

    bool js80p_import_patch(
        char* const buffer,
        size_t* length,
        size_t const max_length
    );

    void js80p_export_patch(char const* const buffer, size_t const length);
}


#include "gui/widgets.hpp"

#endif

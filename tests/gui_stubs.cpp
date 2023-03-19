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
#ifndef JS80P__GUI_STUBS_CPP
#define JS80P__GUI_STUBS_CPP

#include "js80p.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P {

class Widget : public GUI::Object
{
    public:
        typedef Integer Color;

        static GUI::Bitmap load_bitmap(
                GUI::PlatformData platform_data,
                char const* name
        ) {
            return (GUI::Bitmap)new DummyObject();
        }

        static void delete_bitmap(GUI::Bitmap bitmap)
        {
            delete (DummyObject*)bitmap;
        }

        static Color rgb(
            unsigned char const red,
            unsigned char const green,
            unsigned char const blue
        ) {
            return 0;
        }

        virtual ~Widget();

        void show()
        {
        }

        void hide()
        {
        }

        void focus()
        {
        }

        void bring_to_top()
        {
        }

        void redraw()
        {
        }

        Widget* own(Widget* widget);

        GUI::Bitmap set_bitmap(GUI::Bitmap bitmap);

    protected:
        Widget() : Object(), window(NULL), platform_data(NULL), bitmap(NULL)
        {
        }

        Widget(
                char const* const label,
                int const left,
                int const top,
                int const width,
                int const height,
                Type const type = Type::CLICKABLE
        ) : Object(left, top, width, height),
            window(NULL),
            platform_data(NULL),
            bitmap(NULL)
        {
        }

        Widget(GUI::PlatformData platform_data, GUI::Window window)
            : Object(),
            window(window),
            platform_data(platform_data),
            bitmap(NULL)
        {
        }

        virtual bool paint() override
        {
            return false;
        }

        virtual void set_up(GUI::PlatformData platform_data, Widget* parent)
        {
        }

        void start_timer(Frequency const frequency)
        {
        }

        void fill_rectangle(
                int const left,
                int const top,
                int const width,
                int const height,
                Color const color
        ) {
        }

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
        ) {
        }

        GUI::Bitmap copy_bitmap_region(
                GUI::Bitmap source,
                int const left,
                int const top,
                int const width,
                int const height
        ) {
            return (GUI::Bitmap)new DummyObject();
        }

        GUI::Window window;
        GUI::PlatformData platform_data;
        GUI::Bitmap bitmap;

    private:
        class DummyObject
        {
        };

        void destroy_window()
        {
            window = NULL;
        }

        void release_captured_mouse()
        {
        }

        GUI::Widgets children;
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
        virtual void click() override
        {
        }

    private:
        Synth& synth;
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
        virtual void click() override
        {
        }

    private:
        Synth& synth;
        TabBody* synth_gui_body;
};

}

#endif

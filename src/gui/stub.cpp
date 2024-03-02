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
#ifndef JS80P__GUI_STUB_CPP
#define JS80P__GUI_STUB_CPP

#include "js80p.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P {

void GUI::idle()
{
}


void GUI::initialize()
{
}


void GUI::destroy()
{
}


class Widget : public WidgetBase
{
    public:
        explicit Widget(char const* const text) : WidgetBase(text)
        {
        }

        virtual ~Widget()
        {
            destroy_children();
        }

        virtual GUI::Image load_image(
                GUI::PlatformData platform_data,
                char const* name
        ) override {
            return (GUI::Image)new DummyObject();
        }

        virtual void delete_image(GUI::Image image) override
        {
            delete (DummyObject*)image;
        }

    protected:
        Widget(
                char const* const text,
                int const left,
                int const top,
                int const width,
                int const height,
                Type const type
        ) : WidgetBase(text, left, top, width, height, type)
        {
        }

        Widget(
                GUI::PlatformData platform_data,
                GUI::PlatformWidget platform_widget,
                Type const type
        ) : WidgetBase(platform_data, platform_widget, type)
        {
        }

        GUI::Image copy_image_region(
                GUI::Image source,
                int const left,
                int const top,
                int const width,
                int const height
        ) {
            return (GUI::Image)new DummyObject();
        }

    private:
        class DummyObject
        {
        };
};

}


#include "gui/widgets.hpp"
#include "gui/gui.cpp"


namespace JS80P {

void ImportPatchButton::click()
{
}


void ExportPatchButton::click()
{
}

}


#include "gui/widgets.cpp"

#endif

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

class Widget
{
    public:
        static GUI::Bitmap load_bitmap(
                GUI::PlatformData platform_data,
                char const* name
        ) {
            return NULL;
        }

        static void delete_bitmap(GUI::Bitmap bitmap)
        {
        }

        Widget* own(Widget* widget)
        {
            return NULL;
        }

        void show()
        {
        }

        void hide()
        {
        }

        GUI::Bitmap set_bitmap(GUI::Bitmap bitmap)
        {
            return NULL;
        }
};


class Background : public Widget
{
    public:
        void replace_body(TabBody* new_body)
        {
        }
};


class ControllerSelector : public Widget
{
    public:
        ControllerSelector(Background& background, Synth& synth)
        {
        }
};


class ExportPatchButton : public Widget
{
    public:
        ExportPatchButton(
                int const left,
                int const top,
                int const width,
                int const height,
                Synth& synth
        ) {
        }
};


class ExternallyCreatedWindow : public Widget
{
    public:
        ExternallyCreatedWindow(
                GUI::PlatformData platform_data,
                GUI::Window parent_window
        ) {
        }
};


class ImportPatchButton : public Widget
{
    public:
        ImportPatchButton(
                int const left,
                int const top,
                int const width,
                int const height,
                Synth& synth,
                TabBody* synth_gui_body
        ) {
        }
};


class ParamEditor : public Widget
{
    public:
        static constexpr int WIDTH = 58;
        static constexpr int HEIGHT = 100;

        static void initialize_knob_states(
                GUI::Bitmap active,
                GUI::Bitmap inactive
        ) {
        }

        static void free_knob_states()
        {
        }

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
        ) {
        }

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
        ) {
        }

        void refresh()
        {
        }

        bool has_controller() const
        {
            return false;
        }
};


class TabBody : public Widget
{
    public:
        TabBody(char const* const label)
        {
        }
};


class TabSelector : public Widget
{
    public:
        static constexpr int LEFT = 3;
        static constexpr int WIDTH = 114;

        TabSelector(
                Background* background,
                GUI::Bitmap bitmap,
                TabBody* tab_body,
                char const* const label,
                int const left
        ) {
        }
};

}

#endif

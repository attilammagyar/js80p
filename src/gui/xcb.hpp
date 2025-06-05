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

#ifndef JS80P__GUI__XCB_HPP
#define JS80P__GUI__XCB_HPP

#include <map>
#include <string>
#include <sys/types.h>

#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <cairo/cairo-xcb.h>

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


namespace JS80P
{

class Widget;


class XcbPlatform
{
    public:
        static xcb_window_t gui_platform_widget_to_xcb_window(
            GUI::PlatformWidget platform_widget
        );

        static GUI::PlatformWidget xcb_window_to_gui_platform_widget(
            xcb_window_t window_id
        );

        XcbPlatform();
        ~XcbPlatform();

        xcb_connection_t* get_connection();
        int get_fd() const;
        xcb_screen_t* get_screen() const;
        xcb_visualid_t get_screen_root_visual_id() const;
        xcb_visualtype_t* get_screen_root_visual() const;

        cairo_font_face_t* get_font_face(
            cairo_t* const cairo,
            WidgetBase::FontWeight const weight
        );

        void register_widget(xcb_window_t window_id, Widget* const widget);
        Widget* find_widget(xcb_window_t window_id) const;
        void unregister_widget(xcb_window_t window_id);

        void export_patch(std::string const& patch);
        void import_patch(ImportPatchButton* const import_patch_button);
        void handle_file_selector_dialog();
        void cancel_file_selector_dialog();
        bool is_file_selector_dialog_open() const;

    private:
        enum FileSelectorDialogType {
            NONE = 0,
            EXPORT = 1,
            IMPORT = 2,
        };

        class Pipe
        {
            public:
                Pipe();
                Pipe(Pipe const& pipe);
                ~Pipe();

                void close_read_fd();
                void close_write_fd();

                int read_fd;
                int write_fd;
                bool is_usable;
        };

        static char const* const FONT_TEST_STR;
        static char const* const FONTS[];

        static char const* const KDIALOG[];
        static char const* const KDIALOG_SAVE_ARGUMENTS[];
        static char const* const KDIALOG_OPEN_ARGUMENTS[];

        static char const* const ZENITY[];
        static char const* const ZENITY_SAVE_ARGUMENTS[];
        static char const* const ZENITY_OPEN_ARGUMENTS[];

        typedef std::map<xcb_window_t, Widget*> WindowIdToWidgetMap;

        xcb_visualtype_t* find_screen_root_visual() const;

        cairo_font_face_t* find_narrowest_font(
            cairo_t* const cairo,
            cairo_font_weight_t weight
        );

        char const* find_executable(char const* const* const alternatives) const;

        void start_file_selector_dialog(
            char const* const executable,
            char const* const* const arguments
        );

        void clear_active_file_selector_dialog_data();

        void build_file_selector_argv(
            char const* const executable,
            char const* const* const arguments,
            std::vector<char*>& argv
        ) const;

        void build_file_selector_env(std::vector<char*>& env) const;

        void run_file_selector_child_process(
            std::vector<char*> const& argv,
            std::vector<char*> const& env,
            Pipe* const pipe
        ) const;

        void read_file_selector_output();
        bool has_file_selector_exited(int* const exit_code) const;
        void finish_exporting_patch();
        void finish_importing_patch();

        WindowIdToWidgetMap widgets;
        std::string file_path;
        std::string file_contents;
        xcb_connection_t* connection;
        xcb_screen_t* screen;
        xcb_visualtype_t* screen_root_visual;
        cairo_font_face_t* font_face_normal;
        cairo_font_face_t* font_face_bold;
        ImportPatchButton* import_patch_button;
        Pipe* active_file_selector_dialog_pipe;
        FileSelectorDialogType active_file_selector_dialog_type;
        pid_t active_file_selector_dialog_pid;
        int xcb_fd;
};


class Widget : public WidgetBase
{
    public:
        static void process_events(XcbPlatform* const xcb);

        explicit Widget(char const* const text);
        virtual ~Widget();

        virtual GUI::Image load_image(
            GUI::PlatformData platform_data,
            char const* const name
        ) override;

        virtual void delete_image(GUI::Image image) override;

        virtual void show() override;
        virtual void hide() override;

        virtual void focus() override;
        virtual void bring_to_top() override;
        virtual void redraw() override;

        virtual GUI::Image set_image(GUI::Image image) override;

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

        virtual bool paint() override;

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

        XcbPlatform* xcb() const;

    private:
        static constexpr xcb_timestamp_t DOUBLE_CLICK_TIME_DELTA = 500;
        static constexpr int DOUBLE_CLICK_POS_DELTA = 5;
        static constexpr double COLOR_COMPONENT_SCALE = 1.0 / 255.0;
        static constexpr unsigned int TRANSPARENT_WIDGETS = (
            0
            | Type::EXPORT_PATCH_BUTTON
            | Type::IMPORT_PATCH_BUTTON
            | Type::KNOB_PARAM_EDITOR
            | Type::TAB_BODY
            | Type::TAB_SELECTOR
            | Type::STATUS_LINE
            | Type::TOGGLE_SWITCH
            | Type::DISCRETE_PARAM_EDITOR
        );

        class Resource;

        typedef std::map<std::string, Resource> StringToImageMap;

        static StringToImageMap const IMAGES;

        class Resource
        {
            public:
                Resource(unsigned char* const start, unsigned char* const end);

                /*
                These should be unsigned char const* const, but
                cairo_read_func_t wants unsigned char*.
                */
                unsigned char* const start;
                unsigned char* const end;
        };

        class PNGStreamState
        {
            public:
                PNGStreamState(
                    unsigned char* const start,
                    unsigned char* const end
                );

                unsigned char* const end;
                unsigned char* data;
        };

        static void process_all_events(
            XcbPlatform* const xcb,
            xcb_connection_t* const xcb_connection
        );

        static void process_non_editing_events(
            XcbPlatform* const xcb,
            xcb_connection_t* const xcb_connection
        );

        static cairo_status_t read_png_stream_from_array(
            void *closure,
            unsigned char* const data,
            unsigned int length
        );

        static void handle_error_event(
            XcbPlatform const* const xcb,
            xcb_generic_error_t const* const error
        );

        static void handle_expose_event(
            XcbPlatform const* const xcb,
            xcb_expose_event_t const* const event
        );

        static void handle_button_press_event(
            XcbPlatform const* const xcb,
            xcb_button_press_event_t const* const event
        );

        static void handle_button_release_event(
            XcbPlatform const* const xcb,
            xcb_button_release_event_t const* const event
        );

        static void handle_enter_notify_event(
            XcbPlatform const* const xcb,
            xcb_enter_notify_event_t const* const event
        );

        static void handle_motion_notify_event(
            XcbPlatform const* const xcb,
            xcb_motion_notify_event_t const* const event
        );

        static void handle_leave_notify_event(
            XcbPlatform const* const xcb,
            xcb_leave_notify_event_t const* const event
        );

        static void handle_client_message_event(
            XcbPlatform const* const xcb,
            xcb_client_message_event_t const* const event
        );

        static void handle_destroy_notify_event(
            XcbPlatform const* const xcb,
            xcb_destroy_notify_event_t const* const event
        );

        static bool is_double_click(
            Widget const* const widget,
            xcb_timestamp_t const time,
            int const x,
            int const y
        );

        static bool is_modifier_active(uint16_t event_state);

        void initialize();

        xcb_connection_t* xcb_connection() const;
        xcb_window_t window_id() const;

        void update_fake_transparency();
        cairo_surface_t* find_first_parent_image();
        void destroy_fake_transparent_background();

        cairo_surface_t* cairo_surface;
        cairo_device_t* cairo_device;
        cairo_surface_t* fake_transparent_background;
        cairo_surface_t* fake_transparent_background_source;
        cairo_t* cairo;
        WidgetBase* first_parent_with_image;
        int fake_transparent_background_left;
        int fake_transparent_background_top;
        int mouse_down_x;
        int mouse_down_y;
        xcb_timestamp_t mouse_down_time;
        bool need_to_destroy_window;
        bool is_transparent;
        bool is_hidden;
};

}

#include "gui/widgets.hpp"

#endif

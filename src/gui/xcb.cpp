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

#ifndef JS80P__GUI__X11_CPP
#define JS80P__GUI__X11_CPP

#include <cerrno>
#include <cstring>
#include <csignal>
#include <istream>
#include <fstream>
#include <ostream>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gui/xcb.hpp"

#include "gui/gui.cpp"


extern unsigned char _binary_gui_img_about_png_start;
extern unsigned char _binary_gui_img_about_png_end;
extern unsigned char _binary_gui_img_controllers_png_start;
extern unsigned char _binary_gui_img_controllers_png_end;
extern unsigned char _binary_gui_img_effects_png_start;
extern unsigned char _binary_gui_img_effects_png_end;
extern unsigned char _binary_gui_img_envelopes_png_start;
extern unsigned char _binary_gui_img_envelopes_png_end;
extern unsigned char _binary_gui_img_knob_states_free_png_start;
extern unsigned char _binary_gui_img_knob_states_free_png_end;
extern unsigned char _binary_gui_img_knob_states_controlled_png_start;
extern unsigned char _binary_gui_img_knob_states_controlled_png_end;
extern unsigned char _binary_gui_img_lfos_png_start;
extern unsigned char _binary_gui_img_lfos_png_end;
extern unsigned char _binary_gui_img_synth_png_start;
extern unsigned char _binary_gui_img_synth_png_end;
extern unsigned char _binary_gui_img_vst_logo_png_start;
extern unsigned char _binary_gui_img_vst_logo_png_end;


extern "C" char** environ;


namespace JS80P
{

std::map<std::string, Widget::Resource> const Widget::IMAGES{
    {
        "ABOUT",
        Widget::Resource(
            &_binary_gui_img_about_png_start,
            &_binary_gui_img_about_png_end
        )
    },
    {
        "CONTROLLERS",
        Widget::Resource(
            &_binary_gui_img_controllers_png_start,
            &_binary_gui_img_controllers_png_end
        )
    },
    {
        "EFFECTS",
        Widget::Resource(
            &_binary_gui_img_effects_png_start,
            &_binary_gui_img_effects_png_end
        )
    },
    {
        "ENVELOPES",
        Widget::Resource(
            &_binary_gui_img_envelopes_png_start,
            &_binary_gui_img_envelopes_png_end
        )
    },
    {
        "KNOBSTATESFREE",
        Widget::Resource(
            &_binary_gui_img_knob_states_free_png_start,
            &_binary_gui_img_knob_states_free_png_end
        )
    },
    {
        "KNOBSTATESCONTROLLED",
        Widget::Resource(
            &_binary_gui_img_knob_states_controlled_png_start,
            &_binary_gui_img_knob_states_controlled_png_end
        )
    },
    {
        "LFOS",
        Widget::Resource(
            &_binary_gui_img_lfos_png_start,
            &_binary_gui_img_lfos_png_end
        )
    },
    {
        "SYNTH",
        Widget::Resource(
            &_binary_gui_img_synth_png_start,
            &_binary_gui_img_synth_png_end
        )
    },
    {
        "VSTLOGO",
        Widget::Resource(
            &_binary_gui_img_vst_logo_png_start,
            &_binary_gui_img_vst_logo_png_end
        )
    },
};


char const* const XcbPlatform::FONT_TEST_STR = (
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    " +-*/=()[]{}<>%&.;:,?!'\"^~_#@$\\"
);

char const* const XcbPlatform::FONTS[] = {
    "Arial",
    "Nimbus Sans L",
    "FreeSans",
    "sans-serif",
    "Droid Sans",
    "Roboto",
    "Bitstream Vera Sans",
    "DejaVu Sans",
    "Liberation Sans",
    NULL,
};


char const* const XcbPlatform::KDIALOG[] = {
    "/usr/bin/kdialog",
    "/usr/local/bin/kdialog",
    NULL,
};

char const* const XcbPlatform::KDIALOG_SAVE_ARGUMENTS[] = {
    "--getsavefilename",
    "--title",
    "Save As",
    ".",
    "JS80P Patches (*.js80p)\nAll Files (*.*)",
    NULL,
};

char const* const XcbPlatform::KDIALOG_OPEN_ARGUMENTS[] = {
    "--getopenfilename",
    "--title",
    "Open",
    ".",
    "JS80P Patches (*.js80p)\nAll Files (*.*)",
    NULL,
};


char const* const XcbPlatform::ZENITY[] = {
    "/usr/bin/zenity",
    "/usr/local/bin/zenity",
    NULL,
};

char const* const XcbPlatform::ZENITY_SAVE_ARGUMENTS[] = {
    "--file-selection",
    "--save",
    "--confirm-overwrite",
    "--title=Save As",
    "--file-filter=JS80P Patches (*.js80p) | *.js80p",
    "--file-filter=All Files (*.*) | *.*",
    NULL,
};

char const* const XcbPlatform::ZENITY_OPEN_ARGUMENTS[] = {
    "--file-selection",
    "--title=Open",
    "--file-filter=JS80P Patches (*.js80p) | *.js80p",
    "--file-filter=All Files (*.*) | *.*",
    NULL,
};


xcb_window_t XcbPlatform::gui_platform_widget_to_xcb_window(
        GUI::PlatformWidget platform_widget
) {
    return *((xcb_window_t*)&platform_widget);
}


XcbPlatform::XcbPlatform()
    : connection(NULL),
    screen(NULL),
    screen_root_visual(NULL),
    font_face_normal(NULL),
    font_face_bold(NULL),
    xcb_fd(-1)
{
}


XcbPlatform::~XcbPlatform()
{
    if (font_face_normal != NULL) {
        cairo_font_face_destroy(font_face_normal);
        font_face_normal = NULL;
    }

    if (font_face_bold != NULL) {
        cairo_font_face_destroy(font_face_bold);
        font_face_bold = NULL;
    }

    if (connection != NULL) {
        xcb_disconnect(connection);
        connection = NULL;
        screen = NULL;
        screen_root_visual = NULL;
        xcb_fd = -1;
    }
}


xcb_connection_t* XcbPlatform::get_connection()
{
    if (connection == NULL) {
        for (int i = 0; connection == NULL && i != 2; ++i) {
            connection = xcb_connect(NULL, NULL);

            if (xcb_connection_has_error(connection)) {
                xcb_disconnect(connection);
                connection = NULL;
            }
        }

        if (connection != NULL) {
            screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
            screen_root_visual = find_screen_root_visual();
            xcb_fd = xcb_get_file_descriptor(connection);
        }
    }

    return connection;
}


xcb_visualtype_t* XcbPlatform::find_screen_root_visual() const
{
    xcb_visualid_t screen_root_visual_id = screen->root_visual;
    xcb_screen_iterator_t screen_iterator = (
        xcb_setup_roots_iterator(xcb_get_setup(connection))
    );

    for (; screen_iterator.rem; xcb_screen_next(&screen_iterator)) {
        xcb_depth_iterator_t depth_iterator = (
            xcb_screen_allowed_depths_iterator(screen_iterator.data)
        );

        for (; depth_iterator.rem; xcb_depth_next(&depth_iterator)) {
            xcb_visualtype_iterator_t visual_iterator = (
                xcb_depth_visuals_iterator(depth_iterator.data)
            );

            for (; visual_iterator.rem; xcb_visualtype_next(&visual_iterator)) {
                if (screen_root_visual_id == visual_iterator.data->visual_id) {
                    return visual_iterator.data;
                }
            }
        }
    }

    return NULL;
}


int XcbPlatform::get_fd() const
{
    return xcb_fd;
}


xcb_screen_t* XcbPlatform::get_screen() const
{
    return screen;
}


xcb_visualid_t XcbPlatform::get_screen_root_visual_id() const
{
    return screen->root_visual;
}


xcb_visualtype_t* XcbPlatform::get_screen_root_visual() const
{
    return screen_root_visual;
}


cairo_font_face_t* XcbPlatform::get_font_face(
        cairo_t* cairo,
        WidgetBase::FontWeight const font_weight
) {
    if (font_weight == WidgetBase::FontWeight::NORMAL) {
        if (font_face_normal == NULL) {
            font_face_normal = find_narrowest_font(cairo, CAIRO_FONT_WEIGHT_NORMAL);
        }

        return font_face_normal;
    }

    if (font_face_bold == NULL) {
        font_face_bold = find_narrowest_font(cairo, CAIRO_FONT_WEIGHT_BOLD);
    }

    return font_face_bold;
}


cairo_font_face_t* XcbPlatform::find_narrowest_font(
        cairo_t* cairo,
        cairo_font_weight_t font_weight
) {
    cairo_font_face_t* narrowest_font_face = NULL;
    double narrowest_width = 0.0;

    for (int i = 0; FONTS[i] != NULL; ++i) {
        cairo_text_extents_t text_extents;
        cairo_font_face_t* font_face = cairo_toy_font_face_create(
            FONTS[i], CAIRO_FONT_SLANT_NORMAL, font_weight
        );

        if (font_face == NULL) {
            continue;
        }

        cairo_set_font_face(cairo, font_face);
        cairo_text_extents(cairo, FONT_TEST_STR, &text_extents);

        if (text_extents.width <= 0.000001) {
            cairo_font_face_destroy(font_face);

            continue;
        }

        if (narrowest_font_face == NULL || text_extents.width < narrowest_width) {
            narrowest_width = text_extents.width;

            if (narrowest_font_face != NULL) {
                cairo_font_face_destroy(narrowest_font_face);
            }

            narrowest_font_face = font_face;
        } else {
            cairo_font_face_destroy(font_face);
            font_face = NULL;
        }
    }

    /*
    Not restoring the original font face, because the caller will set the
    returned font anyways.
    */

    if (narrowest_font_face != NULL) {
        return narrowest_font_face;
    }

    return NULL;
}


void XcbPlatform::register_widget(xcb_window_t window_id, Widget* widget)
{
    widgets[window_id] = widget;
}


Widget* XcbPlatform::find_widget(xcb_window_t window_id) const
{
    WindowIdToWidgetMap::const_iterator it = widgets.find(window_id);

    if (it == widgets.end()) {
        return NULL;
    }

    return it->second;
}


void XcbPlatform::unregister_widget(xcb_window_t window_id)
{
    widgets.erase(window_id);
}


std::string const XcbPlatform::get_save_file_name()
{
    char const* const zenity = find_executable(ZENITY);

    if (zenity != NULL) {
        return run_file_selector(zenity, ZENITY_SAVE_ARGUMENTS);
    }

    char const* const kdialog = find_executable(KDIALOG);

    if (kdialog != NULL) {
        return run_file_selector(kdialog, KDIALOG_SAVE_ARGUMENTS);
    }

    return "";
}


std::string const XcbPlatform::get_open_file_name()
{
    char const* const zenity = find_executable(ZENITY);

    if (zenity != NULL) {
        return run_file_selector(zenity, ZENITY_OPEN_ARGUMENTS);
    }

    char const* const kdialog = find_executable(KDIALOG);

    if (kdialog != NULL) {
        return run_file_selector(kdialog, KDIALOG_OPEN_ARGUMENTS);
    }

    return "";
}


char const* XcbPlatform::find_executable(char const* const* alternatives) const
{
    for (int i = 0; alternatives[i] != NULL; ++i) {
        if (access(alternatives[i], X_OK) != -1) {
            return alternatives[i];
        }
    }

    return NULL;
}


std::string const XcbPlatform::run_file_selector(
        char const* executable,
        char const* const* arguments
) {
    if (is_file_selector_open) {
        return "";
    }

    is_file_selector_open = true;

    std::vector<char*> argv;
    std::vector<char*> env;
    Pipe pipe;
    std::string path;
    int exit_code;

    if (!pipe.is_usable) {
        is_file_selector_open = false;

        return "";
    }

    build_argv(executable, arguments, argv);
    build_env(env);

    pid_t pid = vfork();

    if (pid == -1) {
        is_file_selector_open = false;

        return "";
    }

    if (pid == 0) {
        run_file_selector_child_process(argv, env, pipe);
    }

    pipe.close_write_fd();

    path = read_file_selector_output(pipe.read_fd);
    exit_code = wait_or_kill(pid);

    for (std::vector<char*>::iterator it = argv.begin(); it != argv.end(); ++it) {
        if (*it != NULL) {
            delete[] *it;
        }
    }

    for (std::vector<char*>::iterator it = env.begin(); it != env.end(); ++it) {
        if (*it != NULL) {
            delete[] *it;
        }
    }

    pipe.close_read_fd();
    pipe.write_fd = -1;
    pipe.read_fd = -1;

    if (exit_code == 0) {
        is_file_selector_open = false;

        return path;
    }

    is_file_selector_open = false;

    return "";
}


void XcbPlatform::build_argv(
        char const* executable,
        char const* const* arguments,
        std::vector<char*>& argv
) const {
    size_t length = strlen(executable) + 1;
    char* copy = new char[length];
    strncpy(copy, executable, length);
    argv.push_back(copy);

    for (int i = 0; arguments[i] != NULL; ++i) {
        length = strlen(arguments[i]) + 1;
        copy = new char[length];
        strncpy(copy, arguments[i], length);
        argv.push_back(copy);
    }

    argv.push_back(NULL);
}


void XcbPlatform::build_env(std::vector<char*>& env) const
{
    env.reserve(256);

    for (char const* const* it = environ; *it != NULL; ++it) {
        if (strncmp(*it, "LD_LIBRARY_PATH=", 16) == 0) {
            continue;
        }

        /* Should we put our own window id into WINDOWID? */

        size_t const length = strlen(*it) + 1;
        char* copy = new char[length];
        strncpy(copy, *it, length);
        env.push_back(copy);
    }

    env.push_back(NULL);
}


void XcbPlatform::run_file_selector_child_process(
        std::vector<char*> const& argv,
        std::vector<char*> const& env,
        Pipe& pipe
) const {
    pipe.close_read_fd();

    if (dup2(pipe.write_fd, STDOUT_FILENO) == -1) {
        _exit(1);
    }

    pipe.close_write_fd();

    execve(argv[0], argv.data(), env.data());

    _exit(1);
}


std::string const XcbPlatform::read_file_selector_output(int const read_fd)
{
    fd_set read_fds;

    struct timespec ts_timeout;
    ts_timeout.tv_sec = 0;
    ts_timeout.tv_nsec = (time_t)(JS80P::XcbPlatform::NANOSEC_SCALE * 0.3);

    ssize_t read_bytes;
    char buffer[512];
    std::string path;
    path.reserve(512);

    while (true) {
        int result;

        FD_ZERO(&read_fds);
        FD_SET(xcb_fd, &read_fds);
        FD_SET(read_fd, &read_fds);

        result = pselect(
            std::max(xcb_fd, read_fd) + 1, &read_fds, NULL, NULL, &ts_timeout, NULL
        );

        if (result == 0) {
            continue;
        }

        if (result < 0) {
            return "";
        }

        if (FD_ISSET(read_fd, &read_fds)) {
            read_bytes = read(read_fd, buffer, sizeof(buffer));

            if (read_bytes == -1 && errno == EINTR) {
                continue;
            }

            if (read_bytes < 1) {
                break;
            }

            path.append(buffer, read_bytes);
        }

        if (FD_ISSET(xcb_fd, &read_fds)) {
            Widget::process_non_editing_events(this);
        }
    }

    if (read_bytes == -1) {
        return "";
    }

    if (path[0] != '/') {
        return "";
    }

    if (path.back() == '\n') {
        path.pop_back();
    }

    return path;
}


int XcbPlatform::wait_or_kill(pid_t const pid) const
{
    int status = 0;
    pid_t result;

    for (int i = 0; i < 20; ++i) {
        result = waitpid(pid, &status, WNOHANG);

        if (result == 0) {
            usleep(50000);
        } else if (result < 0) {
            return -1;
        } else if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }

    result = waitpid(pid, &status, WNOHANG);

    if (result == 0) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);

        return -1;
    } else if (result < 0) {
        return -1;
    } else if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}


XcbPlatform::Pipe::Pipe() : read_fd(-1), write_fd(-1), is_usable(false)
{
    int fd[2] = {-1, -1};

    is_usable = pipe(fd) == 0;

    if (is_usable) {
        read_fd = fd[0];
        write_fd = fd[1];
    }
}


XcbPlatform::Pipe::Pipe(Pipe const& pipe)
    : read_fd(pipe.read_fd),
    write_fd(pipe.write_fd),
    is_usable(pipe.is_usable)
{
}


XcbPlatform::Pipe::~Pipe()
{
    is_usable = false;

    close_read_fd();
    close_write_fd();
}


void XcbPlatform::Pipe::close_read_fd()
{
    if (read_fd != -1) {
        close(read_fd);
    }
}


void XcbPlatform::Pipe::close_write_fd()
{
    if (write_fd != -1) {
        close(write_fd);
    }
}


void Widget::process_events(XcbPlatform* xcb)
{
    xcb_connection_t* xcb_connection = xcb->get_connection();
    xcb_generic_event_t* event;

    while ((event = xcb_poll_for_event(xcb_connection))) {
        switch (event->response_type & ~0x80) {
            case 0: handle_error_event(xcb, (xcb_generic_error_t*)event); break;
            case XCB_EXPOSE: handle_expose_event(xcb, (xcb_expose_event_t*)event); break;
            case XCB_BUTTON_PRESS: handle_button_press_event(xcb, (xcb_button_press_event_t*)event); break;
            case XCB_BUTTON_RELEASE: handle_button_release_event(xcb, (xcb_button_release_event_t*)event); break;
            case XCB_ENTER_NOTIFY: handle_enter_notify_event(xcb, (xcb_enter_notify_event_t*)event); break;
            case XCB_MOTION_NOTIFY: handle_motion_notify_event(xcb, (xcb_motion_notify_event_t*)event); break;
            case XCB_LEAVE_NOTIFY: handle_leave_notify_event(xcb, (xcb_leave_notify_event_t*)event); break;
            case XCB_CLIENT_MESSAGE: handle_client_message_event(xcb, (xcb_client_message_event_t*)event); break;
            case XCB_DESTROY_NOTIFY: handle_destroy_notify_event(xcb, (xcb_destroy_notify_event_t*)event); break;
            default: break;
        }

        free(event);
    }

    xcb_flush(xcb_connection);
}


void Widget::process_non_editing_events(XcbPlatform* xcb)
{
    xcb_connection_t* xcb_connection = xcb->get_connection();
    xcb_generic_event_t* event;

    while ((event = xcb_poll_for_event(xcb_connection))) {
        switch (event->response_type & ~0x80) {
            case 0: handle_error_event(xcb, (xcb_generic_error_t*)event); break;
            case XCB_EXPOSE: handle_expose_event(xcb, (xcb_expose_event_t*)event); break;
            case XCB_CLIENT_MESSAGE: handle_client_message_event(xcb, (xcb_client_message_event_t*)event); break;
            case XCB_DESTROY_NOTIFY: handle_destroy_notify_event(xcb, (xcb_destroy_notify_event_t*)event); break;
            default: break;
        }

        free(event);
    }

    xcb_flush(xcb_connection);
}


void Widget::handle_error_event(
        XcbPlatform const* xcb,
        xcb_generic_error_t const* error
) {
    // fprintf(
        // stderr,
        // "XCB ERROR: error_code=%u, sequence=%u, resource_id=%u, major_code=%u, minor_code=%u\n",
        // (unsigned int)error->error_code,
        // (unsigned int)error->sequence,
        // (unsigned int)error->resource_id,
        // (unsigned int)error->major_code,
        // (unsigned int)error->minor_code
    // );
}


void Widget::handle_expose_event(
        XcbPlatform const* xcb,
        xcb_expose_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->window);

    if (widget == NULL) {
        return;
    }

    widget->paint();
}


void Widget::handle_button_press_event(
        XcbPlatform const* xcb,
        xcb_button_release_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->event);

    if (widget == NULL) {
        return;
    }

    switch (event->detail) {
        case XCB_BUTTON_INDEX_1: {
            int x = (int)event->event_x;
            int y = (int)event->event_y;

            if (is_double_click(widget, event->time, x, y)) {
                widget->mouse_down_time = (
                    event->time - DOUBLE_CLICK_TIME_DELTA - 100
                );
                widget->is_clicking = false;
                widget->double_click();
            } else {
                widget->mouse_down_time = event->time;
                widget->mouse_down_x = x;
                widget->mouse_down_y = y;
                widget->is_clicking = true;
                widget->mouse_down(x, y);
            }
            break;
        }

        case XCB_BUTTON_INDEX_4:
            widget->mouse_wheel(1.0, is_modifier_active(event->state));
            break;

        case XCB_BUTTON_INDEX_5:
            widget->mouse_wheel(-1.0, is_modifier_active(event->state));
            break;

        default:
            break;
    }
}


bool Widget::is_double_click(
    Widget const* widget,
    xcb_timestamp_t const time,
    int const x,
    int const y
) {
    return (
        time - widget->mouse_down_time <= DOUBLE_CLICK_TIME_DELTA
        && std::abs(widget->mouse_down_x - x) <= DOUBLE_CLICK_POS_DELTA
        && std::abs(widget->mouse_down_y - y) <= DOUBLE_CLICK_POS_DELTA
    );
}


void Widget::handle_button_release_event(
        XcbPlatform const* xcb,
        xcb_button_release_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->event);

    if (widget == NULL) {
        return;
    }

    switch (event->detail) {
        case XCB_BUTTON_INDEX_1:
            widget->mouse_up((int)event->event_x, (int)event->event_y);

            if (widget->is_clicking) {
                widget->is_clicking = false;
                widget->click();
            }

            break;

        default:
            break;
    }
}


void Widget::handle_enter_notify_event(
        XcbPlatform const* xcb,
        xcb_enter_notify_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->event);

    if (widget == NULL) {
        return;
    }

    widget->mouse_move(
        (int)event->event_x, (int)event->event_y, is_modifier_active(event->state)
    );
}


void Widget::handle_motion_notify_event(
        XcbPlatform const* xcb,
        xcb_motion_notify_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->event);

    if (widget == NULL) {
        return;
    }

    widget->mouse_move(
        (int)event->event_x, (int)event->event_y, is_modifier_active(event->state)
    );
}


void Widget::handle_leave_notify_event(
        XcbPlatform const* xcb,
        xcb_leave_notify_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->event);

    if (widget == NULL) {
        return;
    }

    widget->mouse_leave((int)event->event_x, (int)event->event_y);
}


void Widget::handle_client_message_event(
        XcbPlatform const* xcb,
        xcb_client_message_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->window);

    if (widget == NULL) {
        return;
    }

    widget->need_to_destroy_window = false;
}


void Widget::handle_destroy_notify_event(
        XcbPlatform const* xcb,
        xcb_destroy_notify_event_t const* event
) {
    Widget* widget = xcb->find_widget(event->event);

    if (widget == NULL) {
        return;
    }

    widget->need_to_destroy_window = false;
}


bool Widget::is_modifier_active(uint16_t event_state)
{
    return 0 < (event_state & XCB_MOD_MASK_CONTROL);
}


Widget::Resource::Resource(unsigned char* start, unsigned char* end)
    : start(start),
    end(end)
{
}


Widget::PNGStreamState::PNGStreamState(unsigned char* start, unsigned char* end)
    : data(start),
    end(end)
{
}


cairo_status_t Widget::read_png_stream_from_array(
        void *closure,
        unsigned char* data,
        unsigned int length
) {
    PNGStreamState* state = (PNGStreamState*)closure;

    if (state->data + length > state->end) {
        return CAIRO_STATUS_READ_ERROR;
    }

    memcpy(data, state->data, length);
    state->data += length;

    return CAIRO_STATUS_SUCCESS;
}


void GUI::idle()
{
    Widget::process_events((XcbPlatform*)platform_data);

    if (background != NULL) {
        XcbPlatform* xcb = (XcbPlatform*)platform_data;

        background->refresh();
        xcb_flush(xcb->get_connection());
    }
}


void GUI::initialize()
{
    if (platform_data == NULL) {
        XcbPlatform* xcb = new XcbPlatform();

        platform_data = (PlatformData)xcb;
    }
}


void GUI::destroy()
{
    /*
    The owner of the XcbPlatform object is the GUI, even if the GUI was
    instantiated with an already created XcbPlatform object.
    */
    XcbPlatform* xcb = (XcbPlatform*)platform_data;

    platform_data = NULL;

    delete xcb;
}


GUI::Image Widget::load_image(GUI::PlatformData platform_data, char const* name)
{
    StringToImageMap::const_iterator it = IMAGES.find(name);

    if (it == IMAGES.end()) {
        return NULL;
    }

    PNGStreamState state(it->second.start, it->second.end);

    return (GUI::Image)cairo_image_surface_create_from_png_stream(
        &read_png_stream_from_array, &state
    );
}


void Widget::delete_image(GUI::Image image)
{
    cairo_surface_destroy((cairo_surface_t*)image);
}


Widget::Widget(char const* const text) : WidgetBase(text)
{
    initialize();
}


void Widget::initialize()
{
    cairo_surface = NULL;
    fake_transparent_background = NULL;
    fake_transparent_background_source = NULL;
    cairo = NULL;
    first_parent_with_image = NULL;
    fake_transparent_background_left = 0;
    fake_transparent_background_top = 0;
    mouse_down_x = 0;
    mouse_down_y = 0;
    mouse_down_time = 0;
    need_to_destroy_window = false;
    is_transparent = (type & TRANSPARENT_WIDGETS) != 0;
    is_hidden = false;
}


Widget::Widget(
        GUI::PlatformData platform_data,
        GUI::PlatformWidget platform_widget,
        Type const type
) : WidgetBase(platform_data, platform_widget, type)
{
    initialize();
}


Widget::Widget(
        char const* const text,
        int const left,
        int const top,
        int const width,
        int const height,
        Type const type
) : WidgetBase(text, left, top, width, height, type)
{
    initialize();
}


Widget::~Widget()
{
    destroy_children();

    destroy_fake_transparent_background();

    if (cairo_surface != NULL) {
        xcb()->unregister_widget(window_id());

        cairo_surface_finish(cairo_surface);
        cairo_surface_destroy(cairo_surface);
        cairo_destroy(cairo);

        cairo_surface = NULL;
        cairo = NULL;
    }

    if (need_to_destroy_window) {
        need_to_destroy_window = false;

        xcb_destroy_window(xcb_connection(), window_id());
    }

    platform_widget = NULL;
}


void Widget::destroy_fake_transparent_background()
{
    if (fake_transparent_background != NULL) {
        cairo_surface_destroy(fake_transparent_background);
        fake_transparent_background = NULL;
        fake_transparent_background_source = NULL;
    }
}


void Widget::set_up(GUI::PlatformData platform_data, WidgetBase* parent)
{
    WidgetBase::set_up(platform_data, parent);

    uint32_t event_mask = (
        0
        // | XCB_EVENT_MASK_KEY_PRESS
        // | XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW
        | XCB_EVENT_MASK_POINTER_MOTION
        // | XCB_EVENT_MASK_BUTTON_1_MOTION
        | XCB_EVENT_MASK_EXPOSURE
        // | XCB_EVENT_MASK_VISIBILITY_CHANGE
    );

    xcb_connection_t* xcb_connection = this->xcb_connection();
    XcbPlatform* xcb = this->xcb();

    xcb_window_t window_id = xcb_generate_id(xcb_connection);
    need_to_destroy_window = true;
    xcb->register_widget(window_id, this);
    platform_widget = (GUI::PlatformWidget)window_id;
    GUI::PlatformWidget parent_platform_widget = parent->get_platform_widget();
    xcb_window_t parent_id = (
        XcbPlatform::gui_platform_widget_to_xcb_window(parent_platform_widget)
    );

    xcb_create_window(
        xcb_connection,                     // c
        XCB_COPY_FROM_PARENT,               // depth
        window_id,                          // wid
        parent_id,                          // parent
        left,                               // x
        top,                                // y
        width,                              // width
        height,                             // height
        0,                                  // border_width
        XCB_WINDOW_CLASS_INPUT_OUTPUT,      // class
        XCB_COPY_FROM_PARENT,               // visual
        XCB_CW_EVENT_MASK,                  // value_mask
        &event_mask                         // value_list
    );

    cairo_surface = cairo_xcb_surface_create(
        xcb_connection, window_id, xcb->get_screen_root_visual(), width, height
    );
    cairo = cairo_create(cairo_surface);

    xcb_map_window(xcb_connection, window_id);

    /*
    Sometimes it can help with debugging if widget initialization is not batched,
    and events are processed immediately.
    */
    // xcb_flush(xcb_connection);
    // Widget::process_events(xcb);
}


bool Widget::paint()
{
    if (is_hidden) {
        return true;
    }

    if (is_transparent) {
        update_fake_transparency();

        if (fake_transparent_background != NULL) {
            draw_image(
                (GUI::Image)fake_transparent_background, 0, 0, width, height
            );
        }
    }

    return WidgetBase::paint();
}


void Widget::update_fake_transparency()
{
    /*
    In theory, setting the XCB_CW_BACK_PIXMAP attribute to
    XCB_BACK_PIXMAP_PARENT_RELATIVE should make the widget transparent, but in
    practice, this only works until the first repaint of the main window, but
    after that, the pixels of the background window that fall under the area of
    the should-be-transparent widget get lost. The only reliable way seems to be
    to find the first parent widget which has a non-transparent background, and
    manually copy the area that falls under this widget.

    Assumptions:
    1. Sooner or later, when GUI initialization is done, all widgets will
       either have a background image, or will have a parent widget which has
       one.
    2. A child widget's size and position are so that it is completely within
       the area of the background image of its first parent which has an image.
    3. A background image does not have transparent areas.
    */

    cairo_surface_t* first_parent_image = find_first_parent_image();

    if (first_parent_image == fake_transparent_background_source) {
        return;
    }

    destroy_fake_transparent_background();

    fake_transparent_background_source = first_parent_image;
    fake_transparent_background = (cairo_surface_t*)copy_image_region(
        (GUI::Image)fake_transparent_background_source,
        fake_transparent_background_left,
        fake_transparent_background_top,
        width,
        height
    );
}


cairo_surface_t* Widget::find_first_parent_image()
{
    if (first_parent_with_image == NULL) {
        fake_transparent_background_left = left;
        fake_transparent_background_top = top;
        WidgetBase* widget = parent;

        while (widget != NULL && (cairo_surface_t*)widget->get_image() == NULL) {
            fake_transparent_background_left += widget->get_left();
            fake_transparent_background_top += widget->get_top();
            widget = widget->get_parent();
        }

        if (widget == NULL) {
            return NULL;
        }

        first_parent_with_image = widget;
    }

    return (cairo_surface_t*)first_parent_with_image->get_image();
}


void Widget::fill_rectangle(
        int const left,
        int const top,
        int const width,
        int const height,
        GUI::Color const color
) {
    double const red = COLOR_COMPONENT_SCALE * (double)GUI::red(color);
    double const green = COLOR_COMPONENT_SCALE * (double)GUI::green(color);
    double const blue = COLOR_COMPONENT_SCALE * (double)GUI::blue(color);

    cairo_set_source_rgb(cairo, red, green, blue);
    cairo_rectangle(
        cairo, (double)left, (double)top, (double)width, (double)height
    );
    cairo_fill(cairo);
}


void Widget::draw_text(
        char const* const text,
        int const font_size_px,
        int const left,
        int const top,
        int const width,
        int const height,
        GUI::Color const color,
        GUI::Color const background,
        FontWeight const font_weight,
        int const padding,
        TextAlignment const alignment
) {
    double const red = COLOR_COMPONENT_SCALE * (double)GUI::red(color);
    double const green = COLOR_COMPONENT_SCALE * (double)GUI::green(color);
    double const blue = COLOR_COMPONENT_SCALE * (double)GUI::blue(color);

    cairo_font_face_t* font_face = xcb()->get_font_face(cairo, font_weight);
    double text_left;
    double text_top;
    cairo_font_extents_t font_extents;
    cairo_text_extents_t text_extents;

    fill_rectangle(left, top, width, height, background);

    cairo_set_font_face(cairo, font_face);
    cairo_set_font_size(cairo, (double)font_size_px * 1.25);

    cairo_font_extents(cairo, &font_extents);
    cairo_text_extents(cairo, text, &text_extents);

    text_left = (
        alignment == TextAlignment::CENTER
            ? (double)(left + padding)
                + 0.5 * ((double)(width - 2 * padding) - text_extents.width)
                - text_extents.x_bearing
            : (double)(left + padding)
    );
    text_top = (
        (double)(top + height) - font_extents.height * 0.5 + font_extents.descent
    );

    cairo_move_to(cairo, text_left, text_top);

    cairo_set_source_rgb(cairo, red, green, blue);
    cairo_show_text(cairo, text);
}


void Widget::draw_image(
        GUI::Image image,
        int const left,
        int const top,
        int const width,
        int const height
) {
    cairo_set_source_surface(
        cairo, (cairo_surface_t*)image, (double)left, (double)top
    );
    cairo_paint(cairo);
}


GUI::Image Widget::copy_image_region(
        GUI::Image source,
        int const left,
        int const top,
        int const width,
        int const height
) {
    cairo_surface_t* dest_surface = cairo_image_surface_create(
        cairo_image_surface_get_format((cairo_surface_t*)source), width, height
    );
    cairo_t* cairo = cairo_create(dest_surface);
    cairo_set_source_surface(
        cairo, (cairo_surface_t*)source, (double)-left, (double)-top
    );
    cairo_rectangle(cairo, 0.0, 0.0, (double)width, (double)height);
    cairo_fill(cairo);
    cairo_destroy(cairo);

    return dest_surface;
}


void Widget::show()
{
    is_hidden = false;
    xcb_map_window(xcb_connection(), window_id());
}


void Widget::hide()
{
    is_hidden = true;
    xcb_unmap_window(xcb_connection(), window_id());
}


void Widget::focus()
{
}


void Widget::bring_to_top()
{
    uint32_t value = XCB_STACK_MODE_ABOVE;

    xcb_configure_window(xcb_connection(), window_id(), XCB_CONFIG_WINDOW_STACK_MODE, &value);
}


void Widget::redraw()
{
    if (is_hidden) {
        return;
    }

    xcb_clear_area(xcb_connection(), 0, window_id(), 0, 0, 0, 0);
    paint();
}


GUI::Image Widget::set_image(GUI::Image image)
{
    GUI::Image old_image = WidgetBase::set_image(image);

    for (GUI::Widgets::iterator it = children.begin(); it != children.end(); ++it) {
        Widget* child = (Widget*)*it;

        if (child->is_transparent) {
            child->redraw();
        }
    }

    return old_image;
}


xcb_connection_t* Widget::xcb_connection() const
{
    return xcb()->get_connection();
}


xcb_window_t Widget::window_id() const
{
    return *(xcb_window_t*)&platform_widget;
}


XcbPlatform* Widget::xcb() const
{
    return (XcbPlatform*)platform_data;
}


void ImportPatchButton::click()
{
    std::string file_name = xcb()->get_open_file_name();

    if (file_name == "") {
        return;
    }

    std::ifstream patch_file(file_name, std::ios::in | std::ios::binary);

    if (!patch_file.is_open()) {
        // TODO: handle error
        return;
    }

    std::fill_n(buffer, Serializer::MAX_SIZE, '\x00');
    patch_file.read(buffer, Serializer::MAX_SIZE);

    import_buffer((Integer)patch_file.gcount());
}


void ExportPatchButton::click()
{
    std::string file_name = xcb()->get_save_file_name();

    if (file_name == "") {
        return;
    }

    std::ofstream patch_file(file_name, std::ios::out | std::ios::binary);

    if (!patch_file.is_open()) {
        return;
    }

    std::string const patch = Serializer::serialize(synth);
    patch_file.write(patch.c_str(), patch.size());
}

}

#include "gui/widgets.cpp"

#endif

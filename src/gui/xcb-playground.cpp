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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#include <unistd.h>

#include <sys/select.h>

#include "gui/xcb.hpp"

#include "js80p.hpp"
#include "serializer.hpp"
#include "synth.hpp"


xcb_generic_event_t* const JS80P_XCB_TIMEOUT = (xcb_generic_event_t*)-1;


xcb_generic_event_t* xcb_wait_for_event_with_timeout(
        JS80P::XcbPlatform* xcb,
        double const timeout
)
{
    constexpr double NANOSEC_SCALE = 1000000000.0;

    fd_set fds;
    xcb_connection_t* xcb_connection = xcb->get_connection();
    int xcb_fd = xcb->get_fd();

    double timeout_int = std::floor(timeout);
    struct timespec ts_timeout;
    ts_timeout.tv_sec = std::max((time_t)0, (time_t)timeout_int);
    ts_timeout.tv_nsec = std::max(
        (time_t)0, (time_t)(NANOSEC_SCALE * (timeout - timeout_int))
    );

    FD_ZERO(&fds);
    FD_SET(xcb_fd, &fds);

    if (pselect(xcb_fd + 1, &fds, NULL, NULL, &ts_timeout, NULL) > 0) {
        return xcb_poll_for_event(xcb_connection);
    }

    return JS80P_XCB_TIMEOUT;
}


void timer_tick(
        JS80P::Synth& synth,
        JS80P::GUI& gui,
        JS80P::Integer& rendering_round
) {
    ++rendering_round;
    rendering_round = rendering_round & 0x7fff;
    synth.generate_samples(rendering_round, 1);
    gui.idle();
}


int main(int const argc, char const* argv[])
{
    constexpr int WIDTH = 1020;
    constexpr int HEIGHT = 640;

    std::string const WM_PROTOCOLS = "WM_PROTOCOLS";
    std::string const WM_DELETE_WINDOW = "WM_DELETE_WINDOW";

    int ret = 0;
    bool is_running = true;

    uint32_t event_mask = (
        0
        | XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_VISIBILITY_CHANGE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_PROPERTY_CHANGE
    );

    JS80P::XcbPlatform* xcb = new JS80P::XcbPlatform();
    JS80P::XcbPlatform* gui_xcb = new JS80P::XcbPlatform();
    JS80P::Integer rendering_round = 0;

    xcb_connection_t* xcb_connection = xcb->get_connection();
    xcb_screen_t* screen = xcb->get_screen();

    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        xcb_connection, 0, WM_PROTOCOLS.size(), WM_PROTOCOLS.c_str()
    );
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(
        xcb_connection, wm_protocols_cookie, NULL
    );

    xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom(
        xcb_connection, 0, WM_DELETE_WINDOW.size(), WM_DELETE_WINDOW.c_str()
    );
    xcb_intern_atom_reply_t* wm_delete_window_reply = xcb_intern_atom_reply(
        xcb_connection, wm_delete_window_cookie, NULL
    );

    xcb_generic_event_t* event;

    xcb_window_t window_id;

    cairo_surface_t* cairo_surface;
    cairo_t* cairo;

    window_id = xcb_generate_id(xcb_connection);

    xcb_create_window(
        xcb_connection,                 // c
        XCB_COPY_FROM_PARENT,           // depth
        window_id,                      // wid
        screen->root,                   // parent
        20,                             // x
        20,                             // y
        WIDTH,                          // width
        HEIGHT,                         // height
        10,                             // border_width
        XCB_WINDOW_CLASS_INPUT_OUTPUT,  // class
        screen->root_visual,            // visual
        XCB_CW_EVENT_MASK,              // value_mask
        &event_mask                     // value_list
    );

    xcb_change_property(
        xcb_connection,
        XCB_PROP_MODE_REPLACE,
        window_id,
        wm_protocols_reply->atom,
        XCB_ATOM_ATOM,
        32,
        1,
        &wm_delete_window_reply->atom
    );

    xcb_map_window(xcb_connection, window_id);

    cairo_surface = cairo_xcb_surface_create(
        xcb_connection, window_id, xcb->get_screen_root_visual(), WIDTH, HEIGHT
    );
    cairo = cairo_create(cairo_surface);

    xcb_flush(xcb_connection);

    JS80P::Synth synth;

    JS80P::GUI* gui = new JS80P::GUI(
        NULL,
        (JS80P::GUI::PlatformData)gui_xcb,
        (JS80P::GUI::PlatformWidget)window_id,
        synth,
        true
    );
    gui->show();

    while (is_running && (event = xcb_wait_for_event_with_timeout(xcb, 0.05))) {
        if (event == JS80P_XCB_TIMEOUT) {
            timer_tick(synth, *gui, rendering_round);

            continue;
        }

        // fprintf(stderr, "main()\tevent->response_type=%d\n", event->response_type);

        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                cairo_set_source_rgb(cairo, 0.3, 0.3, 0.3);
                cairo_rectangle(cairo, 0.0, 0.0, WIDTH, HEIGHT);
                cairo_fill(cairo);

                break;
            }

            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t* client_msg = (xcb_client_message_event_t*)event;

                if (client_msg->data.data32[0] == wm_delete_window_reply->atom) {
                    is_running = false;
                }

                break;
            }

            case XCB_DESTROY_NOTIFY: {
                is_running = false;

                break;
            }

            default:
                break;
        }

        free(event);

        timer_tick(synth, *gui, rendering_round);
        xcb_flush(xcb_connection);
    }

    delete gui;

    cairo_surface_finish(cairo_surface);
    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cairo);

    free(wm_protocols_reply);
    free(wm_delete_window_reply);

    delete xcb;

    return ret;
}

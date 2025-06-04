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

#include "plugin/vst3/plugin.hpp"

#include "gui/gui.hpp"
#include "gui/xcb.hpp"


using namespace Steinberg;


namespace JS80P
{

class GUIEventHandler : public Linux::IEventHandler, public FObject
{
    public:
        GUIEventHandler() : gui(NULL)
        {
        }

        void PLUGIN_API onFDIsSet(Linux::FileDescriptor fd) override
        {
            if (gui != NULL) {
                gui->idle();
            }
        }

        DELEGATE_REFCOUNT(Steinberg::FObject);

        DEFINE_INTERFACES
            DEF_INTERFACE(Steinberg::Linux::IEventHandler);
        END_DEFINE_INTERFACES(Steinberg::FObject)

        GUI* gui;
};


class GUITimerHandler : public Linux::ITimerHandler, public FObject
{
    public:
        GUITimerHandler() : gui(NULL)
        {
        }

        void PLUGIN_API onTimer() override
        {
            if (gui != NULL) {
                gui->idle();
            }
        }

        DELEGATE_REFCOUNT(Steinberg::FObject);

        DEFINE_INTERFACES
            DEF_INTERFACE(Steinberg::Linux::ITimerHandler);
        END_DEFINE_INTERFACES(Steinberg::FObject)

        GUI* gui;
};


void Vst3Plugin::GUI::initialize()
{
    /*
    Must be destroyed by the platform-specific implementation of JS80P::GUI::destroy().
    */
    XcbPlatform* const xcb = new XcbPlatform();

    xcb->get_connection();

    Linux::IRunLoop* run_loop = NULL;
    Linux::FileDescriptor xcb_fd = xcb->get_fd();
    Linux::TimerInterval milliseconds = (
        (Linux::TimerInterval)std::ceil(JS80P::GUI::REFRESH_RATE_SECONDS)
    );

    GUIEventHandler* const event_handler = new GUIEventHandler();
    GUITimerHandler* const timer_handler = new GUITimerHandler();

    plugFrame->queryInterface(Steinberg::Linux::IRunLoop::iid, (void**)&run_loop);

    if (run_loop != NULL) {
        run_loop->registerEventHandler(event_handler, xcb_fd);
        run_loop->registerTimer(timer_handler, milliseconds);

        this->run_loop = (void*)run_loop;
    }

    gui = new JS80P::GUI(
        kVstVersionString,
        (JS80P::GUI::PlatformData)xcb,
        (JS80P::GUI::PlatformWidget)systemWindow,
        synth,
        true
    );

    event_handler->gui = gui;
    timer_handler->gui = gui;

    gui->show();
    timer_handler->onTimer();

    this->event_handler = (void*)event_handler;
    this->timer_handler = (void*)timer_handler;
}


void Vst3Plugin::GUI::removedFromParent()
{
    if (gui == NULL) {
        return;
    }

    Steinberg::Linux::IRunLoop* run_loop = (
        (Steinberg::Linux::IRunLoop*)this->run_loop
    );

    if (run_loop != NULL) {
        run_loop->unregisterEventHandler((GUIEventHandler*)event_handler);
        run_loop->unregisterTimer((GUITimerHandler*)timer_handler);
    }

    delete gui;
    delete (GUIEventHandler*)event_handler;
    delete (GUITimerHandler*)timer_handler;

    gui = NULL;
    event_handler = NULL;
    timer_handler = NULL;
    run_loop = NULL;
}

}

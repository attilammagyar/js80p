//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstgui_linux_runloop_support.cpp
// Created by  : Steinberg, 10/2025
// Description : VSTGUI Linux Runloop Support
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/vstgui_linux_runloop_support.h"
#include "vstgui/lib/platform/linux/linuxfactory.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"

//-----------------------------------------------------------------------------
namespace Steinberg::Linux {
namespace {

//-----------------------------------------------------------------------------
class RunLoop : public VSTGUI::IRunLoop, public VSTGUI::AtomicReferenceCounted
{
public:
	struct EventHandler : U::Implements<U::Directly<Steinberg::Linux::IEventHandler>>
	{
		VSTGUI::IEventHandler* handler {nullptr};

		void PLUGIN_API onFDIsSet (FileDescriptor) override
		{
			if (handler)
				handler->onEvent ();
		}
	};
	struct TimerHandler : U::Implements<U::Directly<Steinberg::Linux::ITimerHandler>>
	{
		VSTGUI::ITimerHandler* handler {nullptr};

		void PLUGIN_API onTimer () final
		{
			if (handler)
				handler->onTimer ();
		}
	};

	bool registerEventHandler (int fd, VSTGUI::IEventHandler* handler) final;
	bool unregisterEventHandler (VSTGUI::IEventHandler* handler) final;
	bool registerTimer (uint64_t interval, VSTGUI::ITimerHandler* handler) final;
	bool unregisterTimer (VSTGUI::ITimerHandler* handler) final;

	RunLoop (Steinberg::FUnknown* runLoop);

private:
	using EventHandlers = std::vector<IPtr<Steinberg::Linux::RunLoop::EventHandler>>;
	using TimerHandlers = std::vector<IPtr<Steinberg::Linux::RunLoop::TimerHandler>>;
	EventHandlers eventHandlers;
	TimerHandlers timerHandlers;
	FUnknownPtr<Steinberg::Linux::IRunLoop> runLoop;
};

//-----------------------------------------------------------------------------
bool RunLoop::registerEventHandler (int fd, VSTGUI::IEventHandler* handler)
{
	if (!runLoop)
		return false;

	auto smtgHandler = Steinberg::owned (new EventHandler ());
	smtgHandler->handler = handler;
	if (runLoop->registerEventHandler (smtgHandler, fd) == Steinberg::kResultTrue)
	{
		eventHandlers.push_back (smtgHandler);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool RunLoop::unregisterEventHandler (VSTGUI::IEventHandler* handler)
{
	if (!runLoop)
		return false;

	for (auto it = eventHandlers.begin (), end = eventHandlers.end (); it != end; ++it)
	{
		if ((*it)->handler == handler)
		{
			runLoop->unregisterEventHandler ((*it));
			eventHandlers.erase (it);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
bool RunLoop::registerTimer (uint64_t interval, VSTGUI::ITimerHandler* handler)
{
	if (!runLoop)
		return false;

	auto smtgHandler = Steinberg::owned (new TimerHandler ());
	smtgHandler->handler = handler;
	if (runLoop->registerTimer (smtgHandler, interval) == Steinberg::kResultTrue)
	{
		timerHandlers.push_back (smtgHandler);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool RunLoop::unregisterTimer (VSTGUI::ITimerHandler* handler)
{
	if (!runLoop)
		return false;

	for (auto it = timerHandlers.begin (), end = timerHandlers.end (); it != end; ++it)
	{
		if ((*it)->handler == handler)
		{
			runLoop->unregisterTimer ((*it));
			timerHandlers.erase (it);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
RunLoop::RunLoop (Steinberg::FUnknown* runLoop) : runLoop (runLoop)
{
}

//-----------------------------------------------------------------------------
} // anonymous

//-----------------------------------------------------------------------------
bool setupVSTGUIRunloop (FUnknown* hostContext)
{
	if (auto linuxFactory = VSTGUI::getPlatformFactory ().asLinuxFactory ())
	{
		if (U::cast<Steinberg::Linux::IRunLoop> (hostContext))
		{
			linuxFactory->setRunLoop (VSTGUI::makeOwned<Steinberg::Linux::RunLoop> (hostContext));
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
} // Steinberg::Linux

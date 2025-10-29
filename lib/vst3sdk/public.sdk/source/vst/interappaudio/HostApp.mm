//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/HostApp.mm
// Created by  : Steinberg, 08/2013.
// Description : VST 3 InterAppAudio
// Flags       : clang-format SMTGSequencer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#import "HostApp.h"

#import "AudioIO.h"
#import "PresetManager.h"
#import "SettingsViewController.h"
#import "VST3Plugin.h"
#import "base/source/updatehandler.h"
#import "pluginterfaces/gui/iplugview.h"

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

//------------------------------------------------------------------------
InterAppAudioHostApp* InterAppAudioHostApp::instance ()
{
	static InterAppAudioHostApp gInstance;
	return &gInstance;
}

//-----------------------------------------------------------------------------
InterAppAudioHostApp::InterAppAudioHostApp () = default;

//-----------------------------------------------------------------------------
void InterAppAudioHostApp::setPlugin (VST3Plugin* plugin)
{
	this->plugin = plugin;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::getName (String128 name)
{
	String str ("InterAppAudioHost");
	str.copyTo (name, 0, 127);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::getScreenSize (ViewRect* size, float* scale)
{
	if (size)
	{
		UIScreen* screen = [UIScreen mainScreen];
		CGSize s = [screen currentMode].size;
		UIWindow* window = [[[UIApplication sharedApplication] windows] objectAtIndex:0];
		if (window)
		{
			NSArray* subViews = [window subviews];
			if ([subViews count] == 1)
			{
				s = [[subViews objectAtIndex:0] bounds].size;
			}
		}
		size->left = 0;
		size->top = 0;
		size->right = s.width;
		size->bottom = s.height;
		if (scale)
		{
			*scale = screen.scale;
		}
		return kResultTrue;
	}
	return kInvalidArgument;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::connectedToHost ()
{
	return AudioIO::instance ()->getInterAppAudioConnected () ? kResultTrue : kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::switchToHost ()
{
	return AudioIO::instance ()->switchToHost () ? kResultTrue : kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::sendRemoteControlEvent (uint32 event)
{
	return AudioIO::instance ()->sendRemoteControlEvent (
	           static_cast<AudioUnitRemoteControlEvent> (event)) ?
	           kResultTrue :
	           kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::getHostIcon (void** icon)
{
	if (icon)
	{
		UIImage* hostIcon = AudioIO::instance ()->getHostIcon ();
		if (hostIcon)
		{
			CGImageRef cgImage = [hostIcon CGImage];
			if (cgImage)
			{
				*icon = cgImage;
				return kResultTrue;
			}
		}
		return kNotImplemented;
	}
	return kInvalidArgument;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::scheduleEventFromUI (Event& event)
{
	if (plugin)
	{
		return plugin->scheduleEventFromUI (event);
	}
	return kNotInitialized;
}

//-----------------------------------------------------------------------------
IInterAppAudioPresetManager* PLUGIN_API InterAppAudioHostApp::createPresetManager (const TUID& cid)
{
	return plugin ? new PresetManager (plugin, cid) : nullptr;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API InterAppAudioHostApp::showSettingsView ()
{
	showIOSettings ();
	return kResultTrue;
}
}
}
}

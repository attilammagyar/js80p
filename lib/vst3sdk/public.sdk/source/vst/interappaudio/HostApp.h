//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/HostApp.h
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

#pragma once

/// \cond ignore

#import "public.sdk/source/vst/hosting/hostclasses.h"
#import "base/source/fobject.h"
#import "pluginterfaces/vst/ivstinterappaudio.h"

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

class VST3Plugin;

//-----------------------------------------------------------------------------
class InterAppAudioHostApp : public FObject, public HostApplication, public IInterAppAudioHost
{
public:
//-----------------------------------------------------------------------------
	static InterAppAudioHostApp* instance ();

	void setPlugin (VST3Plugin* plugin);
	VST3Plugin* getPlugin () const { return plugin; }

//-----------------------------------------------------------------------------
	//	IInterAppAudioHost
	tresult PLUGIN_API getScreenSize (ViewRect* size, float* scale) override;
	tresult PLUGIN_API connectedToHost () override;
	tresult PLUGIN_API switchToHost () override;
	tresult PLUGIN_API sendRemoteControlEvent (uint32 event) override;
	tresult PLUGIN_API getHostIcon (void** icon) override;
	tresult PLUGIN_API scheduleEventFromUI (Event& event) override;
	IInterAppAudioPresetManager* PLUGIN_API createPresetManager (const TUID& cid) override;
	tresult PLUGIN_API showSettingsView () override;

//-----------------------------------------------------------------------------
	//	HostApplication
	tresult PLUGIN_API getName (String128 name) override;

	OBJ_METHODS (InterAppAudioHostApp, FObject)
	REFCOUNT_METHODS (FObject)
	DEFINE_INTERFACES
		DEF_INTERFACE (IHostApplication)
		DEF_INTERFACE (IInterAppAudioHost)
	END_DEFINE_INTERFACES (FObject)
protected:
	InterAppAudioHostApp ();

	VST3Plugin* plugin {nullptr};
};

//------------------------------------------------------------------------
} // namespace InterAppAudio
} // namespace Vst
} // namespace Steinberg

/// \endcond

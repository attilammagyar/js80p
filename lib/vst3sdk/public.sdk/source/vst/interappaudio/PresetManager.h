//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/PresetManager.h
// Created by  : Steinberg, 10/2013
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

#include "VST3Plugin.h"
#include "base/source/fstring.h"
#include "pluginterfaces/vst/ivstinterappaudio.h"

#if __OBJC__
@class NSArray, PresetBrowserViewController, PresetSaveViewController;
#else
struct NSArray;
struct PresetBrowserViewController;
struct PresetSaveViewController;
#endif

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

class PresetManager : public FObject, public IInterAppAudioPresetManager
{
public:
	PresetManager (VST3Plugin* plugin, const TUID& cid);

	tresult PLUGIN_API runLoadPresetBrowser () override;
	tresult PLUGIN_API runSavePresetBrowser () override;
	tresult PLUGIN_API loadNextPreset () override;
	tresult PLUGIN_API loadPreviousPreset () override;

	DEFINE_INTERFACES
		DEF_INTERFACE (IInterAppAudioPresetManager)
	END_DEFINE_INTERFACES (FObject)
	REFCOUNT_METHODS (FObject)

private:
	enum PresetPathType
	{
		kFactory,
		kUser
	};
	NSArray* getPresetPaths (PresetPathType type);

	tresult loadPreset (bool next);
	tresult loadPreset (const char* path);
	void savePreset (const char* path);

	VST3Plugin* plugin;
	PresetBrowserViewController* visiblePresetBrowserViewController;
	PresetSaveViewController* visibleSavePresetViewController;
	FUID cid;
	String lastPreset;
};

//------------------------------------------------------------------------
} // namespace InterAppAudio
} // namespace Vst
} // namespace Steinberg

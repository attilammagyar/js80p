//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstaudioeffect.h
// Created by  : Steinberg, 04/2005
// Description : Basic Audio Effect Implementation
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstbus.h"
#include "public.sdk/source/vst/vstcomponent.h"
#include "public.sdk/source/vst/utility/processcontextrequirements.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Default implementation for a VST 3 audio effect.
\ingroup vstClasses
Can be used as base class for a VST 3 effect implementation.
*/
class AudioEffect : public Component, public IAudioProcessor, public IProcessContextRequirements
{
public:
//------------------------------------------------------------------------
	/** Constructor */
	AudioEffect ();

	//---Internal Methods-----------
	/** Creates and adds a new Audio input bus with a given speaker arrangement, busType (kMain or
	 * kAux). */
	AudioBus* addAudioInput (const TChar* name, SpeakerArrangement arr, BusType busType = kMain,
	                         int32 flags = BusInfo::kDefaultActive);

	/** Creates and adds a new Audio output bus with a given speaker arrangement, busType (kMain or
	 * kAux). */
	AudioBus* addAudioOutput (const TChar* name, SpeakerArrangement arr, BusType busType = kMain,
	                          int32 flags = BusInfo::kDefaultActive);

	/** Retrieves an Audio Input Bus by index. */
	AudioBus* getAudioInput (int32 index);

	/** Retrieves an Audio Output Bus by index. */
	AudioBus* getAudioOutput (int32 index);

	/** Creates and adds a new Event input bus with a given speaker arrangement, busType (kMain or
	 * kAux). */
	EventBus* addEventInput (const TChar* name, int32 channels = 16, BusType busType = kMain,
	                         int32 flags = BusInfo::kDefaultActive);

	/** Creates and adds a new Event output bus with a given speaker arrangement, busType (kMain or
	 * kAux). */
	EventBus* addEventOutput (const TChar* name, int32 channels = 16, BusType busType = kMain,
	                          int32 flags = BusInfo::kDefaultActive);

	/** Retrieves an Event Input Bus by index. */
	EventBus* getEventInput (int32 index);

	/** Retrieves an Event Output Bus by index. */
	EventBus* getEventOutput (int32 index);

	//---from IAudioProcessor-------
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
	                                       SpeakerArrangement* outputs,
	                                       int32 numOuts) SMTG_OVERRIDE;
	tresult PLUGIN_API getBusArrangement (BusDirection dir, int32 busIndex,
	                                      SpeakerArrangement& arr) SMTG_OVERRIDE;
	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;
	uint32 PLUGIN_API getLatencySamples () SMTG_OVERRIDE { return 0; }
	tresult PLUGIN_API setupProcessing (ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API setProcessing (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API process (ProcessData& data) SMTG_OVERRIDE;
	uint32 PLUGIN_API getTailSamples () SMTG_OVERRIDE { return kNoTail; }

	//---from IProcessContextRequirements-------
	uint32 PLUGIN_API getProcessContextRequirements () SMTG_OVERRIDE;

	//---Interface---------
	OBJ_METHODS (AudioEffect, Component)
	DEFINE_INTERFACES
		DEF_INTERFACE (IAudioProcessor)
		DEF_INTERFACE (IProcessContextRequirements)
	END_DEFINE_INTERFACES (Component)
	REFCOUNT_METHODS (Component)
//------------------------------------------------------------------------
protected:
	ProcessSetup processSetup;
	ProcessContextRequirements processContextRequirements;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : plublic.sdk/source/vst/vstsinglecomponenteffect.h
// Created by  : Steinberg, 03/2008
// Description : Recombination class of Audio Effect and Edit Controller
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstaudioprocessor.h"

// work around for the name clash of IComponent::setState and IEditController::setState
#define setState setEditorState
#define getState getEditorState
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#undef setState
#undef getState

#include "public.sdk/source/vst/utility/processcontextrequirements.h"
#include "public.sdk/source/vst/vstbus.h"
#include "public.sdk/source/vst/vstparameters.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Default implementation for a non-distributable Plug-in that combines
processor and edit controller in one component.
\ingroup vstClasses
- [released: 3.0.2]

This can be used as base class for a VST 3 effect implementation in case that the standard way of
defining two separate components would cause too many implementation difficulties:
- Cubase 4.2 is the first host that supports combined VST 3 Plug-ins
- <b> Use this class only after giving the standard way of defining two components
serious considerations! </b>
*/
class SingleComponentEffect : public EditControllerEx1,
                              public IComponent,
                              public IAudioProcessor,
                              public IProcessContextRequirements
{
public:
//------------------------------------------------------------------------
	SingleComponentEffect ();
	~SingleComponentEffect () override;

	//---from IPluginBase---------
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	//---from IComponent-----------------------
	tresult PLUGIN_API getControllerClassId (TUID /*classId*/) SMTG_OVERRIDE
	{
		return kNotImplemented;
	}
	tresult PLUGIN_API setIoMode (IoMode /*mode*/) SMTG_OVERRIDE { return kNotImplemented; }
	int32 PLUGIN_API getBusCount (MediaType type, BusDirection dir) SMTG_OVERRIDE;
	tresult PLUGIN_API getBusInfo (MediaType type, BusDirection dir, int32 index,
	                               BusInfo& bus /*out*/) SMTG_OVERRIDE;
	tresult PLUGIN_API getRoutingInfo (RoutingInfo& /*inInfo*/,
	                                   RoutingInfo& /*outInfo*/ /*out*/) SMTG_OVERRIDE
	{
		return kNotImplemented;
	}
	tresult PLUGIN_API activateBus (MediaType type, BusDirection dir, int32 index,
	                                TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool /*state*/) SMTG_OVERRIDE { return kResultOk; }
	tresult PLUGIN_API setState (IBStream* /*state*/) SMTG_OVERRIDE { return kNotImplemented; }
	tresult PLUGIN_API getState (IBStream* /*state*/) SMTG_OVERRIDE { return kNotImplemented; }

	// bus setup methods
	AudioBus* addAudioInput (const TChar* name, SpeakerArrangement arr, BusType busType = kMain,
	                         int32 flags = BusInfo::kDefaultActive);

	AudioBus* addAudioOutput (const TChar* name, SpeakerArrangement arr, BusType busType = kMain,
	                          int32 flags = BusInfo::kDefaultActive);

	EventBus* addEventInput (const TChar* name, int32 channels = 16, BusType busType = kMain,
	                         int32 flags = BusInfo::kDefaultActive);

	EventBus* addEventOutput (const TChar* name, int32 channels = 16, BusType busType = kMain,
	                          int32 flags = BusInfo::kDefaultActive);

	tresult removeAudioBusses ();
	tresult removeEventBusses ();
	tresult removeAllBusses ();

	//---from IAudioProcessor -------------------
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
	                                       SpeakerArrangement* outputs,
	                                       int32 numOuts) SMTG_OVERRIDE;
	tresult PLUGIN_API getBusArrangement (BusDirection dir, int32 index,
	                                      SpeakerArrangement& arr) SMTG_OVERRIDE;
	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;
	uint32 PLUGIN_API getLatencySamples () SMTG_OVERRIDE { return 0; }
	tresult PLUGIN_API setupProcessing (ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API setProcessing (TBool /*state*/) SMTG_OVERRIDE { return kNotImplemented; }
	tresult PLUGIN_API process (ProcessData& /*data*/) SMTG_OVERRIDE { return kNotImplemented; }
	uint32 PLUGIN_API getTailSamples () SMTG_OVERRIDE { return kNoTail; }

	//---from IProcessContextRequirements -------------------
	uint32 PLUGIN_API getProcessContextRequirements () SMTG_OVERRIDE
	{
		return processContextRequirements.flags;
	}

	//---Interface---------
	OBJ_METHODS (SingleComponentEffect, EditControllerEx1)
	tresult PLUGIN_API queryInterface (const TUID iid, void** obj) SMTG_OVERRIDE;
	REFCOUNT_METHODS (EditControllerEx1)

//------------------------------------------------------------------------
protected:
	BusList* getBusList (MediaType type, BusDirection dir);

	ProcessSetup processSetup;
	ProcessContextRequirements processContextRequirements;

	BusList audioInputs;
	BusList audioOutputs;
	BusList eventInputs;
	BusList eventOutputs;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

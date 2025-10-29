//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstsinglecomponenteffect.cpp
// Created by  : Steinberg, 03/2008
// Description : Basic Audio Effect Implementation in one component
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "vstsinglecomponenteffect.h"

//-----------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
// SingleComponentEffect Implementation
//-----------------------------------------------------------------------------
SingleComponentEffect::SingleComponentEffect ()
: audioInputs (kAudio, kInput)
, audioOutputs (kAudio, kOutput)
, eventInputs (kEvent, kInput)
, eventOutputs (kEvent, kOutput)
{
	processSetup.maxSamplesPerBlock = 1024;
	processSetup.processMode = Vst::kRealtime;
	processSetup.sampleRate = 44100.0;
	processSetup.symbolicSampleSize = Vst::kSample32;
}

//-----------------------------------------------------------------------------
SingleComponentEffect::~SingleComponentEffect ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::initialize (FUnknown* context)
{
	return EditControllerEx1::initialize (context);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::terminate ()
{
	parameters.removeAll ();
	removeAllBusses ();

	return EditControllerEx1::terminate ();
}

//-----------------------------------------------------------------------------
int32 PLUGIN_API SingleComponentEffect::getBusCount (MediaType type, BusDirection dir)
{
	BusList* busList = getBusList (type, dir);
	return busList ? static_cast<int32> (busList->size ()) : 0;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::getBusInfo (MediaType type, BusDirection dir, int32 index,
                                                      BusInfo& info)
{
	BusList* busList = getBusList (type, dir);
	if (busList == nullptr || index >= static_cast<int32> (busList->size ()))
		return kInvalidArgument;

	Bus* bus = busList->at (index);
	info.mediaType = type;
	info.direction = dir;
	if (bus->getInfo (info))
		return kResultTrue;
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::activateBus (MediaType type, BusDirection dir,
                                                       int32 index, TBool state)
{
	BusList* busList = getBusList (type, dir);
	if (busList == nullptr || index >= static_cast<int32> (busList->size ()))
		return kInvalidArgument;

	Bus* bus = busList->at (index);
	if (!bus)
		return kResultFalse;

	bus->setActive (state);
	return kResultTrue;
}

//-----------------------------------------------------------------------------
AudioBus* SingleComponentEffect::addAudioInput (const TChar* name, SpeakerArrangement arr,
                                                BusType busType, int32 flags)
{
	auto* newBus = new AudioBus (name, busType, flags, arr);
	audioInputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//-----------------------------------------------------------------------------
AudioBus* SingleComponentEffect::addAudioOutput (const TChar* name, SpeakerArrangement arr,
                                                 BusType busType, int32 flags)
{
	auto* newBus = new AudioBus (name, busType, flags, arr);
	audioOutputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//-----------------------------------------------------------------------------
EventBus* SingleComponentEffect::addEventInput (const TChar* name, int32 channels, BusType busType,
                                                int32 flags)
{
	auto* newBus = new EventBus (name, busType, flags, channels);
	eventInputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//-----------------------------------------------------------------------------
EventBus* SingleComponentEffect::addEventOutput (const TChar* name, int32 channels, BusType busType,
                                                 int32 flags)
{
	auto* newBus = new EventBus (name, busType, flags, channels);
	eventOutputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//-----------------------------------------------------------------------------
tresult SingleComponentEffect::removeAudioBusses ()
{
	audioInputs.clear ();
	audioOutputs.clear ();

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult SingleComponentEffect::removeEventBusses ()
{
	eventInputs.clear ();
	eventOutputs.clear ();

	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult SingleComponentEffect::removeAllBusses ()
{
	removeAudioBusses ();
	removeEventBusses ();

	return kResultOk;
}

//-----------------------------------------------------------------------------
// IAudioProcessor
//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::setBusArrangements (SpeakerArrangement* inputs,
                                                              int32 numIns,
                                                              SpeakerArrangement* outputs,
                                                              int32 numOuts)
{
	if (numIns < 0 || numOuts < 0)
		return kInvalidArgument;

	if (numIns > static_cast<int32> (audioInputs.size ()) ||
	    numOuts > static_cast<int32> (audioOutputs.size ()))
		return kResultFalse;

	for (int32 index = 0; index < static_cast<int32> (audioInputs.size ()); ++index)
	{
		if (index >= numIns)
			break;
		FCast<Vst::AudioBus> (audioInputs[index].get ())->setArrangement (inputs[index]);
	}

	for (int32 index = 0; index < static_cast<int32> (audioOutputs.size ()); ++index)
	{
		if (index >= numOuts)
			break;
		FCast<Vst::AudioBus> (audioOutputs[index].get ())->setArrangement (outputs[index]);
	}

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::getBusArrangement (BusDirection dir, int32 busIndex,
                                                             SpeakerArrangement& arr)
{
	BusList* busList = getBusList (kAudio, dir);
	if (busList == nullptr || busIndex >= static_cast<int32> (busList->size ()))
		return kInvalidArgument;

	if (auto* audioBus = FCast<Vst::AudioBus> (busList->at (busIndex)))
	{
		arr = audioBus->getArrangement ();
		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::setupProcessing (ProcessSetup& newSetup)
{
	if (canProcessSampleSize (newSetup.symbolicSampleSize) != kResultTrue)
		return kResultFalse;

	processSetup = newSetup;
	return kResultOk;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::canProcessSampleSize (int32 symbolicSampleSize)
{
	return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

//-----------------------------------------------------------------------------
BusList* SingleComponentEffect::getBusList (MediaType type, BusDirection dir)
{
	if (type == kAudio)
		return dir == kInput ? &audioInputs : &audioOutputs;
	if (type == kEvent)
		return dir == kInput ? &eventInputs : &eventOutputs;
	return nullptr;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SingleComponentEffect::queryInterface (const TUID iid, void** obj)
{
	if (memcmp (iid, IConnectionPoint::iid, sizeof (::Steinberg::TUID)) == 0)
	{
		// no need to expose IConnectionPoint to the host
		return kNoInterface;
	}
	DEF_INTERFACE (IComponent)
	DEF_INTERFACE (IAudioProcessor)
	DEF_INTERFACE (IProcessContextRequirements)
	return EditControllerEx1::queryInterface (iid, obj);
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

// work around for the name clash of IComponent::setState and IEditController::setState
#if defined(PROJECT_INCLUDES_VSTEDITCONTROLLER) && PROJECT_INCLUDES_VSTEDITCONTROLLER
// make sure that vsteditcontroller.cpp is included by your project
//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Steinberg::Vst::EditController::setEditorState (
    Steinberg::IBStream* /*state*/)
{
	return Steinberg::kNotImplemented;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API Steinberg::Vst::EditController::getEditorState (
    Steinberg::IBStream* /*state*/)
{
	return Steinberg::kNotImplemented;
}

#else
// make sure that vsteditcontroller.cpp is otherwise excluded from your project
#define setState setEditorState
#define getState getEditorState
#include "public.sdk/source/vst/vsteditcontroller.cpp"
#undef setState
#undef getState
#endif // PROJECT_INCLUDES_VSTEDITCONTROLLER

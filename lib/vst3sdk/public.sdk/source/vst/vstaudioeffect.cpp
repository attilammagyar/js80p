//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstaudioeffect.cpp
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

#include "vstaudioeffect.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// AudioEffect
//------------------------------------------------------------------------
AudioEffect::AudioEffect ()
{
	processSetup.maxSamplesPerBlock = 1024;
	processSetup.processMode = Vst::kRealtime;
	processSetup.sampleRate = 44100.0;
	processSetup.symbolicSampleSize = Vst::kSample32;
}

//------------------------------------------------------------------------
AudioBus* AudioEffect::addAudioInput (const TChar* name, SpeakerArrangement arr, BusType busType,
                                      int32 flags)
{
	auto* newBus = new AudioBus (name, busType, flags, arr);
	audioInputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//------------------------------------------------------------------------
AudioBus* AudioEffect::addAudioOutput (const TChar* name, SpeakerArrangement arr, BusType busType,
                                       int32 flags)
{
	auto* newBus = new AudioBus (name, busType, flags, arr);
	audioOutputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//------------------------------------------------------------------------
AudioBus* AudioEffect::getAudioInput (int32 index)
{
	AudioBus* bus = nullptr;
	if (index < static_cast<int32> (audioInputs.size ()))
		bus = FCast<Vst::AudioBus> (audioInputs.at (index));
	return bus;
}

//------------------------------------------------------------------------
AudioBus* AudioEffect::getAudioOutput (int32 index)
{
	AudioBus* bus = nullptr;
	if (index < static_cast<int32> (audioOutputs.size ()))
		bus = FCast<Vst::AudioBus> (audioOutputs.at (index));
	return bus;
}

//------------------------------------------------------------------------
EventBus* AudioEffect::addEventInput (const TChar* name, int32 channels, BusType busType,
                                      int32 flags)
{
	auto* newBus = new EventBus (name, busType, flags, channels);
	eventInputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//------------------------------------------------------------------------
EventBus* AudioEffect::addEventOutput (const TChar* name, int32 channels, BusType busType,
                                       int32 flags)
{
	auto* newBus = new EventBus (name, busType, flags, channels);
	eventOutputs.push_back (IPtr<Vst::Bus> (newBus, false));
	return newBus;
}

//------------------------------------------------------------------------
EventBus* AudioEffect::getEventInput (int32 index)
{
	EventBus* bus = nullptr;
	if (index < static_cast<int32> (eventInputs.size ()))
		bus = FCast<Vst::EventBus> (eventInputs.at (index));
	return bus;
}

//------------------------------------------------------------------------
EventBus* AudioEffect::getEventOutput (int32 index)
{
	EventBus* bus = nullptr;
	if (index < static_cast<int32> (eventOutputs.size ()))
		bus = FCast<Vst::EventBus> (eventOutputs.at (index));
	return bus;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioEffect::setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
                                                    SpeakerArrangement* outputs, int32 numOuts)
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

//------------------------------------------------------------------------
tresult PLUGIN_API AudioEffect::getBusArrangement (BusDirection dir, int32 busIndex,
                                                   SpeakerArrangement& arr)
{
	BusList* busList = getBusList (kAudio, dir);
	if (!busList || busIndex < 0 || static_cast<int32> (busList->size ()) <= busIndex)
		return kInvalidArgument;
	if (auto* audioBus = FCast<Vst::AudioBus> (busList->at (busIndex)))
	{
		arr = audioBus->getArrangement ();
		return kResultTrue;
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioEffect::setupProcessing (ProcessSetup& newSetup)
{
	processSetup.maxSamplesPerBlock = newSetup.maxSamplesPerBlock;
	processSetup.processMode = newSetup.processMode;
	processSetup.sampleRate = newSetup.sampleRate;

	if (canProcessSampleSize (newSetup.symbolicSampleSize) != kResultTrue)
		return kResultFalse;

	processSetup.symbolicSampleSize = newSetup.symbolicSampleSize;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioEffect::setProcessing (TBool /*state*/)
{
	return kNotImplemented;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioEffect::canProcessSampleSize (int32 symbolicSampleSize)
{
	return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AudioEffect::process (ProcessData& /*data*/)
{
	return kNotImplemented;
}

//------------------------------------------------------------------------
uint32 PLUGIN_API AudioEffect::getProcessContextRequirements ()
{
	return processContextRequirements.flags;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

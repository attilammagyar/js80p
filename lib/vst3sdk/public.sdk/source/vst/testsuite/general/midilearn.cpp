//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/midilearn.cpp
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/testsuite/general/midilearn.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstmidilearn.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// MidiLearnTest
//------------------------------------------------------------------------
MidiLearnTest::MidiLearnTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API MidiLearnTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (!controller)
	{
		addMessage (testResult, STR ("No Edit Controller supplied!"));
		return true;
	}

	auto midiLearn = U::cast<IMidiLearn> (controller);
	if (!midiLearn)
	{
		addMessage (testResult, STR ("No MIDI Learn interface supplied!"));
		return true;
	}

	if (midiLearn->onLiveMIDIControllerInput (0, 0, ControllerNumbers::kCtrlPan) != kResultTrue)
		addMessage (testResult, STR ("onLiveMIDIControllerInput do not return kResultTrue!"));
	if (midiLearn->onLiveMIDIControllerInput (0, 0, ControllerNumbers::kCtrlVibratoDelay) !=
	    kResultTrue)
		addMessage (testResult, STR ("onLiveMIDIControllerInput do not return kResultTrue!"));

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

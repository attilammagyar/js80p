//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/validstatetransition.cpp
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

#include "public.sdk/source/vst/testsuite/state/validstatetransition.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ValidStateTransitionTest
//------------------------------------------------------------------------
ValidStateTransitionTest::ValidStateTransitionTest (ITestPlugProvider* plugProvider,
                                                    ProcessSampleSize sampleSize)
: ProcessTest (plugProvider, sampleSize)
{
	if (sampleSize == kSample32)
		strcpy (name, "Valid State Transition 32bits");
	else
		strcpy (name, "Valid State Transition 64bits");
}

//------------------------------------------------------------------------
bool PLUGIN_API ValidStateTransitionTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug || !audioEffect)
		return false;

	printTestHeader (testResult);
	if (!canProcessSampleSize (testResult))
		return true;

	// disable it, it was enabled in setup call
	tresult result = vstPlug->setActive (false);
	if (result != kResultTrue)
		return false;

	auto plugBase = U::cast<IPluginBase> (vstPlug);
	if (!plugBase)
		return false;

	for (int32 i = 0; i < 4; ++i)
	{
		result = audioEffect->setupProcessing (processSetup);
		if (result != kResultTrue)
			return false;

		result = vstPlug->setActive (true);
		if (result != kResultTrue)
			return false;

		result = vstPlug->setActive (false);
		if (result != kResultTrue)
			return false;

		if (activateMainIOBusses (false) == false)
			return false;

		result = plugBase->terminate ();
		if (result != kResultTrue)
			return false;

		result = plugBase->initialize (TestingPluginContext::get ());
		if (result != kResultTrue)
			return false;

		// for the last 2 steps we decide to not reenable the buses, see
		// https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Change+History/3.0.0/Multiple+Dynamic+IO.html?highlight=kDefaultActive#information-about-busses
		if (i < 2)
		{
			if (activateMainIOBusses (true) == false)
				return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

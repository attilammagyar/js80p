//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/repeatidenticalstatetransition.cpp
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

#include "public.sdk/source/vst/testsuite/state/repeatidenticalstatetransition.h"
#include "pluginterfaces/base/funknownimpl.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// RepeatIdenticalStateTransitionTest
//------------------------------------------------------------------------
RepeatIdenticalStateTransitionTest::RepeatIdenticalStateTransitionTest (
    ITestPlugProvider* plugProvider)
: TestEnh (plugProvider, kSample32)
{
}

//------------------------------------------------------------------------
bool RepeatIdenticalStateTransitionTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug || !audioEffect)
		return false;

	printTestHeader (testResult);

	auto plugBase = U::cast<IPluginBase> (vstPlug);
	if (!plugBase)
		return false;

	tresult result = plugBase->initialize (TestingPluginContext::get ());
	if (result != kResultFalse)
		return false;

	result = audioEffect->setupProcessing (processSetup);
	if (result != kResultTrue)
		return false;

	result = vstPlug->setActive (true);
	if (result != kResultOk)
		return false;

	result = vstPlug->setActive (true);
	if (result != kResultFalse)
		return false;

	result = vstPlug->setActive (false);
	if (result != kResultOk)
		return false;

	result = vstPlug->setActive (false);
	if (result == kResultOk)
		return false;

	result = plugBase->terminate ();
	if (result != kResultOk)
		return false;

	result = plugBase->terminate ();
	if (result == kResultOk)
		return false;

	result = plugBase->initialize (TestingPluginContext::get ());
	if (result != kResultOk)
		return false;

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

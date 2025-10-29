//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/state/bypassstate.cpp
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

#include "public.sdk/source/vst/testsuite/state/bypasspersistence.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/vstpresetfile.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// VstBypassSaveParamTest
//------------------------------------------------------------------------
BypassPersistenceTest::BypassPersistenceTest (ITestPlugProvider* plugProvider,
                                              ProcessSampleSize sampl)
: AutomationTest (plugProvider, sampl, 100, 1, false)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API BypassPersistenceTest::run (ITestResult* testResult)
{
	if (!vstPlug || !testResult || !audioEffect)
		return false;
	if (!canProcessSampleSize (testResult))
		return true;

	printTestHeader (testResult);

	if (bypassId == kNoParamId)
	{
		testResult->addMessage (STR ("This plugin does not have a bypass parameter!!!"));
		return true;
	}
	unprepareProcessing ();

	processData.numSamples = 0;
	processData.numInputs = 0;
	processData.numOutputs = 0;
	processData.inputs = nullptr;
	processData.outputs = nullptr;

	audioEffect->setProcessing (true);

	preProcess (testResult);

	// set bypass on
	// if (paramChanges[0].getParameterId () == bypassId)
	{
		paramChanges[0]->init (bypassId, 1);
		paramChanges[0]->setPoint (0, 0, 1);

		controller->setParamNormalized (bypassId, 1);

		if (controller->getParamNormalized (bypassId) < 1)
		{
			testResult->addErrorMessage (STR ("The bypass parameter was not correctly set!"));
		}
	}

	// flush
	tresult result = audioEffect->process (processData);
	if (result != kResultOk)
	{
		testResult->addErrorMessage (
		    STR ("The component failed to process without audio buffers!"));

		audioEffect->setProcessing (false);
		return false;
	}

	postProcess (testResult);

	audioEffect->setProcessing (false);

	// save State
	FUID uid;
	plugProvider->getComponentUID (uid);

	MemoryStream stream;
	PresetFile::savePreset (&stream, uid, vstPlug, controller, nullptr, 0);

	audioEffect->setProcessing (true);

	preProcess (testResult);

	// set bypass off
	if (paramChanges[0]->getParameterId () == bypassId)
	{
		paramChanges[0]->init (bypassId, 1);
		paramChanges[0]->setPoint (0, 0, 0);

		controller->setParamNormalized (bypassId, 0);

		if (controller->getParamNormalized (bypassId) > 0)
		{
			testResult->addErrorMessage (
			    STR ("The bypass parameter was not correctly set in the controller!"));
		}
	}

	// flush
	result = audioEffect->process (processData);
	if (result != kResultOk)
	{
		testResult->addErrorMessage (
		    STR ("The component failed to process without audio buffers!"));

		audioEffect->setProcessing (false);
		return false;
	}

	postProcess (testResult);

	audioEffect->setProcessing (false);

	// load previous preset
	stream.seek (0, IBStream::kIBSeekSet, nullptr);
	PresetFile::loadPreset (&stream, uid, vstPlug, controller);

	if (controller->getParamNormalized (bypassId) < 1)
	{
		testResult->addErrorMessage (
		    STR ("The bypass parameter is not in sync in the controller!"));
		return false;
	}

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

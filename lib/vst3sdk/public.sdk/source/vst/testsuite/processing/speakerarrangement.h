//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/speakerarrangement.h
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

#pragma once

#include "public.sdk/source/vst/testsuite/processing/process.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Test Speaker Arrangement.
 * \ingroup TestClass
 */
class SpeakerArrangementTest : public ProcessTest
{
public:
	SpeakerArrangementTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl,
	                        SpeakerArrangement inSpArr, SpeakerArrangement outSpArr);

	const char* getName () const SMTG_OVERRIDE;
	static const char* getSpeakerArrangementName (SpeakerArrangement spArr);

	// ITest
	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
	bool prepareProcessing () SMTG_OVERRIDE;
	bool verifySA (int32 numBusses, AudioBusBuffers* buses, SpeakerArrangement spArr,
	               ITestResult* testResult);

private:
	SpeakerArrangement inSpArr;
	SpeakerArrangement outSpArr;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg

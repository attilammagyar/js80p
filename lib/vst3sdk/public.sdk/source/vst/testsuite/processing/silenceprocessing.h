//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/silenceprocessing.h
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
/** Test Silence Processing.
 * \ingroup TestClass
 */
class SilenceProcessingTest : public ProcessTest
{
public:
	SilenceProcessingTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);

	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;

	DECLARE_VSTTEST ("Silence Processing")
protected:
	bool isBufferSilent (void* buffer, int32 numSamples, ProcessSampleSize sampl);
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg

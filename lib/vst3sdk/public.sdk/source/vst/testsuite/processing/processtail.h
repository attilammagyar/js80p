//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/processing/processtail.h
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
/** Test ProcesTail.
 * \ingroup TestClass
 */
class ProcessTailTest : public ProcessTest
{
public:
	ProcessTailTest (ITestPlugProvider* plugProvider, ProcessSampleSize sampl);
	~ProcessTailTest () override;

	DECLARE_VSTTEST ("Check Tail processing")

	// ITest
	bool PLUGIN_API setup () SMTG_OVERRIDE;
	bool PLUGIN_API run (ITestResult* testResult) SMTG_OVERRIDE;
	bool preProcess (ITestResult* testResult) SMTG_OVERRIDE;
	bool postProcess (ITestResult* testResult) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
private:
	uint32 mTailSamples;
	uint32 mInTail;

	float* dataPtrFloat;
	double* dataPtrDouble;
	bool mInSilenceInput;
	bool mDontTest;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg

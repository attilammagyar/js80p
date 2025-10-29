//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/source/vst/utility/test/sampleaccuratetest.cpp
// Created by  : Steinberg, 04/2021
// Description : Tests for Sample Accurate Parameter Changes
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/main/moduleinit.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/utility/sampleaccurate.h"
#include "public.sdk/source/vst/utility/testing.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
static ModuleInitializer InitTests ([] () {
	registerTest ("SampleAccurate::Parameter", STR ("Single Change"), [] (ITestResult* result) {
		ParamID pid = 1;
		SampleAccurate::Parameter param (pid, 0.);
		ParameterValueQueue queue (pid);
		int32 index = 0;
		queue.addPoint (0, 0., index);
		queue.addPoint (100, 1., index);

		param.beginChanges (&queue);
		param.advance (50);
		if (Test::notEqual (param.getValue (), 0.5))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.advance (50);
		if (Test::notEqual (param.getValue (), 1.))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.endChanges ();

		return true;
	});
	registerTest ("SampleAccurate::Parameter", STR ("Multi Change"), [] (ITestResult* result) {
		ParamID pid = 1;
		SampleAccurate::Parameter param (pid, 0.);
		ParameterValueQueue queue (pid);
		int32 index = 0;
		queue.addPoint (0, 0., index);
		queue.addPoint (100, 1., index);
		queue.addPoint (120, 0., index);

		param.beginChanges (&queue);
		param.advance (50);
		if (Test::notEqual (param.getValue (), 0.5))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.advance (50);
		if (Test::notEqual (param.getValue (), 1.))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.advance (20);
		if (Test::notEqual (param.getValue (), 0.))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.endChanges ();

		return true;
	});
	registerTest ("SampleAccurate::Parameter", STR ("Edge"), [] (ITestResult* result) {
		ParamID pid = 1;
		SampleAccurate::Parameter param (pid, 0.);
		ParameterValueQueue queue (pid);
		int32 index = 0;
		queue.addPoint (0, 0., index);
		queue.addPoint (1, 1., index);
		queue.addPoint (2, 0., index);

		param.beginChanges (&queue);
		param.advance (2);
		if (Test::notEqual (param.getValue (), 0.))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.endChanges ();

		return true;
	});
	registerTest ("SampleAccurate::Parameter", STR ("Flush"), [] (ITestResult* result) {
		ParamID pid = 1;
		SampleAccurate::Parameter param (pid, 0.);
		ParameterValueQueue queue (pid);
		int32 index = 0;
		queue.addPoint (0, 0., index);
		queue.addPoint (256, 1., index);
		queue.addPoint (258, 0.5, index);

		param.beginChanges (&queue);
		param.flushChanges ();
		if (Test::notEqual (param.getValue (), 0.5))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}
		param.endChanges ();

		return true;
	});
	registerTest ("SampleAccurate::Parameter", STR ("Callback"), [] (ITestResult* result) {
		ParamID pid = 1;
		SampleAccurate::Parameter param (pid, 0.);
		ParameterValueQueue queue (pid);
		int32 index = 0;
		queue.addPoint (0, 0., index);
		queue.addPoint (128, 0., index);
		queue.addPoint (256, 1., index);
		queue.addPoint (258, 0.5, index);

		param.beginChanges (&queue);
		bool failure = false;
		param.advance (128, [&result, &failure] (auto) {
			result->addErrorMessage (STR ("Unexpected Value"));
			failure = true;
		});
		if (failure)
			return false;
		constexpr auto half = 0.5;
		param.advance (514, [&result, &failure, half = half] (auto value) {
			if (Test::notEqual (value, half))
			{
				result->addErrorMessage (STR ("Unexpected Value"));
				failure = true;
			}
			else
				failure = false;
		});
		if (failure)
			return false;

		param.endChanges ();

		return true;
	});
	registerTest ("SampleAccurate::Parameter", STR ("NoChanges"), [] (ITestResult* result) {
		ParamID pid = 1;
		SampleAccurate::Parameter param (pid, 1.);
		param.endChanges ();

		if (Test::notEqual (param.getValue (), 1.))
		{
			result->addErrorMessage (STR ("Unexpected Value"));
			return false;
		}

		return true;
	});
});

//------------------------------------------------------------------------
} // Vst
} // Steinberg

//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/general/parameterfunctionname.cpp
// Created by  : Steinberg, 04/2020
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/testsuite/general/parameterfunctionname.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstparameterfunctionname.h"
#include "pluginterfaces/vst/ivstunits.h"

#include <unordered_map>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ParameterFunctionNameTest
//------------------------------------------------------------------------
ParameterFunctionNameTest::ParameterFunctionNameTest (ITestPlugProvider* plugProvider)
: TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API ParameterFunctionNameTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (!controller)
	{
		addMessage (testResult, STR ("No Edit Controller supplied!"));
		return true;
	}

	auto iParameterFunctionName = U::cast<IParameterFunctionName> (controller);
	if (!iParameterFunctionName)
	{
		addMessage (testResult, STR ("No IParameterFunctionName support."));
		return true;
	}
	addMessage (testResult, STR ("IParameterFunctionName supported."));

	int32 numParameters = controller->getParameterCount ();
	if (numParameters <= 0)
	{
		addMessage (testResult, STR ("This component does not export any parameters!"));
		return true;
	}

	// used for ID check
	std::unordered_map<int32, int32> paramIds;

	for (int32 i = 0; i < numParameters; ++i)
	{
		ParameterInfo paramInfo = {};

		tresult result = controller->getParameterInfo (i, paramInfo);
		if (result != kResultOk)
		{
			addErrorMessage (testResult, printf ("Parameter %03d: is missing!!!", i));
			return false;
		}

		int32 paramId = paramInfo.id;
		if (paramId < 0)
		{
			addErrorMessage (testResult,
			                 printf ("Parameter %03d (id=%d): Invalid Id!!!", i, paramId));
			return false;
		}

		auto search = paramIds.find (paramId);
		if (search != paramIds.end ())
		{
			addErrorMessage (testResult,
			                 printf ("Parameter %03d (id=%d): ID already used by idx=%03d!!!", i,
			                         paramId, search->second));
			return false;
		}
		else
			paramIds[paramId] = i;
	} // end for each parameter

	auto iUnitInfo2 = U::cast<IUnitInfo> (controller);
	const CString arrayFunctionName[] = {FunctionNameType::kCompGainReduction,
	                                     FunctionNameType::kCompGainReductionMax,
	                                     FunctionNameType::kCompGainReductionPeakHold,
	                                     FunctionNameType::kCompResetGainReductionMax,
	                                     FunctionNameType::kLowLatencyMode,
	                                     FunctionNameType::kRandomize,
	                                     FunctionNameType::kDryWetMix};
	ParamID paramID;
	for (auto item : arrayFunctionName)
	{
		if (iParameterFunctionName->getParameterIDFromFunctionName (kRootUnitId, item, paramID) ==
		    kResultTrue)
		{
			addMessage (testResult,
			            printf ("FunctionName %s supported => paramID %d", item, paramID));

			auto search = paramIds.find (paramID);
			if (search == paramIds.end ())
			{
				addErrorMessage (
				    testResult,
				    printf ("Parameter (id=%d) for FunctionName %s: not Found!!!", paramID, item));
				return false;
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

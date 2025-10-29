//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/unit/checkunitstructure.cpp
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

#include "public.sdk/source/vst/testsuite/unit/checkunitstructure.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstunits.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// UnitStructureTest
//------------------------------------------------------------------------
UnitStructureTest::UnitStructureTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool UnitStructureTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (auto iUnitInfo = U::cast<IUnitInfo> (controller))
	{
		int32 unitCount = iUnitInfo->getUnitCount ();
		if (unitCount <= 0)
		{
			addMessage (testResult,
			            STR ("No units found, while controller implements IUnitInfo !!!"));
		}

		UnitInfo unitInfo = {};
		UnitInfo tmpInfo = {};
		bool rootFound = false;
		for (int32 unitIndex = 0; unitIndex < unitCount; unitIndex++)
		{
			if (iUnitInfo->getUnitInfo (unitIndex, unitInfo) == kResultOk)
			{
				// check parent Id
				if (unitInfo.parentUnitId != kNoParentUnitId) //-1: connected to root
				{
					bool noParent = true;
					for (int32 i = 0; i < unitCount; ++i)
					{
						if (iUnitInfo->getUnitInfo (i, tmpInfo) == kResultOk)
						{
							if (unitInfo.parentUnitId == tmpInfo.id)
							{
								noParent = false;
								break;
							}
						}
					}
					if (noParent && unitInfo.parentUnitId != kRootUnitId)
					{
						addErrorMessage (
						    testResult, printf ("Unit %03d: Parent does not exist!!", unitInfo.id));
						return false;
					}
				}
				else if (!rootFound)
				{
					// root Unit have always the rootID
					if (unitInfo.id != kRootUnitId)
					{
						// we should have a root unit id
						addErrorMessage (
						    testResult,
						    printf ("Unit %03d: Should be the Root Unit => id should be %03d!!",
						            unitInfo.id, kRootUnitId));
						return false;
					}
					rootFound = true;
				}
				else
				{
					addErrorMessage (
					    testResult,
					    printf ("Unit %03d: Has no parent, but there is a root already.",
					            unitInfo.id));
					return false;
				}
			}
			else
			{
				addErrorMessage (testResult, printf ("Unit %03d: No unit info.", unitInfo.id));
				return false;
			}
		}
		addMessage (testResult, STR ("All units have valid parent IDs."));
	}
	else
	{
		addMessage (testResult, STR ("This component does not support IUnitInfo!"));
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

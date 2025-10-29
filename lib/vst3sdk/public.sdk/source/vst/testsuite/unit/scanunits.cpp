//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/unit/scanunits.cpp
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

#include "public.sdk/source/vst/testsuite/unit/scanunits.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstunits.h"
#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// UnitInfoTest
//------------------------------------------------------------------------
UnitInfoTest::UnitInfoTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool UnitInfoTest::run (ITestResult* testResult)
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
		else
		{
			addMessage (testResult, printf ("This component has %d unit(s).", unitCount));
		}

		auto unitIds = std::unique_ptr<int32[]> (new int32[unitCount]);

		for (int32 unitIndex = 0; unitIndex < unitCount; unitIndex++)
		{
			UnitInfo unitInfo = {};

			if (iUnitInfo->getUnitInfo (unitIndex, unitInfo) == kResultOk)
			{
				int32 unitId = unitInfo.id;
				unitIds[unitIndex] = unitId;
				if (unitId < 0)
				{
					addErrorMessage (testResult, printf ("Unit %03d: Invalid ID!", unitIndex));
					return false;
				}

				// check if ID is already used by another unit
				for (int32 idIndex = 0; idIndex < unitIndex; idIndex++)
				{
					if (unitIds[idIndex] == unitIds[unitIndex])
					{
						addErrorMessage (testResult,
						                 printf ("Unit %03d: ID already used!!!", unitIndex));
						return false;
					}
				}

				auto unitName = StringConvert::convert (unitInfo.name);
				if (unitName.empty ())
				{
					addErrorMessage (testResult, printf ("Unit %03d: No name!", unitIndex));
					return false;
				}

				int32 parentUnitId = unitInfo.parentUnitId;
				if (parentUnitId < -1)
				{
					addErrorMessage (testResult,
					                 printf ("Unit %03d: Invalid parent ID!", unitIndex));
					return false;
				}
				else if (parentUnitId == unitId)
				{
					addErrorMessage (
					    testResult,
					    printf ("Unit %03d: Parent ID is equal to Unit ID!", unitIndex));
					return false;
				}

				int32 unitProgramListId = unitInfo.programListId;
				if (unitProgramListId < -1)
				{
					addErrorMessage (testResult,
					                 printf ("Unit %03d: Invalid programlist ID!", unitIndex));
					return false;
				}

				addMessage (
				    testResult,
				    printf ("   Unit%03d (ID = %d): \"%s\" (parent ID = %d, programlist ID = %d)",
				            unitIndex, unitId, unitName.data (), parentUnitId, unitProgramListId));

				// test select Unit
				if (iUnitInfo->selectUnit (unitIndex) == kResultTrue)
				{
					UnitID newSelected = iUnitInfo->getSelectedUnit ();
					if (newSelected != unitIndex)
					{
						addMessage (
						    testResult,
						    printf (
						        "The host has selected Unit ID = %d but getSelectedUnit returns ID = %d!!!",
						        unitIndex, newSelected));
					}
				}
			}
			else
			{
				addMessage (testResult, printf ("Unit%03d: No unit info!", unitIndex));
			}
		}
	}
	else
	{
		addMessage (testResult, STR ("This component has no units."));
	}

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/noteexpression/noteexpression.cpp
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

#include "public.sdk/source/vst/testsuite/noteexpression/noteexpression.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"
#include "pluginterfaces/vst/ivstphysicalui.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// NoteExpressionTest
//------------------------------------------------------------------------
NoteExpressionTest::NoteExpressionTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool PLUGIN_API NoteExpressionTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (!controller)
	{
		addMessage (testResult, STR ("No Edit Controller supplied!"));
		return true;
	}

	auto noteExpression = U::cast<INoteExpressionController> (controller);
	if (!noteExpression)
	{
		addMessage (testResult, STR ("No Note Expression interface supplied!"));
		return true;
	}

	auto noteExpressionPUIMapping = U::cast<INoteExpressionPhysicalUIMapping> (controller);
	if (!noteExpressionPUIMapping)
	{
		addMessage (testResult, STR ("No Note Expression PhysicalUIMapping interface supplied!"));
	}

	int32 eventBusCount = vstPlug->getBusCount (kEvent, kInput);

	const uint32 maxPUI = kPUITypeCount;
	PhysicalUIMap puiArray[maxPUI];
	PhysicalUIMapList puiMap;
	puiMap.count = maxPUI;
	puiMap.map = puiArray;
	for (uint32 i = 0; i < maxPUI; i++)
	{
		puiMap.map[i].physicalUITypeID = static_cast<PhysicalUITypeID> (i);
	}

	for (int32 bus = 0; bus < eventBusCount; bus++)
	{
		BusInfo busInfo;
		vstPlug->getBusInfo (kEvent, kInput, bus, busInfo);

		for (int16 channel = 0; channel < busInfo.channelCount; channel++)
		{
			int32 count = noteExpression->getNoteExpressionCount (bus, channel);
			if (count > 0)
			{
				addMessage (testResult, printf ("Note Expression count bus[%d], channel[%d]: %d",
				                                bus, channel, count));
			}

			for (int32 i = 0; i < count; ++i)
			{
				NoteExpressionTypeInfo info;
				if (noteExpression->getNoteExpressionInfo (bus, channel, i, info) == kResultTrue)
				{
					addMessage (testResult, printf ("Note Expression TypeID: %d [%s]", info.typeId,
					                                StringConvert::convert (info.title).data ()));
					NoteExpressionTypeID id = info.typeId;
					NoteExpressionValue valueNormalized = info.valueDesc.defaultValue;
					String128 string;
					if (noteExpression->getNoteExpressionStringByValue (
					        bus, channel, id, valueNormalized, string) != kResultTrue)
					{
						addMessage (
						    testResult,
						    printf (
						        "Note Expression getNoteExpressionStringByValue (%d, %d, %d) return kResultFalse!",
						        bus, channel, id));
					}

					if (noteExpression->getNoteExpressionValueByString (
					        bus, channel, id, string, valueNormalized) != kResultTrue)
					{
						addMessage (
						    testResult,
						    printf (
						        "Note Expression getNoteExpressionValueByString (%d, %d, %d) return kResultFalse!",
						        bus, channel, id));
					}
				}
				else
				{
					addErrorMessage (
					    testResult,
					    printf (
					        "Note Expression getNoteExpressionInfo (%d, %d, %d) return kResultFalse!",
					        bus, channel, i));
					return false;
				}
			}

			if (noteExpressionPUIMapping)
			{
				for (uint32 i = 0; i < maxPUI; i++)
				{
					puiMap.map[i].noteExpressionTypeID = kInvalidTypeID;
				}

				if (noteExpressionPUIMapping->getPhysicalUIMapping (bus, channel, puiMap) ==
				    kResultFalse)
				{
					addMessage (
					    testResult,
					    printf (
					        "Note Expression getPhysicalUIMapping (%d, %d, ...) return kResultFalse!",
					        bus, channel));
				}
				else
				{
					for (uint32 i = 0; i < maxPUI; i++)
					{
						addMessage (testResult,
						            printf ("Note Expression PhysicalUIMapping: %d => %d",
						                    puiMap.map[i].noteExpressionTypeID,
						                    puiMap.map[i].physicalUITypeID));
					}
				}
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

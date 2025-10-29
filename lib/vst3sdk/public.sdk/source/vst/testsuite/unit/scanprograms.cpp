//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/unit/scanprograms.cpp
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

#include "public.sdk/source/vst/testsuite/unit/scanprograms.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// ProgramInfoTest
//------------------------------------------------------------------------
ProgramInfoTest::ProgramInfoTest (ITestPlugProvider* plugProvider) : TestBase (plugProvider)
{
}

//------------------------------------------------------------------------
bool ProgramInfoTest::run (ITestResult* testResult)
{
	if (!testResult || !vstPlug)
		return false;

	printTestHeader (testResult);

	if (auto iUnitInfo = U::cast<IUnitInfo> (controller))
	{
		int32 programListCount = iUnitInfo->getProgramListCount ();
		if (programListCount == 0)
		{
			addMessage (testResult, STR ("This component does not export any programs."));
			return true;
		}
		else if (programListCount < 0)
		{
			addErrorMessage (testResult,
			                 STR ("IUnitInfo::getProgramListCount () returned a negative number."));
			return false;
		}

		// used to check double IDs
		auto programListIds = std::unique_ptr<int32[]> (new int32[programListCount]);

		for (int32 programListIndex = 0; programListIndex < programListCount; programListIndex++)
		{
			// get programm list info
			ProgramListInfo programListInfo;
			if (iUnitInfo->getProgramListInfo (programListIndex, programListInfo) == kResultOk)
			{
				int32 programListId = programListInfo.id;
				programListIds[programListIndex] = programListId;
				if (programListId < 0)
				{
					addErrorMessage (testResult,
					                 printf ("Programlist %03d: Invalid ID!!!", programListIndex));
					return false;
				}

				// check if ID is already used by another parameter
				for (int32 idIndex = 0; idIndex < programListIndex; idIndex++)
				{
					if (programListIds[idIndex] == programListIds[programListIndex])
					{
						addErrorMessage (testResult, printf ("Programlist %03d: ID already used!!!",
						                                     programListIndex));
						return false;
					}
				}

				auto programListName = StringConvert::convert (programListInfo.name);
				if (programListName.empty ())
				{
					addErrorMessage (testResult, printf ("Programlist %03d (id=%d): No name!!!",
					                                     programListIndex, programListId));
					return false;
				}

				int32 programCount = programListInfo.programCount;
				if (programCount <= 0)
				{
					addMessage (
					    testResult,
					    printf (
					        "Programlist %03d (id=%d): \"%s\" No programs!!! (programCount is null!)",
					        programListIndex, programListId,
					        StringConvert::convert (programListName).data ()));
					// return false;
				}

				addMessage (testResult, printf ("Programlist %03d (id=%d):  \"%s\" (%d programs).",
				                                programListIndex, programListId,
				                                programListName.data (), programCount));

				for (int32 programIndex = 0; programIndex < programCount; programIndex++)
				{
					TChar programName[256];
					if (iUnitInfo->getProgramName (programListId, programIndex, programName) ==
					    kResultOk)
					{
						if (programName[0] == 0)
						{
							addErrorMessage (
							    testResult,
							    printf ("Programlist %03d->Program %03d: has no name!!!",
							            programListIndex, programIndex));
							return false;
						}

						auto programNameUTF8 = StringConvert::convert (programName);
						auto msg = printf ("Programlist %03d->Program %03d: \"%s\"",
						                   programListIndex, programIndex, programNameUTF8.data ());

						String128 programInfo {};
						if (iUnitInfo->getProgramInfo (programListId, programIndex,
						                               PresetAttributes::kInstrument,
						                               programInfo) == kResultOk)
						{
							auto programInfoUTF8 = StringConvert::convert (programInfo);
							msg += StringConvert::convert (" (instrument = \"");
							msg += (const char16_t*)programInfo;
							msg += StringConvert::convert ("\")");
						}

						addMessage (testResult, msg.data ());

						if (iUnitInfo->hasProgramPitchNames (programListId, programIndex) ==
						    kResultOk)
						{
							addMessage (testResult, printf (" => \"%s\": supports PitchNames",
							                                programNameUTF8.data ()));

							String128 pitchName = {0};
							for (int16 midiPitch = 0; midiPitch < 128; midiPitch++)
							{
								if (iUnitInfo->getProgramPitchName (programListId, programIndex,
								                                    midiPitch,
								                                    pitchName) == kResultOk)
								{
									msg = printf ("   => MIDI Pitch %d => \"", midiPitch);
									msg += (const char16_t*)pitchName;
									msg += StringConvert::convert ("\"");
									addMessage (testResult, msg.data ());
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		addMessage (testResult, STR ("This component does not export any programs."));

		// check if not more than 1 program change parameter is defined
		int32 numPrgChanges = 0;
		for (int32 i = 0; i < controller->getParameterCount (); ++i)
		{
			ParameterInfo paramInfo = {};
			if (controller->getParameterInfo (i, paramInfo) != kResultOk)
			{
				if (paramInfo.flags & ParameterInfo::kIsProgramChange)
					numPrgChanges++;
			}
		}
		if (numPrgChanges > 1)
		{
			addErrorMessage (
			    testResult,
			    printf ("More than 1 programChange Parameter (%d) without support of IUnitInfo!!!",
			            numPrgChanges));
		}
	}
	return true;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

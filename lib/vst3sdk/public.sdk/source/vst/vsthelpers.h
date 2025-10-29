//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vsthelpers.h
// Created by  : Steinberg, 11/2018
// Description : common defines
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstattributes.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include <cstring>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace Helpers {
//------------------------------------------------------------------------
/** Helpers */
//------------------------------------------------------------------------

//------------------------------------------------------------------------
/** Retrieve from a IBStream the state type, here the StateType::kProject
	return kResultTrue if the state is coming from a project,
	return kResultFalse if the state is coming from a preset,
	return kNotImplemented if the host does not implement such feature
*/
inline tresult isProjectState (IBStream* state)
{
	if (!state)
		return kInvalidArgument;

	auto stream = U::cast<IStreamAttributes> (state);
	if (!stream)
		return kNotImplemented;

	if (IAttributeList* list = stream->getAttributes ())
	{
		// get the current type (project/Default..) of this state
		String128 string = {0};
		if (list->getString (PresetAttributes::kStateType, string, 128 * sizeof (TChar)) ==
		    kResultTrue)
		{
			UString128 tmp (string);
			char ascii[128];
			tmp.toAscii (ascii, 128);
			if (strncmp (ascii, StateType::kProject, strlen (StateType::kProject)) == 0)
			{
				return kResultTrue;
			}
			return kResultFalse;
		}
	}
	return kNotImplemented;
}
/**@}*/

//------------------------------------------------------------------------
} // namespace Helpers
} // namespace Vst
} // namespace Steinberg

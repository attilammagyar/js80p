//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstspeakerarray,.h
// Created by  : Steinberg, 04/2015
// Description : Helper class representing speaker arrangement as array of speaker types.
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include <cstdlib>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// SpeakerArray
/** Helper class representing speaker arrangement as array of speaker types.
*/
class SpeakerArray
{
public:
//------------------------------------------------------------------------
	SpeakerArray (SpeakerArrangement arr = 0) { setArrangement (arr); }

	enum
	{
		kMaxSpeakers = 64
	};

	typedef uint64 SpeakerType;

	int32 total () const { return count; }
	SpeakerType at (int32 index) const { return speaker[index]; }

	void setArrangement (SpeakerArrangement arr)
	{
		count = 0;
		memset (speaker, 0, sizeof (speaker));

		for (int32 i = 0; i < kMaxSpeakers; i++)
		{
			SpeakerType mask = 1ll << i;
			if (arr & mask)
				speaker[count++] = mask;
		}
	}

	SpeakerArrangement getArrangement () const
	{
		SpeakerArrangement arr = 0;
		for (int32 i = 0; i < count; i++)
			arr |= speaker[i];
		return arr;
	}

	int32 getSpeakerIndex (SpeakerType which) const
	{
		for (int32 i = 0; i < count; i++)
			if (speaker[i] == which)
				return i;
		return -1;
	}
//------------------------------------------------------------------------
protected:
	int32 count;
	SpeakerType speaker[kMaxSpeakers];
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

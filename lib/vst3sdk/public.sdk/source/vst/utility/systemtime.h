//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/systemtime.h
// Created by  : Steinberg, 06/2023
// Description : VST Component System Time API Helper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivsteditcontroller.h"

#include <functional>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** SystemTime Helper class
 *
 *	Get the system time on the vst controller side.
 *
 *	If supported by the host this uses the same clock as used in the
 *	realtime audio process block. Otherwise an approximation via platform APIs is used.
 *
 *	This can be used to synchronize audio and visuals. As known, the audio process block is always
 *	called ealier as the audio which was generated passes the audio monitors or headphones.
 *	Depending on the audio graph this can be so long that your eyes will see the visualization (if
 *	not synchronized) earlier then your ears will hear the sound.
 *	To synchronize you need to queue your visualization data on the controller side timestamped with
 *	the time from the process block and dequed when it's time for the data to be visualized.
 */
class SystemTime
{
public:
	SystemTime (IComponentHandler* componentHandler);
	SystemTime (const SystemTime& st);
	SystemTime (SystemTime&& st) noexcept;
	SystemTime& operator= (const SystemTime& st);
	SystemTime& operator= (SystemTime&& st) noexcept;

	/** get the current system time
	 */
	int64 get () const { return getImpl (); }

//------------------------------------------------------------------------
	using GetImplFunc = std::function<int64 ()>;

private:
	GetImplFunc getImpl;
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg

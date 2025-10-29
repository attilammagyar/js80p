//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vsteventshelper.h
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

#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/vst/ivstevents.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace Helpers {
//------------------------------------------------------------------------
/** bound a value between a min and max */
template <class T>
inline T boundTo (T minval, T maxval, T x)
{
	if (x < minval)
		return minval;
	if (x > maxval)
		return maxval;
	return x;
}

//------------------------------------------------------------------------
/** Initialized a Event */
inline Event& init (Event& event, uint16 type, int32 busIndex = 0, int32 sampleOffset = 0,
                    TQuarterNotes ppqPosition = 0, uint16 flags = 0)
{
	event.busIndex = busIndex;
	event.sampleOffset = sampleOffset;
	event.ppqPosition = ppqPosition;
	event.flags = flags;
	event.type = type;
	return event;
}

//------------------------------------------------------------------------
/** Returns normalized value of a LegacyMIDICCOutEvent value [0, 127] */
inline ParamValue getMIDINormValue (uint8 value)
{
	return boundTo<ParamValue> (0., 1., ToNormalized<ParamValue> (value, 127));
}

//------------------------------------------------------------------------
/** Returns LegacyMIDICCOut value [0, 127] from a normalized value [0., 1.] */
inline int8 getMIDICCOutValue (ParamValue value)
{
	return boundTo<int8> (0, 127, FromNormalized<ParamValue> (value, 127));
}

//------------------------------------------------------------------------
/** Returns MIDI 14bit value from a normalized value [0., 1.] */
inline int16 getMIDI14BitValue (ParamValue value)
{
	return boundTo<int16> (0, 0x3FFF, FromNormalized<ParamValue> (value, 0x3FFF));
}

//------------------------------------------------------------------------
/** Returns normalized value of a MIDI 14bit [0., 0x3FFF] */
inline ParamValue getMIDI14BitNormValue (int16 value)
{
	return boundTo<ParamValue> (0.f, 1., ToNormalized<ParamValue> (value, 0x3FFF));
}

//------------------------------------------------------------------------
/** Returns pitchbend value from a PitchBend LegacyMIDICCOut Event */
inline int16 getPitchBendValue (const LegacyMIDICCOutEvent& e)
{
	return ((e.value & 0x7F) | ((e.value2 & 0x7F) << 7));
}

//------------------------------------------------------------------------
/** set a normalized pitchbend value to a LegacyMIDICCOut Event */
inline void setPitchBendValue (LegacyMIDICCOutEvent& e, ParamValue value)
{
	auto newValue = getMIDI14BitValue (value);
	e.value = (newValue & 0x7F);
	e.value2 = ((newValue >> 7) & 0x7F);
}

//------------------------------------------------------------------------
/** Returns normalized pitchbend value from a PitchBend LegacyMIDICCOut Event */
inline ParamValue getNormPitchBendValue (const LegacyMIDICCOutEvent& e)
{
	return getMIDI14BitNormValue (getPitchBendValue (e));
}

//------------------------------------------------------------------------
/** Initialized a LegacyMIDICCOutEvent */
inline LegacyMIDICCOutEvent& initLegacyMIDICCOutEvent (Event& event, uint8 controlNumber,
                                                       uint8 channel = 0, int8 value = 0,
                                                       int8 value2 = 0)
{
	init (event, Event::kLegacyMIDICCOutEvent);
	event.midiCCOut.channel = channel;
	event.midiCCOut.controlNumber = controlNumber;
	event.midiCCOut.value = value;
	event.midiCCOut.value2 = value2;
	return event.midiCCOut;
}
/**@}*/

//------------------------------------------------------------------------
} // namespace Helpers
} // namespace Vst
} // namespace Steinberg

//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/mpeprocessor.h
// Created by  : Steinberg, 07/2017
// Description : VST 3 MIDI-MPE decomposer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace MPE {

using NoteID = int32_t;
using Pitch = uint32_t;
using Channel = uint32_t;
using Velocity = float;
using NormalizedValue = double;

//------------------------------------------------------------------------
/** MPE per note controller enumeration */
enum class Controller : uint32_t
{
	/** Pressure MPE controller */
	Pressure,
	/** X / horizontal MPE controller */
	X,
	/** Y / vertical MPE controller */
	Y,
	/** no MPE controller */
	None
};

//------------------------------------------------------------------------
struct Handler
{
	/** Generate a new noteID
	 *
	 * called by the processor for a new NoteID. The handler has to make sure that the noteID is
	 * not used again until the releaseNoteID method is called.
	 *
	 * 	@param outNoteID on return contains the new noteID if this call succeed
	 * 	@return true if outNoteID was filled with a new noteID
	 */
	virtual bool generateNewNoteID (NoteID& outNoteID) = 0;

	/** Release a noteID
	 *
	 *	called by the processor when the NoteID is no longer used.
	 *
	 *	@param noteID the noteID not longer in use
	 */
	virtual void releaseNoteID (NoteID noteID) = 0;

	/** A note on was transmitted
	 *
	 *	@param noteID unique note identifier
	 *	@param pitch note pitch
	 *	@param velocity note on velocity
	 */
	virtual void onMPENoteOn (NoteID noteID, Pitch pitch, Velocity velocity) = 0;

	/** A note off was transmitted
	 *
	 *	@param noteID unique note identifier
	 *	@param pitch note pitch
	 *	@param velocity note off velocity
	 */
	virtual void onMPENoteOff (NoteID noteID, Pitch pitch, Velocity velocity) = 0;

	/** A new per note controller change was transmitted
	 *
	 *	@param noteID unique note identifier
	 *	@param cc the MIDI controller which changed
	 *	@param value the value of the change in the range [0..1]
	 */
	virtual void onMPEControllerChange (NoteID noteID, Controller cc, NormalizedValue value) = 0;

	/** Non MPE MIDI input data was transmitted
	 *
	 *	@param data MIDI data buffer
	 *	@param dataSize size of the MIDI data buffer in bytes
	 */
	virtual void onOtherInput (const uint8_t* data, size_t dataSize) = 0;

	/** Sysex MIDI data was transmitted
	 *
	 *	@param data Sysex data buffer
	 *	@param dataSize size of sysex data buffer in bytes
	 */
	virtual void onSysexInput (const uint8_t* data, size_t dataSize) = 0;

	// error handling
	/** called when the handler did not return a new note ID */
	virtual void errorNoteDroppedBecauseNoNoteID (Pitch pitch) = 0;
	/** the internal note stack for this channel is full, happens on too many note ons per channel */
	virtual void errorNoteDroppedBecauseNoteStackFull (Channel channel, Pitch pitch) = 0;
	/** called when the internal data has no reference to this note off */
	virtual void errorNoteForNoteOffNotFound (Channel channel, Pitch pitch) = 0;
	/** called when a program change was received inside the MPE zone which is a protocol violation */
	virtual void errorProgramChangeReceivedInMPEZone () = 0;
};

//------------------------------------------------------------------------
/** Input MIDI Message enumeration */
enum InputMIDIMessage : uint32_t
{
	MIDICC_0 = 0,
	MIDICC_127 = 127,
	ChannelPressure = 128,
	PitchBend = 129,
	Aftertouch = 130,
};

//------------------------------------------------------------------------
/** MPE setup structure */
struct Setup
{
	Channel masterChannel {0};
	Channel memberChannelBegin {1};
	Channel memberChannelEnd {14};
	InputMIDIMessage pressure {ChannelPressure};
	InputMIDIMessage x {PitchBend};
	InputMIDIMessage y {static_cast<InputMIDIMessage> (74)};
};

//------------------------------------------------------------------------
/** MPE Decompose Processor
 *
 *	decomposes MPE MIDI messages
 *
 */
class Processor
{
public:
	Processor (Handler* delegate, size_t maxNotesPerChannel = 16);
	~Processor () noexcept;

	const Setup& getSetup () const;
	/** change the MPE setup
	 *
	 *	make sure that MIDI processing is stopped while this is called.
	 *
	 *	@param setup new setup
	 */
	void changeSetup (const Setup& setup);
	/** reset all notes
	 *
	 *	All playing notes will be stopped and note identifiers are released.
	 *
	 */
	void reset ();

	/** feed new native MIDI data
	 *
	 *	@param data MIDI data buffer
	 *	@param dataSize data buffer size in bytes
	 */
	void processMIDIInput (const uint8_t* data, size_t dataSize);

private:
	int32_t onNoteOn (const uint8_t* data, size_t dataSize);
	int32_t onNoteOff (const uint8_t* data, size_t dataSize);
	int32_t onAftertouch (const uint8_t* data, size_t dataSize);
	int32_t onController (const uint8_t* data, size_t dataSize);
	int32_t onProgramChange (const uint8_t* data, size_t dataSize);
	int32_t onChannelPressure (const uint8_t* data, size_t dataSize);
	int32_t onPitchWheel (const uint8_t* data, size_t dataSize);

	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
} // MPE
} // Vst
} // Steinberg

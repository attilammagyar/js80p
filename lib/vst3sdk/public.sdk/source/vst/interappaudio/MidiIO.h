//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/MidiIO.h
// Created by  : Steinberg, 09/2013.
// Description : VST 3 InterAppAudio
// Flags       : clang-format SMTGSequencer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "AudioIO.h"
#include <CoreMIDI/CoreMIDI.h>
#include <vector>

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

//-----------------------------------------------------------------------------
class MidiIO
{
public:
	static MidiIO& instance ();

	bool setEnabled (bool state);
	bool isEnabled () const;

	// MIDI Network is experimental, do not use yet
	void setMidiNetworkEnabled (bool state);
	bool isMidiNetworkEnabled () const;
	void setMidiNetworkPolicy (MIDINetworkConnectionPolicy policy);
	MIDINetworkConnectionPolicy getMidiNetworkPolicy () const;

	void addProcessor (IMidiProcessor* processor);
	void removeProcessor (IMidiProcessor* processor);

//-----------------------------------------------------------------------------
private:
	MidiIO ();
	~MidiIO ();

	void onInput (const MIDIPacketList* pktlist);
	void onSourceAdded (MIDIObjectRef source);
	void onSetupChanged ();
	void disconnectSources ();

	MIDIClientRef client {0};
	MIDIPortRef inputPort {0};
	MIDIEndpointRef destPort {0};

	using MidiProcessors = std::vector<IMidiProcessor*>;
	MidiProcessors midiProcessors;

	using ConnectionList = std::vector<MIDIEndpointRef>;
	ConnectionList connectedSources;

	static void readProc (const MIDIPacketList* pktlist, void* readProcRefCon, void* srcConnRefCon);
	static void notifyProc (const MIDINotification* message, void* refCon);
};

//------------------------------------------------------------------------
} // namespace InterAppAudio
} // namespace Vst
} // namespace Steinberg

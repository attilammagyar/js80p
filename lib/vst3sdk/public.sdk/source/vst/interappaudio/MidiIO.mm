//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/MidiIO.mm
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

#import "MidiIO.h"

#import <CoreMIDI/MIDINetworkSession.h>

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

//-----------------------------------------------------------------------------
MidiIO& MidiIO::instance ()
{
	static MidiIO gInstance;
	return gInstance;
}

//-----------------------------------------------------------------------------
MidiIO::MidiIO () = default;

//-----------------------------------------------------------------------------
MidiIO::~MidiIO ()
{
	setEnabled (false);
}

//-----------------------------------------------------------------------------
void MidiIO::addProcessor (IMidiProcessor* processor)
{
	midiProcessors.push_back (processor);
}

//-----------------------------------------------------------------------------
void MidiIO::removeProcessor (IMidiProcessor* processor)
{
	auto it = std::find (midiProcessors.begin (), midiProcessors.end (), processor);
	if (it != midiProcessors.end ())
	{
		midiProcessors.erase (it);
	}
}

//-----------------------------------------------------------------------------
bool MidiIO::isEnabled () const
{
	return client != 0;
}

//-----------------------------------------------------------------------------
bool MidiIO::setEnabled (bool state)
{
	if (state)
	{
		if (client)
			return true;

		OSStatus err;
		NSString* name = [[NSBundle mainBundle] bundleIdentifier];
		if ((err =
		         MIDIClientCreate ((__bridge CFStringRef)name, notifyProc, this, &client) != noErr))
			return false;

		if ((err = MIDIInputPortCreate (client, CFSTR ("Input"), readProc, this, &inputPort) !=
		           noErr))
		{
			MIDIClientDispose (client);
			client = 0;
			return false;
		}
		name = [[[NSBundle mainBundle] infoDictionary] valueForKey:@"CFBundleDisplayName"];
		if ((err = MIDIDestinationCreate (client, (__bridge CFStringRef)name, readProc, this,
		                                  &destPort) != noErr))
		{
			MIDIPortDispose (inputPort);
			inputPort = 0;
			MIDIClientDispose (client);
			client = 0;
			return false;
		}
	}
	else
	{
		if (client == 0)
			return true;
		disconnectSources ();
		MIDIEndpointDispose (destPort);
		destPort = 0;
		MIDIPortDispose (inputPort);
		inputPort = 0;
		MIDIClientDispose (client);
		client = 0;
	}
	return true;
}

//-----------------------------------------------------------------------------
void MidiIO::setMidiNetworkEnabled (bool state)
{
	if (inputPort && isMidiNetworkEnabled () != state)
	{
		if (!state)
		{
			MIDIPortDisconnectSource (inputPort,
			                          [MIDINetworkSession defaultSession].sourceEndpoint);
		}
		[MIDINetworkSession defaultSession].enabled = state;
		if (state)
		{
			MIDIPortConnectSource (inputPort, [MIDINetworkSession defaultSession].sourceEndpoint,
			                       0);
		}
	}
}

//-----------------------------------------------------------------------------
bool MidiIO::isMidiNetworkEnabled () const
{
	return [MIDINetworkSession defaultSession].isEnabled;
}

//-----------------------------------------------------------------------------
void MidiIO::setMidiNetworkPolicy (MIDINetworkConnectionPolicy policy)
{
	[MIDINetworkSession defaultSession].connectionPolicy = policy;
}

//-----------------------------------------------------------------------------
MIDINetworkConnectionPolicy MidiIO::getMidiNetworkPolicy () const
{
	return [MIDINetworkSession defaultSession].connectionPolicy;
}

//-----------------------------------------------------------------------------
void MidiIO::onInput (const MIDIPacketList* pktlist)
{
	const MIDIPacket* packet = &pktlist->packet[0];
	for (UInt32 i = 0; i < pktlist->numPackets; i++)
	{
		for (auto processor : midiProcessors)
		{
			processor->onMIDIEvent (packet->data[0], packet->data[1], packet->data[2], 0, false);
		}
		packet = MIDIPacketNext (packet);
	}
}

//-----------------------------------------------------------------------------
void MidiIO::onSourceAdded (MIDIObjectRef source)
{
	connectedSources.push_back ((MIDIEndpointRef)source);
	MIDIPortConnectSource (inputPort, (MIDIEndpointRef)source, NULL);
}

//-----------------------------------------------------------------------------
void MidiIO::disconnectSources ()
{
	for (auto source : connectedSources)
		MIDIPortDisconnectSource (inputPort, source);
	connectedSources.clear ();
}

//-----------------------------------------------------------------------------
void MidiIO::onSetupChanged ()
{
	disconnectSources ();
	ItemCount numSources = MIDIGetNumberOfSources ();
	for (ItemCount i = 0; i < numSources; i++)
	{
		onSourceAdded (MIDIGetSource (i));
	}
}

//-----------------------------------------------------------------------------
void MidiIO::readProc (const MIDIPacketList* pktlist, void* readProcRefCon, void* srcConnRefCon)
{
	MidiIO* io = static_cast<MidiIO*> (readProcRefCon);
	io->onInput (pktlist);
}

//-----------------------------------------------------------------------------
void MidiIO::notifyProc (const MIDINotification* message, void* refCon)
{
	if (message->messageID == kMIDIMsgSetupChanged)
	{
		MidiIO* mio = (MidiIO*)refCon;
		mio->onSetupChanged ();
	}
}
}
}
} // namespaces

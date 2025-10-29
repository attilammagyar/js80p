//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/VST3Plugin.h
// Created by  : Steinberg, 08/2013.
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

/// \cond ignore

#import "AudioIO.h"
#import "public.sdk/source/vst/hosting/eventlist.h"
#import "public.sdk/source/vst/hosting/parameterchanges.h"
#import "public.sdk/source/vst/hosting/processdata.h"
#import "public.sdk/source/vst/utility/ringbuffer.h"
#import "base/source/fobject.h"
#import "base/source/timer.h"
#import "pluginterfaces/vst/ivstaudioprocessor.h"
#import "pluginterfaces/vst/ivsteditcontroller.h"
#import "pluginterfaces/vst/ivstprocesscontext.h"
#import <atomic>
#import <map>

#ifndef __OBJC__
struct NSData;
#endif

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

static const int32 kMaxUIEvents = 100;

//------------------------------------------------------------------------
class VST3Plugin : public FObject,
                   public IComponentHandler,
                   public IAudioIOProcessor,
                   public ITimerCallback
{
public:
//------------------------------------------------------------------------
	VST3Plugin ();
	virtual ~VST3Plugin ();

	bool init ();

	IEditController* getEditController () const { return editController; }
	IAudioProcessor* getAudioProcessor () const { return processor; }

	tresult scheduleEventFromUI (Event& event);

	NSData* getProcessorState ();
	bool setProcessorState (NSData* data);

	NSData* getControllerState ();
	bool setControllerState (NSData* data);

	OBJ_METHODS (VST3Plugin, FObject)
	REFCOUNT_METHODS (FObject)
	DEFINE_INTERFACES
		DEF_INTERFACE (IComponentHandler)
	END_DEFINE_INTERFACES (FObject)
protected:
	typedef std::map<uint32, uint32> NoteIDPitchMap;
	typedef uint32 ChannelAndCtrlNumber;
	typedef std::map<ChannelAndCtrlNumber, ParamID> MIDIControllerToParamIDMap;

	void createProcessorAndController ();

	void updateProcessContext (AudioIO* audioIO);
	MIDIControllerToParamIDMap createMIDIControllerToParamIDMap ();

	// IComponentHandler
	tresult PLUGIN_API beginEdit (ParamID id) override;
	tresult PLUGIN_API performEdit (ParamID id, ParamValue valueNormalized) override;
	tresult PLUGIN_API endEdit (ParamID id) override;
	tresult PLUGIN_API restartComponent (int32 flags) override;

	// IAudioIOProcessor
	void willStartAudio (AudioIO* audioIO) override;
	void didStopAudio (AudioIO* audioIO) override;
	void onMIDIEvent (UInt32 status, UInt32 data1, UInt32 data2, UInt32 sampleOffset,
	                  bool withinRealtimeThread) override;
	void process (const AudioTimeStamp* timeStamp, UInt32 busNumber, UInt32 numFrames,
	              AudioBufferList* ioData, bool& outputIsSilence, AudioIO* audioIO) override;

	// ITimerCallback
	void onTimer (Timer* timer) override;

	IAudioProcessor* processor {nullptr};
	IEditController* editController {nullptr};
	Timer* timer {nullptr};

	HostProcessData processData;
	ProcessContext processContext;
	ParameterChangeTransfer inputParamChangeTransfer;
	ParameterChangeTransfer outputParamChangeTransfer;
	ParameterChanges inputParamChanges;
	ParameterChanges outputParamChanges;
	EventList inputEvents;

	NoteIDPitchMap noteIDPitchMap;
	std::atomic<int32> lastNodeID {0};

	bool processing {false};

	MIDIControllerToParamIDMap midiControllerToParamIDMap;

	OneReaderOneWriter::RingBuffer<Event> uiScheduledEvents;

	static ChannelAndCtrlNumber channelAndCtrlNumber (uint16 channel, CtrlNumber ctrler)
	{
		return (channel << 16) + ctrler;
	}
};

//------------------------------------------------------------------------
} // namespace InterAppAudio
} // namespace Vst
} // namespace Steinberg

/// \endcond

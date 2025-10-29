//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/AudioIO.h
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

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AUComponent.h>
#include <vector>

#ifndef __OBJC__
struct UIImage;
struct NSString;
#endif

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

class AudioIO;

//------------------------------------------------------------------------
class IMidiProcessor
{
public:
	virtual void onMIDIEvent (UInt32 status, UInt32 data1, UInt32 data2, UInt32 sampleOffset,
	                          bool withinRealtimeThread) = 0;
};

//------------------------------------------------------------------------
class IAudioIOProcessor : public IMidiProcessor
{
public:
	virtual void willStartAudio (AudioIO* audioIO) = 0;
	virtual void didStopAudio (AudioIO* audioIO) = 0;

	virtual void process (const AudioTimeStamp* timeStamp, UInt32 busNumber, UInt32 numFrames,
	                      AudioBufferList* ioData, bool& outputIsSilence, AudioIO* audioIO) = 0;
};

//------------------------------------------------------------------------
class AudioIO
{
public:
	static AudioIO* instance ();

	tresult init (OSType type, OSType subType, OSType manufacturer, CFStringRef name);

	bool switchToHost ();
	bool sendRemoteControlEvent (AudioUnitRemoteControlEvent event);
	UIImage* getHostIcon ();

	tresult start ();
	tresult stop ();

	tresult addProcessor (IAudioIOProcessor* processor);
	tresult removeProcessor (IAudioIOProcessor* processor);

	// accessors
	AudioUnit getRemoteIO () const { return remoteIO; }
	SampleRate getSampleRate () const { return sampleRate; }
	bool getInterAppAudioConnected () const { return interAppAudioConnected; }

	// host context information
	bool getBeatAndTempo (Float64& beat, Float64& tempo);
	bool getMusicalTimeLocation (UInt32& deltaSampleOffset, Float32& timeSigNumerator,
	                             UInt32& timeSigDenominator, Float64& downBeat);
	bool getTransportState (Boolean& isPlaying, Boolean& isRecording,
	                        Boolean& transportStateChanged, Float64& sampleInTimeLine,
	                        Boolean& isCycling, Float64& cycleStartBeat, Float64& cycleEndBeat);

	void setStaticFallbackTempo (Float64 tempo) { staticTempo = tempo; }
	Float64 getStaticFallbackTempo () const { return staticTempo; }

	static NSString* kConnectionStateChange;
//------------------------------------------------------------------------
protected:
	AudioIO ();
	~AudioIO ();

	static OSStatus inputCallbackStatic (void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
	                                     const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
	                                     UInt32 inNumberFrames, AudioBufferList* ioData);
	static OSStatus renderCallbackStatic (void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
	                                      const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
	                                      UInt32 inNumberFrames, AudioBufferList* ioData);
	static void propertyChangeStatic (void* inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID,
	                                  AudioUnitScope inScope, AudioUnitElement inElement);
	static void midiEventCallbackStatic (void* inRefCon, UInt32 inStatus, UInt32 inData1,
	                                     UInt32 inData2, UInt32 inOffsetSampleFrame);

	OSStatus inputCallback (AudioUnitRenderActionFlags* ioActionFlags,
	                        const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
	                        UInt32 inNumberFrames, AudioBufferList* ioData);
	OSStatus renderCallback (AudioUnitRenderActionFlags* ioActionFlags,
	                         const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
	                         UInt32 inNumberFrames, AudioBufferList* ioData);
	void midiEventCallback (UInt32 inStatus, UInt32 inData1, UInt32 inData2,
	                        UInt32 inOffsetSampleFrame);

	void remoteIOPropertyChanged (AudioUnitPropertyID inID, AudioUnitScope inScope,
	                              AudioUnitElement inElement);

	void setAudioSessionActive (bool state);
	tresult setupRemoteIO (OSType type);
	tresult setupAUGraph (OSType type);
	void updateInterAppAudioConnectionState ();

	AudioUnit remoteIO {nullptr};
	AUGraph graph {nullptr};
	AudioBufferList* ioBufferList {nullptr};
	HostCallbackInfo hostCallback {};
	UInt32 maxFrames {4096};
	Float64 staticTempo {120.};
	SampleRate sampleRate;
	bool interAppAudioConnected {false};
	std::vector<IAudioIOProcessor*> audioProcessors;

	enum InternalState
	{
		kUninitialized,
		kInitialized,
		kStarted,
	};
	InternalState internalState {kUninitialized};
};

//------------------------------------------------------------------------
} // namespace InterAppAudio
} // namespace Vst
} // namespace Steinberg

/// \endcond

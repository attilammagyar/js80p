/*
 * Free Studio Technologies - plugin SDK
 * a plugin SDK that is compatible with a well-known
 * but no longer available commercial plugin SDK.
 *
 * Copyright © 2019, IOhannes m zmölnig, IEM
 *
 * This file is part of FST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FST.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FST_fst_h_
#define FST_fst_h_

#define FST_MAJOR_VERSION 0
#define FST_MINOR_VERSION 177
#define FST_MICRO_VERSION 0

#define FST_VERSIONNUM(X, Y, Z)                                         \
        ((X)*100000 + (Y)*100 + (Z))
#define FST_VERSION FST_VERSIONNUM(FST_MAJOR_VERSION, FST_MINOR_VERSION, FST_MICRO_VERSION)

/* helper macros for compiler specifics */
#define _FST_STRING2(x) #x
#define _FST_STRING(x) _FST_STRING2(x)
#define _FST_PRAGMA(x) _Pragma(#x)
/* warnings silenced for -Werror */
#define FST_WARNING(x)
// #if defined(__GNUC__) || defined(__clang__)
// # define FST_WARNING(x) _FST_PRAGMA(GCC warning x)
// #elif defined _MSC_VER
// # define FST_WARNING(x) __pragma(message(__FILE__ ":" _FST_STRING(__LINE__) ": warning: " x))
// #else
// # define FST_WARNING(x)
// #endif


/* helper macros for marking values as compatible with the original SDK */

/* constants that have not yet been discovered are marked as 'deprecated'
 * in order to get a notification during build
 * constants we are not sure about, are marked with an EXPERIMENTAL macro
 */
#if defined(__GNUC__) || defined(__clang__)
# define FST_DEPRECATE_UNKNOWN(x) x __attribute__ ((deprecated))
#elif defined(_MSC_VER)
# define FST_DEPRECATE_UNKNOWN(x) __declspec(deprecated) x
/* on second thought, MSVC doesn't allow to deprecate enum values, so blow it: */
# define FST_DONT_DEPRECATE_UNKNOWN
#else
# define FST_DEPRECATE_UNKNOWN(x) x
#endif

#ifdef FST_DONT_DEPRECATE_UNKNOWN
# undef  FST_DEPRECATE_UNKNOWN
# define FST_DEPRECATE_UNKNOWN(x) x
#endif

#define FST_DEPRECATE_ENUM(x) FST_DEPRECATE_UNKNOWN(x)

#define FST_UNKNOWN(x) x
#define FST_ENUM(x, y) x = y
#if 0
# define FST_ENUM_EXPERIMENTAL(x, y) FST_DEPRECATE_ENUM(x) = y
#else
# define FST_ENUM_EXPERIMENTAL(x, y) x = y
#endif
#define FST_ENUM_UNKNOWN(x) FST_DEPRECATE_ENUM(x) = (100000 + __LINE__)

/* name mangling */
#ifdef FST2VST
# define _fstEffect AEffect
#endif

# define FST_HOST_OPCODE(x, y) audioMaster##x = y
# define FST_HOST_OPCODE_EXPERIMENTAL(x, y) FST_ENUM_EXPERIMENTAL( audioMaster##x, y)
# define FST_HOST_OPCODE_UNKNOWN(x) FST_ENUM_UNKNOWN( audioMaster##x)
# define FST_EFFECT_OPCODE(x, y) eff##x = y
# define FST_EFFECT_OPCODE_EXPERIMENTAL(x, y) FST_ENUM_EXPERIMENTAL( eff##x, y)
# define FST_EFFECT_OPCODE_UNKNOWN(x) FST_ENUM_UNKNOWN( eff##x)
# define FST_EFFECT_CATEGORY(x, y) kPlugCateg##x = y
# define FST_EFFECT_CATEGORY_EXPERIMENTAL(x, y) FST_ENUM_EXPERIMENTAL( kPlugCateg##x, y)
# define FST_EFFECT_CATEGORY_UNKNOWN(x) FST_ENUM_UNKNOWN( kPlugCateg##x)
# define FST_EFFECT_FLAG(x, y) effFlags##x = (1<<y)
# define FST_SPEAKER(x, y) kSpeaker##x = y
# define FST_SPEAKER_EXPERIMENTAL(x, y) FST_ENUM_EXPERIMENTAL( kSpeaker##x, y)
# define FST_SPEAKER_UNKNOWN(x) FST_ENUM_UNKNOWN( kSpeaker##x)
# define FST_CONST(x, y) k##x = y
# define FST_CONSTANT(x, y) kVst##x = y
# define FST_CONSTANT_EXPERIMENTAL(x, y) FST_ENUM_EXPERIMENTAL( kVst##x, y)
# define FST_CONSTANT_UNKNOWN(x) FST_ENUM_UNKNOWN( kVst##x)
# define FST_FLAG(x, y) kVst##x = (1<<y)
# define FST_FLAG_UNKNOWN(x) FST_DEPRECATE_UNKNOWN(kVst##x) = 0
# define FST_TYPE(x) Vst##x
# define FST_TYPE_UNKNOWN(x) FST_UNKNOWN(Vst##x)

#define kVstVersion 2400

#define VSTCALLBACK

 /* t_fstPtrInt: pointer sized int */
#if defined(_WIN32) && (defined(__x86_64__) || defined (_M_X64) || defined(__loongarch_lp64))
typedef long long t_fstPtrInt;
#else
typedef long t_fstPtrInt;
#endif

typedef int t_fstInt32; /* 32bit int */

typedef enum {
  FST_HOST_OPCODE(Automate, 0), /* IN:index, IN:fvalue, return 0 */
  FST_HOST_OPCODE(Version, 1), /* return 2400 */
  FST_HOST_OPCODE_EXPERIMENTAL(CurrentId, 2), /* return shellUIDToCreate */



  FST_HOST_OPCODE(WantMidi, 6), /* return 0 */
  FST_HOST_OPCODE(GetTime, 7), /* return (fstTimeInfo*) */
  FST_HOST_OPCODE(ProcessEvents, 8), /* IN:ptr(fstEvents*), return 0 */

  FST_HOST_OPCODE(TempoAt, 10), /* IN:ivalue, return (10000*BPM) */


  /* 13: sending latency?? */
  FST_HOST_OPCODE(NeedIdle, 14), /* return 0 */
  FST_HOST_OPCODE(SizeWindow, 15), /* IN:index(width), IN:value(height), return 1 */
  FST_HOST_OPCODE(GetSampleRate, 16), /* return sampleRate */
  FST_HOST_OPCODE(GetBlockSize, 17), /* return blockSize */





  FST_HOST_OPCODE(GetCurrentProcessLevel, 23), /* return (!isRealtime)*4 */







  FST_HOST_OPCODE(GetVendorString, 32), /* OUT:ptr(char[MaxVendorStrLen]), return ptr */
  FST_HOST_OPCODE(GetProductString, 33), /* OUT:ptr(char[MaxProductStrLen]), return ptr */
  FST_HOST_OPCODE(GetVendorVersion, 34), /* return 0x0101 */


  FST_HOST_OPCODE(CanDo, 37), /* IN:ptr(char*), return *ptr in {"sendFstEvents", "sizeWindow",...} */



  FST_HOST_OPCODE(GetDirectory, 41), /* return (char*)plugindirectory */
  FST_HOST_OPCODE(UpdateDisplay, 42), /* return 1; triggers effGetProgram* */
  FST_HOST_OPCODE(BeginEdit, 43), /* IN:index, return 0 */
  FST_HOST_OPCODE(EndEdit, 44), /* IN:index, return 0 */




  FST_HOST_OPCODE_UNKNOWN(CloseWindow), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(OpenWindow), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(SetIcon), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(GetParameterQuantization), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(GetNumAutomatableParameters),
  FST_HOST_OPCODE_UNKNOWN(GetAutomationState), /* return {unsupported=0, off, read, write, readwrite} */

  FST_HOST_OPCODE_UNKNOWN(GetInputLatency), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(GetOutputLatency), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(GetLanguage), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(GetOutputSpeakerArrangement), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(OfflineGetCurrentMetaPass), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(OfflineGetCurrentPass), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(OfflineRead), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(OfflineStart), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(OfflineWrite), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(GetNextPlug), /* ?, return 0 */
  FST_HOST_OPCODE_UNKNOWN(GetPreviousPlug), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(Idle), /* return 0 */

  FST_HOST_OPCODE_UNKNOWN(IOChanged), /* return 0 */
  FST_HOST_OPCODE_UNKNOWN(PinConnected), /* IN:index, IN:ivalue(isOutput), return isValidChannel */

  FST_HOST_OPCODE_UNKNOWN(SetOutputSampleRate),
  FST_HOST_OPCODE_UNKNOWN(SetTime), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(WillReplaceOrAccumulate), /* ?, return 0 */

  FST_HOST_OPCODE_UNKNOWN(VendorSpecific), /* ?, return 0 */

  /* the following opcodes are used by MrsWatson */
  FST_HOST_OPCODE_UNKNOWN(OpenFileSelector),
  FST_HOST_OPCODE_UNKNOWN(CloseFileSelector),
  FST_HOST_OPCODE_UNKNOWN(EditFile),
  FST_HOST_OPCODE_UNKNOWN(GetChunkFile),
  FST_HOST_OPCODE_UNKNOWN(GetInputSpeakerArrangement),

  fst_audioMasterLast /* last enum */
} t_fstHostOpcode;;

typedef enum {
  FST_EFFECT_OPCODE(Open, 0), /* return 0 */
  FST_EFFECT_OPCODE(Close, 1), /* return 0 */
  FST_EFFECT_OPCODE(SetProgram, 2), /* IN:ivalue, return 0 */
  FST_EFFECT_OPCODE(GetProgram, 3), /* return current_program */
  FST_EFFECT_OPCODE(SetProgramName, 4), /* IN:ptr(char*), return 0 */
  FST_EFFECT_OPCODE(GetProgramName, 5), /* OUT:ptr(char[24]), return 0 */

  /* JUCE says that MaxParamStrLen is 8, but hosts allow a bit more (24) */
  FST_EFFECT_OPCODE(GetParamLabel, 6), /* OUT:ptr(char[8]), return 0 */
  FST_EFFECT_OPCODE(GetParamDisplay, 7), /* OUT:ptr(char[8]), return 0 */
  FST_EFFECT_OPCODE(GetParamName, 8), /* OUT:ptr(char[8]), return 0 */

  FST_EFFECT_OPCODE(SetSampleRate, 10), /* IN:fvalue, return 0 */
  FST_EFFECT_OPCODE(SetBlockSize, 11), /* IN:ivalue, return 0 */
  FST_EFFECT_OPCODE(MainsChanged, 12), /* IN:ivalue, return 0;  (handleResumeSuspend) */
  FST_EFFECT_OPCODE(EditGetRect, 13), /* OUT:ptr(ERect*), return ptr */
  FST_EFFECT_OPCODE(EditOpen, 14), /* return 0 */
  FST_EFFECT_OPCODE(EditClose, 15), /* return 0 */



  FST_EFFECT_OPCODE(EditIdle, 19), /* return 0 */


  FST_EFFECT_OPCODE(Identify, 22), /* return ByteOrder::bigEndianInt ("NvEf") 1316373862 or 1715828302 */
  FST_EFFECT_OPCODE(GetChunk, 23), /* IN:index, OUT:ptr(void*), return size */
  FST_EFFECT_OPCODE(SetChunk, 24), /* IN:index, IN:ivalue(size), IN:ptr(void*), return 0 */
  FST_EFFECT_OPCODE(ProcessEvents, 25), /* IN:ptr(fstEvents*), return ((bool)MidiProcessed */
  FST_EFFECT_OPCODE(CanBeAutomated, 26), /* (can parameter# be automated) IN:index, return 0 */
  FST_EFFECT_OPCODE(String2Parameter, 27), /* IN:index, IN:ptr(char*), return (hasParam#) */

  FST_EFFECT_OPCODE(GetProgramNameIndexed, 29), /* IN:index, OUT:ptr(char[24], return (hasProg#) */

  FST_EFFECT_OPCODE(ConnectInput, 31),
  FST_EFFECT_OPCODE(ConnectOutput, 32),
  FST_EFFECT_OPCODE(GetInputProperties, 33), /* IN:index, OUT:ptr(fstPinProperties*), return 1|0 */
  FST_EFFECT_OPCODE(GetOutputProperties, 34), /* IN:index, OUT:ptr(fstPinProperties*), return 1|0 */
  FST_EFFECT_OPCODE(GetPlugCategory, 35), /* return category */






  FST_EFFECT_OPCODE(SetSpeakerArrangement, 42), /* IN:ivalue(fstSpeakerArrangement*in) IN:ptr(fstSpeakerArrangement*out) */


  FST_EFFECT_OPCODE(GetEffectName, 45), /* OUT:ptr(char[64]), return 1 */

  FST_EFFECT_OPCODE(GetVendorString, 47), /* OUT:ptr(char[64]), return 1 */
  FST_EFFECT_OPCODE(GetProductString, 48), /* OUT:ptr(char[64]), return 1 */
  FST_EFFECT_OPCODE(GetVendorVersion, 49), /* return version */
  FST_EFFECT_OPCODE(VendorSpecific, 50), /* behaviour defined by vendor... */
  FST_EFFECT_OPCODE(CanDo, 51), /* IN:ptr(char*), returns 0|1|-1 */
  FST_EFFECT_OPCODE(GetTailSize, 52), /* return audiotailInSamples */
  FST_EFFECT_OPCODE(Idle, 53),



  FST_EFFECT_OPCODE(KeysRequired, 57), /* return ((bool)KeyboardFocusRequireq; */
  FST_EFFECT_OPCODE(GetVstVersion, 58), /* return kVstVersion */




  FST_EFFECT_OPCODE_EXPERIMENTAL(GetCurrentMidiProgram, 63), /* return -1 */


  FST_EFFECT_OPCODE(GetMidiKeyName, 66), /* IN:index=MIDIchannel, IN:ptr({int unknown, int midinote, char*buffer}), OUT:ptr.buffer), return 0|1 */


  FST_EFFECT_OPCODE(GetSpeakerArrangement, 69), /* OUT:ivalue(fstSpeakerArrangement*in) OUT:ptr(fstSpeakerArrangement*out), return (!(hasAUX || isMidi)) */
  FST_EFFECT_OPCODE(ShellGetNextPlugin, 70),
  FST_EFFECT_OPCODE_EXPERIMENTAL(StartProcess, 71),
  FST_EFFECT_OPCODE_EXPERIMENTAL(StopProcess, 72),
  FST_EFFECT_OPCODE(SetTotalSampleToProcess, 73), /* return ivalue */



  FST_EFFECT_OPCODE(SetProcessPrecision, 77), /* IN:ivalue(ProcessPrecision64,..), return !isProcessing */
  FST_EFFECT_OPCODE(GetNumMidiInputChannels, 78), /* return 16*isMidi */
  FST_EFFECT_OPCODE(GetNumMidiOutputChannels, 79), /* return 16*isMidi */



  FST_EFFECT_OPCODE_UNKNOWN(EditDraw),
  FST_EFFECT_OPCODE_UNKNOWN(EditMouse),
  FST_EFFECT_OPCODE_UNKNOWN(EditSleep),
  FST_EFFECT_OPCODE_UNKNOWN(EditTop),

  FST_EFFECT_OPCODE_UNKNOWN(SetBypass), /* IN:ivalue, return 0; effCanDo("bypass") */

FST_WARNING("document origin of eff*SetProgram")
  FST_EFFECT_OPCODE_UNKNOWN(BeginSetProgram),
  FST_EFFECT_OPCODE_UNKNOWN(EndSetProgram),

  fst_effLast, /* the enum */
} t_fstPluginOpcode;

typedef enum {
  FST_EFFECT_FLAG(HasEditor,           0),



  FST_EFFECT_FLAG(CanReplacing,        4),
  FST_EFFECT_FLAG(ProgramChunks,       5),


  FST_EFFECT_FLAG(IsSynth,             8),
  FST_EFFECT_FLAG(NoSoundInStop,       9),


  FST_EFFECT_FLAG(CanDoubleReplacing, 12),
} t_fstEffectFlags;

typedef enum {
  FST_EFFECT_CATEGORY_EXPERIMENTAL(Unknown, 0),
  FST_EFFECT_CATEGORY_EXPERIMENTAL(Effect, 1),
  FST_EFFECT_CATEGORY(Synth, 2),
  FST_EFFECT_CATEGORY(Analysis, 3),
  FST_EFFECT_CATEGORY(Mastering, 4),
  FST_EFFECT_CATEGORY(Spacializer, 5),
  FST_EFFECT_CATEGORY(RoomFx, 6),
  FST_CONST(PlugSurroundFx, 7), /* hmpf, what a stupid name */
  FST_EFFECT_CATEGORY(Restoration, 8),
  FST_EFFECT_CATEGORY(OfflineProcess, 9),
  FST_EFFECT_CATEGORY(Shell, 10),
  FST_EFFECT_CATEGORY(Generator, 11),

  FST_EFFECT_CATEGORY(MaxCount, 12), /* last enum */
} t_fstEffectCategories;

typedef enum {
  FST_SPEAKER(ArrUserDefined, -2),
  FST_SPEAKER(ArrEmpty, -1),
  FST_SPEAKER(ArrMono, 0),
  FST_SPEAKER(ArrStereo, 1),
  FST_SPEAKER_EXPERIMENTAL(ArrStereoSurround, 2),
  FST_SPEAKER_EXPERIMENTAL(ArrStereoCenter, 3),
  FST_SPEAKER_EXPERIMENTAL(ArrStereoSide, 4),
  FST_SPEAKER_EXPERIMENTAL(ArrStereoCLfe, 5),
  FST_SPEAKER(Arr30Cine, 6),
  FST_SPEAKER_EXPERIMENTAL(Arr30Music, 7),
  FST_SPEAKER_EXPERIMENTAL(Arr31Cine, 8),
  FST_SPEAKER_EXPERIMENTAL(Arr31Music, 9),
  FST_SPEAKER_EXPERIMENTAL(Arr40Cine, 10),
  FST_SPEAKER(Arr40Music, 11),
  FST_SPEAKER_EXPERIMENTAL(Arr41Cine, 12),
  FST_SPEAKER_EXPERIMENTAL(Arr41Music, 13),
  FST_SPEAKER(Arr50, 14),
  FST_SPEAKER(Arr51, 15),
  FST_SPEAKER_EXPERIMENTAL(Arr60Cine, 16),
  FST_SPEAKER_EXPERIMENTAL(Arr60Music, 17),
  FST_SPEAKER_EXPERIMENTAL(Arr61Cine, 18),
  FST_SPEAKER_EXPERIMENTAL(Arr61Music, 19),
  FST_SPEAKER_EXPERIMENTAL(Arr70Cine, 20),
  FST_SPEAKER_EXPERIMENTAL(Arr70Music, 21),
  FST_SPEAKER_EXPERIMENTAL(Arr71Cine, 22),
  FST_SPEAKER_EXPERIMENTAL(Arr71Music, 23),
  FST_SPEAKER_EXPERIMENTAL(Arr80Cine, 24),
  FST_SPEAKER_EXPERIMENTAL(Arr80Music, 25),
  FST_SPEAKER_EXPERIMENTAL(Arr81Cine, 26),
  FST_SPEAKER_EXPERIMENTAL(Arr81Music, 27),
  FST_SPEAKER(Arr102, 28),

  //FST_WARNING("document origin of kSpeakerM")
  FST_SPEAKER_EXPERIMENTAL(M, 0),
  FST_SPEAKER(L, 1),
  FST_SPEAKER(R, 2),
  FST_SPEAKER(C, 3),
  FST_SPEAKER(Lfe, 4),
  FST_SPEAKER(Ls, 5),
  FST_SPEAKER(Rs, 6),
  FST_SPEAKER_EXPERIMENTAL(Lc, 7),
  FST_SPEAKER_EXPERIMENTAL(Rc, 8),
  FST_SPEAKER_EXPERIMENTAL(S, 9),
  FST_SPEAKER_EXPERIMENTAL(Sl, 10),
  FST_SPEAKER_EXPERIMENTAL(Sr, 11),
  FST_SPEAKER_EXPERIMENTAL(Tm, 12),
  FST_SPEAKER_EXPERIMENTAL(Tfl, 13),
  FST_SPEAKER_EXPERIMENTAL(Tfc, 14),
  FST_SPEAKER_EXPERIMENTAL(Tfr, 15),
  FST_SPEAKER_EXPERIMENTAL(Trl, 16),
  FST_SPEAKER_EXPERIMENTAL(Trc, 17),
  FST_SPEAKER_EXPERIMENTAL(Trr, 18),
  FST_SPEAKER_EXPERIMENTAL(Lfe2, 19),
  FST_SPEAKER_UNKNOWN(Undefined),
  fst_speakerLast /* last enum */
} t_fstSpeakerArrangementType;

enum { /* fstTimeInfo.flags */
  FST_FLAG(TransportChanged,     0),
  FST_FLAG(TransportPlaying,     1),
  FST_FLAG(TransportCycleActive, 2),
  FST_FLAG(TransportRecording,   3),
  FST_FLAG_UNKNOWN(AutomationReading),
  FST_FLAG_UNKNOWN(AutomationWriting),

  FST_FLAG(NanosValid   ,  8),
  FST_FLAG(PpqPosValid  ,  9),
  FST_FLAG(TempoValid   , 10),
  FST_FLAG(BarsValid    , 11),
  FST_FLAG(CyclePosValid, 12),
  FST_FLAG(TimeSigValid , 13),
  FST_FLAG(SmpteValid   , 14),
  FST_FLAG(ClockValid   , 15)
};

enum {
/* 197782 is where the array passed at opcode:33 overflows */
/* GVST/GChorus crashes with MaxVendorStrLen>130 */
  FST_CONSTANT_EXPERIMENTAL(MaxProductStrLen, 128),
  FST_CONSTANT_EXPERIMENTAL(MaxVendorStrLen, 128),
  FST_CONSTANT_EXPERIMENTAL(MaxLabelLen, 64),
  FST_CONSTANT_EXPERIMENTAL(MaxShortLabelLen, 8),
  FST_CONSTANT(MaxProgNameLen, 25), // effGetProgramName
  FST_CONSTANT_EXPERIMENTAL(MaxNameLen, 64),

  /* JUCE says that MaxParamStrLen is 8, but hosts allow a bit more (24) */
  FST_CONSTANT(MaxParamStrLen, 8),

  /* returned by audioMasterGetAutomationState: */
  FST_FLAG_UNKNOWN(AutomationUnsupported),

  /* used as t_fstPinProperties.flags */
  FST_FLAG_UNKNOWN(PinIsActive),
  FST_FLAG_UNKNOWN(PinUseSpeaker),
  FST_FLAG_UNKNOWN(PinIsStereo),

  /* REAPER: used with effVendorSpecific to indicate that parameter values are enums */
  FST_CONSTANT_UNKNOWN(ParameterUsesIntStep),
};

typedef enum {
  /* used as: t_fstTimeInfo.smpteFrameRate */
  FST_CONSTANT_UNKNOWN(Smpte239fps),
  FST_CONSTANT_UNKNOWN(Smpte24fps),
  FST_CONSTANT_UNKNOWN(Smpte249fps),
  FST_CONSTANT_UNKNOWN(Smpte25fps),
  FST_CONSTANT_UNKNOWN(Smpte2997dfps),
  FST_CONSTANT_UNKNOWN(Smpte2997fps),
  FST_CONSTANT_UNKNOWN(Smpte30dfps),
  FST_CONSTANT_UNKNOWN(Smpte30fps),
  FST_CONSTANT_UNKNOWN(Smpte599fps),
  FST_CONSTANT_UNKNOWN(Smpte60fps),
  FST_CONSTANT_UNKNOWN(SmpteFilm16mm),
  FST_CONSTANT_UNKNOWN(SmpteFilm35mm),
} t_fstSmpteFrameRates;

enum {
  FST_CONSTANT(ProcessPrecision32, 0),
  FST_CONSTANT(ProcessPrecision64, 1),
};

typedef enum {
  FST_CONSTANT(MidiType, 1),
  FST_CONSTANT(SysExType, 6)
} t_fstEventType;

enum {
  /* returned by audioMasterGetCurrentProcessLevel: */
      FST_CONSTANT_EXPERIMENTAL(ProcessLevelUnknown, 0),

      FST_CONSTANT(ProcessLevelRealtime, 2),

      FST_CONSTANT(ProcessLevelOffline, 4),

FST_WARNING("document origin of ProcessLevelUser")
      FST_CONSTANT_UNKNOWN(ProcessLevelUser), /* vstplugin~ */
};

enum {
      /* returned by audioMasterGetLanguage: */
      FST_CONSTANT_UNKNOWN(LangEnglish),
};


/* deltaFrames: used by JUCE as "timestamp" (to sort events) */
#define FSTEVENT_COMMON \
  t_fstEventType type; \
  int byteSize; \
  int deltaFrames; \
  int flags

typedef struct fstEvent_ {
  FSTEVENT_COMMON;
} t_fstEvent;

typedef struct fstMidiEvent_ {
  FSTEVENT_COMMON; /* @0x00-0x0b */
  /* FIXXXME: unknown member order and size */
  /* REAPER: sets everything to 0
   * JMZ: size is set to occupy 12bytes (on amd64) for now
   */
  FST_UNKNOWN(short) noteLength;
  FST_UNKNOWN(short) noteOffset;
  FST_UNKNOWN(short) detune;
  FST_UNKNOWN(short) noteOffVelocity;
  unsigned char midiData[4]; /* @0x18 */
  FST_UNKNOWN(short) reserved1;
  FST_UNKNOWN(short) reserved2;
} FST_UNKNOWN(t_fstMidiEvent);

typedef struct fstMidiSysexEvent_ {
  FSTEVENT_COMMON;
  /* FIXXXME: unknown member order */
  t_fstPtrInt dumpBytes; /* size of sysexDump */
  t_fstPtrInt resvd1; /* ? */
  char*sysexDump; /* heap allocated memory for sysex-data */
  t_fstPtrInt resvd2; /* ? */
} t_fstSysexEvent;

typedef struct fstEvents_ {
  int numEvents;
  FST_UNKNOWN(t_fstPtrInt _pad);
  t_fstEvent*events[];
} t_fstEvents;

typedef struct fstSpeakerProperties_ {
  /* azimuth+elevation+radius+reserved+name take up 80 bytes
   * if the first 4 are doubles, name is char[16]
   * if they are floats, name is char[48]
   */
  FST_UNKNOWN(float) azimuth; /* float? origin:MrsWatson */
  FST_UNKNOWN(float) elevation; /* float? origin:MrsWatson */
  FST_UNKNOWN(float) radius; /* float? origin:MrsWatson */
  FST_UNKNOWN(float) reserved; /* type? origin:MrsWatson */
  FST_UNKNOWN(char name[64]);
  int type;
  char _padding2[28];
} FST_UNKNOWN(t_fstSpeakerProperties);

typedef struct fstSpeakerArrangement_ {
  int type;
  int numChannels;
  t_fstSpeakerProperties speakers[];
} t_fstSpeakerArrangement;

typedef struct fstTimeInfo_ {
  double samplePos; /* OK */
  double sampleRate;/* = rate; // OK */
  double nanoSeconds; /* OK */
  /* ppq: Pulses Per Quaternote */
  double ppqPos; /* (double)position.ppqPosition; // OK */
  double tempo; /* OK */
  double barStartPos; /* (double)ppqPositionOfLastBarStart; // OK */
  double cycleStartPos; /* (double)ppqLoopStart; // OK */
  double cycleEndPos; /* (double)ppqLoopEnd; // OK */
  int timeSigNumerator; /* OK */
  int timeSigDenominator; /* OK */

  int FST_UNKNOWN(currentBar), FST_UNKNOWN(magic); /* we just made these fields up, as their values seem to be neither flags nor smtp* */

FST_WARNING("document origin of samplesToNextClock")
  /* this used to be '_pad' */
  FST_UNKNOWN(int) samplesToNextClock;/* ? */

  FST_UNKNOWN(int) flags;/* = Vst2::kVstNanosValid // ? */
  FST_UNKNOWN(int) smpteFrameRate; /* int32 // ? */
  FST_UNKNOWN(int) smpteOffset; /* int32 // ? */
} FST_UNKNOWN(t_fstTimeInfo);

typedef struct fstPinProperties_ {
  char label[64];
  FST_UNKNOWN(int) flags; /* ? kVstPinIsActive | kVstPinUseSpeaker | kVstPinIsStereo */
  FST_UNKNOWN(int) arrangementType; /* ? */
  char shortLabel[8];
} FST_UNKNOWN(t_fstPinProperties);


struct _fstEffect;

/* t_fstPtrInt dispatcher(effect, opcode, index, ivalue, ptr, fvalue); */
typedef t_fstPtrInt (*AEffectDispatcherProc)(struct _fstEffect*, int, int, t_fstPtrInt, void* const, float);
/* void setParameter(effect, index, fvalue); */
typedef void (*AEffectSetParameterProc)(struct _fstEffect*, int, float);
/* float getParameter(effect, index); */
typedef float (*AEffectGetParameterProc)(struct _fstEffect*, int);
/* void process(effect, indata, outdata, frames); */
typedef void (*AEffectProcessProc)(struct _fstEffect*, float**, float**, int);
/* void processDoubleReplacing(effect, indata, outdata, frames); */
typedef void (*AEffectProcessDoubleProc)(struct _fstEffect*, double**, double**, int);

typedef FST_UNKNOWN(AEffectDispatcherProc) audioMasterCallback;

typedef struct _fstEffect {
  t_fstInt32 magic; /* @0 0x56737450, aka 'VstP' */
  /* auto-padding in place */
  AEffectDispatcherProc dispatcher; /* (AEffect*, Vst2::effClose, 0, 0, 0, 0); */
  AEffectProcessProc process;
  AEffectSetParameterProc setParameter; /* (Aeffect*, int, float) */
  AEffectGetParameterProc getParameter; /* float(Aeffect*, int) */

  t_fstInt32 numPrograms;
  t_fstInt32 numParams;
  t_fstInt32 numInputs;
  t_fstInt32 numOutputs;

  FST_UNKNOWN(t_fstPtrInt) flags; /* ?? */
  FST_UNKNOWN(t_fstPtrInt) FST_UNKNOWN(resvd1); /* ?? */
  FST_UNKNOWN(t_fstPtrInt) FST_UNKNOWN(resvd2); /* ?? */
  FST_UNKNOWN(t_fstInt32) FST_UNKNOWN(initialDelay); /* ??; latency in samples */
  char _pad2[8];

  float float1;
  void* object;
  void*user;
  t_fstInt32 uniqueID; /* @112 */
  t_fstInt32 version;

  AEffectProcessProc processReplacing;
  AEffectProcessDoubleProc processDoubleReplacing;
} FST_UNKNOWN(t_fstEffect);

typedef struct _fstRectangle {
  short top;
  short left;
  short bottom;
  short right;
} t_fstRectangle;

typedef struct _fstMidiKeyName {
  t_fstInt32 thisProgramIndex; /* always 0 ??? */
  t_fstInt32 thisKeyNumber;
  char keyName[]; /* length: kVstMaxNameLen */
} t_fstMidiKeyName;


typedef t_fstHostOpcode AudioMasterOpcodesX;

typedef t_fstEvent FST_TYPE(Event);
typedef t_fstMidiEvent FST_TYPE(MidiEvent);
typedef t_fstSysexEvent FST_TYPE(MidiSysexEvent);
typedef t_fstEvents FST_TYPE(Events);
typedef t_fstSpeakerProperties FST_TYPE(SpeakerProperties);
typedef t_fstSpeakerArrangement FST_TYPE(SpeakerArrangement);
typedef t_fstTimeInfo FST_TYPE(TimeInfo);
typedef t_fstSmpteFrameRates FST_TYPE(SmpteFrameRate);
typedef t_fstPinProperties FST_TYPE(PinProperties);
typedef t_fstEffectCategories FST_TYPE(PlugCategory);
typedef t_fstEffectFlags FST_TYPE(AEffectFlags);
typedef t_fstEffect AEffect;
typedef t_fstRectangle ERect;
typedef t_fstMidiKeyName MidiKeyName;

typedef t_fstPtrInt VstIntPtr;
typedef t_fstInt32 VstInt32;

const int FST_CONST(EffectMagic, 0x56737450); /* 'VstP' */

/* see https://github.com/steinbergmedia/vst3_pluginterfaces/blob/efcfbf8019a2f1803b7be9936a81124abb583507/base/futils.h#L91-L95
 * for a GPL-v3 definition of CCONST
 */
#ifndef CCONST
# define CCONST(a,b,c,d) ((((unsigned char)a)<<24) + (((unsigned char)b)<<16) + (((unsigned char)c)<<8) + ((unsigned char)d))
#endif

#endif /* FST_fst_h_ */

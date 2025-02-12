`effIdle` and `audioMasterNeedIdle` discovered
==============================================

I'm developing a plugin using `fst.h`, and I noticed that REAPER keeps
sending opcode `53` regularly to my plugin from the UI thread, even when my
plugin's window is closed, and even when the plugin is bypassed and its track
is muted:

~~~
op_code=53, op_code_name=UNKNOWN-53, index=0, ivalue=0, fvalue=0.000000
~~~

I suspect that this might be the missing value of `effIdle`, since other
opcodes that one would expect to be used this frequently (like `effEditIdle`
and `effProcessEvent`) are already known, and there aren't any other unknown
opcode names in `fst.h` which would make a lot of sense to be called repeatedly.
(Though there might be unknown-unknown opcodes, but let's be optimistic.)

One way to confirm this would be to see how this opcode interacts with
`audioMasterNeedIdle` which seems to be related, based on its name. The
problem is that the value for `audioMasterNeedIdle` is also unknown.

But first things first, let's take a look at JUCE! `audioMasterNeedIdle` is
handled in `modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp`
like this:

~~~C++
pointer_sized_int handleCallback (int32 opcode, int32 index, pointer_sized_int value, void* ptr, float opt)
{
    switch (opcode)
    {
        // ...
        case Vst2::audioMasterNeedIdle:                 startTimer (50); break;
        // ...
    }

    return 0;
}
~~~

Notice that none of the parameters are used besides the opcode, so there's a
chance that we can get away with calling the host with all other
parameters set to `0` and `NULL`, which means that it's worth trying to
brute-force the value for this opcode. (The highest known host opcode is below
`50` in `fst.h`, and apparently, they are numbered sequentially, and there
are around 30 unknown host opcodes, so the correct value will probably be under
`100`.)

How do we know if we found `audioMasterNeedIdle`?

According to `./modules/juce_events/timers/juce_Timer.h`, the `startTimer()`
method will start a timer which regularly calls the `timerCallback()` virtual
method, which in this case looks like this:

~~~C++
void timerCallback() override
{
    if (dispatch (Vst2::effIdle, 0, 0, nullptr, 0) == 0)
        stopTimer();
}
~~~

So indeed, `audioMasterNeedIdle` and `effIdle` are related: after receiving a
call with `audioMasterNeedIdle`, the host is supposed to regularly send
`effIdle` messages to the plugin, and keep doing so as long as the plugin
responds with a non-zero value. Once the plugin returns `0`, the timer should
be stopped, and `effIdle` should not get called until the plugin requests it
again.

The problem is that it's impossible to correlate our guess for
`audioMasterNeedIdle` with opcode `53` if REAPER keeps sending `53` all the
time, even if the plugin responds with `0` to unknown opcodes!

REAPER behaving so differently from what we've seen in JUCE probably has to do
with some buggy plugins, and if so, then hopefully it is the result of multiple
tweaks and bugfixes over the years, as users kept reporting various bugs with
various plugins.

Fortunately, [REAPER's entire changelog is available on their website](https://www.reaper.fm/whatsnew.txt),
so let's see if there's anything about VST plugins and idle-ness in there:

~~~cmd
$ grep -i 'vst.*idle' whatsnew.txt
+ VST: fix UI idle processing for bridged VST2 on Linux
+ VST: bumped effEditIdle rate back up to 10hz
+ VST: extraneous effIdle for plugins that dont request it
+ VST: better compatibility with plug-ins that require effIdle
+ VST: updated idle processing behavior, vst 2.3 startprocess/stopprocess support
+ vst: support for plug-ins that require audioMasterIdle
~~~

Two of these look very interesting with regards to `effIdle`: the
"_VST: better compatibility with plug-ins that require effIdle_" change
belongs to the `v2.001 - October 12 2007` release, and the
"_VST: extraneous effIdle for plugins that dont request it_" change was
released in the `v2.004 - October 19 2007` version.

Both of these versions can still be downloaded from
[REAPER's old version archive](https://reaper.fm/download-old.php?ver=2x), and
both come as 32 bit Windows-only binaries, packed into a standard NSIS
installer. One advantage of these installers is that they can be extracted even
on a Linux system using the `7z x reaper2001-install.exe` command, without
having to click through any license agreements.

Fortunately again, `v2.001` runs well enough in [Wine](https://www.winehq.org/)
so we can start experimenting with it. Hopefully this old version with its early
`effIdle` support is closer to the behaviour that we saw in JUCE.

I made a test plugin named `idle.cpp` - it only does some very basic
housekeeping, the interesting part is that it will attempt to send
`audioMasterNeedIdle` messages when it's initialized, or its GUI is
closed (when the GUI is open, we don't really need `effIdle`, since we have
`effEditIdle`), or when it's suspended or resumed:

~~~C++
#include <algorithm>
#include <cstdio>
#include <windows.h>

#include "fst.h"

#define _IDLE_LOG(msg, ...) \
    do { \
        fprintf(stderr, "%s:%d/%s():\t", __FILE__, __LINE__, __FUNCTION__); \
        fprintf(stderr, "TID=%#x\t", (unsigned int)GetCurrentThreadId()); \
        fprintf(stderr, msg, ##__VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (false)

namespace Idle {

HINSTANCE dll_instance = NULL;
constexpr int OP_CODE_NAMES_LEN = 78;
constexpr char const* OP_CODE_NAMES[OP_CODE_NAMES_LEN] = {
    "Open",                         /*    0 */
    "Close",                        /*    1 */
    "SetProgram",                   /*    2 */
    "GetProgram",                   /*    3 */
    "SetProgramName",               /*    4 */
    "GetProgramName",               /*    5 */
    "GetParamLabel",                /*    6 */
    "GetParamDisplay",              /*    7 */
    "GetParamName",                 /*    8 */
    "UNKNOWN-9",                    /*    9 */
    "SetSampleRate",                /*   10 */
    "SetBlockSize",                 /*   11 */
    "MainsChanged",                 /*   12 */
    "EditGetRect",                  /*   13 */
    "EditOpen",                     /*   14 */
    "EditClose",                    /*   15 */
    "UNKNOWN-16",                   /*   16 */
    "UNKNOWN-17",                   /*   17 */
    "UNKNOWN-18",                   /*   18 */
    "EditIdle",                     /*   19 */
    "UNKNOWN-20",                   /*   20 */
    "UNKNOWN-21",                   /*   21 */
    "Identify",                     /*   22 */
    "GetChunk",                     /*   23 */
    "SetChunk",                     /*   24 */
    "ProcessEvents",                /*   25 */
    "CanBeAutomated",               /*   26 */
    "String2Parameter",             /*   27 */
    "UNKNOWN-28",                   /*   28 */
    "GetProgramNameIndexed",        /*   29 */
    "UNKNOWN-30",                   /*   30 */
    "UNKNOWN-31",                   /*   31 */
    "UNKNOWN-32",                   /*   32 */
    "GetInputProperties",           /*   33 */
    "GetOutputProperties",          /*   34 */
    "GetPlugCategory",              /*   35 */
    "UNKNOWN-36",                   /*   36 */
    "UNKNOWN-37",                   /*   37 */
    "UNKNOWN-38",                   /*   38 */
    "UNKNOWN-39",                   /*   39 */
    "UNKNOWN-40",                   /*   40 */
    "UNKNOWN-41",                   /*   41 */
    "SetSpeakerArrangement",        /*   42 */
    "UNKNOWN-43",                   /*   43 */
    "UNKNOWN-44",                   /*   44 */
    "GetEffectName",                /*   45 */
    "UNKNOWN-46",                   /*   46 */
    "GetVendorString",              /*   47 */
    "GetProductString",             /*   48 */
    "GetVendorVersion",             /*   49 */
    "VendorSpecific",               /*   50 */
    "CanDo",                        /*   51 */
    "UNKNOWN-52",                   /*   52 */
    "UNKNOWN-53",                   /*   53 */
    "UNKNOWN-54",                   /*   54 */
    "UNKNOWN-55",                   /*   55 */
    "UNKNOWN-56",                   /*   56 */
    "UNKNOWN-57",                   /*   57 */
    "GetVstVersion",                /*   58 */
    "UNKNOWN-59",                   /*   59 */
    "UNKNOWN-60",                   /*   60 */
    "UNKNOWN-61",                   /*   61 */
    "UNKNOWN-62",                   /*   62 */
    "GetCurrentMidiProgram",        /*   63 */
    "UNKNOWN-64",                   /*   64 */
    "UNKNOWN-65",                   /*   65 */
    "GetMidiNoteName",              /*   66 */
    "UNKNOWN-67",                   /*   67 */
    "UNKNOWN-68",                   /*   68 */
    "GetSpeakerArrangement",        /*   69 */
    "ShellGetNextPlugin",           /*   70 */
    "StartProcess",                 /*   71 */
    "StopProcess",                  /*   72 */
    "SetTotalSampleToProcess",      /*   73 */
    "UNKNOWN-74",                   /*   74 */
    "UNKNOWN-75",                   /*   75 */
    "UNKNOWN-76",                   /*   76 */
    "SetProcessPrecision",          /*   77 */
};

class Plugin
{
    public:
        static constexpr VstInt32 VERSION = 999000;
        static constexpr int WIDTH = 640;
        static constexpr int HEIGHT = 480;

        static AEffect* create_instance(audioMasterCallback host_callback)
        {
            AEffect* effect = new AEffect();

            Plugin* plugin = new Plugin(effect, host_callback);

            memset(effect, 0, sizeof(AEffect));

            effect->dispatcher = &dispatch;
            effect->flags = (
                effFlagsHasEditor
                | effFlagsIsSynth
                | effFlagsCanReplacing
                | effFlagsCanDoubleReplacing
                | effFlagsProgramChunks
            );
            effect->magic = kEffectMagic;
            effect->numInputs = 0;
            effect->numOutputs = 2;
            effect->numPrograms = 0;
            effect->numParams = 0;
            effect->object = (void*)plugin;
            effect->process = &process_accumulating;
            effect->processReplacing = &process_replacing;
            effect->processDoubleReplacing = &process_double_replacing;
            effect->getParameter = &get_parameter;
            effect->setParameter = &set_parameter;
            effect->uniqueID = CCONST('i', 'd', 'l', 'e');
            effect->version = VERSION;

            return effect;
        }

        static VstIntPtr VSTCALLBACK dispatch(
                AEffect* effect,
                VstInt32 op_code,
                VstInt32 index,
                VstIntPtr ivalue,
                void* pointer,
                float fvalue
        ) {
            Plugin* plugin = (Idle::Plugin*)effect->object;

            _IDLE_LOG(
                "plugin=%p, op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f",
                effect->object,
                (int)op_code,
                ((op_code < OP_CODE_NAMES_LEN) ? OP_CODE_NAMES[op_code] : "???"),
                (int)index,
                (int)ivalue,
                fvalue
            );

            switch (op_code) {
                case effOpen:
                case effEditClose:
                    plugin->need_idle();
                    return 0;

                case effMainsChanged:
                    plugin->need_idle();
                    return 0;

                case 53:
                    return plugin->idle();

                case effProcessEvents:
                    return 1;

                case effClose:
                    delete plugin;
                    return 0;

                case effEditGetRect:
                    *((ERect**)pointer) = &plugin->window_rect;
                    return (VstIntPtr)pointer;

                case effEditOpen:
                    plugin->open_gui((HWND)pointer);
                    return 1;

                case effGetChunk:
                    return plugin->get_chunk((void**)pointer, index ? true : false);

                case effGetPlugCategory:
                    return kPlugCategSynth;

                case effGetEffectName:
                case effGetProductString:
                    strncpy((char*)pointer, "Idle", 8);
                    return 1;

                case effGetVendorString:
                    strncpy((char*)pointer, "Idle", 24);
                    return 1;

                case effGetVendorVersion:
                    return VERSION;

                case effGetVstVersion:
                    return kVstVersion;

                case effIdentify:
                    return CCONST('N', 'v', 'E', 'f');

                case effCanDo:
                    if (strcmp("receiveVstMidiEvent", (char const*)pointer) == 0) {
                        return 1;
                    }

                    return 0;

                default:
                    return 0;
            }
        }

        static void VSTCALLBACK process_accumulating(
                AEffect* effect,
                float** indata,
                float** outdata,
                VstInt32 frames
        ) {
        }

        static void VSTCALLBACK process_replacing(
                AEffect* effect,
                float** indata,
                float** outdata,
                VstInt32 frames
        ) {
            std::fill_n(outdata[0], frames, 0.0f);
            std::fill_n(outdata[1], frames, 0.0f);
        }

        static void VSTCALLBACK process_double_replacing(
                AEffect* effect,
                double** indata,
                double** outdata,
                VstInt32 frames
        ) {
            std::fill_n(outdata[0], frames, 0.0);
            std::fill_n(outdata[1], frames, 0.0);
        }

        static float VSTCALLBACK get_parameter(AEffect* effect, VstInt32 index)
        {
            return 0.0f;
        }

        static void VSTCALLBACK set_parameter(
                AEffect* effect,
                VstInt32 index,
                float fvalue
        ) {
        }

        Plugin(AEffect* effect, audioMasterCallback host_callback)
            : effect(effect),
            host_callback(host_callback),
            chunk("IDLE"),
            window(NULL),
            counter(-1)
        {
            window_rect.top = 0;
            window_rect.left = 0;
            window_rect.bottom = HEIGHT;
            window_rect.right = WIDTH;
        }

        ~Plugin()
        {
            close_gui();
        }

        void need_idle()
        {
        }

        VstIntPtr idle() noexcept
        {
            return 0;
        }

        VstIntPtr get_chunk(void** buffer, bool is_preset)
        {
            *buffer = (void*)chunk;
            return 5;
        }

        void open_gui(HWND parent_window)
        {
            window = CreateWindow(
                TEXT("STATIC"),         /* lpClassName  */
                TEXT("idle"),           /* lpWindowName */
                WS_CHILD | WS_VISIBLE,  /* dwStyle      */
                0,                      /* x            */
                0,                      /* y            */
                WIDTH,                  /* nWidth       */
                HEIGHT,                 /* nHeight      */
                parent_window,          /* hWndParent   */
                NULL,                   /* hMenu        */
                NULL,                   /* hInstance    */
                NULL                    /* lpParam      */
            );
        }

        void close_gui()
        {
            if (window != NULL) {
                DestroyWindow(window);
                window = NULL;
            }
        }

    private:
        AEffect* const effect;
        audioMasterCallback const host_callback;
        char const* chunk;
        ERect window_rect;
        HWND window;
        int counter;
};

}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    _IDLE_LOG("loaded");
    Idle::dll_instance = hinstDLL;
    return TRUE;
}

extern "C" __declspec(dllexport) AEffect* VSTPluginMain(audioMasterCallback host_callback)
{
    _IDLE_LOG("creating plugin instance");
    return Idle::Plugin::create_instance(host_callback);
}
~~~

This can be cross-compiled on Linux into a 32 bit DLL with the following
`Makefile`, using MinGW:
~~~make
CC=/usr/bin/i686-w64-mingw32-g++
idle.dll: idle.o idle.def
    $(CC) \
        -Wall \
        -shared \
        -static \
        $^ \
        -o idle.dll \
        -lgdi32 -luser32 -lkernel32 -municode -lcomdlg32 -lole32

idle.o: idle.cpp fst.h
	$(CC) \
	    -DFST_DONT_DEPRECATE_UNKNOWN \
        -D OEMRESOURCE \
        -Wall -Werror \
        -c $< -o $@
~~~

The `idle.def` file looks like this:

~~~
LIBRARY idle.dll
EXPORTS
    VSTPluginMain   @1
~~~

But when tested with this old version of REAPER, the plugin never gets
instantiated!

There's a clue in JUCE in
`modules/juce_audio_plugin_client/juce_audio_plugin_client_VST2.cpp`:
~~~C
#if ! defined (JUCE_64BIT) && JUCE_MSVC // (can't compile this on win64, but it's not needed anyway with VST2.4)
 extern "C" __declspec (dllexport) int main (Vst2::audioMasterCallback audioMaster)
 {
     return (int) pluginEntryPoint (audioMaster);
 }
#endif
~~~

Apparently, these very old 32 bit plugins might use `main()` instead of
`VSTPluginMain()`, so let's change our plugin:

~~~C++
extern "C" __declspec(dllexport) int main(audioMasterCallback host_callback)
{
    _IDLE_LOG("creating plugin instance");
    return (int)Idle::Plugin::create_instance(host_callback);
}
~~~

And `idle.def`:

~~~
LIBRARY idle.dll
EXPORTS
    main   @1
~~~

This requires adding `-Wno-main` to the compiler options as well in the
`Makefile`:

~~~make
idle.o: idle.cpp fst.h
    $(CC) \
        -DFST_DONT_DEPRECATE_UNKNOWN \
        -D OEMRESOURCE \
        -Wall -Werror \
        -Wno-main \
        -c $< -o $@
~~~
Now the plugin is loaded successfully, and it doesn't get flooded with opcode
`53` messages, so we can start the brute-force trial and error process.

The idea is to try to invoke `audioMasterNeedIdle` from `effOpen`,
`effEditClose`, and `effMainsChanged`, and count how many `53` opcodes we get
after that. We are going to respond with `1` to opcode `53` messages, but when
the counter reaches `10`, we are going to start responding with `0`. Assuming
that this early version of REAPER behaves similarly to what we've seen in JUCE,
the correct guess for `audioMasterNeedIdle` should result in exactly 11
invocations of opcode `53`. (At least, if `effIdle` is really `53`.)

~~~C++
void need_idle()
{
    constexpr int audioMasterNeedIdle = ???;

    counter = 0;

    _IDLE_LOG("trying; opcode=%d", audioMasterNeedIdle);

    VstIntPtr result = host_callback(
        effect, audioMasterNeedIdle, 0, 0, NULL, 0.0f
    );

    _IDLE_LOG("  result; result=%lld", (long long int)result);
}

VstIntPtr idle() noexcept
{
    _IDLE_LOG("counter=%d", counter);

    if (counter < 0) {
        /* we don't have an ongoing need_idle() test at the moment */
        return 0;
    }

    if (counter < 10) {
        ++counter;
        return 1;
    }

    counter = -1;

    return 0;
}
~~~

Long story short, `audioMasterNeedIdle = 14` results in exactly 11 invocations
of opcode `53`, confirming that `effIdle = 53`.

**Edit**: I forgot to mention about `-Wall -Werror` that I use `fst.h` with the warnings commented out:

~~~diff
diff --git a/lib/fst/fst.h b/lib/fst/fst.h
index 314b14f..7571e6d 100644
--- a/lib/fst/fst.h
+++ b/lib/fst/fst.h
@@ -36,13 +36,15 @@
 #define _FST_STRING2(x) #x
 #define _FST_STRING(x) _FST_STRING2(x)
 #define _FST_PRAGMA(x) _Pragma(#x)
-#if defined(__GNUC__) || defined(__clang__)
-# define FST_WARNING(x) _FST_PRAGMA(GCC warning x)
-#elif defined _MSC_VER
-# define FST_WARNING(x) __pragma(message(__FILE__ ":" _FST_STRING(__LINE__) ": warning: " x))
-#else
-# define FST_WARNING(x)
-#endif
+/* warnings silenced for -Werror */
+#define FST_WARNING(x)
+// #if defined(__GNUC__) || defined(__clang__)
+// # define FST_WARNING(x) _FST_PRAGMA(GCC warning x)
+// #elif defined _MSC_VER
+// # define FST_WARNING(x) __pragma(message(__FILE__ ":" _FST_STRING(__LINE__) ": warning: " x))
+// #else
+// # define FST_WARNING(x)
+// #endif


 /* helper macros for marking values as compatible with the original SDK */
~~~

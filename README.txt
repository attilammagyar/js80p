JS80P
=====

A MIDI driven, performance oriented, versatile synthesizer VST plugin.

To download JS80P, visit its website at https://attilammagyar.github.io/js80p/

The source code is available at https://github.com/attilammagyar/js80p under
the terms of the GNU General Public License Version 3.

VST® is a trademark of Steinberg Media Technologies GmbH, registered in
Europe and other countries.

System Requirements
-------------------

 * Operating System: Windows 7 or newer, or Linux (e.g. Ubuntu 22.04)
 * CPU: SSE2 support, 32 bit (i686) or 64 bit (x86-64)
    * separate packages are available for AVX capable 64 bit processors
 * RAM: 200-500 MB per instance, depending on buffer sizes, etc.

Dependencies on Linux
---------------------

On Linux, the "libxcb", "libxcb-render", and "libcairo" libraries, and either
the "kdialog" or the "zenity" application are required to run JS80P. To install
them on Debian based distributions (e.g. Ubuntu), you can use the following
command:

    sudo apt-get install libxcb1 libxcb-render0 libcairo2 zenity kdialog

Note that if you want to run the 32 bit version of JS80P on a 64 bit system,
then you will have to install the 32 bit version of the libraries, for example:

    sudo apt-get install libxcb1:i386 libxcb-render0:i386 libcairo2:i386 zenity kdialog

Before Installing
-----------------

If your plugin host application does not support VST 3, but does support
VST 2.4, then you have to download and install the FST version of JS80P.
Otherwise, you should go with the VST 3 bundle on both Windows and Linux.

If your CPU supports AVX instructions and you use a 64 bit plugin host
application, then you should download a JS80P package that is optimized for AVX
compatible processors. If you have an older computer, or if you experience
crashes, then you should go with one of the SSE2 compatible JS80P packages.

If your plugin host application fails to recognize JS80P from the VST 3 bundle,
then you have to download and install the VST 3 Single File version that
matches the CPU architecture for which your plugin host application was built.

(For example, some 32 bit (i686) versions of Reaper are known to be unable to
recognize VST 3 bundles when running on a 64 bit Linux system, so you would
have to download the 32 bit VST 3 Single File JS80P package.)

Installing the VST 3 Bundle on Windows
--------------------------------------

1. [Download the plugin](https://attilammagyar.github.io/js80p/).
2. Extract the ZIP archive.
3. Copy the entire `js80p.vst3` directory to your VST 3 directory which is
   usually `C:\Users\YourUserName\AppData\Local\Programs\Common\VST3`.

Installing the VST 3 Bundle on Linux
------------------------------------

1. [Download the plugin](https://attilammagyar.github.io/js80p/).
2. Extract the ZIP archive.
3. Copy the entire `js80p.vst3` directory to your VST 3 directory which is
   usually `~/.vst3`.

Installing the VST 3 Single File Version on Windows
---------------------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the "js80p.vst3" file to your VST 3 directory which is usually
   "C:\Users\YourUserName\AppData\Local\Programs\Common\VST3".

Installing the VST 3 Single File Version on Linux
-------------------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the "js80p.vst3" file to your VST 3 directory which is usually
   "~/.vst3".

Installing the FST (VST 2.4) Version on Windows
-----------------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the "js80p.dll" file to the directory where you keep your VST 2.4
   plugins.

Installing the FST (VST 2.4) Version on Linux
---------------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the "js80p.so" file to the directory where you keep your VST 2.4
   plugins.

Usage
-----

 * Move the cursor over a knob, and use the mouse wheel for adjusting its
   value, or start dragging it.

 * Hold down the Control key while adjusting a knob for fine grained
   adjustments.

 * Double click on a knob to reset it to its default value.

 * Click on the area below a knob to assign a controller to it.

 * A buffer size of around 6 ms (256 samples at 44.1 kHz sample rate) usually
   gives good performance and low latency.

Tuning
------

Each oscillator can be tuned separately by clicking on the tuning selector in
the title bar of the oscillator, allowing maximum flexibility; for example, you
can have continuously updated tuning for one oscillator, and have the other
update its frequency tables only for "NOTE ON" events.

The following tuning options are available:

 * "C MTS-ESP": continuously query information from an MTS-ESP tuning provider
   (usually another plugin) when at least one note is playing, and update the
   frequency of all sounding notes that haven't reached the Release portion of
   their volume envelopes yet, on the fly. The tuning selector displays "on"
   when a tuning provider is available, and "off" when no tuning provider is
   found.  In the latter case, notes will fall back to 440 Hz 12 tone equal
   temperament.

 * "N MTS-ESP": query tuning information from an MTS-ESP tuning provider
   (usually another plugin) for evey "NOTE ON" MIDI event. Already sounding
   notes are kept unchanged. The tuning selector displays "on" when a tuning
   provider is available, and "off" when no tuning provider is found. In the
   latter case, new notes will fall back to 440 Hz 12 tone equal temperament.

 * "440 12TET": the usual 12 tone equal temperament tuning, where the "A4"
   MIDI note is 440 Hz.

 * "432 12TET": 12 tone equal temperament with "A4" set to 432 Hz.

Setting up MTS-ESP on Windows
-----------------------------

Download and install either the free MTS-ESP MINI plugin from
https://oddsound.com/mtsespmini.php or the paid MTS-ESP SUITE plugin from
https://oddsound.com/mtsespsuite.php by ODDSound. Follow the instuctions on
the website, where you can also find the User Guide documentation for each
product.

Note: it is possible to use a different tuning provider without installing any
of the above plugins, but you may have to manually download the "LIBMTS.dll"
library from the ODDSound/MTS-ESP GitHub repository at
https://github.com/ODDSound/MTS-ESP/tree/main/libMTS/Win

Put the downloaded "LIBMTS.dll" in a folder where Windows can find it:

 * On 64 bit Windows:
    * put the 64 bit DLL into the "Program Files\Common Files\MTS-ESP" folder,
    * put the 32 bit DLL into the "Program Files (x86)\Common Files\MTS-ESP"
      folder.
 * On 32 bit Windows, put the 32 bit version of "LIBMTS.dll" into the
   "Program Files\Common Files\MTS-ESP" folder.

Setting up MTS-ESP on Linux
---------------------------

As of November, 2023, there is no official distribution of the MTS-ESP tuning
provider plugins by ODDSound (https://oddsound.com/) for Linux, however, there
are plugins which can act as a tuning provider, for example, Surge XT:
https://surge-synthesizer.github.io/

To use MTS-ESP, you may have to download the "libMTS.so" library from the
ODDSound/MTS-ESP GitHub repository at
https://github.com/ODDSound/MTS-ESP/tree/main/libMTS/Linux and put it in the
"/usr/local/lib" directory, if it is not already installed on your system.
As of November, 2023, "libMTS.so" is only available for "x86_64" Linux systems.

Presets
-------

JS80P has a few built-in presets, and in case you don't like your DAW's preset
browser, you can load and save them as ordinary files. For each plugin type,
you can find these presets in the "presets" folder in the ZIP archive, and you
can load them into JS80P by clicking on the Import Patch icon near the top left
corner of the main screen of the plugin.

Bugs
----

If you find bugs, please report them at
https://github.com/attilammagyar/js80p/issues

Documentation
-------------

See "README.html" for more information.

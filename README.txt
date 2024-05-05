JS80P
=====

JS80P is a MIDI driven, performance oriented, versatile, free and open source
synthesizer VST plugin for Linux and Windows.

JS80P has two oscillators (and a sub-harmonic sine), and a lot of filters,
effects, envelopes, LFOs, and powerful macros to shape your sound with
subtractive, additive, FM, PM, and AM synthesis, complete with polyphonic,
monophonic, and split keyboard modes, MTS-ESP tuning support, and analog
imperfection emulation.

To download JS80P, visit its website at https://attilammagyar.github.io/js80p ,
or look for the "Releases" section at its GitHub page at
https://github.com/attilammagyar/js80p . (The source code is also available on
GitHub under the terms of the GNU General Public License Version 3.)

VST® is a trademark of Steinberg Media Technologies GmbH, registered in
Europe and other countries.

System Requirements
-------------------

 * Operating System: Windows 7 or newer, or Linux (e.g. Ubuntu 22.04)
 * CPU: SSE2 support, 32 bit (i686) or 64 bit (x86-64)
    * separate packages are available for AVX capable 64 bit processors
 * RAM: 200-300 MB per instance, depending on buffer sizes, sample rate, etc.

Note: a RISC-V 64 port is available as a separate project by @aimixsaka at
https://github.com/aimixsaka/js80p/

Dependencies on Windows
-----------------------

Typical Windows systems usually have the MSVC library already installed, but in
case you need it, you can download it from Microsoft's website at
https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist .

(Most people need the "X64" version of this library. To use the 32 bit version
of the plugin, you will need the "X86" version of the library. See the
Before Installing: Choosing a Distribution section below for more information.)

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

Before Installing: Choosing a Distribution
------------------------------------------

If your plugin host application does not support VST 3, but does support
VST 2.4, then you have to download and install the FST version of JS80P.
Otherwise, you should go with a VST 3 bundle on both Windows and Linux.

If your CPU supports AVX instructions and you use a 64 bit plugin host
application (also known as "x86_64"), then you should download a JS80P package
that is optimized for AVX compatible processors. If you have an older computer,
or if you experience crashes, then you should go with one of the
SSE2 compatible JS80P packages.

If you are using an older VST 3 host, or if you are running a 32 bit (also
known as "i686" or "x86") VST 3 host on a 64 bit Linux system, then it might
not be able to load the VST 3 bundle, so you will have to go with a VST 3
single file JS80P package that matches the architecture of your host
application.

The 32 bit versions are usually only needed by those who deliberately use a 32
bit plugin host application, e.g. because they want to keep using some really
old plugins which are not available for 64 bit systems.

If you are in doubt, then try the VST 3 bundle, and if your plugin host
application doesn't recognize it, then try the 64 bit VST 3 single file
version, then the 64 bit FST version, then the 32 bit VST 3 single file
version, and so on.

Note that all versions use the same high-precision sound synthesis engine
internally, so the CPU architecture does not affect the sound quality.

Packages
~~~~~~~~

 * VST 3 bundles for both Windows and Linux:

    * "js80p-X_Y_Z-avx-vst3_bundle.zip": for modern (AVX), 64 bit CPUs. This is
      what most people need.

    * "js80p-X_Y_Z-sse2-vst3_bundle.zip": for older (SSE2), 64 or 32
      bit CPUs.

 * VST 3 single file editions for Windows:

    * "js80p-X_Y_Z-windows-x86_64-avx-vst3_single_file.zip": for modern
      (AVX), 64 bit CPUs.

    * "js80p-X_Y_Z-windows-x86_64-sse2-vst3_single_file.zip": for older
      (SSE2), 64 bit CPUs.

    * "js80p-X_Y_Z-windows-x86-sse2-vst3_single_file.zip": for older (SSE2),
      32 bit CPUs.

 * VST 3 single file editions for Linux:

    * "js80p-X_Y_Z-linux-x86_64-avx-vst3_single_file.zip": for modern (AVX),
      64 bit CPUs.

    * "js80p-X_Y_Z-linux-x86_64-sse2-vst3_single_file.zip": for older
      (SSE2), 64 bit CPUs.

    * "js80p-X_Y_Z-linux-x86-sse2-vst3_single_file.zip": for older (SSE2),
      32 bit CPUs.

 * FST editions for Windows:

    * "js80p-X_Y_Z-windows-x86_64-avx-fst.zip": for modern (AVX), 64 bit
      CPUs.

    * "js80p-X_Y_Z-windows-x86_64-sse2-fst.zip": for older (SSE2), 64 bit
      CPUs.

    * "js80p-X_Y_Z-windows-x86-sse2-fst.zip": for older (SSE2), 32 bit CPUs.

 * FST editions for Linux:

    * "js80p-X_Y_Z-linux-x86_64-avx-fst.zip": for modern (AVX), 64 bit CPUs.

    * "js80p-X_Y_Z-linux-x86_64-sse2-fst.zip": for older (SSE2), 64 bit
      CPUs.

    * "js80p-X_Y_Z-linux-x86-sse2-fst.zip": for older (SSE2), 32 bit CPUs.

 * Source:

    * "js80p-X_Y_Z-src.zip": the source code of the plugin which you can
      compile for any CPU architecture. (See the "Development" section in the
      documentation.)

Installing the VST 3 Bundle on Windows
--------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the entire "js80p.vst3" folder to your VST 3 folder which is
   usually "C:\Users\YourUserName\AppData\Local\Programs\Common\VST3".

Installing the VST 3 Bundle on Linux
------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the entire "js80p.vst3" directory to your VST 3 directory which is
   usually "~/.vst3".

Installing the VST 3 Single File Version on Windows
---------------------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the "js80p.vst3" file to your VST 3 folder which is usually
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
3. Copy the "js80p.dll" file to the folder where you keep your VST 2.4
   plugins.
4. Optionally, if your host application can load ".vstxml" files, it is
   recommended to copy the "js80p.vstxml" file as well to the folder where
   you keep your VST 2.4 plugins.

Note: VST 2.4 plugins are usually put in the "C:\Program Files\VstPlugins"
folder.

Installing the FST (VST 2.4) Version on Linux
---------------------------------------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Copy the "js80p.so" file to the directory where you keep your VST 2.4
   plugins.
4. Optionally, if your host application can load ".vstxml" files, it is
   recommended to copy the "js80p.vstxml" file as well to the directory where
   you keep your VST 2.4 plugins.

Note: VST 2.4 plugins are usually put in the "~/.vst" directory.

Usage
-----

Most of the parameters that control various aspects of the sound that are
produced by JS80P can be adjusted via virtual knobs on the screen:

 * Move the mouse cursor over a knob or a small screw icon, and use the mouse
   wheel or move the mouse while holding down the left mouse button for
   adjusting the value.

 * Hold down the Control key on the keyboard while adjusting a knob for fine
   grained adjustments.

 * Double click on a knob to reset it to its default value.

 * Click on the area below a knob to assign a controller to it, or to remove a
   previously assigned one. When a knob has a controller assigned to it, its
   value can no longer be changed manually, it's set and continuously adjusted
   by the selected controller. (Note: if you accidentally open the controller
   selector screen, and you want to close it without changing anything, then
   just click on the "none" option or the controller that is already selected.)

   Controllers are various events and automations that can be used for setting
   the momentary value (on a relative scale between 0% and 100%) of a JS80P
   parameter knob.

Open the "README.html" file from the ZIP archive in a browser for a detailed
explanation of each parameter and controller.

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
https://oddsound.com/mtsespsuite.php by ODDSound. Follow the instructions on
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

Open the "README.html" file from the ZIP archive in a browser for more
information.

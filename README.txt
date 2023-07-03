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

 * Operating System: Windows 7 or newer, or Linux (e.g. Ubuntu 20.04 or newer)
 * CPU: SSE2 support, 32 bit (i686) or 64 bit (x86-64)
 * RAM: 150-300 MB per instance, depending on buffer sizes, etc.

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
Otherwise, you should go with the VST 3 bundle on all supported operating
systems.

If your plugin host application fails to recognize JS80P from the VST 3 bundle,
then you have to download and install the VST 3 Single File version that
matches the CPU architecture for which your plugin host application was built.
(For example, some 32 bit (i686) versions of Reaper are known to be unable to
recognize VST 3 bundles when running on a 64 bit system.)

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

 * It is recommended to use a small buffer size for lower latency, for example,
   3-6 milliseconds, or 128 or 256 samples at 44.1 kHz sample rate.

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

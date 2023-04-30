JS80P
=====

A MIDI driven, performance oriented, versatile synthesizer VST plugin.

VST® is a trademark of Steinberg Media Technologies GmbH, registered in
Europe and other countries.

To download JS80P, visit its website at https://attilammagyar.github.io/js80p/

The source code is available at https://github.com/attilammagyar/js80p under
the terms of the GNU General Public License Version 3.

System Requirements
-------------------

 * Operating System: Windows 7 or newer, or Linux (e.g. Ubuntu 20.04 or newer)
 * CPU: 32 or 64 bit, SSE2 support
 * RAM: 50-200 MB, depending on buffer size

On Linux, the "libxcb", "libxcb-render", and "libcairo" libraries, and either
the "kdialog" or the "zenity" application are required to run JS80P. To install
them on Debian based distributions (e.g. Ubuntu), you can use the following
command:

    sudo apt-get install libxcb1 libxcb-render0 libcairo2 zenity kdialog

Installation
------------

1. Download JS80P from its website at https://attilammagyar.github.io/js80p
2. Extract the ZIP archive.
3. Depending on which plugin type you downloaded, copy "js80p.dll" on Windows,
   or "js80p.so" on Linux to the folder where you keep your VST 2.4 plugins,
   or copy "js80p.vst3" to your VST 3 folder.

The "presets" folder in the archive contains a few sounds that you can load by
clicking the Import Patch icon near the top left corner of the main screen of
the plugin.

Bugs
----

If you find bugs, please report them at
https://github.com/attilammagyar/js80p/issues

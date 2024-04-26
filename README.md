JS80P
=====

JS80P is a MIDI driven, performance oriented, versatile, free and open source
[synthesizer VST plugin][plugin] for Linux and Windows.

  [plugin]: https://en.wikipedia.org/wiki/Virtual_Studio_Technology

<img src="https://raw.githubusercontent.com/attilammagyar/js80p/master/js80p.png" alt="Screenshot of JS80P" />

JS80P has two oscillators (and a sub-harmonic sine), and a lot of filters,
effects, envelopes, LFOs, and powerful macros to shape your sound with
subtractive, additive, FM, PM, and AM synthesis, complete with polyphonic,
monophonic, and split keyboard modes, MTS-ESP tuning support, and analog
imperfection emulation.

To download JS80P, visit its website at
[https://attilammagyar.github.io/js80p/](https://attilammagyar.github.io/js80p/).

The source code is available at
[https://github.com/attilammagyar/js80p](https://github.com/attilammagyar/js80p)
under the terms of the GNU General Public License Version 3.

> VST® is a trademark of Steinberg Media Technologies GmbH, registered in
> Europe and other countries.

<a id="toc"></a>

Table of Contents
-----------------

 * [Table of Contents](#toc)
 * [Features](#features)
 * [Installation](#install)
    * [Choosing a Distribution](#install-dist)
    * [System Requirements](#system-reqs)
       * [Dependencies on Windows](#windows-deps)
       * [Dependencies on Linux](#linux-deps)
    * [VST 3 Bundle on Windows](#vst3-bundle-windows)
    * [VST 3 Bundle on Linux](#vst3-bundle-linux)
    * [VST 3 Single File on Windows](#vst3-single-windows)
    * [VST 3 Single File on Linux](#vst3-single-linux)
    * [FST (VST 2.4) on Windows](#fst-windows)
    * [FST (VST 2.4) on Linux](#fst-linux)
    * [Setting up MTS-ESP Tuning Providers](#install-tuning)
       * [Windows](#mts-esp-windows)
       * [Linux](#mts-esp-linux)
 * [Usage](#usage)
    * [Signal Chain (Simplified)](#usage-signal)
    * [Knobs](#usage-knobs)
    * [Controllers](#usage-controllers)
    * [Synthesizer (Synth)](#usage-synth)
       * [Main Panel](#usage-synth-main)
       * [Common Oscillator Settings](#usage-synth-common)
       * [Oscillator 1 (Modulator)](#usage-synth-modulator)
       * [Oscillator 2 (Carrier)](#usage-synth-carrier)
    * [Effects](#usage-effects)
       * [Volume Controls](#usage-effects-volume)
       * [Distortions](#usage-effects-distortions)
       * [Filters](#usage-effects-filters)
       * [Chorus](#usage-effects-chorus)
       * [Echo](#usage-effects-echo)
       * [Reverb](#usage-effects-reverb)
    * [Macros (MC)](#usage-macros)
    * [Envelopes Generators (ENV)](#usage-envelopes)
    * [Low-Frequency Oscillators (LFOs)](#usage-lfos)
 * [Presets](#presets)
    * [Blank](#preset-blank)
    * [Bright Organ](#preset-bright-organ)
    * [Chariots-Aftertouch](#preset-chariots-aftertouch)
    * [Chariots](#preset-chariots)
    * [Demo 1](#preset-demo-1)
    * [Demo 2](#preset-demo-2)
    * [Kalimba](#preset-kalimba)
    * [Rock Organ](#preset-rock-organ)
    * [Sandstorm](#preset-sandstorm)
    * [Stereo Saw](#preset-stereo-saw)
    * [Acid Lead 1](#preset-acid-lead-1)
    * [Acid Lead 2](#preset-acid-lead-2)
    * [Acid Lead 3](#preset-acid-lead-3)
    * [Bells 1](#preset-bells-1)
    * [Bells 2](#preset-bells-2)
    * [Flute](#preset-flute)
    * [FM Womp 1](#preset-fm-womp-1)
    * [FM Womp 2](#preset-fm-womp-2)
    * [FM Womp 3](#preset-fm-womp-3)
    * [Tech Noir Lead 1](#preset-tech-noir-lead-1)
    * [Tech Noir Lead 2](#preset-tech-noir-lead-2)
    * [Tech Noir Lead 3](#preset-tech-noir-lead-3)
    * [Derezzed](#preset-derezzed)
    * [Ambient Pad 1](#preset-ambient-pad-1)
    * [Ambient Pad 2](#preset-ambient-pad-2)
    * [Ambient Pad 3](#preset-ambient-pad-3)
    * [Saw Piano](#preset-saw-piano)
    * [Saw Piano Reversed](#preset-saw-piano-reversed)
    * [Nightmare Lead](#preset-nightmare-lead)
    * [Tremolo Lead](#preset-tremolo-lead)
    * [Monophonic Saw](#preset-monophonic-saw)
    * [Dystopian Cathedral](#preset-dystopian-cathedral)
    * [Gloomy Brass](#preset-gloomy-brass)
    * [Gloomy Brass Raindrops](#preset-gloomy-brass-raindrops)
    * [Sawyer](#preset-sawyer)
    * [Analog Brass AT](#preset-analog-brass-at)
    * [Analog Brass mod](#preset-analog-brass-mod)
    * [Bouncy](#preset-bouncy)
 * [Bugs](#bugs)
 * [Frequently Asked Questions](#faq)
    * [Which distribution should I download?](#faq-which)
    * [Mac version?](#faq-mac)
    * [Parameters, Envelopes, LFOs, and polyphony: how do they work?](#faq-params-polyphony)
    * [The knobs in the waveform harmonics section don't do anything, is this a bug?](#faq-custom-wave)
    * [How can parameters be automated? What parameters does the plugin export?](#faq-automation)
    * [Aren't Phase Modulation and Frequency Modulation equivalent? Why have both?](#faq-pm-fm)
    * [Where does the name come from?](#faq-name)
    * [FL Studio: How to assign a MIDI CC to a JS80P knob?](#faq-flstudio-midicc)
    * [FL Studio: How to assign Channel Pressure (Aftertouch) to a JS80P knob?](#faq-flstudio-aftertouch)
    * [FL Studio: How to use the Sustain Pedal?](#faq-flstudio-sustain)
 * [Development](#dev)
    * [Tools](#dev-tools)
    * [Dependencies](#dev-dep)
    * [Compiling](#dev-compile)
    * [Theory](#dev-theory)

<a id="features"></a>

Features
--------

 * 64 notes polyphony.
 * Last-note priority monophonic mode.
    * Legato playing will either retrigger or smoothly glide to the next note,
      depending on the portamento length setting.
 * Polyphonic and monophonic hold mode to keep notes ringing without a sustain
   pedal.
 * 2 oscillators with 10 waveforms:
    * sine,
    * sawtooth,
    * soft sawtooth,
    * inverse sawtooth,
    * soft inverse sawtooth,
    * triangle,
    * soft triangle,
    * square,
    * soft square,
    * custom.
 * 2 filters for each oscillator, 7 filter types:
    * low-pass,
    * high-pass,
    * band-pass,
    * notch,
    * bell (peaking),
    * low-shelf,
    * high-shelf.
 * Sub-harmonic sine wave for oscillator 1, distortion for oscillator 2.
 * Adjustable oscillator pitch inaccuracy and instability, for analog-like
   liveliness and warmth.
    * Set the same inaccuracy or instability level for the two oscillators to
      synchronize their imperfections within a single polyphonic voice, so that
      the effect will only occur between multiple voices, but not between the
      oscillators within a single voice.
 * Microtuning support via the MTS-ESP tuning protocol by
   [ODDSound](https://oddsound.com/).
 * Portamento.
 * Wavefolder.
 * Split keyboard.
 * Amplitude modulation.
 * Frequency modulation.
 * Phase modulation.
 * Built-in effects:
    * overdrive,
    * distortion,
    * 2 more filters,
    * chorus,
    * stereo echo (with distortion and side-chaining),
    * stereo reverb (with distortion and side-chaining),
    * volume controls at various points of the signal chain.
 * 12 envelopes with customizable shapes.
 * 8 low-frequency oscillators (LFO) with optional amount envelope and
   polyphony.
 * Filter and envelope imperfection settings for analog-like feel.
 * MIDI controllers and macros.
 * Channel pressure (aftertouch).
 * MIDI learn.
 * Logarithmic or linear scale filter cutoff frequencies.
 * Tempo synchronization for envelopes, LFOs, and effects.
 * Use the peak level at various points of the signal chain to control
   parameters:
    * oscillator 1 output,
    * oscillator 2 output,
    * volume control 1 input,
    * volume control 2 input,
    * volume control 3 input.

<a id="install" href="#toc">Table of Contents</a>

Installation
------------

<a id="install-dist"></a>

### Choosing a Distribution

If your plugin host application does not support VST 3, but does support
VST 2.4, then you have to download and install the FST version of JS80P.
Otherwise, you should go with the VST 3 bundle on both Windows and Linux.

If your CPU supports [AVX instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions)
and you use a 64 bit plugin host application, then you should download a JS80P
package that is optimized for AVX compatible processors. If you have an older
computer, or if you experience crashes, then you should go with one of the
[SSE2](https://en.wikipedia.org/wiki/SSE2) compatible JS80P packages.

If your plugin host application fails to recognize JS80P from the VST 3 bundle,
then you have to download and install the VST 3 Single File version that
matches the CPU architecture for which your plugin host application was built.

(For example, some 32 bit (i686) versions of Reaper are known to be unable to
recognize VST 3 bundles when running on a 64 bit Linux system, so you would
have to download the 32 bit VST 3 Single File JS80P package.)

<a id="system-reqs" href="#toc">Table of Contents</a>

### System Requirements

 * **Operating System**: Windows 7 or newer, or Linux (e.g. Ubuntu 22.04).
 * **CPU**: SSE2 support, 32 bit (i686) or 64 bit (x86-64).
    * Separate packages are available for AVX capable 64 bit processors for
      better performance and CPU utilization.
 * **RAM**: 200-300 MB per instance, depending on buffer sizes, sample rate,
   etc.

Tested with [REAPER](https://www.reaper.fm/) 7.14.

A buffer size of around 6 ms (256 samples at 44.1 kHz sample rate) usually
gives good performance and low latency.

Note: a RISC-V 64 port is available as a
[separate project](https://github.com/aimixsaka/js80p/) by
[@aimixsaka](https://github.com/aimixsaka/).

<a id="windows-deps"></a>

#### Dependencies on Windows

Typical Windows systems usually have the MSVC library already installed, but in
case you need it, you can download it from
[Microsoft's website](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist).

(Most people need the `X64` version of this library. To use the 32 bit version
of the plugin, you will need the `X86` version of the library. See the
[Choosing a Distribution](#install-dist) section for more information.)

<a id="linux-deps"></a>

#### Dependencies on Linux

On Linux, the `libxcb`, `libxcb-render`, and `libcairo` libraries, and either
the `kdialog` or the `zenity` application are required to run JS80P. To install
them on Debian based distributions (e.g. Ubuntu), you can use the following
command:

    sudo apt-get install libxcb1 libxcb-render0 libcairo2 zenity kdialog

Note that if you want to run the 32 bit version of JS80P on a 64 bit system,
then you will have to install the 32 bit version of the libraries, for example:

    sudo apt-get install libxcb1:i386 libxcb-render0:i386 libcairo2:i386 zenity kdialog

<a id="vst3-bundle-windows"></a>

### VST 3 Bundle on Windows

1. [Download JS80P](https://attilammagyar.github.io/js80p/index.html#download).
2. Extract the ZIP archive.
3. Copy the entire `js80p.vst3` folder to your VST 3 folder which is
   usually `C:\Users\YourUserName\AppData\Local\Programs\Common\VST3`.

<a id="vst3-bundle-linux"></a>

### VST 3 Bundle on Linux

1. [Download JS80P](https://attilammagyar.github.io/js80p/index.html#download).
2. Extract the ZIP archive.
3. Copy the entire `js80p.vst3` directory to your VST 3 directory which is
   usually `~/.vst3`.

<a id="vst3-single-windows"></a>

### VST 3 Single File on Windows

1. [Download JS80P](https://attilammagyar.github.io/js80p/index.html#download).
2. Extract the ZIP archive.
3. Copy the `js80p.vst3` file to your VST 3 folder which is usually
   `C:\Users\YourUserName\AppData\Local\Programs\Common\VST3`.

<a id="vst3-single-linux"></a>

### VST 3 Single File on Linux

1. [Download JS80P](https://attilammagyar.github.io/js80p/index.html#download).
2. Extract the ZIP archive.
3. Copy the `js80p.vst3` file to your VST 3 directory which is usually
   `~/.vst3`.

<a id="fst-windows"></a>

### FST (VST 2.4) on Windows

1. [Download JS80P](https://attilammagyar.github.io/js80p/index.html#download).
2. Extract the ZIP archive.
3. Copy the `js80p.dll` file to the folder where you keep your VST 2.4 plugins.
4. Optionally, if your host application can load `.vstxml` files, it is
   recommended to copy the `js80p.vstxml` file as well to the folder where
   you keep your VST 2.4 plugins.

Note: VST 2.4 plugins are usually put in the `C:\Program Files\VstPlugins`
folder.

<a id="fst-linux"></a>

### FST (VST 2.4) on Linux

1. [Download JS80P](https://attilammagyar.github.io/js80p/index.html#download).
2. Extract the ZIP archive.
3. Copy the `js80p.so` file to the directory where you keep your VST 2.4
   plugins.
4. Optionally, if your host application can load `.vstxml` files, it is
   recommended to copy the `js80p.vstxml` file as well to the directory where
   you keep your VST 2.4 plugins.

Note: VST 2.4 plugins are usually put in the `~/.vst` directory.

<a id="install-tuning"></a>

### Setting up MTS-ESP Tuning Providers

<a id="mts-esp-windows"></a>

#### Windows

Download and install either the free [MTS-ESP Mini](https://oddsound.com/mtsespmini.php)
plugin or the paid [MTS-ESP Suite](https://oddsound.com/mtsespsuite.php) plugin
by [ODDSound](https://oddsound.com/). Follow the instuctions on the website,
where you can also find the User Guide documentation for each product.

Note: it is possible to use a different tuning provider without installing any
of the above plugins, but you may have to manually download the `LIBMTS.dll`
library from the [ODDSound/MTS-ESP GitHub repository](https://github.com/ODDSound/MTS-ESP/tree/main/libMTS/Win),
and put it in a folder where Windows can find it:

 * On 64 bit Windows:
    * put the 64 bit DLL into the `Program Files\Common Files\MTS-ESP` folder,
    * put the 32 bit DLL into the `Program Files (x86)\Common Files\MTS-ESP`
      folder.
 * On 32 bit Windows, put the 32 bit version of `LIBMTS.dll` into the
   `Program Files\Common Files\MTS-ESP` folder.

<a id="mts-esp-linux"></a>

#### Linux

As of November, 2023, there is no official distribution of the MTS-ESP tuning
provider plugins by [ODDSound](https://oddsound.com/) for Linux, however, there
are plugins which can act as a tuning provider, for example,
[Surge XT](https://surge-synthesizer.github.io/).

To use MTS-ESP, you may have to download the `libMTS.so` library from the
[ODDSound/MTS-ESP GitHub repository](https://github.com/ODDSound/MTS-ESP/tree/main/libMTS/Linux),
and put it in the `/usr/local/lib` directory, if it is not already installed on
your system.  As of November, 2023, `libMTS.so` is only available for `x86_64`
Linux systems.

<a id="usage" href="#toc">Table of Contents</a>

Usage
-----

<a id="usage-signal"></a>

### Signal Chain (Simplified)

                                                               (x64)
    Oscillator --> Filter --> Wavefolder --> Filter --> Volume ---------> Mixer
                     ^                                       |            ^   |
                     |                                       |            |   |
    Sub-oscillator --+                                       |            |   |
                                                             |            |   |
     (Frequency, Phase, and Amplitude Modulation)            |            |   |
     +-------------------------------------------------------+            |   |
     |                                                                    |   |
     v                                                                    |   |
     Oscillator                                                           |   |
     |                                                                    |   |
     v                                                          (x64)     |   |
     Filter --> Wavefolder --> Distortion --> Filter --> Volume ----------+   |
                                                                              |
            +-----------------------------------------------------------------+
            |
            v
            Volume --> Overdrive --> Distortion --> Filter --> Filter --+
                                                                        |
            +-----------------------------------------------------------+
            |
            v
            Volume --> Chorus --> Echo --> Reverb --> Volume --> Out

<a id="usage-knobs" href="#toc">Table of Contents</a>

### Knobs

Most of the parameters that control various aspects of the sound that are
produced by JS80P can be adjusted via virtual knobs on the screen:

 * Move the mouse cursor over a knob or a small screw icon, and use the mouse
   wheel or move the mouse while holding down the left mouse button for
   adjusting the value.

 * Hold down the Control key on the keyboard while adjusting a knob for fine
   grained adjustments.

 * Double click on a knob to reset it to its default value.

 * Click on the area below a knob to assign a [controller](#usage-controllers)
   to it, or to remove a previously assigned one. When a knob has a controller
   assigned to it, its value can no longer be changed manually, it's set and
   continuously adjusted by the selected controller. (Note: if you accidentally
   open the controller selector screen, and you want to close it without
   changing anything, then just click on the controller or the "none" option
   that is already selected.)

<a id="usage-controllers" href="#toc">Table of Contents</a>

### Controllers

Controllers are various events and automations that can be used for setting the
momentary value (on a relative scale between 0% and 100%) of a
[JS80P parameter knob](#usage-knobs):

 * **MIDI CC**: [MIDI Control Change](https://midi.org/midi-1-0-control-change-messages)
   messages are usually sent when various faders, knobs, and wheels are
   adjusted on a MIDI device. For example, the modulation wheel on a keyboard
   usually sends a CC 1 message, the expression pedal is usually associated
   with CC 11 messages, the sustain pedal is CC 64, etc.

 * **MIDI Learn**: this is a wild card MIDI CC controller: when it is assigned
   to a parameter, JS80P will replace the assignment with the first MIDI CC
   event type that it receives. This is useful when you don't know the MIDI CC
   number of a knob or fader on your MIDI device.

   (Note that JS80P does not process certain MIDI CC event types that are
   reserved in the [MIDI specification](https://midi.org/midi-1-0-control-change-messages)
   for special uses. If your device sends such messages as if they were general
   purpose messages, then it is recommended to change its configuration in
   order to avoid compatibility problems.)

 * **Triggered Note**: a value which is associated with note pitch and which is
   updated everytime a new note starts to play, e.g. when a key is pressed on a
   MIDI keyboard. Using this in [macros](#usage-macros) is an extremely
   flexible way to set up key tracking for any parameter of the sound.

 * **Triggered Velocity**: the speed with which a key on a MIDI keyboard is
   pressed, or more generally, a measure of how strongly a note is to be played
   (where 50% of the nominal range would be mezzo-forte). JS80P automatically
   uses this value to adjust the amplitude of oscillator signals (depending on
   the [VEL S](#usage-synth-common-vels) parameter), but it can also be used
   for controlling other parameters as well. For example, by assigning this
   controller to [filter cutoff frequency](#usage-synth-common-freq) parameters
   (especially with using [macros](#usage-macros)), you can brighten up forte
   notes while keeping softer notes warmer, darker.

 * **Released Note**:  a value which is associated with note pitch and which is
   updated everytime a note stops, e.g. when a key is released on a MIDI
   keyboard.

 * **Released Velocity**: the speed with which a key on a MIDI keyboard is
   released, or more generally, a measure of how softly a note is to be
   stopped. Unfortunately, most MIDI devices don't provide this data.

 * **Pitch Wheel**: this represents the controller that can be used on MIDI
   instruments to do pitch bends. (On a typical MIDI keyboard, this is a wheel
   that automatically returns to the center position when released.)

 * **Osc 1 Out Peak**, **Osc 2 Out Peak**, **Vol 1 In Peak**,
   **Vol 2 In Peak**, **Vol 3 In Peak**: JS80P continuously measures the peak
   amplitude of the signal at various points of the signal chain. These
   controllers make it possible to use this information for adjusting the
   parameters of the sound.

 * **Channel Aftertouch**: some keyboards can measure how the pressure on
   pressed keys change over time, making it possible to add dynamic expression
   to notes over time while they are playing.

 * **Macros**: MIDI-based controller values go from 0% to 100% on a linear
   scale, which is often quite limiting. [Macros](#usage-macros) are JS80P's
   way of customizing how a MIDI event affects a certain parameter.

 * **LFOs**: see [low-frequency oscillators](#usage-lfos).

 * **Envelopes**: see [envelope generators](#usage-envelopes).

<a id="usage-synth" href="#toc">Table of Contents</a>

### Synthesizer (Synth)

This tab contains the foundational settings of the sound.

<a id="usage-synth-main"></a>

#### Main Panel

##### Import Patch, Export Patch

The two icons at the top left corner allow saving and loading synth settings
(often called "patches" or "presets") as ordinary files, e.g. for transferring
sounds across projects, different host applications, computers, etc.

<a id="usage-synth-main-nh"></a>

##### Note Handling

Click on the black bar below the Import Patch an Export Patch icons, or use the
mouse wheel while holding the mouse cursor over it to change how JS80P handles
note events. The available options are:

 * **POLY**: polyphonic mode, 64 notes can be played simultaneously.

 * **MONO**: monophonic mode, only a single note can be played at a time.

    * When a new note start event is received while a previous note is still
      playing (e.g. legato), then the new note takes priority: the old note is
      stopped, and the new note is started.

    * If the new note is released, and the previous note is still held, then
      the previous note is restarted.

    * If the [PRT](#usage-synth-common-prt) setting of an oscillator is set
      to a value above 0, then that oscillator will smootly glide its frequency
      and volume to match the new note's pitch and velocity.

 * **M HOLD**: same as **MONO**, but note stop events are ignored until a
   different note handling setting is selected, or until a sustain pedal off
   event is received.

 * **P HOLD**: same as **POLY**, but note stop events are ignored until a
   different note handling setting is selected, or until a sustain pedal off
   event is received.

<a id="usage-synth-main-mode"></a>

##### Operating Mode (MODE)

Choose whether to mix the sound of the two oscillators or route low notes to
one oscillator, and higher notes to the other. The available options are:

 * **Mix&Mod**: the sound of the two oscillators can be mixed into the output,
   and the sound of the second oscillator (carrier) can be modulated in various
   ways by the first oscillator (modulator).

 * **Split C3**, **Split Db3**, etc.: notes below the selected split point are
   played only on the first oscillator, notes above the split point (inclusive)
   are played only on the second oscillator.

##### Modulator Additive Volume (MIX)

When the [MODE](#usage-synth-main-mode) knob is in the Mix&Mod position, this
knob controls how loud the first oscillator can be heard in the output. Turn it
down to 0% in order to make the first oscillator contribute to the final sound
only via modulation.

##### Phase Modulation (PM)

Turn it up to have the first oscillator modulate the phase of the second
oscillator when the [MODE](#usage-synth-main-mode) knob is in the Mix&Mode
position.

##### Frequency Modulation (FM)

Turn it up to have the first oscillator modulate the frequency of the second
oscillator when the [MODE](#usage-synth-main-mode) knob is in the Mix&Mode
position.

##### Amplitude Modulation (AM)

Turn it up to have the first oscillator modulate the amplitude of the second
oscillator when the [MODE](#usage-synth-main-mode) knob is in the Mix&Mode
position. (This is also known as "ring modulation".)

<a id="usage-synth-common" href="#toc">Table of Contents</a>

#### Common Oscillator Settings

<a id="usage-synth-common-tuning"></a>

##### Tuning

Click on the black bar next to the oscillator's name, or use the mouse wheel
while holding the mouse cursor over it to change the tuning. The available
options are:

 * **C MTS-ESP**: use an [external tuning provider](#install-tuning) for
   determining the frequency of each note. Frequencies of notes which haven't
   reached the release stage of their envelopes are updated continuously. The
   connection to the tuning provider is indicated by "on" or "off" appearing in
   the tuning selector box. If the connection to the tuning provider is broken,
   then 440 Hz based
   [12 tone equal temperament](https://en.wikipedia.org/wiki/Equal_temperament)
   is used.

 * **N MTS-ESP**: use an [external tuning provider](#install-tuning) for
   determining the frequency of each note. Note frequencies are updated only
   when a new note is started. The connection to the tuning provider is
   indicated by the "on" or "off" sign. If the connection to the tuning
   provider is broken, then 440 Hz based 12 tone equal temperament is used.

 * **440 12TET**: the usual 12 tone equal temperament with the A4 note being
   tuned to 440 Hz.

 * **432 12TET**: 12 tone equal temperament with the A4 note being tuned to
   432 Hz.

##### Oscillator Inaccuracy

The first screw icon next to the [tuning selector](#usage-synth-common-tuning)
adds a level of randomization to the oscillator's frequency, mimicing the
imperfectness of analog synthesizers. The more the screw is turned, the more
off each note will be from the correct pitch.

When the inaccuracy of the two oscillators is set to the same value, and the
[MODE](#usage-synth-main-mode) knob is in the Mix&Mod position, then the
oscillators will synchronize their randomness: each note will be off from the
correct pitch by a random amount, but within a single voice, the two
oscillators will be off by the same amount.

##### Oscillator Instability

The second screw icon next to the [tuning selector](#usage-synth-common-tuning)
adds a level of randomization to the oscillator's frequency over time, mimicing
the imperfectness of analog synthesizers. The more the screw is turned, the
more the oscillator will diverge from the correct pitch, hovering sometimes
above, and sometimes below.

When the instability of the two oscillators is set to the same value, and the
[MODE](#usage-synth-main-mode) knob is in the Mix&Mod position, then the
oscillators will synchronize their random divergence: each note will be off
from the correct pitch by a random amount, but within a single voice, the two
oscillators will always be off by the same amount, and will always diverge by
the same amount in the same direction.

##### Finde Detune x4 (FIN x4)

The [Fine Detune (FIN)](#usage-synth-common-fin) parameter goes from -1200
cents to +1200 cents (-1 octave to +1 octave), but if you want to go extreme
with pitch bends, this toggle switch can increase the range to -4800 cents to
+4800 cents (-4 octaves to +4 octaves).

<a id="usage-synth-common-prt"></a>

##### Portamento Length (PRT)

The time it takes for the oscillator's frequency to reach the current note's
pitch starting from the pitch of the previous note.

When the synthesizer is in [MONO](#usage-synth-main-nh) mode, and
notes are played in a legato fashion (notes are started before the previous
ones would have ended), then it also controls how long the oscillator's
amplitude will take to match the velocity of the latest note.

##### Portamento Depth (PRD)

When set to a non-zero value (specified in cents), then oscillator frequency
will glide to the current note's pitch from a fixed level of detuning in
accordance with the [PRT](#usage-synth-common-prt) setting.

When its value is 0 s, then the starting frequency of the glide will be the
pitch of the previous note.

##### Detune (DTN)

Coarse detune, specifying how many semitones higher or lower the oscillator
should play above or below the note's actual pitch.

<a id="usage-synth-common-fin"></a>

##### Fine Detune (FIN)

Fine detune, specifying how many cents higher or lower the oscillator should
play above or below the note's actual pitch.

##### Amplitude (AMP)

Sets the volume of the oscillator before any filtering and shaping.

<a id="usage-synth-common-filter"></a>

##### Filter Logarithmic Frequency (LG F)

The toggle switch above the [FREQ](#usage-synth-common-freq) knob can switch
between using a linear or a logarithmic scale for the knob.

<a id="usage-synth-common-lgq"></a>

##### Filter Logarithmic Q Factor (LG Q)

The toggle switch above the [Q](#usage-synth-common-q) knob can switch
between using a linear or a logarithmic scale for the knob.

##### Filter Cutoff Frequency Inaccuracy

The first little screw next to the [LG Q](#usage-synth-common-lgq) switch adds
a level of randomization to the filter's frequency to mimic imperfections of
analog synthesizers.

Be careful with this, because with certain filter settings, too much
randomization can produce loud noises and sound artifacts or make the filter
completely silent.

##### Filter Q Inaccuracy

The second little screw next to the [LG Q](#usage-synth-common-lgq) switch adds
a level of randomization to the filter's Q factor to mimic imperfections of
analog synthesizers.

Be careful with this, because with certain filter settings, too much
randomization can produce loud noises and sound artifacts or make the filter
completely silent.

<a id="usage-synth-common-type"></a>

##### Filter Type (TYPE)

The following filter types are available:

 * **LP**: low-pass filter, the frequencies above the
   [cutoff frequency](#usage-synth-common-freq) are attenuated. The
   [Q factor](#usage-synth-common-q) controls the resonance.

 * **HP**: high-pass filter, the frequencies below the
   [cutoff frequency](#usage-synth-common-freq) are attenuated. The
   [Q factor](#usage-synth-common-q) controls the resonance.

 * **BP**: band-pass filter, the frequencies outside a band around the
   [cutoff frequency](#usage-synth-common-freq) are filtered out. The
   [Q factor](#usage-synth-common-q) controls the width of the band. (Low Q
   value makes the band wide, high Q value makes the band narrow.)

 * **Notch**: band-stop filter, the frequencies inside a band around the
   [cutoff frequency](#usage-synth-common-freq) are filtered out. The
   [Q factor](#usage-synth-common-q) controls the width of the band. (Low Q
   value makes the band wide, high Q value makes the band narrow.)

 * **Bell**: boosts or attenuates a band of frequencies around the
   [cutoff frequency](#usage-synth-common-freq). The
   [Q factor](#usage-synth-common-q) controls the width of the band. (Low Q
   value makes the band wide, high Q value makes the band narrow.)

 * **LS**: low-shelf filter, boosts or attenuates frequencies belove the
   [cutoff frequency](#usage-synth-common-freq).

 * **HS**: high-shelf filter, boosts or attenuates frequencies above the
   [cutoff frequency](#usage-synth-common-freq).

<a id="usage-synth-common-freq"></a>

##### Filter Cutoff Frequency (FREQ)

The cutoff frequency of the filter.

Be careful with this, because depending on other settings of the filter, a too
low or too high cutoff frequency can produce loud noises and sound artifacts or
it can make the filter completely silent.

<a id="usage-synth-common-q"></a>

##### Filter Q Factor (Q)

The quality factor of the filter. Its precise meaning depends on the position
of the [TYPE](#usage-synth-common-type) knob. This knob has no effect for
the low-shelf and high-shelf filters.

Be careful with this, because depending on other settings of the filter, a too
low or too high Q factor can produce loud noises and sound artifacts or it can
make the filter completely silent.

##### Filter Gain (GAIN)

Controls the boosting or attenuation when the [TYPE](#usage-synth-common-type)
knob is in the Bell, LS (low-shelf), or HS (high-shelf) positions. It has no
effect on other filter types.

Be careful with this, because too much boosting can make the signal too loud.

##### Wave Folding (FOLD)

A wave folder is a similar wave shaping effect to a clipping distortion, but
the portion of the signal that is above the maximum level is not clipped, but
reflected back on itself. Then if it reaches the maximum amplitue again in the
opposite direction, then it is reflected again and again. This process adds a
lot of complexity to the sound, making it sound metallic.

<a id="usage-synth-common-vels"></a>

##### Velocity Sensitivity (VEL S)

Controls how the oscillator's volume reacts to note velocity. When it's set to
0%, then the oscillator will use a fix volume for each note. Between 0% and
100%, the oscillator's volume will react to velocity linearly. Above 100%, the
oscillator's volume will react to note velocity more and more logarithmically.

##### Volume (VOL)

The last chance to control the oscillator's volume after wave shaping and
filtering.

##### Width (WID)

When set to 0%, the oscillator will play all notes exactly in the middle of the
stereo field. When set to a positive value, higher notes will lean towards the
right side of the stereo field, and bass notes will lean towards the left. When
set to a negative value, high notes will lean towards the left side, and bass
notes will lean towards the right.

##### Panning (PAN)

Changes the position of the oscillator in the stereo field. Positive values
move the oscillator towards the right side, negative values move it towards the
left side.

<a id="usage-synth-common-wav"></a>

##### Waveform (WAV)

Selects the waveform of the oscillator. The available options are:

 * **Sine**: simple sine wave without any harmonics above the fundamental.

 * **Saw**: sawtooth wave, ramps slowly, drops quickly. Contains both even and
   odd harmonics.

 * **Soft Sw**: a softer, warmer version of the sawtooth wave. Less prone to
   aliasing when various modulations and wave folding are engaged.

 * **Inv Saw**: inverse sawtooth wave, rises quickly, drops slowly. Contains
   both even and odd harmonics.

 * **Soft I S**: a softer, warmer version of the inverse sawtooth wave. Less
   prone to aliasing when various modulations and wave folding are engaged.

 * **Triangle**: rises slowly, drops slowly. Contasins only odd harmonics.

 * **Soft Tri**: a softer, warmer version of the triangle wave. Less prone to
    aliasing when various modulations and wave folding are engaged.

 * **Square**: rises quickly, stays up for half a period, then drops quickly
   and stays down for half a period. Contains only odd harmonics.

 * **Soft Sqr**: a softer, warmer version of the square wave. Less prone to
    aliasing when various modulations and wave folding are engaged.

 * **Custom**: the amplitudes of the waveform's harmonics can be set using the
   10 knobs in the _Waveform & harmonics for the Custom waveform_ section.
   Useful for emulating tonewheel organ sounds.

<a id="usage-synth-modulator" href="#toc">Table of Contents</a>

#### Oscillator 1 (Modulator)

##### Subharmonic Amplitude (SUB)

Sets how loud the subharmonic oscillator should be. The subharmonic oscillator
plays a sine wave exactly one octave below the oscillator's main frequency.

<a id="usage-synth-carrier"></a>

#### Oscillator 2 (Carrier)

##### Distortion (DIST)

Soft-clipping distortion.

<a id="usage-effects" href="#toc">Table of Contents</a>

### Effects

<a id="usage-effects-volume"></a>

#### Volume Controls

There are three volume controls placed at strategic points of the effects
chain. Their purpose is twofold: they control the loudness of the signal, and
they also measure the incoming peaks for the signal loudness based
[controllers](#usage-controllers).

Note that the first volume controller goes up to 200% so that you can boost the
signal which is going into the
[distortion effects](#usage-effects-distortions).

<a id="usage-effects-distortions"></a>

#### Distortions

##### Overdrive (Odrv, GAIN)

Subtle saturation and soft-clipping distortion to add a little bit of
crunchiness to the sound.

##### Distortion (Dist, GAIN)

Heavy distortion for angrier sounds with a lot of bite.

<a id="usage-effects-filters"></a>

#### Filters

See [oscillator filters](#usage-synth-common-filter).

<a id="usage-effects-chorus"></a>

#### Chorus

The chorus effect mixes a signal with slightly delayed copies of itself, and
modulates the delay times with low-frequency oscillators so that the cloned
signals have a subtle variation in their phase and pitch relative to the
original signal and to each other. This creates the illusion of having many
different instances of the signal, making it sound bigger and fatter.

##### Logarithmic High-pass Filter Q Factor (LG Q)

The first component in the Chorus effect is a high-pass filter, in order to
to control the low end of the sound, and to avoid bass frequencies becoming
muddy. This toggle switch makes the resonance parameter of the high-pass filter
use a logarithmic scale instead of a linear scale.

##### Logarithmic LFO Frequency (LG LFO FREQ)

Switch between a linear and a logarithmic scale for the
[LFO frequency (FREQ)](#usage-effects-chorus-freq) parameter.

##### Logarithmic Filter Cutoff Frequencies (LG FILTER FREQS)

Switch between a linear and a logarithmic scale for the high-pass
filter at the beginning of the Chorus effect's signal chain, and for the
high-shelf filter near the end of it.

##### Tempo Synchronization (BPM SYNC)

Turn on tempo synchronization for the low-frequency oscillators and the delays
in the effect, so that what they normally measure in terms of seconds will be
measured in terms of beats instead. (This only works in hosts which provide
tempo information to plugins.)

Note: in order to prevent the memory that is allocated for delay buffers from
growing arbitrarily large, JS80P will not go below 30 BPM for tempo
synchronization.

##### High-pass Filter Cutoff Frequency (HP F)

Set the cutoff frequency of the high-pass filter that is at the beginning of
the signal chain of the effect in order to keep it from making bass frequencies
too muddy.

##### High-pass Filter Cutoff Frequency (HP Q)

Set the resonance of the high-pass filter that is at the beginning of
the signal chain of the effect.

##### Type (TYPE)

Select the chorus type. Different chorus types feature a different number of
voices (ranging from 3 up to 7) which are arranged in various different
positions in the stereo field (with regards to the
[WIDTH](#usage-effects-chorus-wid) parameter as well), with various different
loudness values, etc.

##### Delay Time (DEL)

The maximum time by which the chorus voices are lagging behind the main voice.

<a id="usage-effects-chorus-freq"></a>

##### LFO Frequency (FREQ)

Controls how quickly the lag of each chorus voice varies.

##### Depth (DPT)

Control how much the chorus voices vary their lag behind the main voice.

##### Dampening Frequency (DF)

Chorus voices are sent into a high-shelf filter which can be used for
attenuating higher frequencies, making the chorused sound warmer, darker.
This parameter sets the cutoff frequency of the filter.

##### Dampening Gain (DG)

Control how much the high-shelf filter attenuates high frequencies.

##### Feedback (FB)

Tell how loud the output of the effect is to be mixed back into its input. Be
careful with this, because too much feedback can lead to a loud runaway signal
feedback loop.

<a id="usage-effects-chorus-wid"></a>

##### Stereo Width (WID)

Set the stereo spread of the chorus. When set to 0%, all the voices are placed
in the middle of the stereo field.

##### Wet Volume (WET)

Control the loudness of the chorused signal.

##### Dry Volume (DRY)

Control the loudness of the original, unmodified input signal.

<a id="usage-effects-echo" href="#toc">Table of Contents</a>

#### Echo

The Echo effect repeats the input signal with a delay, optionally with various
levels of filtering and distortion, and with side-chain compression.

##### Logarithmic High-pass Filter Q Factor (LG Q)

The first component in the Echo effect is a high-pass filter, in order to
to control the low end of the sound, and to avoid bass frequencies becoming
muddy. This toggle switch makes the resonance parameter of the high-pass filter
use a logarithmic scale instead of a linear scale.

##### Logarithmic Filter Cutoff Frequencies (LG FILTER FREQS)

Switch between a linear and a logarithmic scale for the high-pass
filter at the beginning of the Echo effect's signal chain, and for the
high-shelf filter near the end of it.

##### Tempo Sync (BPM SYNC)

Turn on tempo synchronization for the delay lines in the effect, so that time
will be measured in therms of beats instead of in terms of seconds. (This only
works in hosts which provide tempo information to plugins.)

Note: in order to prevent the memory that is allocated for delay buffers from
growing arbitrarily large, JS80P will not go below 30 BPM for tempo
synchronization.

##### Input Volume (IN V)

Control the input signal level for the effect. This parameter goes from 0% to
200%, so that the signal can be boosted for the
[distortion](#usage-effects-echo-dist) that is built into the effect.

##### High-pass Filter Cutoff Frequency (HP F)

Set the cutoff frequency of the high-pass filter that is at the beginning of
the signal chain of the effect in order to keep it from making bass frequencies
too muddy.

##### High-pass Filter Cutoff Frequency (HP Q)

Set the resonance for the high-pass filter that is at the beginning of
the signal chain of the effect.

##### Delay Time (DEL)

Set how long the echo signal will lag behind the original signal.

Note: to simulate the feel of old tape delays, assign an [LFO](#usage-lfos) to
the delay time parameter which is set to oscillate very slowly, by a really low
amount, maybe with a tiny amount of randomization.

<a id="usage-effects-echo-dist"></a>

##### Distortion (DIST)

Add some saturation or soft-clipping distortion to the delayed signal. This can
also come handy for simulating slightly overdriven analog tape delay effect
units.

##### Dampening Frequency (DF)

The delayed signal is sent into a high-shelf filter which can be used for
attenuating higher frequencies, making the echo sound warmer, darker.
This parameter sets the cutoff frequency of the filter.

##### Dampening Gain (DG)

Control how much the high-shelf filter attenuates high frequencies.

##### Feedback (FB)

Tell how loud the output of the effect is to be mixed back into its input. Be
careful with this, because too much feedback can lead to a loud runaway signal
feedback loop.

##### Stereo Width (WID)

Set the stereo spread of the echos. The further away this value is from 0%, the
further from the center the echos will be, bouncing back and forth between the
right and left speaker.

<a id="usage-effects-echo-cth"></a>

##### Side-chain Compression Threshold (CTH)

Threshold for the side-chain compression: when the input signal goes above this
level, the effect will reduce the volume of the echos in order to keep them
from stealing the spotlight from the original signal in the mix. Once the input
signal goes below the threshold, the echos will be brought up to their intended
level.

##### Side-chain Compression Attack Time (CATK)

Set how fast or how slow the compression should kick in when the raw signal
goes above the threshold.

##### Side-chain Compression Release Time (CREL)

Set how fast or how slow the compression should bring back the echo signal to
its normal level once the raw input signal goes below the threshold.

##### Side-chain Compression Ratio (CR)

Control the amount of compression that is applied to the echo signal based on
the loudness of the raw input signa.

For example, if the [compression threshold](#usage-effects-echo-cth) is -11 dB,
and the input signal is -5 dB, then the input is 6 dB louder than the
threshold, because `-11 + 6 = -5`. A compression ratio of 3 will turn this 6 dB
difference into `6 / 3 = 2` dB, so the target gain will be `-11 + 2 = -9` dB,
which means a -4 dB signal reduction. The trick with side-chaining is that the
necessary loudness reduction is calculated for one signal, but applied to
another; in this case, the echo signal is the one which gets the -4 dB
reduction, in order to let the raw sound shine.

Later, if the input signal reaches -2 dB, then the difference from the -11 dB
threshold will be 9 dB. Since `9 / 3 = 3`, and `-11 + 3 = -8`, the -2 dB signal
will need a -6 dB reduction in order to hit the -8 dB signal level. Therefore,
a reduction of -6 dB will be applied to the echo signal.

Then when the input signal goes below -11 dB, no more loudness reduction will
be applied to the echo signal.

##### Wet Volume (WET)

Control the loudness of the echo signal.

##### Dry Volume (DRY)

Control the loudness of the original, unmodified input signal.

<a id="usage-effects-reverb" href="#toc">Table of Contents</a>

#### Reverb

The Reverb effect combines multiple delay lines to simulate sound reflections
that can be heard in various rooms, optionally with various levels of filtering
and distortion, and with side-chain compression.

The signal chain is similar to the one in the
[Echo effect](#usage-effects-echo), so many of the parameters work the same way
as described there.

Only the parameters that are unique to the Reverb effect are listed below.

##### Type (TYPE)

Select from various distributions and numbers of reflections, ranging from
small rooms with just a handful of reflection points, to large cathedrals with
many spread out reflections.

##### Room Reflectivity (ROOM)

Control how reflective the room is, in other words, how loud are the sounds
that bounce off its walls. The higher this value, the longer it takes for the
reverberation to decay into silence after the raw input signal goes quiet.

<a id="usage-macros" href="#toc">Table of Contents</a>

### Macros (MC)

Macros are [controllers](#usage-controllers) that can combine and transform
other momentary value based controllers, like
[MIDI CC](https://midi.org/midi-1-0-control-change-messages), pitch wheel
position, and even other macros. Basic controllers like the modulation wheel of
a MIDI keyboard go from 0% to 100% on a linear scale. Macros can (for example)
make it go from 80% down to 30% and use a non-linear curve in between.

The typical usage of a macro is to assign a controller to its
[input (IN)](#usage-macros-in) parameter, set up the minimum, maximum,
distortion, etc.  parameters, and then assign the macro to an oscillator,
filter, effect, envelope, or LFO parameter. Assigning various other controllers
(e.g. note value, velocity, other macros, etc.) to the parameters of a macros
can result in interesting and creative interactions between controllers.

#### Midpoint

Move the mouse cursor over the first function graph icon at the top right
corner of a macro, and start moving it while holding the left mouse button
down, or start using the mouse wheel to adjust the midpoint of the macro's
[input](#usage-macros-in).

This is most useful when the input of the macro is associated with the pitch
bend wheel of a MIDI keyboard, and you want precise control over the range that
is covered by the lower and the upper half of the wheel's movement.

#### Distortion Shape

Click on the second function graph icon at the top right corner of a macro, or
use the mouse wheel while holding the mouse cursor over it to select the
non-linearity shape that the macro will use when its
[distortion (DIST)](#usage-macros-dist) parameter is set to a value above 0%.

<a id="usage-macros-in"></a>

#### Input (IN)

Assign the controller that you want to transform via the macro to this
parameter.

<a id="usage-macros-min"></a>

#### Minimum Value (MIN)

The macro will output this value when either its [input](#usage-macros-in) or
its [amount](#usage-macros-amt) parameter is at the 0% position.

<a id="usage-macros-max"></a>

#### Maximum Value (MAX)

The macro will output this value when both its [input](#usage-macros-in) and
its [amount](#usage-macros-amt) parameters are at the 100% position.

<a id="usage-macros-amt"></a>

#### Amount (AMT)

Scale the [input](#usage-macros-in) by this amount.

<a id="usage-macros-dist"></a>

#### Distortion (DIST)

Tell how far the macro should diverge from the linear path between its
[minimum](#usage-macros-min) and [maximum](#usage-macros-max) values.

#### Randomness (RAND)

Set how much the macro's value should be randomized while still staying between
the [minimum](#usage-macros-min) and [maximum](#usage-macros-max) values.

<a id="usage-envelopes" href="#toc">Table of Contents</a>

### Envelope Generators (ENV)

An envelope generator is a [controller](#usage-controllers) that shapes the
sound over time by defining target values for parameters which will be reached
in a given amount of time after a note is triggered.

Many of the [synthesis parameters](#usage-synth) are polyphonic, which means
that each simultaneously playing voice can use different values and run
their own individual timelinle for those parameters, as long as an envelope is
assigned to them.

Envelopes in JS80P use DAHDSR stages to reach four different values for their
assigned parameters:

 * **Delay**: when a note is triggered, the parameter will stay on the initial
   level for this amount of time.

 * **Attack**: the parameter will reach the peak level in this amount of time.

 * **Hold**: the parameter will stay on the peak level for this long.

 * **Decay**: the parameter will reach the sustain level in this amount of
   time.

 * **Sustain**: once the D-A-H-D stages are complete, the parameter will stay
   on the sustain level until the note is stopped. (If the sustain level is 0%,
   then JS80P will automatically release the note when this stage is reached.)

 * **Release**: once the note is stopped, the parameter will take this long to
   reach the final level.

Note: when an envelope is assigned to an amplitude or volume parameter, it is
recommended to have its initial and final level be set to 0%.

The transitions between values can take various different shapes:

 * **Linear**: follow a straight line towards the target value.

 * **Smooth-smooth**: start slowly, then pick up some speed, and then slow down
   again before landing on the target value.

 * **Smooth-sharp**: start slowly, then as the time to reach the target value
   starts to run out, pick up more and more speed, and reach it quickly.

 * **Sharp-smooth**: start quickly, then as the target value is getting closer,
   slow down and land on it gently and smoothly.

 * **Sharp-sharp**: start quickly, then lose momentum near the midpoint, and
   when the time to reach the target value starts to run out, pick up the speed
   again and reach it quickly.

Each non-linear shape has 3 different versions with various steepness.

#### Attack Shape

The first icon in the header row of envelope settings selects the shape that
defines how the envelope will go from the initial level to the peak level.
Click on it to select a different shape, or use the mouse wheel while holding
the cursor over it.

#### Decay Shape

The second icon in the header row of envelope settings selects the shape that
defines how the envelope will go from the peak level to the sustain level.
Click on it to select a different shape, or use the mouse wheel while holding
the cursor over it.

#### Release Shape

The third icon in the header row of envelope settings selects the shape that
defines how the envelope will go from the sustain level to the final level.
Click on it to select a different shape, or use the mouse wheel while holding
the cursor over it.

#### Update Mode

When a note is triggered and a voice starts to track its own envelope timeline
for a parameter, its default behaviour is to capture a snapshot of the envelope
settings at that moment, and run the envelope generator with those values.
Changing the envelope settings after this moment has no effect at all on
already engaged voices while they are playing. This behaviour can be adjusted
by selecting a different update strategy by clicking on the box next to the
transition shapes, or using the mouse wheel while holding the cursor over it.
The available options are:

 * **STA**: static envelope: the default behaviour where the envelope settings
   are never updated by voices while they are playing a note.

 * **END**: the same as the static mode, except that the envelope settings
   snapshot is updated once when the note is released.

 * **DYN**: dynamic envelope: the voice will continuously update the envelope
   snapshot, and adjust the controlled parameter's value so that it converges
   to the level where it should be according to the momentary state of the
   envelope settings.

#### Tempo Synchronization (BPM)

The time interval of each stage will be measured in terms of beats instead of
in seconds if the host provides tempo information to the plugin.

#### Time Inaccuracy

The first little screw in the top right corner of envelope settings adds
randomization to envelope stage lengths. Useful for simulating imperfectness of
analog hardware.

#### Level Inaccuracy

The second little screw in the top right corner of envelope settings adds
randomization to envelope levels. Useful for simulating imperfectness of analog
hardware.

#### Amount (AMT)

Scales all four levels of the envelope.

#### Initial Level (INI)

The level where the envelope starts.

#### Peak Level (PEAK)

Target level for the Attack stage.

#### Sustain Level (SUS)

Target level for the Decay stage.

#### Final Level (FIN)

Target level for the Release stage.

#### Delay Time (DEL)

How long the envelope will stay on the initial level before the Attack stage
begins.

#### Attack Time (ATK)

How long it will take to reach the peak level from the initial level.

#### Hold Time (HOLD)

How long the envelope will stay on the peak level.

#### Decay Time (DEC)

How long it will take to reach the sustain level from the peak level.

#### Release Time (REL)

How long it will take to reach the final level from the value where the
parameter is at when the note stop event is received.

<a id="usage-lfos" href="#toc">Table of Contents</a>

### Low-Frequency Oscillators (LFOs)

A low-frequency oscillator (LFO) is a controller which makes a parameter's
value oscillate back and forth between two values.

#### Logarithmic Frequency (LG FREQ)

Toggle the [frequency](#usage-lfos-freq) parameter of the LFO between a
logarithmic and a linear scale.

<a id="usage-lfos-center"></a>

#### Center (CENTER)

Make the LFO oscillate around the midpoint of the [minimum](#usage-lfos-min)
and [maximum](#usage-lfos-max) values instead of between the two extremes.
This makes a difference when the [amount](#usage-lfos-amt) parameter of the LFO
is set to a value below 100%: by default, the LFO will oscillate above the
minimum value, and it won't reach the maximum, but when the LFO is centered, it
will not reach either extremes, it will stay within a radius of the midpoint
which is half the value of the amount parameter. (This is similar to the
unipolar/bipolar LFO modes in other synthesizers.)

#### Amount Envelope (AMT ENV)

Click on the black box next to the [centering](#usage-lfos-center) switch in
the header row of an LFO, or use the mouse wheel while holding the cursor above
it to associate an [envelope generator](#usage-envelopes) with the LFO. When an
LFO with an amount envelope is assigned to a
[synthesis parameter](#usage-synth) that can accept control from envelope
generators as well, then each voice will run their own envelope and LFO
timeline for that parameter when a new note is triggered.

Furthermore, if a parameter of an LFO which has an amount envelope is
controlled by another LFO which also has an amount envelope, then each new note
will have its own timeline for both of those LFOs and envelopes. (Up to 6
timelines can be maintained by each parameter for each note. When that limit
runs out, or when there's a dependency cycle between LFOs, then some of the
LFOs will be run globally, as if there was no envelope assigned to them.)

Note that when the LFO is assigned to a parameter which doesn't accept control
from envelope generators, then the amount envelope has no effect.

<a id="usage-lfos-bpm"></a>

#### Tempo Synchronization (BPM SYNC)

The LFO's [frequency](#usage-lfos-freq) is normally measured in Hertz (Hz)
which means "cycles per second". Click on the BPM SYNC switch to change that to
"cycles per beat" instead. (This only works in hosts which provide tempo
information to plugins.)

#### Waveform (WAV)

Same as the [waveform parameter](#usage-synth-common-wav) of the audio-range
oscillators, except that LFOs don't have the "Custom" option.

<a id="usage-lfos-freq"></a>

#### Frequency (FREQ)

Set how many oscillations to do per second (or per beat, when the
[BPM SYNC](#usage-lfos-bpm) switch is turned on).

#### Phase (PHS)

Shift the (initial) phase of the oscillator.

<a id="usage-lfos-min"></a>

#### Minimum Value (MIN)

Set the lowest value of the oscillation. When it is greater than the
[maximum](#usage-lfos-max) value, then the waveform is flipped upside down.

<a id="usage-lfos-max"></a>

#### Maximum Value (MAX)

Set the highest value of the oscillation. When it is less than the
[minimum](#usage-lfos-min) value, then the waveform is flipped upside down.

<a id="usage-lfos-amt"></a>

#### Amount (AMT)

Set the amplitude of the oscillation.

#### Distortion (DIST)

Distort the waveform with a soft-clipping distortion.

#### Randomness (RAND)

Randomize the waveform.

<a id="presets" href="#toc">Table of Contents</a>

Presets
-------

JS80P has a few built-in presets, and in case you don't like your plugin host
application's preset browser, you can load and save them as ordinary files. For
each plugin type, you can find these presets in the `presets` folder in the ZIP
archive, and you can load them into JS80P by clicking on the _Import Patch_
icon near the top left corner of the main screen of the plugin.

<a id="preset-blank"></a>

### Blank

The default, empty patch, a blank canvas.

<a id="preset-bright-organ"></a>

### Bright Organ

A bright, clean, Hammond-like organ sound. Aftertouch and mod wheel increase
the vibrato. The softer you play, the slower the attack, the harder you play,
the harder the attack.

<a id="preset-chariots-aftertouch"></a>

### Chariots-Aftertouch

A Vangelis-inspired split keyboard patch. Notes below C3 are modulated with
and inverse sawtooth LFO, notes above C3 get some wavefolding treatment if
you use aftertouch. The mod wheel controls the vibrato of notes above C3, and
the pitch wheel also affects only these notes.

<a id="preset-chariots"></a>

### Chariots

A Vangelis-inspired split keyboard patch. Notes below C3 are modulated with
and inverse sawtooth LFO, notes above C3 get some wavefolding treatment,
depending on the velocity. The harder you play, the longer it takes for the
wavefolder to kick in. The mod wheel controls the vibrato of notes above
C3, and the pitch wheel also affects only these notes.

<a id="preset-demo-1"></a>

### Demo 1

The patch from the first demo video of JS80P. The mod wheel makes the sound
brighter, the volume knob (CC 7) adjusts the inverse sawtooth LFO which
controls the filter before the wavefolder.

<a id="preset-demo-2"></a>

### Demo 2

The patch from the second demo video of JS80P. This split keyboard patch has
slow tremolo bass notes below Gb3, with extremely long release (just press a
key, and you're good for about 2 measures with your left hand being free, even
without a sustain pedal), and plucky lead notes, with the mod wheel and the
volume knob (CC 7) controlling the timbre. (However, it is not recommended
with this patch to adjust the mod wheel while a right hand note is playing.)

<a id="preset-kalimba"></a>

### Kalimba

A simple kalimba sound with percussive attack and short decay. The sustain
pedal adds more reverb and echo, and lengthens note decay.

<a id="preset-rock-organ"></a>

### Rock Organ

A little darker, overdriven Hammond-like organ sound. Aftertouch and mod wheel
increase the vibrato. The softer you play, the slower the attack, the harder
you play, the harder the attack.

<a id="preset-sandstorm"></a>

### Sandstorm

A dirty, harsh, detuned FM lead sound. Mod wheel and aftertouch make it even
dirtier and harsher.

<a id="preset-stereo-saw"></a>

### Stereo Saw

A little bit metallic sounding sawtooth wave. Note velocity slightly affects
the timbre, mod wheel and aftertouch add wavefolding. The volume knob
adjusts an LFO which controls filter resonance.

<a id="preset-acid-lead-1"></a>

### Acid Lead 1

Sawtooth wave based acid lead sound. Aftertouch and mod wheel increase
the vibrato, the sustain pedal lengthens the decay.

<a id="preset-acid-lead-2"></a>

### Acid Lead 2

Square wave based acid lead sound. Aftertouch and mod wheel increase
the vibrato, the sustain pedal lengthens the decay.

<a id="preset-acid-lead-3"></a>

### Acid Lead 3

Another sawtooth wave based acid lead sound. Aftertouch and mod wheel increase
the vibrato, the sustain pedal lengthens the decay.

<a id="preset-bells-1"></a>

### Bells 1

A bright bell sound. Aftertouch and mod wheel increase the vibrato,
the sustain pedal lengthens the decay.

<a id="preset-bells-2"></a>

### Bells 2

A slightly darker bell sound. Aftertouch and mod wheel increase the vibrato,
the sustain pedal lengthens the decay.

<a id="preset-flute"></a>

### Flute

A synth flute sound with a couple of tricks: the pitch of the notes decreases
slightly when they end, and the mod wheel adds a little embellishment to the
beginning of each note. Aftertouch adds more emphasis to the note.

<a id="preset-fm-womp-1"></a>

### FM Womp 1

A clean FM sound where the modulator and the carrier use different envelopes,
so the notes start with a slight "wah" effect. Aftertouch and mod wheel
increase the vibrato. Aftertouch also adds more modulation.

<a id="preset-fm-womp-2"></a>

### FM Womp 2

A slightly distorted FM sound where the modulator and the carrier use
different envelopes, so the notes start with a slight "wah" effect. Aftertouch
and mod wheel increase the vibrato. Aftertouch also adds more modulation,
making the sound brighter and more distorted.

<a id="preset-fm-womp-3"></a>

### FM Womp 3

A more distorted FM sound where the modulator and the carrier use different
envelopes, so the notes start with a slight "wah" effect. Aftertouch and mod
wheel increase the vibrato. Aftertouch also adds more modulation, making the
sound brighter and more distorted.

<a id="preset-tech-noir-lead-1"></a>

### Tech Noir Lead 1

A brass sound for futuristic sci-fi dystopias with a slower filter sweep at
the beginning of notes, depending on note velocity. Mod wheel adds vibrato,
aftertouch adds emphasis and brightness.

<a id="preset-tech-noir-lead-2"></a>

### Tech Noir Lead 2

A brass sound for futuristic sci-fi dystopias with a harsher filter sweep at
the beginning of notes, depending on note velocity. Mod wheel adds vibrato,
aftertouch adds emphasis and brightness.

<a id="preset-tech-noir-lead-3"></a>

### Tech Noir Lead 3

A darker brass sound for futuristic sci-fi dystopias with a filter sweep at
the beginning of notes, depending on note velocity. Mod wheel adds vibrato,
aftertouch adds emphasis and brightness.

<a id="preset-derezzed"></a>

### Derezzed

This Daft Punk inspired patch is built around sawtooth waves with lots of
distortions. Mod wheel makes upcoming notes brighter (without affecting
the currently sounding ones), and it increases the distortion. Aftertouch
adds vibrato.

<a id="preset-ambient-pad-1"></a>

### Ambient Pad 1

Slowly evolving pad sound with sci-fi vibe. The mod wheel destabilizes
the tuning, the aftertouch increases that effect, and adds slight distortions.

<a id="preset-ambient-pad-2"></a>

### Ambient Pad 2

Slowly evolving pad sound with a dark sci-fi vibe. The mod wheel destabilizes
the tuning, the aftertouch increases that effect, and adds slight distortions.

<a id="preset-ambient-pad-3"></a>

### Ambient Pad 3

Slowly evolving pad sound with a dark sci-fi vibe. The mod wheel destabilizes
the tuning, the aftertouch increases that effect, and adds ghostly, distant
screams.

<a id="preset-saw-piano"></a>

### Saw Piano

A sawtooth wave based sound where high notes decay quickly, lower notes
decay slowly, but decay time is also affected by note velocity. The harder
you play, the brighter and richer the timbre, and the harder the note attack.
Mod wheel controls the vibrato, and aftertouch controls filtering. The
sustain pedal lengthens note decay.

<a id="preset-saw-piano-reversed"></a>

### Saw Piano Reversed

A sawtooth wave based sound which sounds like as if you were playing
a recording backwards. The time it takes for notes to un-decay depends on
note pitch and velocity. The harder you play, the brighter and richer the
sound gets. Mod wheel controls the vibrato, aftertouch adjusts the filtering.

<a id="preset-nightmare-lead"></a>

### Nightmare Lead

Starts out as a nice, filtered sawtooth wave, but as you begin to turn the mod
wheel and add some aftertouch, it becomes more and more menacing and distorted,
until it finally descends into madness.

<a id="preset-tremolo-lead"></a>

### Tremolo Lead

Thick lead sound. Aftertouch adds vibrato and harmonics, mod wheel opens up
the filter and adds a tremolo effect. Filtering also responds to note velocity.

<a id="preset-monophonic-saw"></a>

### Monophonic Saw

A sawtooth wave based monophonic, sustained sound with smooth legato glides.
The harder you play, the brighter and richer the timbre, and the harder the
note attack. Mod wheel controls the vibrato, and aftertouch controls the
filtering and the wavefolder. The sustain pedal lengthens note decay.

<a id="preset-dystopian-cathedral"></a>

### Dystopian Cathedral

Play softly and this futuristic organ-like patch will sound smooth as butter,
but when you start playing with more force, the sound will become angry, fat,
and in-your-face. The mod wheel increases the level of madness.

<a id="preset-gloomy-brass"></a>

### Gloomy Brass

Turn the mod wheel down for a soft, warm, and fat sound, turn it up for
brassy swells, distortion, and reverb. Aftertouch adds some harshness and
vibrato.

<a id="preset-gloomy-brass-raindrops"></a>

### Gloomy Brass Raindrops

A version of Gloomy Brass where each note is followed by a raindrop after a
random amount of time.

<a id="preset-sawyer"></a>

### Sawyer

Recreation of the iconic bass and lead sound from Tom Sawyer by Rush. Notes
below middle C will do the well-known resonance sweep and then turn into a
dark growl (which you can spice up with the mod wheel and some aftertouch),
and notes above middle C will be bright, but will lose much of their
brightness while doing a huge vibrato when you turn up the mod wheel or apply
aftertouch. (Note that this is not a split-keyboard patch, because both
oscillators are required to make this sound good in stereo - instead, the
effect is achieved through trickery with the Note controller and the
Distortion knob of Macro 12 and 13.)

<a id="preset-analog-brass-at"></a>

### Analog Brass AT

Play softly, pianissimo for a soft, smooth, warm, and fat analog brass sound.
Play fortissimo and use aftertouch for big bright swells. The mod wheel adds
vibrato, and the pitch wheel goes from -3 octaves to +1 octave.

<a id="preset-analog-brass-mod"></a>

### Analog Brass mod

Turn the mod wheel down and play softly, pianissimo for a soft, smooth, warm,
and fat analog brass sound. Turn it up and play fortissimo for big bright
swells. Aftertouch adds vibrato, and the pitch wheel goes from -3 octaves to
+1 octave.

<a id="preset-bouncy"></a>

### Bouncy

The sound of a ping-pong ball dropping on a glockenspiel, showcasing a use of
LFO amount envelopes.

<a id="bugs" href="#toc">Table of Contents</a>

Bugs
----

If you find bugs, please report them at
[https://github.com/attilammagyar/js80p/issues](https://github.com/attilammagyar/js80p/issues).

<a id="faq" href="#toc">Table of Contents</a>

Frequently Asked Questions
--------------------------

<a id="faq-which"></a>

### Which distribution should I download?

If your plugin host application does not support VST 3, but does support
VST 2.4, then you have to download and install the FST version of JS80P.
Otherwise, you should go with the VST 3 bundle on both Windows and Linux.

If your CPU supports [AVX instructions](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions)
and you use a 64 bit plugin host application, then you should download a JS80P
package that is optimized for AVX compatible processors. If you have an older
computer, or if you experience crashes, then you should go with one of the
[SSE2](https://en.wikipedia.org/wiki/SSE2) compatible JS80P packages.

If your plugin host application fails to recognize JS80P from the VST 3 bundle,
then you have to download and install the VST 3 Single File version that
matches the CPU architecture for which your plugin host application was built.

(For example, some 32 bit (i686) versions of Reaper are known to be unable to
recognize VST 3 bundles when running on a 64 bit Linux system, so you would
have to download the 32 bit VST 3 Single File JS80P package.)

The 32 bit versions are usually only needed by those who deliberately use a 32
bit plugin host application, e.g. because they want to keep using some really
old plugins which are not available for 64 bit systems.

If you are in doubt, then try the VST 3 bundle, and if your plugin host
application doesn't recognize it, then try the 64 bit VST 3 version, then the
64 bit FST version, then the 32 bit VST 3 version, and so on.

Note that all versions use the same high-precision sound synthesis engine
internally, so the CPU architecture does not affect the sound quality.

<a id="faq-mac" href="#toc">Table of Contents</a>

### Mac version?

Sorry, it's not likely to happen anytime soon, unless someone is willing to
create and maintain a Mac fork of JS80P. For me to do it, it would require
quite a large investment, both in terms of effort and financially. If MacOS
would be available (at a reasonable price) for installing it in a virtual
machine that could be used for testing, I'd consider that. But as long as it
cannot be obtained (legally) without also buying a Mac, and I'm happy with my
current computer, I'm not going to invest in a new one.

<a id="faq-params-polyphony" href="#toc">Table of Contents</a>

### Parameters, Envelopes, LFOs, and polyphony: how do they work?

By default, knobs and toggles act globally. This means that if you adjust a
knob with your mouse, or if you assign a MIDI value (controller, note velocity,
etc.), a Macro, or an LFO to it and adjust the parameter via that, or if you
use <a href="#faq-automation">automation in your plugin host application</a>,
then that parameter will change for all sounding notes.

But if you assign an Envelope as a controller to a parameter, then each
polyphonic note will use its own timeline for that parameter, and the
parameter's value will change over time for each note independently according
to the envelope's settings. By default, these settings are only evaluated once
for each note, at the very beginning of the note, so if the parameters of the
envelope are changed, then it will only affect the notes that start after the
adjustment.

Similarly, if you assign an LFO to a parameter which supports Envelopes, and
the LFO is associated with an Envelope, then the LFO becomes polyphonic, and
its amount will be controlled by the Envelope for each polyphonic voice
independently from the other voices. (Note that this is transitive up to a
limit: if a parameter of a polyphonic LFO is controlled by another LFO, then
the second LFO will also become polyphonic if it has an Envelope assigned to
it. An LFO chain like this can contain up to 6 LFOs and Envelopes, and if a
chain contains more than that, or has dependency cycles where LFOs control
each other's parameters, then some of the LFOs in the chain will be treated as
if there weren't any Envelopes assigned to them.)

To have polyphonic notes *sample and hold* a MIDI value or a Macro's momentary
value for a parameter for the entire duration of the note, independently of
other notes and subsequent changes of the value (e.g. to use lower filter
cutoff frequency for low-velocity notes so that they sound softer), then you
have to use an Envelope: turn up all the levels of the Envelope to 100%, assign
the MIDI value or the Macro to the Amount parameter of the Envelope, and assign
the Envelope to control the parameter.

If an Envelope is switched to Dynamic update mode, then polyphonic notes will
still track their own independent timelines for each parameter that has that
Envelope assigned, but the parameter's value will converge to the value that it
should have at each moment according to the momentary settings of the Envelope.

<a id="faq-custom-wave" href="#toc">Table of Contents</a>

### The knobs in the waveform harmonics section don't do anything, is this a bug?

To hear the effect of those knobs, you have to select the _Custom_ waveworm
using the `WAV` knob in the Waveform section of the oscillators.

(Note that these parameters are CPU-intensive to process, so they are not
sample accurate, they are not smoothed, and they are processed only once for
each rendering block. Due to this, if you change them while a note is playing,
or assign a controller to them, then you might hear "steps" or even clicks.)

<a id="faq-automation" href="#toc">Table of Contents</a>

### How can parameters be automated? What parameters does the plugin export?

The intended way of automating JS80P's parameters is to assign a
[MIDI Control Change][midicc] (MIDI CC) message to a parameter (or use _MIDI
Learn_), and turn the corresponding knob on your MIDI keyboard while playing,
or edit the MIDI CC events in your host application's MIDI editor.

However, the VST 3 plugin format requires plugins to export a proxy parameter
for each MIDI CC message that they want to receive, and as a side-effect, these
parameters can also be automated using the host application's usual automation
editor. For the sake of consistency, the FST plugin also exports automatable
parameters for each supported MIDI CC message.

  [midicc]: https://midi.org/midi-1-0-control-change-messages

For example, in both plugin types, you might assign the
`MIDI CC 1 (Modulation Wheel)` controller to the Phase Modulation (PM)
parameter of the synthesizer, and then add automation in the host application
to the `MIDI CC 1 (Modulation Wheel)` (VST 3) or `ModWh` (FST) parameter. JS80P
will then interpret the changes of this parameter the same way as if you were
turning the modulation wheel on a MIDI keyboard.

<a id="faq-pm-fm" href="#toc">Table of Contents</a>

### Aren't Phase Modulation and Frequency Modulation equivalent? Why have both?

The reason for JS80P having both kinds of modulation is that they are not
always equivalent. They are only identical when the modulator signal is a
sinusoid, but with each added harmonic, PM and FM start to differ more and
more. A detailed mathematical explanation of this is shown in
[doc/pm-fm-equivalence.pdf](https://github.com/attilammagyar/js80p/blob/main/doc/pm-fm-equivalence.pdf).

<a id="faq-name" href="#toc">Table of Contents</a>

### Where does the name come from?

In 2022, I started developing a browser-based synthesizer using the [Web Audio
API][webaudio], mostly being inspired by the [Yamaha CS-80][cs80]. I named that
project [JS-80][js80]. Then I started adding one little feature and
customization option after the other, then it got out of hand, and I also
started finding limitations of doing audio in the browser. So I decided to
implement a cleaned up version of this synth in C++ as a DAW plugin (with a
better waveshaper antialiasing method than what's available in the browser),
and so JS80P was born.

  [webaudio]: https://www.w3.org/TR/webaudio/
  [cs80]: https://en.wikipedia.org/wiki/Yamaha_CS-80
  [js80]: https://attilammagyar.github.io/toys/js-80/

<a id="faq-flstudio-midicc" href="#toc">Table of Contents</a>

### FL Studio: How to assign a MIDI CC to a JS80P knob?

[FL Studio](https://www.image-line.com/fl-studio/) does not send all MIDI
events that come out of a MIDI keyboard to plugins by default, and
unfortunately, [MIDI Control Change (MIDI CC)][midicc2] messages are among the
kinds of MIDI data that it swallows. To make everything work, you have to
connect the MIDI CC events of your device to one of the MIDI CC helper proxy
parameters that are provided by JS80P. (JS80P does not directly expose its
parameters to the host, in order to avoid conflicts between the host's
automations and JS80P's internal controller assignments.)

  [midicc2]: https://midi.org/midi-1-0-control-change-messages

For example, let's say a physical knob on your MIDI keyboard is configured to
send its changes in `MIDI CC 7` messages. To make this knob turn the _Phase
Modulation (PM)_ virtual knob in JS80P, you have to do the following steps:

1. Click on the small triangle in the top left corner of the plugin window of
   JS80P, and select the "_Browse parameters_" menu item.

2. Find the parameter named "_Vol_" (FST) or "_MIDI CC 7 (Volume)_" (VST 3) in
   the browser. Click on it with the right mouse button.

3. Select the "_Link to controller..._" menu item.

4. Turn the knob on your MIDI keyboard until FL Studio recognizes it.

5. Now click on the area below the _Phase Modulation (PM)_ virtual knob in
   JS80P's interface.

6. Select either the "_MIDI CC 7 (Volume)_" option, or the "_MIDI Learn_"
   option, and turn the physical knob on your MIDI keyboard again.

<a id="faq-flstudio-aftertouch" href="#toc">Table of Contents</a>

### FL Studio: How to assign Channel Pressure (Aftertouch) to a JS80P knob?

[FL Studio](https://www.image-line.com/fl-studio/) does not send all MIDI
events that come out of a MIDI keyboard to plugins by default, and
unfortunately, Channel Pressure (also known as Channel Aftertouch) messages are
among the kinds of MIDI data that it swallows.

Getting the Channel Pressure to work in FL Studio is a similar, but slightly
more complicated procedure than [setting up MIDI CC](#faq-flstudio-midicc).
For example, to make Channel Pressure control the _Phase Modulation (PM)_
virtual knob in JS80P, you have to do the following steps:

1. Click on the small triangle in the top left corner of the plugin window of
   JS80P, and select the "_Browse parameters_" menu item.

2. Press a piano key on your MIDI keyboard, and hold it down without triggering
   aftertouch.

3. While holding the piano key down, find the parameter named "_Ch AT_" (FST)
   or "_Channel Aftertouch_" (VST 3) in FL Studio's browser. Click on it with
   the right mouse button.

4. Select the "_Link to controller..._" menu item (keep holding the piano key).

5. Now push the piano key harder to trigger aftertouch.

6. Click on the area below the _Phase Modulation (PM)_ virtual knob in
   JS80P's interface.

7. Select the "_Channel Aftertouch_" option.

<a id="faq-flstudio-sustain" href="#toc">Table of Contents</a>

### FL Studio: How to use the Sustain Pedal?

[FL Studio](https://www.image-line.com/fl-studio/) does not send all MIDI
events that come out of a MIDI keyboard to plugins by default, and
unfortunately, the [MIDI Control Change (MIDI CC)][midicc3] message which
contains information about the sustain pedal's state is among the kinds of MIDI
data that it swallows.

To add insult to injury, FL Studio pretends that it notifies the plugin about
the pedal's state, but in reality, it just defers sending `NOTE OFF` events
until the pedal is released. This might give the desired effect in some cases,
but it breaks patches that assign additional functionality to the pedal besides
sustaining notes, like for example lengthening envelope release times.

To make everything work, you have to assign the sustain pedal's MIDI CC events
to the MIDI CC helper proxy parameter where JS80P expects them, and you have to
turn off FL Studio's default behavior of handling the pedal on behalf of
plugins.

  [midicc3]: https://midi.org/midi-1-0-control-change-messages

1. Open "_Options / MIDI settings_" from the main menu.

2. Turn off the "_Foot pedal controls note off_" option.

3. Close the _Settings_ dialog window.

4. Click on the small triangle in the top left corner of the plugin window of
   JS80P, and select the "_Browse parameters_" menu item.

5. Find the parameter named "_Sustn_" (FST) or "_MIDI CC 64 (Sustain Pedal)_"
   (VST 3) in the browser. Click on it with the right mouse button.

6. Select the "_Link to controller..._" menu item.

7. Press the pedal.

<a id="dev" href="#toc">Table of Contents</a>

Development
-----------

This section contains information for those who downloaded the source code of
JS80P and want to compile it themselves.

<a id="dev-tools"></a>

### Tools

#### Linux

 * [GNU Make 4.3+](https://www.gnu.org/software/make/)
 * [G++ 11.4.0+](https://gcc.gnu.org/)
 * [MinGW-w64 10+](https://www.mingw-w64.org/)
 * [Valgrind 3.18.1+](https://valgrind.org/)
 * [Cppcheck 2.7](https://github.com/danmar/cppcheck)
 * [Doxygen 1.9.1+](https://www.doxygen.nl/)

#### Windows

 * [WinLibs MinGW-w64 13.1.0+ (MSVCRT)](https://winlibs.com/)
 * [Doxygen 1.9.6+](https://www.doxygen.nl/)

<a id="dev-dep"></a>

### Dependencies

The `lib/` directory contains code from the following projects:

 * [FST](https://git.iem.at/zmoelnig/FST)
 * [MTS-ESP](https://github.com/ODDSound/MTS-ESP)
 * [VST 3 SDK](https://github.com/steinbergmedia/vst3sdk)

Note that the `lib/` directory does not include the whole SDK packages, it only
contains what's required for compiling JS80P.

#### Linux

To compile JS80P on e.g. Ubuntu Linux 22.04 for all supported platforms, the
following packages need to be installed:

    apt-get install \
        binutils \
        build-essential \
        g++ \
        gcc-multilib \
        g++-multilib \
        libcairo2-dev \
        libcairo2-dev:i386 \
        libx11-dev \
        libx11-dev:i386 \
        libxcb1-dev \
        libxcb1-dev:i386 \
        libxcb-render0-dev \
        libxcb-render0-dev:i386 \
        mingw-w64

<a id="dev-compile"></a>

### Compiling

#### Windows

Assuming that you have installed MinGW-w64 to `C:\mingw64`, you can use the
following commands to run tests and compile JS80P for Windows:

    SET PATH=C:\mingw64\bin;%PATH%
    SET TARGET_PLATFORM=x86_64-w64-mingw32
    SET DEV_OS=windows

    mingw32-make.exe check
    mingw32-make.exe all

#### Linux

Run `make check` for running tests.

The following commands (on a 64 bit Linux environment) will compile JS80P for
64 bit Windows, 32 bit Windows, `x86_64` Linux, `x86` Linux, and
`RISC-V 64` Linux respectively:

    TARGET_PLATFORM=x86_64-w64-mingw32 make all
    TARGET_PLATFORM=i686-w64-mingw32 make all
    TARGET_PLATFORM=x86_64-gpp make all
    TARGET_PLATFORM=i686-gpp make all
    TARGET_PLATFORM=riscv64-gpp make all

<a id="dev-theory" href="#toc">Table of Contents</a>

### Theory

 * [Wavetable Synthesis from McGill University](https://www.music.mcgill.ca/~gary/307/week4/wavetables.html)

 * [Bandlimited Synthesis of Classic Waveforms from McGill University](https://www.music.mcgill.ca/~gary/307/week5/bandlimited.html)

 * [W3C Web Audio API (2021. June 17)](https://www.w3.org/TR/2021/REC-webaudio-20210617/)

 * [W3C's adaptation of Audio-EQ-Cookbook.txt by Robert Bristow-Johnson](https://www.w3.org/TR/2021/NOTE-audio-eq-cookbook-20210608/)

 * [Amplitude Modulation from Sound on Sound](https://www.soundonsound.com/techniques/amplitude-modulation)

 * [An Introduction To Frequency Modulation from Sound on Sound](https://www.soundonsound.com/techniques/introduction-frequency-modulation)

 * [Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution
   by Julian D. Parker, Vadim Zavalishin, Efflam Le
   Bivic](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf)
   (a.k.a. Antiderivative Antialiasing, ADAA)

 * [Freeverb (by Jezar at Dreampoint) explained in Physical Audio Signal Processing, by Julius O. Smith III](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html)

 * [Lock-Free Programming With Modern C++ by Timur Doumler](https://www.youtube.com/watch?v=qdrp6k4rcP4)

 * [Lagrange Interpolation with Equally-Spaced Nodes from NIST Digital Library of Mathematical Functions](https://dlmf.nist.gov/3.3#ii)

 * [Using Faster Exponential Approximation from CodingForSpeed.COM ](https://codingforspeed.com/using-faster-exponential-approximation/)

 * [Multiply-with-carry pseudorandom number generator](https://en.wikipedia.org/wiki/Multiply-with-carry_pseudorandom_number_generator)

 * [The World's Smallest Hash Table by Orson Peters](https://orlp.net/blog/worlds-smallest-hash-table/)

 * [Horner's method for polynomial evaluation](https://en.wikipedia.org/wiki/Horner%27s_method)

 * [Splines](https://en.wikipedia.org/wiki/Spline_(mathematics))

 * [Matrix inversion](https://en.wikipedia.org/wiki/Invertible_matrix)

<a href="#toc">Table of Contents</a>

JS80P
=====

A highly customizable [synthesizer plugin][plugin]. (Work in progress.)

  [plugin]: https://en.wikipedia.org/wiki/Virtual_Studio_Technology

Signal chain (simplified):

    Oscillator --> Filter --> Folder --> Filter --> Effects --> Out
                                              |     ^
        (Frequency & Amplitude Modulation)    |     |
      +---------------------------------------+     |
      |                                             |
      v                                             |
      Oscillator --> Filter --> Folder --> Filter --+

Effects:

    Overdrive --> Distortion --> Filter --> Filter --> Echo --> Reverb

Development progress: 48%
-------------------------

 * Oscillator features: 80%
 * Modulation & mixing modes: 75%
 * Filters: 100%
 * Wave folder: 100%
 * Envelopes: 100%
 * LFOs: 0%
 * MIDI Controller Customization: 0%
 * GUI: 30%
 * Effects: 0%
 * Patch persistency: 0%

Features
--------

 * 16 notes polyphony
 * 2 oscillators (waveforms: sine, sawtooth, triangle, square, inverse
   sawtooth, custom)
 * 2 filters for each oscillator (low-pass, high-pass, band-pass, notch, bell,
   low-shelf, high-shelf)
 * portamento
 * wave folder
 * amplitude modulation
 * frequency modulation
 * built-in effects:
    * overdrive
    * distortion
    * filters (low-pass, high-pass, band-pass, notch, bell, low-shelf,
      high-shelf)
    * stereo echo
    * stereo reverb
 * 6 envelopes
 * 8 low-frequency oscillators (LFO)
 * configurable MIDI controllers

Name
----

In 2022, I started developing a browser-based synthesizer using the [Web Audio
API][webaudio], mostly being inspired by the [Yamaha CS-80][cs80]. I named that
project [JS-80][js80]. Then I started adding one little feature and
customization option after the other, then it got out of hand, and I also
started finding limitations of doing audio in the browser. So I decided to
implement a cleaned up (and antialiased!) version of this synth in C++ as a DAW
plugin, and so JS80P was born.

  [webaudio]: https://www.w3.org/TR/webaudio/
  [cs80]: https://en.wikipedia.org/wiki/Yamaha_CS-80]
  [js80]: https://attilammagyar.github.io/toys/js-80/

Development
-----------

### Tools

#### Linux

 * [GNU Make 4.2.1+](https://www.gnu.org/software/make/)
 * [GCC 9.3+](https://gcc.gnu.org/)
 * [MinGW-w64 7.0.0+](https://www.mingw-w64.org/)
 * [Valgrind 3.15.0+](https://valgrind.org/)
 * [Doxygen 1.8.17+](https://www.doxygen.nl/)

#### Windows

 * [WinLibs MinGW-w64 7.0.0+ (MSVCRT)](https://winlibs.com/)
 * [Doxygen 1.8.17+](https://www.doxygen.nl/)

### Dependencies

 * [FST](https://git.iem.at/zmoelnig/FST)

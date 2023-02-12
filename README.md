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

Development progress: 51.8%
---------------------------

 * Oscillator features: 80%
 * Modulation & mixing modes: 75%
 * Filters: 100%
 * Wave folder: 100%
 * Envelopes: 100%
 * LFOs: 0%
 * MIDI Controller Customization: 0%
 * GUI: 30%
 * Effects: 33%
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

### Theory

 * [Wavetable Synthesis from McGill University](https://www.music.mcgill.ca/~gary/307/week4/wavetables.html)

 * [Bandlimited Synthesis of Classic Waveforms from McGill University](https://www.music.mcgill.ca/~gary/307/week5/bandlimited.html)

 * [W3C Web Audio API (2021. June 17)](https://www.w3.org/TR/2021/REC-webaudio-20210617/)

 * [W3C's adaptation of Audio-EQ-Cookbook.txt by Robert Bristow-Johnson](https://www.w3.org/TR/2021/NOTE-audio-eq-cookbook-20210608/)

 * [Amplitude Modulation from Sound on Sound](https://www.soundonsound.com/techniques/amplitude-modulation)

 * [An Introduction To Frequency Modulation from Sound on Sound](https://www.soundonsound.com/techniques/introduction-frequency-modulation)

 * [Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution
   by Julian D. Parker, Vadim Zavalishin, Efflam Le Bivic](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf)

 * [Freeverb (by Jezar at Dreampoint) explained in Physical Audio Signal Processing, by Julius O. Smith III](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html)

 * [Lock-Free Programming With Modern C++ by Timur Doumler](https://www.youtube.com/watch?v=qdrp6k4rcP4)

 * [Lagrange Interpolation with Equally-Spaced Nodes from NIST Digital Library of Mathematical Functions](https://dlmf.nist.gov/3.3#ii)

 * [Using Faster Exponential Approximation from CodingForSpeed.COM ](https://codingforspeed.com/using-faster-exponential-approximation/)

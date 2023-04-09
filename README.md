JS80P
=====

A MIDI driven, performance oriented, versatile [synthesizer VST plugin][plugin].
(Work in progress.)

  [plugin]: https://en.wikipedia.org/wiki/Virtual_Studio_Technology

> VST® is a trademark of Steinberg Media Technologies GmbH, registered in
> Europe and other countries.

<a name="toc"></a>

Table of Contents
-----------------

 * [Table of Contents](#toc)
 * [Development Progress](#prog)
    * [Future Plans](#future)
 * [Features](#features)
    * [Signal Chain (Simplified)](#signal)
 * [Name](#name)
 * [Development](#dev)
    * [Tools](#dev-tools)
    * [Dependencies](#dev-dep)
    * [Theory](#dev-theory)

<a name="prog"></a>

Development progress: 91.5%
---------------------------

 * Oscillator features: 100%
 * Modulation & mixing modes: 100%
 * Filters: 100%
 * Wave folder: 100%
 * Envelopes: 100%
 * LFOs: 100%
 * Flexible Controllers: 100%
 * GUI: 90%
 * Effects: 100%
 * Patch persistency: 100%
 * FST support: 100%
 * VST3 support: 100%
 * Presets: 0%

<a name="future"></a>

### Future Plans

 * Aftertouch support: 0%
 * LV2 support: 0%
 * Linux support: 0%

<a name="features"></a>

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
 * phase modulation
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

#### Aren't Phase Modulation and Frequency Modulation equivalent? Why have both?

The reason for JS80P having both kinds of modulation is that they are not
always equivalent. They are only identical when the modulator signal is a
sinusoid, but with each added harmonic, PM and FM start to differ more and
more. A detailed mathematical explanation of this is shown in
[pm-fm-equivalence.md](pm-fm-equivalence.md).

<a name="signal"></a>

### Signal Chain (Simplified)

                                                        (x16)
    Oscillator --> Filter --> Wavefolder --> Filter ---------> Mixer --+
                                                  |            ^       |
        (Frequency & Amplitude Modulation)        |            |       |
      +-------------------------------------------+            |       |
      |                                                        |       |
      v                                                 (x16)  |       |
      Oscillator --> Filter --> Wavefolder --> Filter ---------+       |
                                                                       |
    +------------------------------------------------------------------+
    |
    v
    Overdrive --> Distortion --> Filter --> Filter --> Echo --> Reverb --> Out

<a name="name"></a>

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

<a name="dev"></a>

Development
-----------

<a name="dev-tools"></a>

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

<a name="dev-dep"></a>

### Dependencies

 * [FST](https://git.iem.at/zmoelnig/FST)
 * [VST3 SDK](https://github.com/steinbergmedia/vst3sdk)

<a name="dev-theory"></a>

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

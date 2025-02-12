effConnectInput & effConnectOutput
==================================

# environment
- [pluginval-1.0.4](https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip)
- `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/3e5252f26be4ec68029f1cc5d4a7c191a3f98e13/src/FstPlugin)
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) sources

# walkthrough

Running our test plugin through `pluginval`, we notice that a couple of yet unknown are being triggered.
Notably, opcodes `31` and `32` are called repeatedly with a running index, that goes from 0..5,
but this is covered  [here](effConnectInput.md).

The other new opcode is `52`.
*pluginval* prints the following right after it issued it:
> Reported taillength: 0

So probably `effCode:52` is a way to query the "taillength" of a plugin
(there's an unknown opcode `effGetTailsize` that would fit neatly).

First, we check by returning some value other than *0*, when our plugin receives that opcode.
And indeed, if we return `4321`, the printout changes to:
> Reported taillength: 0.0979819

Now `0.0979819` just happens to be the length (in seconds) of 4321 samples at a samplerate of 44100 Hz.
*pluginval* uses `instance.getTailLengthSeconds()` to report the value,
and JUCE indeed uses `effGetTailSize` in the member function.

| opcode           | value |
|------------------|-------|
| `effGetTailSite` | 52    |

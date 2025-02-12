effGetNumMidiInputChannels & effGetNumMidiOutputChannels
========================================================

# environment
- `FstHost` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/1381a3f099df229e5c7a1ccf9061c02da54d6b9a/src/FstHost)
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) sources

## plugins
- [DecentSampler v1.12.8](https://www.decentsamples.com/product/decent-sampler-plugin/) (accessed 2025-02-18)

- [TAL-Vocoder v3.0.3.0](https://tal-software.com/downloads/plugins/TAL-Vocoder-2_64_linux.zip)
- many more plugins

# walkthrough

From the [JUCE sources](https://github.com/juce-framework/JUCE/blob/8.0.6/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST2.cpp)
we know, that JUCE plugins will typically respond to the
`effGetNumMidiInputChannels` & `effGetNumMidiOutputChannels` opcodes with the value `16`:
```C++
  //
  case Vst2::effGetNumMidiInputChannels:  return handleGetNumMidiInputChannels();
  case Vst2::effGetNumMidiOutputChannels: return handleGetNumMidiOutputChannels();
  //
}
pointer_sized_int handleGetNumMidiInputChannels()
{
    if (supportsMidiIn)
    {
       #ifdef JucePlugin_VSTNumMidiInputs
        return JucePlugin_VSTNumMidiInputs;
       #else
        return 16;
       #endif
    }
    return 0;
}
pointer_sized_int handleGetNumMidiOutputChannels()
{
    if (supportsMidiOut)
    {
       #ifdef JucePlugin_VSTNumMidiOutputs
        return JucePlugin_VSTNumMidiOutputs;
       #else
        return 16;
       #endif
    }
    return 0;
}
```

This should make it simple to find these opcodes, by just trying all (yet unknown) opcodes
and see, which one return `16`.

Unfortunately most plugins I have, never return `16`.
So I decided to just test all effCodes between 9..128 on all the plugins i found on my disk,
and see whether *any* of them returns `16`.

For such batch processing, keep in mind that some opcodes require pointers to valid structures,
so we have to skip a few (unless we want to populate the various structures, which I don't).
So let's try all opcodes *except* for these
- `effEditOpen` (14)
- `effProcessEvents` (25)
- `effSetSpeakerArrangement` (42)
- `effGetSpeakerArrangement` (69)

I found 42 free-as-in-beer, commercial plugins on my disk.
Most plugins would return `0` for any yet-unknown effCode we sent to it.
But very few do not:

| opcode | result | number of plugins |
|--------|--------|-------------------|
| 44     | 1      | 37                |
| 78     | 16     | 2                 |
| 79     | 16     | 1                 |

As we can see, there's a large number of plugins that respond with `1` to the `effCode:44`.
But more importantly, we found a few that respond with `16` to some opcode.
These are the `DecentSampler` (both `effCode:78` and `effCode:79`) and the `TAL-Vocoder` (`effCode:78`).

I'm pretty sure these to are the `eff[GS]etNumMidiInputChannels` opcodes,
but which is which?

The manual of the `TAL-Vocoder` only speaks of the capability to send MIDI date to the plugin,
so I assume that `effCode:78` is `effGetNumMidiInputChannels`.
(also, in other i/o opcode pairs, the input comes right before the output).


| opcode                        | value |
|-------------------------------|-------|
| `effGetNumMidiInputChannels`  | 78    |
| `effGetNumMidiOutputChannels` | 79    |


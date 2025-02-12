effGetMidiKeyName, struct MidiKeyName and kVstMaxNameLen
========================================================

# Part1 (2022-08-18)
## environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/d9393b06c62619c48abb84a893e8072b16a6100b/src/FstPlugin)
- [JUCE-7.0.2](https://github.com/juce-framework/JUCE/releases/tag/7.0.2)


## walkthrough

adding a MIDI-item in REAPER and pressing some keys
will send plenty of messages with effect opcode `66` to a plugin.
There's also the occasional opcode `62`:

~~~
FstClient::dispatcher(0x19b9250, 66, 0, 0, 0xeae040, 0.000000);
FstClient::dispatcher(0x19b9250, 62, 0, 0, 0x7ffe232a7660, 0.000000);
FstClient::dispatcher(0x19b9250, 66, 0, 0, 0xeae040, 0.000000);
FstClient::dispatcher(0x19b9250, 66, 0, 0, 0xeae040, 0.000000);
~~~

the index is the MIDI channel number (zero-based).

the pointer is an address to a memory region,
where the first 4 bytes are 0,
and the 2nd 4 bytes are an int32 between 34 and 72.

i have no idea what the first number is (it is always 0),
but the second number is a MIDI-note number.
Writing a string into the buffer right after the first 8 bytes (that hold the two numbers),
will make REAPER show this string as labels for the given MIDI-note on the
virtual MIDI keyboard.

Unfortunately we do not now the opcode name for this.
Given that there's a `effGetCurrentMidiProgram` opcode,
my guess would be something along the lines of `effGetMidiNoteName`.


# Part2 (2025-02-11)
## environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/bb3ec933fde0bd47fc62f19d2ffa807227d9755c/src/FstPlugin)
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6)

## walkthrough

trying to compile a VST2 plugin that uses JUCE-8.0.6 now starts throwing errors, due to a couple of
unknown VST2 related symbols:

- `effGetMidiKeyName` - an eff-opcode
- `MidiKeyName` - a type
- `kVstMaxNameLen` - a constant

These new symbols were introduced in [JUCE@96e4ba06](https://github.com/juce-framework/JUCE/commit/96e4ba06afa466227023bab075d7b696b2aa9bd0), leading up to JUCE-8.0.5.

This looks suspiciously like the opcode we just discovered, so we got a name:

| opcode              | value |
|---------------------|-------|
| `effGetMidiKeyName` | 66    |

JUCE's handler casts the pointer to `MidiKeyName*`, which has (at least) three members: `thisKeyNumber`, `thisProgramIndex` and `keyName`.
The latter is a string of size `kVstMaxNameLen` (where the key name should be written into).
The first is an integer, which is passed along as MIDI note number) along with the *index* (as MIDI channel number)
to JUCE's `AudioProcessor::getNameForMidiNoteNumber`.
The program index is an integer (as retrieved by `effGetProgram`). Most likely this is the value that is always 0.

So the `MidiKeyName` struct most likely looks like:
~~~C
typedef struct MidiKeyName_ {
  int thisProgramIndex;
  int thisKeyNumber;
  char keyName[kVstMaxNameLen];
} MidiKeyName;
~~~

We do not know the value of `kVstMaxNameLen`, it's probably in the range of 8 .. 128.
(Writing in a string of 512 chars makes REAPER crash; with 192 chars, valgrind shows an additional error (but cannot locate it)).
It's probably smaller though.
In any case, the string must be NULL-terminated (REAPER will happily display all characters).

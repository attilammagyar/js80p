effMainsChanged, audioMasterWantMidi and audioMasterGetCurrentProcessLevel
==========================================================================

# environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- `FstPlugin` as of today (2022-08-18)
- [JUCE-7.0.2](https://github.com/juce-framework/JUCE/releases/tag/7.0.2)

## plugins
- [Danaides](https://ineardisplay.com/plugins/legacy/) (accessed 2019-02-16; dead?)
- [Digits](http://www.extentofthejam.com/) (accessed 2019-02-16)
- [Hypercyclic](http://www.mucoder.net/en/hypercyclic/) (accessed 2019-02-16)

# audioMasterGetCurrentProcessLevel
REAPER calls `effcode:12` twice (with varying `ivalue`) whenever playback gets started:

~~~
FstClient::dispatcher(0x39166c0, 12, 0, 0, (nil), 0.000000);
FstClient::dispatcher(0x39166c0, 12, 0, 1, (nil), 0.000000);
~~~

Looking up [JUCE's effect opcodes](#juce-effect-opcodes), the following opcodes could be called:
- `effMainsChanged`
- `effSetBypass`
- `effSetProcessPrecision`

The `effSetBypass` can probably be ruled out, as the host wouldn't *enable* bypassing just before starting playback.


If we play back this sequence with effcode:12 to our test plugins,
most of them respond immediately by calling the host callback:


| plugin                | response to 12 |
|-----------------------|----------------|
| Digits                | 6              |
| BowEcho/Danaides      | 23+6           |
| hypercyclic/tonespace | 23+7+6         |

We already know that the `hostcode:7` (as called by hypercyclic/tonespace) is `audioMasterGetTime`.

When looking at how JUCE actually handles the two remaining effect opcodes, we see that `effSetProcessPrecision`
only sets some internal state, leaving little chance to call back to the host.

That leaves us with `effMainsChanged`: and indeed, JUCE potentially does callbacks to the host
when `resume()`ing operation, which is done when handling this opcode with `ivalue=1`
- it calls `audioMasterGetCurrentProcessLevel` without arguments (indirectly, via the `isProcessLevelOffline()` method)
- if the plugin somehow handles MIDI, it will also issue an `audioMasterWantMidi` call (with `ivalue=1`)

Checking how *BowEcho* actually calls the opcodes 23+6, we can log:

~~~
FstHost::dispatcher(23, 0, 0, (nil), 0.000000);
FstHost::dispatcher(7, 0, 1, (nil), 0.000000);
~~~

And *hypercyclic*:

~~~
FstHost::dispatcher(23, 0, 0, (nil), 0.000000);
FstHost::dispatcher(audioMasterGetTime, 0, 65024, (nil), 0.000000);
FstHost::dispatcher(7, 0, 1, (nil), 0.000000);
~~~

So the calling conventions kind of match the expectations we have for `audioMasterGetCurrentProcessLevel` and `audioMasterWantMidi`,
giving us three new opcodes:

| opcode                            | value |
|-----------------------------------|-------|
| effMainsChanged                   | 12    |
| audioMasterWantMidi               | 6     |
| audioMasterGetCurrentProcessLevel | 23    |

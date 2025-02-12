audioMasterUpdateDisplay
========================

# environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/d9393b06c62619c48abb84a893e8072b16a6100b/src/FstPlugin)
- [JUCE-7.0.2](https://github.com/juce-framework/JUCE/releases/tag/7.0.2)


# walkthrough

Around 2023-06, somebody complained that `audioMasterUpdateDisplay` is not implemented by FST,
and therefore some DAWs (like REAPER) do not update the display of any parameters that change "under the hood".

Checking the JUCE source code ("modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp"),
we see that the `audioMasterUpdateDisplay` opcode is used whenever
- a "parameter info" changes
- a "program change" occurs
(we also see that if there is a "latency change", JUCE is going to call the `audioMasterIOChanged` opcode).

So we need to find a host-opcode, that in turn triggers a re-query of the plugin's program and parameter info.

In JUCE, the `audioMasterUpdateDisplay` opcode is called with all parameters set to `0`:
~~~C++
constexpr FlagPair pairs[] { { Vst2::audioMasterUpdateDisplay, audioMasterUpdateDisplayBit },
                             { Vst2::audioMasterIOChanged,     audioMasterIOChangedBit } };

for (const auto& pair : pairs)
    if ((callbacksToFire & pair.bit) != 0)
    callback (&owner.vstEffect, pair.opcode, 0, 0, nullptr, 0);
~~~

So we hook something like the following in a callback (e.g. to `effEditClose`),
assuming that the opcode has an ID lower than 256 (which seems to be a safe assumption,
given that the highest audioMaster opcode we've seen so far is 44...):


~~~C++
for(size_t opcode=0; opcode<256; opcode++) {
    int res;
    if(hostKnown(opcode))
      continue;
    printf("test opcode(%d)", opcode);
    res =  dispatch_v(eff, opcode, 0, 0, NULL, 0.0);
    printf("test opcode(%d) -> %d\n", opcode, res);
}
~~~

Only two opcodes have any interesting results:

| opcode | return | side-effect                                   |
|--------|--------|-----------------------------------------------|
| 11     | 3      |                                               |
| 12     | 1      |                                               |
| 13     | 1      |                                               |
| 19     | 0x600  |                                               |
| 42     | 1      | calls `effGetProgram` and `effGetProgramName` |
| other  | 0      |                                               |

So, opcode `42` rescans the the current program and retrieves its name,
which closely matches our expectation of what `audioMasterUpdateDisplay`
is supposed to do ("triggers a re-query of the plugin's program [...] info.").

So for now, we assume that we have found a new audioMaster opcode:

| opcode                            | value |
|-----------------------------------|-------|
| `audioMasterUpdateDisplay`        | 42    |

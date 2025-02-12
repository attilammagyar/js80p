process levels
==============

# environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- `FstPlugin` as of today (2022-08-18)
- [JUCE-7.0.2](https://github.com/juce-framework/JUCE/releases/tag/7.0.2)
- [MrsWatson-0.9.7](http://static.teragonaudio.com/downloads/MrsWatsonMrsWatson.zip) (accessed 2024-02-17)


# walk through

So far we know that there is a host opcode `audioMasterGetCurrentProcessLevel`, which returns `kVstProcessLevelUnknown` in MrsWatson.
The presence of the host opcode and the generic name of the value returned by MrsWatson, suggest that there are other, more specific values to return.

After another round of googling for terms `vst` and `processlevel`, I eventually stumbled upon a [post in the "Deprecated REAPER issue tracker"](https://forums.cockos.com/project.php?issueid=3382) (accessed 2020-01-20):

> Offline Render causes glitching with some plugs (Reaper 3.76 sending kVstProcessLevelRealtime to AudioEffectX during Offline Render)
>
> First encountered using EW Spaces convolution reverb. The verb always glitches in offline render, and runs much faster than would be expected (i.e. 60x)
> In further investigation it seems that when Reaper is in offline render mode, for any plug-in that calls AudioEffectX::getCurrentProcessLevel Reaper 3.76 returns kVstProcessLevelRealtime instead of kVstProcessLevelOffline.

and further on:

> Just discovered this isn't a bug. There is a setting in Reaper to enable the sending of this report variable.
> In the Reaper Preferences under Plug-ins > VST there is a check-box for "inform plug-ins of offline rendering state" ... that fixes the problem.

This post tells us that there are at least two more process levels, named `kVstProcessLevelRealtime` and `kVstProcessLevelOffline`.
And that REAPER would report the former during offline rendering, but can be made into reporting the latter (by checking some option in the preferences).

Nice. Now we only need to ask the host for the current processLevel (calling `audioMasterGetCurrentProcessLevel` e.g. in the `processReplacing` callback) while telling REAPER to "Render to disk", and we get

| "Inform plug-ins of offline rendering..." | value | name                       |
|-------------------------------------------|-------|----------------------------|
| unchecked (OFF)                           | 2     | `kVstProcessLevelRealtime` |
| checked (ON)                              | 4     | `kVstProcessLevelOffline`  |

REAPER also reports a process-level of `1`, but only at the beginning (e.g. while the host calls the plugin with `effMainsChanged`).

| name                       | value | note                                        |
|----------------------------|-------|---------------------------------------------|
| ??                         | 0     | returned by JUCE is in realtime mode        |
| ??                         | 1     | returned by REAPER during `effMainsChanged` |
| `kVstProcessLevelRealtime` | 2     | returned by REAPER during normal rendering  |
| ??                         | 3     | (in between)                                 |
| `kVstProcessLevelOffline`  | 4     | returned by REAPER when offline-rendering; returned by JUCE if NOT in realtime mode |
| `kVstProcessLevelUnknown`  | ??    | used by MrsWatson                           |

The value of `kVstProcessLevelUnknown` is most likely something special, like *-1*, or (if we follow the schema used for `kPlugCategUnknown`) *0*.
I'm not sure, why JUCE would return `kPlugCategUnknown` for the realtime-case though.


data from JUCE
==============

# speaker and their arrangements

JUCE uses their own speaker arrangements,
but provides some mapping to/from the various audio plugin APIs (including VST2).
This is done in [modules/juce_audio_processors/format_types/juce_VSTCommon.h](https://github.com/juce-framework/JUCE/blob/8.0.6/modules/juce_audio_processors/format_types/juce_VSTCommon.h).

There are mappings from a `kSpeakerArr`angements to (JUCE) speakers (the last `unknown` speaker is obviously used to terminate the list):

```C++
{ Vst2::kSpeakerArr40Music, { left, right, leftSurround, rightSurround, unknown } },
```

or to channel sets:
```C++
else if (arr == Vst2::kSpeakerArr40Music) return AudioChannelSet::quadraphonic();
```

as well as mapping from JUCE speakers to VST speakers:
```C++
{ AudioChannelSet::leftSurround,      Vst2::kSpeakerLs },
```

## speakers

This is the mapping between JUCE and VST channels:

| JUCE                                 | VST            | code |
|--------------------------------------|----------------|------|
| `AudioChannelSet::left`              | `kSpeakerL`    | 1    |
| `AudioChannelSet::right`             | `kSpeakerR`    | 2    |
| `AudioChannelSet::centre`            | `kSpeakerC`    | 3    |
| `AudioChannelSet::LFE`               | `kSpeakerLfe`  | 4    |
| `AudioChannelSet::leftSurround`      | `kSpeakerLs`   | 5    |
| `AudioChannelSet::rightSurround`     | `kSpeakerRs`   | 6    |
| `AudioChannelSet::leftCentre`        | `kSpeakerLc`   |      |
| `AudioChannelSet::rightCentre`       | `kSpeakerRc`   |      |
| `AudioChannelSet::surround`          | `kSpeakerS`    |      |
| `AudioChannelSet::leftSurroundRear`  | `kSpeakerSl`   |      |
| `AudioChannelSet::rightSurroundRear` | `kSpeakerSr`   |      |
| `AudioChannelSet::topMiddle`         | `kSpeakerTm`   |      |
| `AudioChannelSet::topFrontLeft`      | `kSpeakerTfl`  |      |
| `AudioChannelSet::topFrontCentre`    | `kSpeakerTfc`  |      |
| `AudioChannelSet::topFrontRight`     | `kSpeakerTfr`  |      |
| `AudioChannelSet::topRearLeft`       | `kSpeakerTrl`  |      |
| `AudioChannelSet::topRearCentre`     | `kSpeakerTrc`  |      |
| `AudioChannelSet::topRearRight`      | `kSpeakerTrr`  |      |
| `AudioChannelSet::LFE2`              | `kSpeakerLfe2` |      |


# speaker arrangements


| arrangement                 | alternative name | speaker channels (omitting the `kSpeaker` prefix)   | channels |
|-----------------------------|------------------|-----------------------------------------------------|----------|
| `kSpeakerArrEmpty`          | disabled         |                                                     | 0        |
| `kSpeakerArrMono`           | mono             | C                                                   | 1        |
| `kSpeakerArrStereo`         | stereo           | L, R                                                | 2        |
| `kSpeakerArrStereoSurround` |                  | Ls, Rs                                              | 2        |
| `kSpeakerArrStereoCenter`   |                  | Lc, Rc                                              | 2        |
| `kSpeakerArrStereoSide`     |                  | Sl, Sr                                              | 2        |
| `kSpeakerArrStereoCLfe`     |                  | C, Lfe                                              | 2        |
| `kSpeakerArr30Cine`         | LCR              | L, R, C                                             | 3        |
| `kSpeakerArr30Music`        | LRS              | L, R, S                                             | 3        |
| `kSpeakerArr31Cine`         |                  | L, R, C, Lfe                                        | 4        |
| `kSpeakerArr31Music`        |                  | L, R, Lfe, S                                        | 4        |
| `kSpeakerArr40Cine`         | LCRS             | L, R, C, S                                          | 4        |
| `kSpeakerArr40Music`        | quadrophonic     | L, R, Ls, Rs                                        | 4        |
| `kSpeakerArr41Cine`         |                  | L, R, C, Lfe, S                                     | 5        |
| `kSpeakerArr41Music`        |                  | L, R, Lfe, Ls, Rs                                   | 5        |
| `kSpeakerArr50`             | 5.0              | L, R, C, Ls, Rs                                     | 5        |
| `kSpeakerArr51`             | 5.1              | L, R, C, Lfe, Ls, Rs                                | 6        |
| `kSpeakerArr60Cine`         | 6.0              | L, R, C, Ls, Rs, S                                  | 6        |
| `kSpeakerArr60Music`        | 6.0 music        | L, R, Ls, Rs, Sl, Sr                                | 6        |
| `kSpeakerArr61Cine`         | 6.1              | L, R, C, Lfe, Ls, Rs, S                             | 7        |
| `kSpeakerArr61Music`        | 6.1 music        | L, R, Lfe, Ls, Rs, Sl, Sr                           | 7        |
| `kSpeakerArr70Cine`         | 7.0 sdds         | L, R, C, Ls, Rs, Tfl, Tfr                           | 7        |
| `kSpeakerArr70Music`        | 7.0              | L, R, C, Ls, Rs, Sl, Sr                             | 7        |
| `kSpeakerArr71Cine`         | 7.1 sdds         | L, R, C, Lfe, Ls, Rs, Tfl, Tfr                      | 8        |
| `kSpeakerArr71Music`        | 7.1              | L, R, C, Lfe, Ls, Rs, Sl, Sr                        | 8        |
| `kSpeakerArr80Cine`         |                  | L, R, C, Ls, Rs, Tfl, Tfr, S                        | 8        |
| `kSpeakerArr80Music`        |                  | L, R, C, Ls, Rs, S, Sl, Sr                          | 8        |
| `kSpeakerArr81Cine`         |                  | L, R, C, Lfe, Ls, Rs, Tfl, Tfr, S                   | 9        |
| `kSpeakerArr81Music`        |                  | L, R, C, Lfe, Ls, Rs, S, Sl, Sr                     | 9        |
| `kSpeakerArr102`            |                  | L, R, C, Lfe, Ls, Rs, Tfl, Tfc, Tfr, Trl, Trr, Lfe2 | 12       |

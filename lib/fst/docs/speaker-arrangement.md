speaker arrangement
===================


# VstSpeakerArrangment

## environment
- [JUCE-5.4.1](https://github.com/juce-framework/JUCE/releases/tag/5.4.1) sources

## walkthrough
When doing an initial compilation of JUCE's *AudioPluginHost*, we get a number of errors
due to missing symbols.
One of them is a missing type named `VstSpeakerArrangment`.

Since `VstSpeakerArrangement` is supposed to be a type, let's naively set it to `int`.
This will yield errors about not being able to access members in a non-class type.

~~~bash
make -C JUCE/extras/AudioPluginHost/Builds/LinuxMakefile 2>&1 \
| grep VstSpeakerArrangement | grep "error:"                  \
| sed -e 's|.* error: request for member ||' -e 's| in .*||'  \
| sort -u
~~~

This gives us three members, `type`, `speakers`, `numChannels`.
To get the types of these members, let's set it experimentally to something stupid, like `void*`.
That reveleals that we need to be able to convert `type` to `int32` (but more likely some enumeration),
`numChannels` to `int`, and that `speakers` really is an array to `VstSpeakerProperties`.

Somewhere in `modules/juce_audio_processors/format_types/juce_VSTCommon.h`, we can see that
the `type` is populated by `int channelSetToVstArrangementType()` which really returns values like
`kSpeakerArrStereo`.

I'd rather assign the `kSpeakerArr*` enumeration a name `t_fstSpeakerArrangementType` and use that for `type`,
but alas, C++ does not allow use to implicitly cast `int` to `enum`, so we need to use `int` instead.

~~~C
typedef struct VstSpeakerArrangement_ {
  int type;
  int numChannels;
  VstSpeakerProperties speakers[];
} VstSpeakerArrangement;
~~~

For `VstSpeakerProperties` see [here](REVERSE_ENGINEERING.md#VstSpeakerProperties).


#### sidenote: varsized arrays
The `VstSpeakerArrangement.speakers` could be a pointer (`VstSpeakerProperties*`) or an array (`VstSpeakerProperties[]`).
Because they are often used almost synonymously, my first attempt used a pointer.
Much later, i tried compiling and *running* JUCE's AudioPluginHost and it would spectacularily segfault
when allocating memory for `VstSpeakerArrangement` and assigning values to it.
It turned out, that using a var-sized array instead of the pointer fixes this issue.

the `type var[]` syntax is C99, for older C-implementations use `type var[0]` instead.



# effGetSpeakerArrangement & effSetSpeakerArrangement


## environment
- [JUCE-5.4.1](https://github.com/juce-framework/JUCE/releases/tag/5.4.1) sources
- [REAPER-5.x](https://www.reaper.fm/download-old.php?ver=5x)
- `FstPlugin` as of today (unknown date)

### Plugins
all accessed before 2019-02-16
- [Danaides](https://ineardisplay.com/plugins/legacy/)
- [BowEcho](https://ineardisplay.com/plugins/legacy/)
- [Hypercyclic](http://www.mucoder.net/en/hypercyclic/)
- [Tonespace](http://www.mucoder.net/en/tonespace/)
- [U-he Protoverb](https://u-he.com/products/protoverb/)
- [Digits](http://www.extentofthejam.com/)
- [InstaLooper](https://www.audioblast.me/instalooper.html)

a number of these plugins are no longer available (or are now at different versions)


## walkthrough

Running our test plugin in REAPER, we can also see calls to `effcode:42` in the startup phase.

~~~
FstClient::dispatcher(0x1ec2080, 42, 0,  32252624, 0x1ec2740, 0.000000);
~~~

in another run this is:
~~~
FstClient::dispatcher(0x9e36510, 42, 0, 172519840, 0xa487610, 0.000000);
~~~

The `ivalue` is a bit strange, unless it is printed in hex (`0x1ec22d0` resp . `0xa4871a0`),
where it becomes apparent that this is really another address (just compare the hex representation
with the `ptr` value; the difference is 1136, which is practically nothing in address space)!

According to [JUCE](#juce-effect-opcodes), there are only very few opcodes
where both `ivalue` and `ptr` point both to addresses:
- `effGetSpeakerArrangement`
- `effSetSpeakerArrangement`

Both opcodes get pointers to `VstSpeakerArrangement`.

Here is a dump of the first 96 bytes of the data found at those addresses
(the data is the same on both addresses,
at least if our plugin is configured with 2 IN and 2 OUT channels):
~~~
01 00 00 00 02 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00
~~~

The int32 at position @4-7 is the `numChannels`,
the int32 at position @0-3 is most likely the `type`.
I have no explanation for the value at position @58,
it's probably just uninitialized data.

By setting the `numChannels` of the plugin, REAPER responds
with following types

| numChannels  | type | type(hex)  |
|--------------|------|------------|
| 1            | 0    | 0x0        |
| 2            | 1    | 0x1        |
| 3            | 11   | 0xb        |
| 4            | 11   | 0xb        |
| 5            | 15   | 0xf        |
| 6            | 15   | 0xf        |
| 7            | 23   | 0x17       |
| 8            | 23   | 0x17       |
| 12           | 28   | 0x1C       |
| all the rest | -2   | 0xFFFFFFFE |

Since the values are filled in by REAPER, it's likely that `effcode:42` is `effSetSpeakerArrangement`.
This is somewhat confirmed by *Protoverb*, that prints out

>  resulting in SpeakerArrangement $numIns - $numOuts

with $numIns and $numOuts replaced by 0, 1 or 2, depending on the chosen speaker arrangement.
It seems that *Protoverb* doesn't support more than 2 channels.


`effGetSpeakerArrangement` is most likely close by (`41` or `43`),
A simple test would first set the speaker-arrangement, and then try to query it back.
According to JUCE, the plugin should return `1` in case of success.
The calling convention is slightly different from `effSetSpeakerArrangement`, as `ptr` and `ivalue`
hold addresses of pointer-sized memory regions, where the plugin is supposed to write
the addresses of the `VstSpeakerArrangement` structs to (cf. JUCE code).

~~~C
for(size_t opcode=40; opcode<45; opcode++) {
  VstSpeakerArrangement *arrptr[2] = {0,0};
  if(42 == opcode)continue;
  dispatch_v(effect, opcode, 0, (VstIntPtr)(arrptr+0), (arrptr+1), 0.f);
  print_hex(arrptr[0], 32);
  print_hex(arrptr[1], 32);
}
~~~

Unfortunately, this is not very successful.
Only `opcode:44` returns 1 for *some* plugins, but none write data into our `VstSpeakerArrangement` struct.

| plugin    | opcode | result |
|-----------|--------|--------|
|Danaides   | 44     | 1      |
|BowEcho    | 44     | 1      |
|hypercyclic| 44     | 1      |
|tonespace  | 44     | 1      |
|Protoverb  | *      | 0      |
|Digits     | *      | 0      |
|InstaLooper| *      | 0      |


If we try a bigger range of opcodes (e.g. 0..80), we need to skip the opcodes 45, 47 and 48 (`effGet*String`)
to prevent crashes.

Interestingly, *Protoverb* will now react to `opcode:69`, returning the same data we just set via opcode:42.
So we probably have just found `effGetSpeakerArrangement` as well.



# Speaker Arrangement Types

## environment
- [JUCE-5.4.1](https://github.com/juce-framework/JUCE/releases/tag/5.4.1) sources
- [REAPER-5.x](https://www.reaper.fm/download-old.php?ver=5x)
- `FstPlugin` as of today (unknown date)

## walkthrough

So what do we know about the `VstSpeakerArrangement.type` field?
In [VstSpeakerArrangement](#vstspeakerarrangement), we found out
that this member is really of type `t_fstSpeakerArrangementType`,
that has (so far) symbolic names starting with `kSpeakerArr*`.

JUCE uses the following names matching this pattern (in a regex notation):

| VST name                             | JUCE name    |
|--------------------------------------|--------------|
| `kSpeakerArrEmpty`                   | disabled     |
| `kSpeakerArrMono`                    | mono         |
| `kSpeakerArrStereo`                  | stereo       |
| `kSpeakerArrStereoCLfe`              |              |
| `kSpeakerArrStereoCenter`            |              |
| `kSpeakerArrStereoSide`              |              |
| `kSpeakerArrStereoSurround`          |              |
| `kSpeakerArrUserDefined`             |              |
| `kSpeakerArr[34678][01](Cine,Music)` |              |
| `kSpeakerArr30Cine`                  | LCR          |
| `kSpeakerArr30Music`                 | LRS          |
| `kSpeakerArr40Cine`                  | LCRS         |
| `kSpeakerArr60Cine`                  | 6.0          |
| `kSpeakerArr61Cine`                  | 6.1          |
| `kSpeakerArr60Music`                 | 6.0Music     |
| `kSpeakerArr61Music`                 | 6.1Music     |
| `kSpeakerArr70Music`                 | 7.0          |
| `kSpeakerArr70Cine`                  | 7.0SDDS      |
| `kSpeakerArr71Music`                 | 7.1          |
| `kSpeakerArr71Cine`                  | 7.1SDDS      |
| `kSpeakerArr40Music`                 | quadraphonic |
| `kSpeakerArr50`                      | 5.0          |
| `kSpeakerArr51`                      | 5.1          |
| `kSpeakerArr102`                     |              |


Comparing these names to what REAPER fills in for various channel counts (see above),
we can at least make some simple guesses.
For convenience, we repeat the table from above:

| numChannels  | type |
|--------------|------|
| 1            | 0    |
| 2            | 1    |
| 3            | 11   |
| 4            | 11   |
| 5            | 15   |
| 6            | 15   |
| 7            | 23   |
| 8            | 23   |
| 12           | 28   |
| all the rest | -2   |

Mono is a single channel, Stereo needs two channels, for which REAPER fills in `0` and `1` (`kSpeakerArrMono` resp `kSpeakerArrStereo`).
That's a bit unconventional: personally I would have used `0` for `kSpeakerArrEmpty`, and `1` and `2` for Mono and Stereo.

Unfortunately, REAPER doesn't report anything for 0 channels, so we don't know that value of `kSpeakerArrEmpty`.
I also don't really understand why we always get pairwise assignments for the next 6 number of channels.
E.g. assigning `11` to both 3 and 4 channels would make sense
if there was a speaker-arrangement for 4 channels (e.g. quadrophony aka `kSpeakerArr40Music`),
but none for 3 channels (so REAPER just assumes some "degraded" arrangement).
But we do have `kSpeakerArr30Music`, which should serve 3 channels just fine.
It's probably safe to assume that REAPER does the "degraded" arrangement thing nevertheless.
It's tricky to get the correct assignment though, since we have so many variants with the same
number of channels.
E.g. 6 channels could be `kSpeakerArr51`, `kSpeakerArr60Music` and `kSpeakerArr60Cine`.

There's also the catch-all type *-2*. Given the `kSpeakerArr*` names we know so far, `kSpeakerArrUserDefined` might be this catch-all.
The value for `kSpeakerArrEmpty` might be "special" as well (and therefore not be assigned some ordered value like, say, *7*) -
it could well be *-1*, but we don't have any evidence of that yet.

Here's some possible assignments (the names for 3, 5 & 7 channels are in parentheses, as the type has the same value as arrangement with one more channel):

| numChannels  | type (value) | type (name)                                                                            |
|--------------|--------------|----------------------------------------------------------------------------------------|
| 1            | 0            | `kSpeakerArrMono`                                                                      |
| 2            | 1            | `kSpeakerArrStereo`                                                                    |
| 3            | 11           | (`kSpeakerArr30Music`, `kSpeakerArr30Cine`)                                            |
| 4            | 11           | `kSpeakerArr31Music`, `kSpeakerArr31Cine`, `kSpeakerArr40Music`, `kSpeakerArr40Cine`   |
| 5            | 15           | (`kSpeakerArr41Music`, `kSpeakerArr41Cine`, `kSpeakerArr50Music`)                      |
| 6            | 15           | `kSpeakerArr51`, `kSpeakerArr60Music`, `kSpeakerArr60Cine`                             |
| 7            | 23           | (`kSpeakerArr61Music`, `kSpeakerArr61Cine`, `kSpeakerArr70Music`, `kSpeakerArr70Cine`) |
| 8            | 23           | `kSpeakerArr71Music`, `kSpeakerArr71Cine`, `kSpeakerArr80Music`, `kSpeakerArr80Cine`   |
| 12           | 28           | `kSpeakerArr102`                                                                       |
| all the rest | -2           | `kSpeakerArrUserDefined`                                                               |

So we can at least be pretty confident of the values of `kSpeakerArrMono`, `kSpeakerArrStereo`,
`kSpeakerArr102` & `kSpeakerArrUserDefined`.


### Amendment: wantsChannelCountNotifications

REAPER doesn't *always* call the `effSetSpeakerArrangement`.
It seems the plugin must return `1` for the `effCanDo` opcode with a value of `wantsChannelCountNotifications`
in order to receive this opcode (which kind of makes sense).


# Speaker Arrangement revisited
## environment
- [REAPER](https://www.reaper.fm/) (unknown version; probably 5.x)
- [MrsWatson-0.9.7](http://static.teragonaudio.com/downloads/MrsWatsonMrsWatson.zip) (accessed 2024-02-17)
- `FstPlugin` as of today (unknown date)

## walkthrough
We still haven't found out the details of the speaker arrangement structs.

So far, we know the `effGetSpeakerArrangement` and `effSetSpeakerArrangement` opcodes, which return (resp. take)
pointers to the `VstSpeakerArrangement`, which in turn includes an array of type `VstSpeakerProperties`.

We currently only know of a single member (`type`) of the `VstSpeakerProperties` struct, which is certainly wrong
(a struct only makes sense if there are more members. creating an array of this struct requires it to have a
fixed size, so the struct cannot be extended "later".)

We already know the positions of `VstSpeakerArrangement.type` resp. `.numChannels`, but don't really know whether
there are more members in this struct (so we don't know the exact position of the `.speakers` member).
However, we do now that the `.speakers` member contains `.numChannels` instances of `VstSpeakerProperties`.

~~~C
typedef struct VstSpeakerProperties_ {
  /* misses members; order undefined */
  int type;
} VstSpeakerProperties;

typedef struct VstSpeakerArrangement_ {
  int type;
  int numChannels;
  /* missing members? */
  VstSpeakerProperties speakers[];
} VstSpeakerArrangement;
~~~

When we [first discovered](#speaker-arrangement) some details of the Speaker Arrangement,
we noticed non-null values a position @58, which we concluded might be uninitialized data.

We can easily test this assumption: since `VstSpeakerProperties` is at least 4 bytes large
(assuming that it's `type` member is a 32bit `int` like all other types we have seen so far),
and REAPER can handle up to 64 channels, we can force the full size of `speakers[]` to 256 bytes
(64 * 4), which is way beyond the 88 bytes of position @58.

Printing the first 512 bytes a 64channel plugin receives with the `effSetSpeakerArrangement` opcode, gives:

~~~
0000	  FE FF FF FF 40 00 00 00  00 00 00 00 00 00 00 00
0010	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0020	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0030	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0040	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0050	  00 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00
0060	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0070	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0080	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0090	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00a0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00b0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00c0	  00 00 00 00 00 00 00 00  02 00 00 00 00 00 00 00
00d0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00e0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00f0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0100	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0110	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0120	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0130	  00 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00
0140	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0150	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0160	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0170	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0180	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0190	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01a0	  00 00 00 00 00 00 00 00  02 00 00 00 00 00 00 00
01b0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01c0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01d0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01e0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01f0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
~~~

This is interesting as we still have a value (`01`) at position @58 -
which according to the math we did above cannot be "uninitialized memory".

If we set the number of channels our plugin can process to *2*, we get the following instead:

~~~
0000	  01 00 00 00 02 00 00 00  00 00 00 00 00 00 00 00
0010	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0020	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0030	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0040	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0050	  00 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00
0060	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0070	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0080	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0090	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00a0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00b0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00c0	  00 00 00 00 00 00 00 00  02 00 00 00 00 00 00 00
00d0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00e0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00f0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0100	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0110	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0120	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0130	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0140	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0150	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0160	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0170	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0180	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
0190	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01a0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01b0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01c0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01d0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01e0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
01f0	  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
~~~

Apart from the differing first 8 bytes (that's the `.type` and `.numChannels`
members, which we already established to be differing), we can *also* see some
pattern:

The 2-channel plugin gets values `01` and `02` at the positions @58 resp @c8 and after that only zeros,
whereas the 64-channel version has alternating `01` and `02` values until the printout ends.
Could it be that we are actually seeing `VstSpeakerProperties` that are not occupying only 4 bytes but rather
112 bytes in length (112 being the difference between @c8 and @58)?

So printing 8192 bytes (which should cover 64 channels if each really takes 112 bytes), we observe:

| channels | 112-bytes pattern                                         | garbage     |
|----------|-----------------------------------------------------------|-------------|
| 2        | `01`@58, `02`@c8                                          | after @468  |
| 64       | alternating `01`/`02` from @58 to @1be8, every 0x70 bytes | after @1f88 |
| 3        | `01`@58, `02`@c8, `03`@138                                | after @4da  |

After a certain point the data is densely filled with non-null bytes, which probably really is "uninitialized memory" (aka "garbage").

The important part to notice is that the position difference between the first lonely byte @58 and the last one (@c8, @138, @1be8)
is always `112 * (numChannels-1)`.
So we can conclude, that the `VstSpeakerProperties` has a size of 112 bytes (of which we know that 4 bytes contain the `type` member).
If we assume that the `VstSpeakerArrangment.speakers` array starts immediately after the first 8 bytes,
the `VstSpeakerProperties.type` cannot be at the beginning or the end of the `VstSpeakerProperties`,
but is located at some 80 bytes offset.
There's no real evidence for *any* position of the `type` field.
Since the surrounding bytes don't carry information (at least when filled in by REAPER), we just pick one randomly.
There's also no real evidence whether the stray non-null bytes actually *are* the `type` member,
but since this is the only member of which we know and the only bytes that carry some information, the guess is probably correct.


~~~C
typedef struct VstSpeakerProperties_ {
  /* misses members; order undefined */
  char _padding1[80];
  int type;
  char _padding2[28];
} VstSpeakerProperties;

typedef struct VstSpeakerArrangement_ {
  int type;
  int numChannels;
  /* missing members? */
  VstSpeakerProperties speakers[];
} VstSpeakerArrangement;
~~~

Creating 65 plugins with varying channel count (from 0 to 64) and printing the `VstSpeakerProperties.type` for each channel,
we get:

| `VstSpeakerArrangement.numChannels` | `VstSpeakerArrangement.type` | `VstSpeakerProperties.type` |
|-------------------------------------|------------------------------|-----------------------------|
| 00                                  | -1                           | -                           |
| 01                                  | 0                            | 0                           |
| 02                                  | 1                            | 1,2                         |
| 03                                  | 11                           | 1,2,3                       |
| 04                                  | 11                           | 1,2,1,2                     |
| 05                                  | 15                           | 1,2,1,2,3                   |
| 06                                  | 15                           | 1,2,1,2,1,2                 |
| 07                                  | 23                           | 1,2,1,2,1,2,3               |
| 08                                  | 23                           | 1,2,1,2,1,2,1,2             |
| 09                                  | -2                           | 1,2,1,2,1,2,1,2,3           |
| 10                                  | -2                           | 1,2,1,2,1,2,1,2,1,2         |
| 11                                  | -2                           | 1,2,1,2,1,2,1,2,1,2,3       |
| 12                                  | 28                           | 1,2,1,2,1,2,1,2,1,2,1,2     |
| odd                                 | -2                           | 1,2,...,1,2,3               |
| even                                | -2                           | 1,2,...,1,2,1,2             |

The 108 unknown bytes in each `VstSpeakerProperties` struct are always `0x00`.
So the `VstSpeakerProperties.type` is always pairs of `1` and `2`,
and if the number of speakers is odd, the leftover (last) speaker has a type `3`.

JUCE (juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp) assigns values like
`kSpeakerL`, `kSpeakerR` or `kSpeakerLfe` to the `VstSpeakerProperties.type` member.
Obviously, REAPER doesn't make full use of these values (at least not in my configuration).
The values `1` and `2` are probably `kSpeakerL` resp. `kSpeakerR`.
The value `3` could be `kSpeakerC`, `kSpeakerLfe` or `kSpeakerS`, but it's hard to say.
The value `0` (only seen with the mono setup, tentatively `kSpeakerArrMono`) would be some mono channel,
either `kSpeakerC`, `kSpeakerUndefined` or - following the naming pattern of the Left/Right channels and
[confirmed to exist by some random googling](http://convolver.sourceforge.net/vst.html) -
`kSpeakerM`.

As a sidenote, we also see now that a plugin without channels has a `VstSpeakerArrangement.type` of *-1*,
which is most likely `kSpeakerArrEmpty` (something we already guessed above, but had no backing yet).

| name               | value |
|--------------------|-------|
| `kSpeakerArrEmpty` | -1    |
| `kSpeakerL`        | 1     |
| `kSpeakerR`        | 2     |

## enter MrsWatson
After trying to [compile MrsWatson](REVERSE_ENGINEERING.md#compiling-mrswatson), it becomes a bit clearer, why there's quite an offset
at the beginning of `VstSpeakerProperties`, as there are some additional members.

*MrsWatson* assigns to the properties like so:

~~~C
speakerArrangement->speakers[i].azimuth = 0.0f;
speakerArrangement->speakers[i].elevation = 0.0f;
speakerArrangement->speakers[i].radius = 0.0f;
speakerArrangement->speakers[i].reserved = 0.0f;
speakerArrangement->speakers[i].name[0] = '\0';
speakerArrangement->speakers[i].type = kSpeakerUndefined;
~~~

Assuming that the order of members follows the struct definition,
and that the first four values are indeed single-precision (since the only
reason why *MrsWatson* would use the `f` specifier is to avoid compiler warnings),
we get something like:

~~~C
typedef struct VstSpeakerProperties_ {
  float azimuth;
  float elevation;
  float radius;
  float reserved;
  char name[64];
  int type;
  char _padding2[28];
} VstSpeakerProperties;
~~~



# Speaker Arrangement - part3

## environment
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) sources
- [pluginval v1.0.4](https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip)
- `FstPlugin` as of today (2025-02-12)


## walkthrough

Unlike REAPER, `pluginval` sends us both `effGetSpeakerArrangement` and `effSetSpeakerArrangement` opcodes.

```
FST::SpeakerArrangement[input>] @ 0x5608da07d550: kSpeakerArrStereo[1] 
FST::   #0: kSpeakerL[1] 
FST::   #1: kSpeakerR[2] 
FST::SpeakerArrangement[output>] @ 0x5608da075eb0: 23 
FST::   #0: kSpeakerL[1] 
FST::   #1: kSpeakerR[2] 
FST::   #2: 3 [0x3] 
FST::   #3: 4 [0x4] 
FST::   #4: 0 [0x0] 
FST::   #5: 0 [0x0] 
FST::   #6: 10 [0xA] 
FST::   #7: 11 [0xB]
```
We can use this behaviour to query various speaker layouts,
by just assuming different number of input or output channels
and see which speakerlayout he plugin validator suggests for them.

Apart from `kSpeakerArrEmpty` (for zero channels),
and `kSpeakerArrStereo` (for 2 channels which are of of type `kSpeakerL` and `kSpeakerR` - as expected),
we get `kSpeakerArrMono` (1 channel of type `3`) and some unknown layouts:

| numChannels | layout | channel types (ordered)                                  |
|-------------|--------|----------------------------------------------------------|
| 3           | 6      | `kSpeakerL`, `kSpeakerR`, `3`                            |
| 4           | 11     | `kSpeakerL`, `kSpeakerR`, `5`, `6`                       |
| 5           | 14     | `kSpeakerL`, `kSpeakerR`, `3`, `5`, `6`                  |
| 6           | 15     | `kSpeakerL`, `kSpeakerR`, `3`, `4`, `5`, `6`             |
| 7           | 21     | `kSpeakerL`, `kSpeakerR`, `3`, `0`, `0`, `10`, `11`      |
| 8           | 23     | `kSpeakerL`, `kSpeakerR`, `3`, `4`, `0`, `0`, `10`, `11` |

The vast majority of layouts is, however, of type `-2` (aka `kSpeakerArrUserDefined`),
where all channels are of type `0`.

Let's recap [what REAPER fills in for various channel counts](#speaker-arrangements):

| numChannels  | REAPER | pluginval |
|--------------|--------|-----------|
| 3            |   11   |    6      |
| 4            | **11** | **11**    |
| 5            |   15   |   14      |
| 6            | **15** | **15**    |
| 7            |   23   |   21      |
| 8            | **23** | **23**    |

And which speaker types we do know about:

According to the [JUCE speaker arrangements](JUCE.md#speaker-arrangements),
there are only two "relevant" 3-channel layouts ("relevant" meaning, that JUCE cares to support it)
| layout               | speakers |
|----------------------|----------|
| `kSpeakerArr30Cine`  | L,R,C    |
| `kSpeakerArr30Music` | L,R,S    |


So `speaker:3` can only by `kSpeakerC` resp. `kSpeakerS`.

otoh, the three (relevant) 5-channel layouts are:

| layout               | speakers          |
|----------------------|-------------------|
| `kSpeakerArr41Cine`  | L, R, C  , Lfe, S |
| `kSpeakerArr41Music` | L, R, Lfe, Ls, Rs |
| `kSpeakerArr50`      | L, R, C  , Ls, Rs |

So `speaker:3` can only by `kSpeakerC` resp. `kSpeakerLfe`.

Combining the 3-channel layouts with the 5-channel layouts we can be pretty sure that `speaker:3` is indeed `kSpeakerC`.
This also means that the `kSpeakerArrMono` uses a single `kSpeakerC`,
and the 3-channel layout `speakerArr:6` is really `kSpeakerArr30Cine`.

According to the 5-channel layouts, `speaker:5` and `speaker:6` could be either `Lfe+S` or `Ls+Rs`,
which can be satisfied by `kSpeakerArr31Music` resp. `kSpeakerArr40Music`.
However, JUCE does not know about any 6-channel layout that uses `Lfe+S` as the last two channels,
only `Ls+Rs` or `Rs+S` are allowed.
So let's settle `speaker:5` and `speaker:6` as `kSpeakerLs` resp `kSpeakerRs`,
by which we learn that `speakerArr:11` must be `kSpeakerArr40Music`,
and `speakerArr:14` is `kSpeakerArr50`.
Furthermore, the only valid 6-channel layout is now `kSpeakerArr51`
(and admittedly, 5.1 is a more popular format than 6.0),
which also settles `speaker:4` to `kSpeakerLfe`.

According to JUCE, `speaker:10` and `speaker:11` could either be `Sl+Sr` (left/right surround rear) or `Tfl+Tfr` (top front left/right).
Interestingly enough both the 7-channel and the 8-channel layouts of JUCE have `Ls+Rs` channels at places
where pluginval uses a `speaker:0`.


So by now we know the following speakers:

| speaker       | code |
|---------------|------|
| `kSpeakerL`   | 1    |
| `kSpeakerR`   | 2    |
| `kSpeakerC`   | 3    |
| `kSpeakerLfe` | 4    |
| `kSpeakerLs`  | 5    |
| `kSpeakerRs`  | 6    |


We also note that when JUCE creates its mapping between JUCE channels and VST channels,
it uses exactly the ordering we have discovered so far.
If this ordering is influenced by the Vst channels, we could add some more speaker channels:

| speaker        | code |
|----------------|------|
| `kSpeakerLc`   | 7 ?  |
| `kSpeakerRc`   | 8 ?  |
| `kSpeakerS`    | 9 ?  |
| `kSpeakerSl`   | 10 ? |
| `kSpeakerSr`   | 11 ? |
| `kSpeakerTm`   | 12 ? |
| `kSpeakerTfl`  | 13 ? |
| `kSpeakerTfc`  | 14 ? |
| `kSpeakerTfr`  | 15 ? |
| `kSpeakerTrl`  | 16 ? |
| `kSpeakerTrc`  | 17 ? |
| `kSpeakerTrr`  | 18 ? |
| `kSpeakerLfe2` | 19 ? |

In this case, `speaker:10` and `speaker:11` map to `Sl+Sr` which would fit our observations above.
Note however, that the order might as well come from JUCEs own channel naming (and the two might differ).




We also know the following speaker arrangements:

| layout                   | code   | channel types (ordered)                                                            |
|--------------------------|--------|------------------------------------------------------------------------------------|
| `kSpeakerArrUserDefined` | -2     | `kSpeakerM`,...                                                                    |
| `kSpeakerArrEmpty`       | -1     | (none)                                                                             |
| `kSpeakerArrMono`        | 0      | `kSpeakerC` (pluginval) or `kSpeakerM` (REAPER)                                    |
| `kSpeakerArrStereo`      | 1      | `kSpeakerL`, `kSpeakerR`                                                           |
| `kSpeakerArr30Cine`      | 6      | `kSpeakerL`, `kSpeakerR`, `kSpeakerC`                                              |
| `kSpeakerArr40Music`     | 11     | `kSpeakerL`, `kSpeakerR`, `kSpeakerLs`, `kSpeakerLr`                               |
| `kSpeakerArr50`          | 14     | `kSpeakerL`, `kSpeakerR`, `kSpeakerC`, `kSpeakerLs`, `kSpeakerLr`                  |
| `kSpeakerArr51`          | 15     | `kSpeakerL`, `kSpeakerR`, `kSpeakerC`, `kSpeakerLfe`, `kSpeakerLs`, `kSpeakerLr`   |
|                          | **21** | `kSpeakerL`, `kSpeakerR`, `kSpeakerC`, **0**, **0**, **10**, **11**                |
|                          | **23** | `kSpeakerL`, `kSpeakerR`, `kSpeakerC`, `kSpeakerLfe`, **0**, **0**, **10**, **11** |
| `kSpeakerArr102`         | **28** |                                                                                    |


Again, if JUCE kept the order of arrangements (for which there is no evidence;
however, there's a strong smell, as the already known arrangements fit perfectly in - and this time there are large gaps between their codes)
we can conjecture some more:

| layout                      | code |
|-----------------------------|------|
| `kSpeakerArrStereoSurround` | 2 ?  |
| `kSpeakerArrStereoCenter`   | 3 ?  |
| `kSpeakerArrStereoSide`     | 4 ?  |
| `kSpeakerArrStereoCLfe`     | 5 ?  |
| `kSpeakerArr30Music`        | 7 ?  |
| `kSpeakerArr31Cine`         | 8 ?  |
| `kSpeakerArr31Music`        | 9 ?  |
| `kSpeakerArr40Cine`         | 10 ? |
| `kSpeakerArr41Cine`         | 12 ? |
| `kSpeakerArr41Music`        | 13 ? |
| `kSpeakerArr60Cine`         | 16 ? |
| `kSpeakerArr60Music`        | 17 ? |
| `kSpeakerArr61Cine`         | 18 ? |
| `kSpeakerArr61Music`        | 19 ? |
| `kSpeakerArr70Cine`         | 20 ? |
| `kSpeakerArr70Music`        | 21 ? |
| `kSpeakerArr71Cine`         | 22 ? |
| `kSpeakerArr71Music`        | 23 ? |
| `kSpeakerArr80Cine`         | 24 ? |
| `kSpeakerArr80Music`        | 25 ? |
| `kSpeakerArr81Cine`         | 26 ? |
| `kSpeakerArr81Music`        | 27 ? |

effEditIdle
===========

# environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- `FstPlugin` as of today (2022-08-18)
- [JUCE-7.0.2](https://github.com/juce-framework/JUCE/releases/tag/7.0.2)


# walkthrough

JUCE-7.0.2 now issues `effEditIdle` in the host's `VSTPluginInstance::handleIdle()`
(at least if the plugin is currently being shown).
On Linux, JUCE-7.0.2 plugins react on the `effEditIdle` by processing any pending events.

These pending events appear to be responsible for updating the GUI,
at least the GUI now shows nothing if I compile a JUCE-plugin (e.g. the
*ArpeggiatorTutorial* plugin from the JUCE Plugin examples)
and load it into REAPER:
"nothing" being a rectangle that is not being updated at all.
I'm sure it did show something in older versions of JUCE.

As we want our GUI to be updated continuously, we need to check for a (yet unknown) opcode
that is sent periodically to our plugin.
With REAPER, this gives us two potential candidates: `opcode:19` and `opcode:53`.
Both are called about every 50ms with only `0` as arguments.

We also notice that both are only called when we open the *FX* window in REAPER.
However, while `opcode:53` is always polled as soon as the *FX* window is open,
`opcode:19` is only polled if we enable the built-in GUI.
As soon as we switch to the generic GUI, only `opcode:53` remains.

It's hard to tell which one we should pick, but
*19* is close to the range of already known `effEdit*` opcodes (e.g. `effEditClose` is 15),
whereas *53* is somewhere near `effCanDo`.

So let's go for:

| opcode        |    |
|---------------|----|
| `effEditIdle` | 19 |

Maybe `opcode:53` is for `effIdle` (as it also gets continuously called),
but why does it only get polled if the *FX* window is open?

NOTE: while this makes the *ArpeggiatorTutorial* and friends work,
they still crash when opening a second JUCE plugin. hmmm...

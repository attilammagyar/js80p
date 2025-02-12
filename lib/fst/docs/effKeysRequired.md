effKeysRequired
===============

# environment
- [pluginval-1.0.4](https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip)
- [Tonespace-v2.9](https://www.mucoder.net/en/tonespace/v0209/download/tonespace-2.9.20220726.1-linux.zip)
- `FstProxy` and `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/33b0be39efa6c36f593b94abf65af16eee63b561/src)
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) sources

# walkthrough

running the *tonespace* plugin via our FstProxy through *pluginval*, we see
~~~
canDo: 'supportsViewDpiScaling' -> 1
host2plugin(&eff, effEditGetRect     , 0, 0, 0x7ffd146182f8, 0.f) -> ERect(...)
host2plugin(&eff, effEditOpen        , 0, 0, 0x3e00004, 0.f) -> 1
host2plugin(&eff, effEditGetRect     , 0, 0, 0x7ffd146182f8, 0.f) -> ERect(...)
host2plugin(&eff, effGetProgram      , 0, 0, (nil), 0.f) -> 93
host2plugin(&eff, 57                 , 0, 0, (nil), 0.f) -> 0
host2plugin(&eff, effGetProductString, 0, 0, 0x7ffd14618310, 0.f) -> 1 ('tonespace')
host2plugin(&eff, effEditClose       , 0, 0, (nil), 0.f) -> 0
~~~

First, the plugin is queried whether it can do `supportsViewDpiScaling`, which our simple test plugin never gets.
Then, the plugin receives an `effCode:57`, but again our test plugin never got this.
So: are the two related? And how can we make make our test plugin so it receives both?

Running the plugin validator with `--skip-gui-tests` on the *tonespace*,
we neither see the `supportsViewDpiScaling` not `effCode:57`.

Checking the JUCE code for the `supportsViewDpiScaling` string, we [find](https://github.com/juce-framework/JUCE/blob/51a8a6d7aeae7326956d747737ccf1575e61e209/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp#L3146-L3163):
~~~C++
pluginRespondsToDPIChanges = plugin.pluginCanDo ("supportsViewDpiScaling") > 0;

setContentScaleFactor();
Vst2::ERect* rect = nullptr;
dispatch (Vst2::effEditGetRect, 0, 0, &rect, 0);
auto* handle = getWindowHandle();
dispatch (Vst2::effEditOpen, 0, 0, handle, 0);
dispatch (Vst2::effEditGetRect, 0, 0, &rect, 0);
dispatch (Vst2::effGetProgram, 0, 0, nullptr, 0);

pluginWantsKeys = (dispatch (Vst2::effKeysRequired, 0, 0, nullptr, 0) == 0);
~~~

This is in the `openPluginWindow()` method, which is apparently only called when opening the GUI window of a plugin.
Given the order of appearance, and that it only happens for GUI plugins, we can be pretty confident to have found a new opcode:

| opcode            | code |
|-------------------|------|
| `effKeysRequired` | 57   |

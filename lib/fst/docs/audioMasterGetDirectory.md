audioMasterGetDirectory
=======================

# environment
- [pluginval-1.0.4](https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip)
- `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/cbf233a5dcd342c08ed2aa72968042c740dffdfa/src/FstPlugin)
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) sources

# walkthrough


In the [JUCE sources](https://github.com/juce-framework/JUCE/blob/51a8a6d7aeae7326956d747737ccf1575e61e209/modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp#L1660) we see
~~~C++
case Vst2::audioMasterGetDirectory:             return getVstDirectory();
~~~

This should allow us to brute-force the `audioMasterGetDirectory` opcode.
When we tried to brute-force this with REAPER, it never returned anything.
However *pluginval* uses JUCE as a backend, so it should properly return something.

Here's our little test-code to brute-force hostCodes:

```C++
void test_hostcodes(AEffect*effect) {
  size_t fromopcode = 0;
  size_t toopcode = 128;
  post("testing host's dispatcher");
  for(size_t opcode=fromopcode; opcode<toopcode; opcode++) {
    if (hostKnown(opcode))
      continue;
    char buf[1024] = {0};
    char opcodestr[256] = {0};
    snprintf(buf, 1023, "%s", "fudelDudelDa");
    t_fstPtrInt result = dispatch_v(effect, opcode, 0, 0, buf, 0.f);
    if(result>0xFFFF) {
      const char*res = (const char*)result;
      post("\tRESULT: '%.*s'", 1024, res);
      post_hex((void*)result, 64);
    }
  }
  post("tested host's dispatcher with opcodes %d..%d", fromopcode, toopcode);
}
```

Calling this function in the callback for `effOpen` is rather disappointing.
Only `hostCode:24` returns `1`, all other unknown host opcodes return `0`.
None of these can be our expected string.

What if `effOpen` is too early, and `getVstDirectory()` has not yet been initialized?
Let's try it later, e.g. when shutting down the plugin with `effClose`.
`hostCode:24` now returns `0` (like most other host opcodes), *but* `hostCode:41` returns
`94628345787360` (aka `0x561060EFB3E0`).
This looks suspiciously like a pointer (and running the test multiple times, the value changes - which supports this).
Casting the number to a `const char*` (as in the code above), and printing it gives us...
the directory where the dummy plugin lives in.
This is what we would have expected of `getVstDirectory()`, so we found a new opcode:


| opcode                    | value |
|---------------------------|-------|
| `audioMasterGetDirectory` | 41    |

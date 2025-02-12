effConnectInput & effConnectOutput
==================================

# environment
- [pluginval-1.0.4](https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip)
- `FstPlugin` as of [today](https://git.iem.at/zmoelnig/FST/-/tree/3e5252f26be4ec68029f1cc5d4a7c191a3f98e13/src/FstPlugin)
- [JUCE-8.0.6](https://github.com/juce-framework/JUCE/releases/tag/8.0.6) sources

# walkthrough

Running our test plugin through `pluginval`, we notice that a couple of yet unknown are being triggered.
Notably, opcodes `31` and `32` are called repeatedly with a running index, that goes from 0..5.
Checking with the plugin, I notice that I have set both the number of inputs and outputs in the `AEffect` structure to 6.
Changing this to `numInputs=2` and `numOutputs=8`, the calls now change:
~~~C++
FST::host2plugin(FstPlugin, effSetProgram, 0, 0, (nil), 0.0);
FST::host2plugin(FstPlugin, 31, 1, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 31, 0, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 7, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 6, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 5, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 4, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 3, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 2, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 1, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, 32, 0, 1, (nil), 0.0);
FST::host2plugin(FstPlugin, effGetPlugCategory, 0, 0, (nil), 0.0);
~~~

So indeed, `effCode:31` is called once for each input and `effCode:32` is called once of each output.
We also note that the other i/o-related opcodes (`effGetInputProperties` (33) resp `effGetOutputProperties` (34))
are in the immediate vicinity.
There are two unknown opcodes whose name indicate they have something to do with i/o: `effConnectInput` and `effConnectOutput`.
The two opcodes are sent from the plugin tester pretty early, after setting samplerate, blocksize,
directly after setting the program (`effSetProgram`),
and right before it even queries the plugin category (`effGetPlugCategory`).
So the opcode is probably about some basic capability of the plugin.
(Weird that REAPER seems to not care at all...).

The `effConnect*put` theory is strongly supported by the JUCE sources, that use something like:

~~~C++
    dispatch (Vst2::effSetProgram, 0, 0, nullptr, 0);

for (int i = vstEffect->numInputs;  --i >= 0;)  dispatch (Vst2::effConnectInput,  i, 1, nullptr, 0);
for (int i = vstEffect->numOutputs; --i >= 0;)  dispatch (Vst2::effConnectOutput, i, 1, nullptr, 0);

if (getVstCategory() != Vst2::kPlugCategShell) // (workaround for Waves 5 plugins which crash during this call)
~~~

The `getVstCategory()` method sends out an `effGetPlugCategory`
(so the `effConn*put` calls are right between `effSetProgram` and `effGetPlugCategory`),
*and* the number of inputs/outputs are counted backwards.

| opcode             | value |
|--------------------|-------|
| `effConnectInput`  | 31    |
| `effConnectOutput` | 32    |

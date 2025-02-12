effEditDraw
===========

# environment
- [REAPER-v6.42](https://www.reaper.fm/files/6.x/reaper642_linux_x86_64.tar.xz)
- [REAPER-SDK](https://github.com/justinfrankel/reaper-sdk/blob/6c755b2696ff02a2f5d15d685712d6f2b7e4ed6d)
- `FstPlugin` as of today (2022-12-05)


# walkthrough


The reaper-sdk [https://github.com/justinfrankel/reaper-sdk](https://github.com/justinfrankel/reaper-sdk/blob/6c755b2696ff02a2f5d15d685712d6f2b7e4ed6d/sdk/reaper_plugin_fx_embed.h)  mentions:

```md
* to support via VST2: canDo("hasCockosEmbeddedUI") should return 0xbeef0000
* dispatcher will be called with opcode=effVendorSpecific, index=effEditDraw, value=parm2, ptr=(void*)(INT_PTR)parm3, opt=message (REAPER_FXEMBED_WM_*)
```

This should give us a good guess on `effEditDraw`.
Arming our dummy plugin to output `0xBEEF0000` when it is asked whether it can do "hasCockosEmbeddedUI", should make REAPER send it an `effVendorSpecific` opcode with the index being some meaningful opcode.

Unfortunately, a quick check shows, that REAPER (reaper-6.71) will send exactly the same to `index` values with the `effVendorSpecific` opcodes as when we answer the "hasCockosEmbeddedUI" with `0x0`:

| opcode            | index        | value        | ptr            | opt   |
|-------------------|--------------|--------------|----------------|-------|
| effVendorSpecific | `0x73744341` | `0x46554944` | 0x7ffc4fd12ea0 | `0.0` |
| effVendorSpecific | `0x2D`       | `0x50`       | 0x7ffc4fd133a0 | `0.0` |

the index and value for both calls are constant (for different runs; and for 32bit vs 64bit versions of REAPER), so they appear to be magic numbers.
The ptr changes and appears to point to some memory location.

`0x73744341` is called always and early in the life-cycle of the plugin, whereas `0x2D` is only
called if we try to open the GUI of the plugin.

`0x2D` is already known as `effGetEffectName` which we know is another REAPER extension.

This makes a strong case for `0x2F` being `effEditDraw`
(which is supported by `hostBeginEdit` having a similar value (`0x2B`);
there really is nothing forcing `host*Edit*` and `eff*Edit*` values to be in the same range, but it does make for nice symmetry;
OTOH the `effEdit*` opcodes are in the range `0x0d..0x13`).



| opcode            | value  |
|-------------------|--------|
| `effEditDraw`     | ~47~   |

OTOH, `47` is already taken by `effGetVendorString`...

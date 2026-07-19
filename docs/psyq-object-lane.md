# Optional GS_107 original-object lane

`./Build check-psyq-gs107` proves that the complete stock `GS_107.OBJ` can
replace the current mixed raw/C reconstruction without changing one byte of
`main.exe`.  It is an audit target, not a dependency of `./Build check`.

Supply a local PsyQ 4.4, 4.5, or 4.6 `LIBGS.LIB`:

```sh
nix develop -c env TENCHU_PSYQ_LIBGS=/path/to/LIBGS.LIB \
  ./Build check-psyq-gs107
```

If already inside `nix develop` (or a direnv shell), omit the
`nix develop -c env` prefix. The Nix shell supplies only the open-source
`psyqdump` and `psyq2elf` tools.
No SDK archive or object is fetched or tracked.  The target first checks the
whole archive against the allowlist in
`config/psyq-objects.main.exe.json`, extracts `GS_107.OBJ`, and independently
checks that member's size and SHA-256.  All extracted and converted files live
under the already-ignored `.shake/psyq/GS_107` directory.

The manifest also pins facts that matter to relocation, rather than merely a
post-link byte count:

- the complete 0x570-byte `.text` section and 0x40-byte `.bss` section;
- all five named symbols and three anonymous local text symbols;
- all 16 MIPS relocations, including their targets;
- the original object's input alignment and the executable's `SUBALIGN(4)`;
- the linked text range, BSS address, and complete executable hash.

Upstream `psyq2elf` currently emits a malformed ELF symbol-table boundary when
an object has anonymous locals after globals.  The lane runs the converted file
through a no-op GNU `objcopy`, then checks that all locals precede globals and
that `sh_info` names the first global.  This is a structural repair, not a code
rewrite; the pinned unlinked `.text` hash and relocation table are checked only
afterward.

For the alternate link, `GS_107.o(.text)` replaces the owner beginning at file
offset `0x54DE4` as one input section.  Its eight bytes of original tail padding
move the following raw owner's start from `0x5534C` to `0x55354`.  The object's
`.bss` and `COMMON` inputs go into a separate `NOLOAD`, `SUBALIGN(4)` section at
`0x800C64A0`; temporary copies of the symbol scripts remove the three absolute
definitions so the object itself owns `GsSetFlatLight`, `GsLIGHTWSMATRIX`, and
`_LC`.  The final binary must have the known-good 555,008-byte SHA-256 and pass
a direct byte comparison.

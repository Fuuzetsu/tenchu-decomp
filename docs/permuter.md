# decomp-permuter (`tools/permute.py`)

When a function's C is *arithmetically* correct but doesn't byte-match — the
instructions are right but the register allocation / scheduling differs (the
classic case: `get_held_buttons`/`GetRealPad`, see
[matching-get-held-buttons.md](matching-get-held-buttons.md)) — the
[decomp-permuter](https://github.com/simonlindholm/decomp-permuter) randomly
perturbs the source and recompiles until it finds the variant that scores 0.

It's wired into the devShell (flake input + a Python env with pycparser/toml/
pynacl, plus a `mips-linux-gnu-objdump` shim the scorer needs). Drive it with:

```console
$ nix develop
$ tools/permute.py <name> [-- <extra permuter args>]
```

`tools/permute.py`:

1. builds (so the function's nonmatching `.s` exists),
2. sets up `.shake/permuter/<name>/` with
   - `compile.sh` — this repo's exact pipeline (`cc1 -G8 | maspsx | as`),
   - `base.c` — the preprocessed `src/main.exe/<name>.c` (compiled with
     `-DPERMUTER`, so `INCLUDE_ASM` and the `.include macro.inc` drop out and it
     parses),
   - `target.o` — the original function assembled from its nonmatching `.s`,
3. runs `permuter.py --stop-on-zero -j4` (override by passing args after `--`).

On success it prints the winning source
(`.shake/permuter/<name>/output-0-*/source.c`). Clean that up into
`src/main.exe/<name>.c`, drop the `INCLUDE_ASM`, and confirm with `./Build check`.

## Notes

- The starting `src/main.exe/<name>.c` should be your best attempt at matching C
  (real body, right types) — the permuter perturbs *that*. A pure `INCLUDE_ASM`
  stub has nothing to permute.
- Permuter wins often look odd (a `do { … } while (0)`, an extra temporary, an
  `if (1)`). Keep whatever is load-bearing, with a comment — see the GetRealPad
  case study.
- It only nudges register allocation / scheduling. If the *instructions* are
  wrong, fix the C (or, for gp/div/nop issues, that's maspsx — see
  [toolchain.md](toolchain.md)), not the permuter.
- Validated by re-discovering the `GetRealPad` match from the natural
  (non-matching) C.

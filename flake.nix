{
  description = "tenchu-decomp";
  nixConfig.bash-prompt = "[nix-develop]$ ";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    splat = {
      url = "github:Fuuzetsu/splat-overlay";
    };
    # m2c (mips2c) straight from upstream, pinned to a rev we patch on top of.
    # The old Fuuzetsu/m2c-overlay pin was ~343 commits (3 years) stale and had
    # no PS1 GTE/COP2 support, so it emitted `M2C_ERROR(unknown instruction:
    # lwc2 ...)` for the whole renderer region. See nix/m2c/*.patch.
    m2c-src = {
      url = "github:matt-kempster/m2c/94098d4de68c2fcc13fb8cf1096a1520eb171abe";
      flake = false;
    };
    spimdisasm = {
      url = "github:Fuuzetsu/spimdisasm-overlay";
    };
    asm-differ = {
      url = "github:simonlindholm/asm-differ";
      flake = false;
    };
    maspsx = {
      url = "github:mkst/maspsx";
      flake = false;
    };
    decomp-permuter = {
      url = "github:simonlindholm/decomp-permuter";
      flake = false;
    };
    # A SECOND, modern nixpkgs used ONLY for Python dev tooling (tree-sitter for
    # tools/autorules.py). The main `nixpkgs` is pinned to 2023-03 and is
    # load-bearing for the byte-matching toolchain (cc1/binutils/ghc) — never
    # bump it for a tool. This input keeps the toolchain frozen while giving the
    # tools a recent interpreter + packages.
    nixpkgs-tools.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    inputs@{ self
    , nixpkgs
    , flake-utils
    , flake-compat
    , splat
    , m2c-src
    , asm-differ
    , spimdisasm
    , maspsx
    , decomp-permuter
    , nixpkgs-tools
    }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      lib = (import nixpkgs { inherit system; }).lib;
      pkgs = import nixpkgs {
        inherit system;
        overlays = [
          inputs.splat.outputs.overlays.default
          inputs.spimdisasm.outputs.overlays.default
        ];
      };
      mipselPkgs = pkgs.pkgsCross.mipsel-linux-gnu;

      # Python for the dev tools (tools/*.py), from the modern tools-nixpkgs so
      # tree-sitter is available for tools/autorules.py's AST rules. The devShell
      # otherwise ships no python3 (it comes from the user profile), so this
      # single interpreter cleanly becomes `python3` on the devShell PATH.
      pkgsTools = import inputs.nixpkgs-tools { inherit system; };
      toolsPython = pkgsTools.python3.withPackages (ps: with ps; [
        tree-sitter
        tree-sitter-language-pack
      ]);


      asm-differ =
        let
          asm-differ = pkgs.poetry2nix.mkPoetryApplication {
            projectDir = inputs.asm-differ;
            python = pkgs.python3;
            poetrylock = ./nix/asm-differ-poetry.lock;
            preferWheels = true;

          };
        in
        pkgs.writeShellScriptBin "diff.py" ''
              ${asm-differ.dependencyEnv}/bin/python ${inputs.asm-differ}/diff.py "$@"
          '';

      # maspsx: post-processes cc1's asm so GNU as reproduces PSY-Q ASPSX bytes
      # (replaces ASPSX.EXE — no wine needed). Single-file, stdlib-only Python.
      # Patched (nix/maspsx-gp-extern.patch) to add an opt-in `--gp-extern SYM`
      # flag: ASPSX gp-addresses only symbols *defined* in the file it assembles
      # (verified against the original binary — think's TU gp-addresses
      # FRAMES_UNTIL_END_OF_ALERT while the item TU addresses the same symbol
      # absolutely), but this decomp declares everything `extern` (fixed-address
      # link). Per file, Build.hs lists the smalls the ORIGINAL translation unit
      # defined (maspsxGpExterns) so exactly those go via $gp — e.g. Think1sleep.
      maspsx-src = pkgs.applyPatches {
        name = "maspsx-patched";
        src = inputs.maspsx;
        patches = [ ./nix/maspsx-gp-extern.patch ];
      };
      maspsx-bin = pkgs.writeShellScriptBin "maspsx" ''
        exec ${pkgs.python3}/bin/python3 ${maspsx-src}/maspsx.py "$@"
      '';

      # m2c, upstream master + our PS1 patch series (nix/m2c/*.patch, authored by
      # Patrick Gill and kept as standalone commits so they stay upstreamable).
      # What they buy us, in order of value to this repo:
      #   * PSX GTE/COP2 support — the ~25 renderer functions used to decompile
      #     to `M2C_ERROR(unknown instruction: lwc2 ...)` and nothing else.
      #   * `--input-regs a,b,...` — declares registers live at function entry, so
      #     the GTE region's non-ABI calling convention ($t2..$t5 etc.) stops
      #     producing `M2C_ERROR(Read from unset register)` for every use.
      #   * correctness: swc2 no longer silently drops its memory write; lwr/lwl
      #     handles the PSX lwr-before-lwl order; rfe/bltzal/bgezal/bc0f/bc0t/
      #     bc2f/bc2t no longer abort a function.
      # `run_tests.py` is green (393/393) on this series; the last patch fixes a
      # snapshot the series itself had left stale.
      m2c-patched = pkgs.applyPatches {
        name = "m2c-patched";
        src = inputs.m2c-src;
        patches = lib.filesystem.listFilesRecursive ./nix/m2c;
      };
      m2c-bin =
        let pyenv = pkgs.python3.withPackages (ps: [ ps.graphviz ]);
        in pkgs.writeShellScriptBin "m2c.py" ''
          exec ${pyenv}/bin/python3 ${m2c-patched}/m2c.py "$@"
        '';

      # The PS1 GCC 2.8.1 compiler (cc1). Pinned by sha256 to decompals/old-gcc
      # 0.17 `gcc-2.8.1-psx` instead of committing the ~3 MB binary. This exact
      # build is verified equivalent to the real Sony CC1PSX.EXE (== decomp.me
      # `psyq4.3`) for our corpus — see docs/toolchain.md. The previously-committed
      # tools/cc1-281 was a DIFFERENT, non-canonical build with a codegen bug (it
      # read the high halfword for `(short)(int)` truncation). Static i386 ELF, runs
      # directly on x86_64 via i386 kernel support; skip fixup so it isn't mangled.
      cc1-281 = pkgs.runCommandLocal "cc1-281"
        {
          src = pkgs.fetchurl {
            url = "https://github.com/decompals/old-gcc/releases/download/0.17/gcc-2.8.1-psx.tar.gz";
            sha256 = "f6f6e883942d4d3289d048236c672e71ed410e546aaae8ff655952f1567e1be0";
          };
          dontFixup = true;
        } ''
        mkdir -p "$out/bin"
        tar xzf "$src" cc1
        install -m755 cc1 "$out/bin/cc1-281"
      '';

      # mkpsxiso + dumpsxiso: dump/rebuild the game's CD image (`./Build iso`).
      mkpsxiso = pkgs.callPackage ./nix/mkpsxiso.nix { };

      # decomp-permuter: randomly perturbs C to find the source that byte-matches
      # (register allocation / scheduling). tools/permute.py drives it.
      permuter =
        let
          pyenv = pkgs.python3.withPackages (ps: [ ps.pycparser ps.toml ps.pynacl ps.attrs ]);
        in
        pkgs.symlinkJoin {
          name = "decomp-permuter";
          paths = [
            (pkgs.writeShellScriptBin "permuter.py" ''
              export PYTHONPATH=${inputs.decomp-permuter}''${PYTHONPATH:+:$PYTHONPATH}
              exec ${pyenv}/bin/python3 ${inputs.decomp-permuter}/permuter.py "$@"
            '')
            (pkgs.writeShellScriptBin "permuter-import.py" ''
              export PYTHONPATH=${inputs.decomp-permuter}''${PYTHONPATH:+:$PYTHONPATH}
              exec ${pyenv}/bin/python3 ${inputs.decomp-permuter}/import.py "$@"
            '')
            # the permuter's scorer looks for `mips-linux-gnu-objdump`; forward to
            # this repo's cross objdump (ELF carries endianness, so -EL is implicit).
            (pkgs.writeShellScriptBin "mips-linux-gnu-objdump" ''
              exec mipsel-unknown-linux-gnu-objdump "$@"
            '')
          ];
        };
    in
    {
      legacyPackages = pkgs;
      packages = flake-utils.lib.flattenTree
        { };

      apps = { };

      devShell = mipselPkgs.mkShell {
        nativeBuildInputs = [
          # pkgs.busybox
          pkgs.less
          pkgs.spimdisasm
          m2c-bin
          pkgs.splat
          (pkgs.runCommand "cpp" { } ''
            mkdir -p "$out"/bin
            ln -s "${pkgs.gcc}/bin/cpp" "$out"/bin 
          '')
          maspsx-bin
          cc1-281
          mkpsxiso
          permuter
          # GHC with the Shake build tool's deps baked into its global package
          # DB, so `./Build` compiles shake/src/Build.hs with plain `ghc` fully
          # offline — no `cabal update`/Hackage fetch on a fresh checkout.
          (pkgs.haskellPackages.ghcWithPackages (p: [
            p.shake
            p.aeson
            p.uuid
            p.hashable
          ]))
          pkgs.cabal-install
          pkgs.haskell-language-server
          # TODO: asm-differ into overlay
          asm-differ
          # Modern python with tree-sitter for tools/autorules.py (see toolsPython).
          toolsPython
        ];
        buildInputs = [

        ];
        LANG = "C.UTF-8";
      };
    });
}

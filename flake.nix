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
    m2c = {
      url = "github:Fuuzetsu/m2c-overlay";
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
  };

  outputs =
    inputs@{ self
    , nixpkgs
    , flake-utils
    , flake-compat
    , splat
    , m2c
    , asm-differ
    , spimdisasm
    , maspsx
    , decomp-permuter
    }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      lib = (import nixpkgs { inherit system; }).lib;
      pkgs = import nixpkgs {
        inherit system;
        overlays = [
          inputs.splat.outputs.overlays.default
          inputs.m2c.outputs.overlays.default
          inputs.spimdisasm.outputs.overlays.default
        ];
      };
      mipselPkgs = pkgs.pkgsCross.mipsel-linux-gnu;


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
      # Patched (nix/maspsx-gp-extern.patch) to gp-address size-bearing small
      # externs — cc1 -G emits `.extern SYM,SIZE` for referenced small globals,
      # which real `as -G`/ASPSX gp-address but upstream maspsx leaves absolute.
      # This is what lets gp-relative functions (e.g. Think1sleep) byte-match.
      maspsx-src = pkgs.applyPatches {
        name = "maspsx-patched";
        src = inputs.maspsx;
        patches = [ ./nix/maspsx-gp-extern.patch ];
      };
      maspsx-bin = pkgs.writeShellScriptBin "maspsx" ''
        exec ${pkgs.python3}/bin/python3 ${maspsx-src}/maspsx.py "$@"
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
          pkgs.m2c
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
        ];
        buildInputs = [

        ];
        LANG = "C.UTF-8";
      };
    });
}

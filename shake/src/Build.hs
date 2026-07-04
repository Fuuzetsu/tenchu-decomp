{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE LambdaCase #-}
{-# LANGUAGE NamedFieldPuns #-}
{-# LANGUAGE TypeFamilies #-}

import Control.Monad (when)
import qualified Data.Aeson as A
import Data.Binary (Binary)
import Data.Functor ((<&>))
import Data.Hashable (Hashable)
import Data.List (sort)
import qualified Data.Set as Set
import qualified Data.UUID as UUID
import Data.UUID.V4 (nextRandom)
import Development.Shake
import Development.Shake.Classes (NFData)
import Development.Shake.FilePath
import Development.Shake.Util (neededMakefileDependencies, parseMakefile)
import GHC.Generics (Generic)
import qualified System.Directory as IO
import System.Environment (lookupEnv)

shakeDir :: FilePath
shakeDir = ".shake"

genDir :: FilePath
genDir = shakeDir </> "gen"

mainGenDir :: FilePath
mainGenDir = genDir </> "main.exe"

mainGen :: FilePath
mainGen = genDir </> "main.exe.done"

configDir :: FilePath
configDir = "config"

asmDir :: FilePath
asmDir = "asm"

srcDir :: FilePath
srcDir = "src"

buildDir :: FilePath
buildDir = shakeDir </> "build"

-- C files ran through CPP
processedDir :: FilePath
processedDir = shakeDir </> "processed"

mainExe :: FilePath
mainExe = buildDir </> "tenchu" </> "main.exe"

-- | The modded (non-matching) build: hooked functions patched in place by
-- tools/mkmod.py, so it stays the same size as main.exe (disc rebuild is faithful).
mainModExe :: FilePath
mainModExe = buildDir </> "tenchu" </> "main_mod.exe"

-- | Bootable disc images repacked from our exe. The matching build and the mod
-- build get distinct names so they can each be their own Shake target.
tenchuBin, tenchuCue, tenchuModBin, tenchuModCue :: FilePath
tenchuBin = buildDir </> "tenchu" </> "tenchu.bin"
tenchuCue = buildDir </> "tenchu" </> "tenchu.cue"
tenchuModBin = buildDir </> "tenchu" </> "tenchu-mod.bin"
tenchuModCue = buildDir </> "tenchu" </> "tenchu-mod.cue"

cross :: String -> FilePath
cross t = "mipsel-unknown-linux-gnu-" <> t

objcopy :: FilePath
objcopy = cross "objcopy"

objcopyFlags :: [String]
objcopyFlags = ["-O", "binary"]

asFlags :: [String]
asFlags = ["-EL", "-Iinclude", "-march=r3000", "-mtune=r3000", "-no-pad-sections", "-O1", "-G0"]

cpp :: FilePath
cpp = cross "cpp"

cc :: FilePath
cc = "cc1-281"          -- PS1 GCC 2.8.1 cc1, provided on PATH by the nix devShell

-- | maspsx post-processes cc1's asm so GNU @as@ reproduces PSY-Q ASPSX's bytes
-- (div traps, load-delay nops, $at/li/move expansion, $gp small-data layout).
-- Runs as a filter between cc1 and @as@; provided by the nix devShell.
maspsx :: FilePath
maspsx = "maspsx"

-- | @--aspsx-version=2.77@ is the evidenced version for Tenchu (gp symbol+offset
-- addressing present, 1993-1997 SDK). @-G8@ is the small-data threshold maspsx
-- emulates for $gp layout (it matches cc1's @-G8@; @as@ still gets @-G0@).
maspsxFlags :: [String]
maspsxFlags = ["--aspsx-version=2.77", "-G8"]

-- | Per-file @--gp-extern SYM@ flags (our nix/maspsx-gp-extern.patch). ASPSX
-- $gp-addresses only symbols *defined* in the file being assembled; externs are
-- always absolute (@lui $at@). Verified in the original binary: the think TU
-- gp-addresses FRAMES_UNTIL_END_OF_ALERT (its own .sdata starts right at it,
-- 0x800979c0) while the item TU addresses the very same symbol absolutely.
-- Our decomp declares everything @extern@ (symbols come from the fixed-address
-- link), so per file we list the small globals its ORIGINAL translation unit
-- defined — maspsx then gp-addresses exactly those and leaves the rest absolute.
maspsxGpExterns :: FilePath -> [String]
maspsxGpExterns src = extra (takeBaseName src) <> concat [["--gp-extern", s] | s <- syms (takeBaseName src)]
  where
    -- Files whose original code divides by a *variable* need ASPSX's guarded
    -- div expansion (bnez/break 7/break 6); scoped per-file so nothing else
    -- in the image can change.
    extra "GetAreaMapLevel" = ["--expand-div"]
    extra _ = []
    -- Think1sleep.c is a fragment of the original think TU, which defines these.
    syms "Think1sleep" = ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"]
    -- ReqItemDrop.c is part of the original item TU, which defines its
    -- round-robin counter (the item TU's .sdata block starts at 0x80097ac8).
    syms "ReqItemDrop" = ["COUNTER_FOR_ITEM_ARRAY_"]
    -- FUN_8004a42c.c is also part of the original item TU (same counter).
    syms "FUN_8004a42c" = ["COUNTER_FOR_ITEM_ARRAY_"]
    -- GetAreaMapLevel.c is part of the original area-map TU, which defines
    -- these small globals (.sdata around 0x80097ec0).
    syms "GetAreaMapLevel" = ["FieldIndex", "FieldArea", "D_80097EC0", "D_80097EC4"]
    -- DoInfoViewProc.c is part of the original info-view TU, which defines
    -- these smalls (fInitialize / item cursor / PutMap latch).
    syms "DoInfoViewProc" = ["D_80097B1C", "CURRENTLY_SELECTED_ITEM_KIND_1_", "D_80097BB1"]
    -- BriefingAndInventorySelectionScreen.c's original TU defines the one-shot
    -- capacity-cheat latch (.sdata, 0x80097cdc); SkipFrame stays absolute.
    syms "BriefingAndInventorySelectionScreen" = ["CARRY_30_ITEMS_CHEAT_APPLIED"]
    -- LayoutEnemyOption's TU defines this small (debug enemy-layout state).
    syms "LayoutEnemyOption" = ["D_80097D44"]
    -- debug_menu_stage_option.c is part of the same debug-menu TU, which
    -- defines SystemFlag (info-view TU accesses it absolutely — both proven).
    syms "debug_menu_stage_option" = ["SystemFlag"]
    syms "FileOption" = ["SystemFlag"]
    syms "ReqItemJirai" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemDokudango" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemSmoke" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemFire" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemShinsoku" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemNinken" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemKaengeki" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemGoshikimai" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemNemuri" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemKusuri" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemKawarimi" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemGosin" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemHenshin" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemManebue" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemMakibishi" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemLightningBolt" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemNingyo" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ReqItemLaunch" = ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F48"]
    syms "ReqItemArrow" = ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F4C"]
    syms "ReqItemHappou" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ProcItemHappou" = ["HAPPOU_SCRATCH_MODEL"]
    syms "Sound" = ["D_80097CB4"]
    syms "DeleteConflict" = ["ConflictObjects"]
    syms "InsertConflict" = ["ConflictObjects"]
    syms _ = []

as :: FilePath
as = cross "as"

linkerDir :: FilePath
linkerDir = "linker"

metaDir :: FilePath
metaDir = "meta"

assetsDir :: FilePath
assetsDir = "assets"

splatDirs :: [FilePath]
splatDirs =
  [ metaDir,
    linkerDir,
    asmDir,
    srcDir,
    assetsDir
  ]

ld :: FilePath
ld = cross "ld"

ldFlags :: [String]
ldFlags =
  [ "-EL"
  ]

ccFlags :: [String]
ccFlags =
  [ "-mcpu=3000",
    "-quiet",
    -- -G8: small globals go in .sdata/.sbss and are addressed via $gp, as the
    -- original ASPSX build did. maspsx reproduces the resulting layout; `as`
    -- itself still runs with -G0 (see asFlags).
    "-G8",
    "-w",
    "-O2",
    "-funsigned-char",
    "-fpeephole",
    "-ffunction-cse",
    "-fpcc-struct-return",
    "-fcommon",
    "-fverbose-asm",
    "-fgnu-linker",
    "-mgas",
    "-msoft-float"
  ]

cppFlags :: [String]
cppFlags =
  [ "-Iinclude",
    "-undef",
    "-Wall",
    "-lang-c",
    "-fno-builtin",
    "-gstabs",
    "-Dmips",
    "-D__GNUC__=2",
    "-D__OPTIMIZE__",
    "-D__mips__",
    "-D__mips",
    "-Dpsx",
    "-D__psx__",
    "-D__psx",
    "-D_PSYQ",
    "-D__EXTENSIONS__",
    "-D_MIPSEL",
    "-D_LANGUAGE_C",
    "-DLANGUAGE_C",
    "-DHACKS"
  ]

-- | Per-file NON_MATCHING opt-in. @NON_MATCHING=Name1,Name2 ./Build@ (or
-- @NON_MATCHING=all@) compiles those files' @#else@ draft instead of their
-- @INCLUDE_ASM@ stub, so a work-in-progress function can be built (and
-- byte-compared) without hand-editing the source. Unset (the default) keeps
-- every stub, i.e. the green byte-identical image. @getEnv@ is Shake-tracked,
-- so flipping the variable reprocesses exactly the affected files.
nonMatchingFlags :: FilePath -> Action [String]
nonMatchingFlags src = do
  mval <- getEnv "NON_MATCHING"
  let names = maybe [] (words . map (\c -> if c == ',' then ' ' else c)) mval
  pure ["-DNON_MATCHING" | mval == Just "all" || takeBaseName src `elem` names]

-- | The sha256 of the known-good target @disks/tenchu/main.exe@. Pinned so that
-- @check@ can detect a swapped/corrupt base image rather than merely proving the
-- build is self-consistent with whatever splat happened to extract.
expectedSha256 :: String
expectedSha256 = "0690a5c14ff3e975ebcb3de26e196a4dbb6afc992677d0de2cbcf86af9993558"

-- | Register the @.include@'d files recorded by @as --MD@ as dependencies.
--
-- @as@ resolves @.include@ directives (the INCLUDE_ASM'd nonmatching .s and
-- @include/macro.inc@) at assembly time; cpp's @-MMD@ never sees them, so without
-- this the object would go stale when that asm changes. splat bakes a @config/..@
-- prefix into the INCLUDE_ASM paths, so normalise them to match the generated-file
-- rules. Mirrors 'neededMakefileDependencies' but with the normalisation.
neededAsmDeps :: FilePath -> Action ()
neededAsmDeps depFile =
  needed . map normaliseEx . concatMap snd . parseMakefile =<< liftIO (readFile depFile)

main :: IO ()
main = do
  topD <- getProjectRoot
  IO.setCurrentDirectory topD
  let opts =
        shakeOptions
          { shakeColor = True,
            shakeVerbosity = Verbose,
            shakeTimings = True,
            shakeChange = ChangeDigest,
            shakeFiles = shakeDir,
            -- Bump to force a full rebuild when a rule's *command* changes in a
            -- way Shake can't see (it doesn't track cmd_ contents). Bumped when
            -- main.exe.elf stopped being stripped (needed by the `mod` target).
            -- "3": one-time bump so every .s rule re-runs once and records the
            -- new GpFlags oracle dependency; afterwards the oracle invalidates
            -- exactly the affected .s on a gp-list edit — no more bumps needed
            -- for gp changes.
            shakeVersion = "3"
          }
  shakeArgs opts rules

rules :: Rules ()
rules = do
  _ <- addOracle (liftIO . runIdOracle)
  _ <- addOracle (\(GpFlags f) -> pure (maspsxGpExterns f))
  mainRules
  phonyRules
  objRules

objRules :: Rules ()
objRules = do
  [buildDir </> "*" </> "data" <//> "*.s.o", buildDir </> "*" </> "*.s.o"] |%> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        src = genDir </> target </> asmDir </> dropDirectory1 (dropExtension fileComponent)
    need [src]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, src]
      neededAsmDeps depFile

  [processedDir <//> "*.c", processedDir <//> "*.h"] |%> \out -> do
    _generatedFiles <- getGeneratedFiles mainGen
    let fileComponent = makeRelative processedDir out
        target = takeDirectory1 fileComponent
        file = dropDirectory1 fileComponent
        header = srcDir </> target </> target <.> "h"
        genPath = genDir </> target </> "src" </> file
        srcPath = srcDir </> target </> file

    -- If corresponding file is in `src` then use it, otherwise use it from gen
    -- dir.
    src <-
      doesFileExist srcPath <&> \case
        True -> srcPath
        False -> genPath

    orderOnly [header]
    -- Make sure we have generated sources. Whether and what we need from them
    -- exactly, we'll find out from the compiler soon enough and we'll `needed`
    -- it.
    --
    -- TODO: Note that it doesn't seem to include the files inside INCLUDE_ASM,
    -- only headers :(. Very sad. Not sure what to do about it. Parse the file
    -- and find INCLUDE_ASM by hand? But maybe it'll already recompile if
    -- anything in mainGen changes.
    need [mainGen, src]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    nm <- nonMatchingFlags src
    withTempFile $ \makeOut -> do
      cmd_ cpp (cppFlags <> nm <> ["-MMD", "-MF", makeOut, "-I", takeDirectory header]) src out
      neededMakefileDependencies makeOut

  processedDir <//> "*.s" %> \out -> do
    let processed = replaceExtension out "c"
    need [processed]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    putInfo $ "Feeding " <> processed <> " to cc | maspsx"
    -- cc1 -> maspsx (filter) -> .s. maspsx massages cc1's asm so that `as`
    -- reproduces ASPSX's bytes; it leaves INCLUDE_ASM's .include/.section/.set
    -- directives untouched, so stubs pass through unchanged.
    gpFlags <- askOracle (GpFlags processed)
    withTempFile $ \ccOut -> do
      cmd_ (FileStdin processed) (FileStdout ccOut) cc ccFlags
      cmd_ (FileStdin ccOut) (FileStdout out) maspsx
        (maspsxFlags <> gpFlags)

  buildDir <//> "*.c.o" %> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        processed = processedDir </> target </> replaceExtension (dropExtension (dropDirectory1 fileComponent)) "s"

    need [processed]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    putInfo $ "Feeding " <> processed <> " to as"
    -- `as` resolves the INCLUDE_ASM'd nonmatching .s (and include/macro.inc)
    -- here, not cpp; --MD lets us depend on them so an asm edit rebuilds this.
    withTempFile $ \depFile -> do
      cmd_ (FileStdin processed) as asFlags ["--MD", depFile, "-o", out]
      neededAsmDeps depFile

  buildDir <//> "*.bin.o" %> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        asset = genDir </> target </> assetsDir </> dropExtension (dropDirectory1 fileComponent)
    -- The linker script places assets with ELF section syntax `x.bin.o(.data)`,
    -- which needs a real relocatable object, not a raw blob (matches Makefile).
    need [asset]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ ld ["-r", "-b", "binary", "-o", out, asset]

mainRules :: Rules ()
mainRules = do
  want [mainExe]

  priority 2 $ generator mainGen [mainGenDir <//> "*"] $ do
    -- genereratedFiles <- getGeneratedFiles mainGen
    let splatFile = configDir </> "splat.main.exe.yaml"
    need [splatFile, "disks/tenchu/main.exe", configDir </> "symbols.main.exe.txt"]
    -- Remove the gen directory if it's present: this means that if some file
    -- changes upstream, we can regenerate easily without worrying about having
    -- stale things around.
    -- liftIO $ removeFiles mainGenDir ["//"]
    liftIO $ mapM_ (\d -> IO.createDirectoryIfMissing True (mainGenDir </> d)) splatDirs
    cmd_ "split.py" [splatFile]
    -- Clean up paths in linker to not be buildDir </> genDir as we want to put
    -- object files in separate place for aesthetics or whatever.
    let mainBuildDir = buildDir </> "main.exe"
        beforeAsm = mainBuildDir </> mainGenDir </> asmDir
        beforeSrc = mainBuildDir </> mainGenDir </> srcDir
        beforeAssets = mainBuildDir </> mainGenDir </> assetsDir
    cmd_ "sed" ["-i", "s|" <> beforeAsm <> "|" <> mainBuildDir <> "|g", mainGenDir </> linkerDir </> "main.exe.ld"]
    cmd_ "sed" ["-i", "s|" <> beforeSrc <> "|" <> mainBuildDir <> "|g", mainGenDir </> linkerDir </> "main.exe.ld"]
    cmd_ "sed" ["-i", "s|" <> beforeAssets <> "|" <> mainBuildDir <> "|g", mainGenDir </> linkerDir </> "main.exe.ld"]

  let mainElf = mainExe <.> "elf"
  mainElf %> \out -> do
    _generatedFiles <- getGeneratedFiles mainGen
    mainSFiles <- liftIO $ getDirectoryFilesIO (mainGenDir </> asmDir) ["*.s", "data/*.s"]
    cFiles <- liftIO $ do
      userFiles <- Set.fromList <$> getDirectoryFilesIO (srcDir </> "main.exe") ["//*.c"]
      genFiles <- Set.fromList <$> getDirectoryFilesIO (mainGenDir </> "src") ["//*.c"]
      pure $ Set.toList (userFiles <> genFiles)
    mainAssetFiles <- liftIO $ getDirectoryFilesIO (mainGenDir </> assetsDir) ["//*.bin"]
    let mainOFiles = map (\f -> buildDir </> "main.exe" </> f <.> "o") (cFiles <> mainSFiles <> mainAssetFiles)
        ldFile = mainGenDir </> linkerDir </> "main.exe.ld"
        definedSymbols = configDir </> "symbols.main.exe.txt"
        undefinedSymbols = mainGenDir </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = mainGenDir </> metaDir </> "undefined_functions_auto.main.exe.txt"
        mainSFilesExp = map (\f -> mainGenDir </> asmDir </> f) mainSFiles
        -- mainCFilesExp = map (\f -> srcDir </> "main.exe" </> f) mainCFiles
        mainAssetFilesExp = map (\f -> mainGenDir </> assetsDir </> f) mainAssetFiles
    need (mainSFilesExp <> mainAssetFilesExp <> mainOFiles <> [ldFile, undefinedSymbols, undefinedFunctions])
    -- need [mainCFilesExp]

    liftIO $ IO.createDirectoryIfMissing True (buildDir </> "tenchu")
    cmd_
      ld
      ldFlags
      [ "-o",
        out,
        "-Map",
        buildDir </> "tenchu" </> "main.exe.map",
        "-T",
        ldFile,
        -- -T <( cut -d '/' -f1 $(CONFIG_DIR)/symbols.main.exe.txt ) \
        "-T",
        definedSymbols,
        "-T",
        undefinedSymbols,
        "-T",
        undefinedFunctions,
        "--no-check-sections",
        "-nostdlib"
        -- NB: not stripped (`-s`) on purpose — keeping the symbol table lets the
        -- `mod` target look up function slot addresses via nm. objcopy -O binary
        -- ignores the symbol table, so main.exe is unaffected.
      ]

  mainExe %> \_out -> do
    need [mainElf]
    cmd_ objcopy objcopyFlags [mainElf, mainExe]

  -- Non-matching build: mkmod patches hooked functions in place. It reads main.exe's
  -- symbol table (via nm on the elf), compiles every src/mod/main.exe/*.c, and aborts
  -- if one outgrows its slot — so depend on the exe+elf, the mod sources, AND the
  -- tool itself, or an edit to a mod (or to mkmod) wouldn't rebuild and the size
  -- guard wouldn't re-run. See docs/modding-and-nonmatching.md.
  mainModExe %> \_out -> do
    modSrcs <- getDirectoryFiles srcDir ["mod/main.exe/*.c"]
    need $ [mainExe, mainExe <.> "elf", "tools/mkmod.py"]
        <> map (srcDir </>) modSrcs
    cmd_ "tools/mkmod.py"

  -- Bootable CD images (.bin/.cue) with our exe swapped in — repacked only when the
  -- exe changes, not on every `run-iso`. mkiso.py discovers the *original* disc
  -- dynamically ($TENCHU_CUE / disks/ / ~/tenchu-iso/), so a disc swap isn't a
  -- tracked input: `./Build clean` (or touch the exe) to force a repack then.
  [tenchuBin, tenchuCue] &%> \_out -> do
    need [mainExe]
    cmd_ "tools/mkiso.py"

  [tenchuModBin, tenchuModCue] &%> \_out -> do
    need [mainModExe]
    cmd_ "tools/mkiso.py" ["--exe", mainModExe,
                           "--out", buildDir </> "tenchu" </> "tenchu-mod"]

phonyRules :: Rules ()
phonyRules = do
  phony "clean" $ do
    liftIO $
      removeFiles
        "."
        [genDir, buildDir, processedDir]

  phony "extract_main.exe" $ do
    need [mainGen]

  -- Each of these is just a `need` on a real file target now, so the exe/mod/iso
  -- are (re)built only when their inputs change — `run-iso` no longer repacks the
  -- ~750 MB image every launch. `mod` -> main_mod.exe; `iso`/`iso-mod` -> the
  -- bootable .bin/.cue (matching vs grown build). See docs/modding-and-nonmatching.md
  -- and docs/building-an-iso.md.
  phony "mod" $ need [mainModExe]
  phony "iso" $ need [tenchuCue]
  phony "iso-mod" $ need [tenchuModCue]

  -- `run` / `run-mod`: fast path — mount the original disc and `-loadexe` our exe
  -- over it, no ISO repack. Tenchu boots SLPS_019.01 -> ... -> MAIN.EXE, so this
  -- skips that launcher — fine for iterating on main.exe; use `run-iso` for the
  -- faithful full boot. Set PCSX_REDUX / PCSX_REDUX_ARGS to tweak the emulator.
  phony "run" $ do
    need [mainExe]
    launchLoadExe [] mainExe

  phony "run-mod" $ do
    need [mainModExe]
    launchLoadExe [] mainModExe

  -- `run-iso` / `run-iso-mod`: faithful — boot the repacked disc (built by the file
  -- rules above only when the exe changed), so the real SLPS_019.01 -> ... -> MAIN.EXE
  -- chain runs.
  phony "run-iso" $ do
    need [tenchuCue]
    launchIso [] tenchuCue

  -- main_mod.exe is patched in place (same size as main.exe), so the mod disc keeps
  -- forced LBAs and is byte-faithful except our function — streamed cutscenes and the
  -- full SLPS→MENU→MAIN boot all work. See docs/modding-and-nonmatching.md.
  phony "run-iso-mod" $ do
    need [tenchuModCue]
    launchIso [] tenchuModCue

  phony "check" $ do
    let reference = "disks" </> "tenchu" </> "main.exe"
    need [mainExe, reference]
    StdoutTrim ref <- cmd "sha256sum" reference
    StdoutTrim ours <- cmd "sha256sum" mainExe
    let refSha = head $ words ref
        ourSha = head $ words ours
    -- Guard against a swapped/corrupt base image: the reference we diff against
    -- must itself be the known-good Tenchu main.exe, not just self-consistent
    -- with whatever splat happened to extract.
    when (refSha /= expectedSha256) $
      fail $ unwords ["Reference", reference, "has sha256", refSha, "but expected known-good", expectedSha256, "- wrong/corrupt base image?"]
    when (ourSha /= expectedSha256) $
      fail $ unwords ["Expected", mainExe, "to have sha256 of", expectedSha256, "but it's", ourSha]

-- | Launch our exe fast: mount the original disc and @-loadexe@ over it (no repack).
-- Paths are absolutised — pcsx-redux resolves them against its own cwd. @extra@ are
-- extra emulator flags (e.g. @-8mb@ for the grown mod, whose region is above 2MB).
launchLoadExe :: [String] -> FilePath -> Action ()
launchLoadExe extra exe = do
  disc <- liftIO $ findDisc >>= IO.makeAbsolute
  exeAbs <- liftIO $ IO.makeAbsolute exe
  runPcsx (["-run", "-iso", disc, "-loadexe", exeAbs] <> extra)

-- | Launch a repacked disc image (the faithful full boot). @extra@ as 'launchLoadExe'.
launchIso :: [String] -> FilePath -> Action ()
launchIso extra cue = do
  cueAbs <- liftIO $ IO.makeAbsolute cue
  runPcsx (["-run", "-iso", cueAbs] <> extra)

-- | Run pcsx-redux with the given base args plus any @$PCSX_REDUX_ARGS@. It falls
-- back to OpenBIOS when no BIOS is configured, so no BIOS is required.
--
-- We force @-interpreter@ by default: pcsx-redux's x64 dynarec crashes
-- (\"Unrecoverable error while running recompiler\") with OpenBIOS — a regression
-- of grumpycoders/pcsx-redux#695 (that combo was fixed in 2022, re-broke in the
-- memory-system rework since). Since we default to OpenBIOS, the dynarec is
-- unusable here. The user opts back into the dynarec (with a real BIOS) by putting
-- @-dynarec@/@-bios@/@-interpreter@ in @$PCSX_REDUX_ARGS@.
runPcsx :: [String] -> Action ()
runPcsx baseArgs = do
  pcsx <- liftIO findPcsx
  extra <- liftIO $ maybe [] words <$> lookupEnv "PCSX_REDUX_ARGS"
  let userChoseCpu = any (`elem` ["-interpreter", "-dynarec", "-bios"]) extra
      cpu = ["-interpreter" | not userChoseCpu]
  cmd_ pcsx (baseArgs <> cpu <> extra)

-- | The pcsx-redux binary: @$PCSX_REDUX@, else on @$PATH@, else the usual checkout.
findPcsx :: IO FilePath
findPcsx =
  lookupEnv "PCSX_REDUX" >>= \case
    Just p -> do
      ok <- IO.doesFileExist p
      if ok then pure p else fail ("$PCSX_REDUX=" <> p <> " does not exist")
    Nothing -> do
      onPath <- IO.findExecutable "pcsx-redux"
      home <- IO.getHomeDirectory
      let fallback = home </> "programming" </> "pcsx-redux" </> "pcsx-redux"
      fbOk <- IO.doesFileExist fallback
      case onPath of
        Just p -> pure p
        Nothing | fbOk -> pure fallback
        _ -> fail "pcsx-redux not found: set PCSX_REDUX, put it on PATH, or build it at ~/programming/pcsx-redux"

-- | The original (copyrighted) disc you provide: @$TENCHU_CUE@, else a @.cue@ under
-- @disks/@ or @~\/tenchu-iso/@. Mirrors tools/mkiso.py's discovery.
findDisc :: IO FilePath
findDisc =
  lookupEnv "TENCHU_CUE" >>= \case
    Just c -> do
      ok <- IO.doesFileExist c
      if ok then pure c else fail ("$TENCHU_CUE=" <> c <> " not found")
    Nothing -> do
      home <- IO.getHomeDirectory
      firstCue ["disks", home </> "tenchu-iso"]
  where
    firstCue [] = fail "original disc .cue not found; set TENCHU_CUE=/path/to/game.cue"
    firstCue (d : ds) = do
      exists <- IO.doesDirectoryExist d
      cues <-
        if exists
          then sort . filter ((== ".cue") . takeExtension) <$> IO.listDirectory d
          else pure []
      case cues of
        (c : _) -> pure (d </> c)
        [] -> firstCue ds

-- | Gets the absolute root directory of the project. Errors on
-- failure. Assumes we're somewhere inside the project.
getProjectRoot :: IO FilePath
getProjectRoot = do
  currentDir <- IO.getCurrentDirectory
  let loop "/" =
        ioError $
          userError $
            "getProjectRoot: couldn't find source tree in " ++ currentDir
      loop path =
        IO.doesFileExist (path </> ".project.root") >>= \case
          True -> return path
          False -> loop (takeDirectory path)
  loop currentDir

data GetGenId = SplatGenId
  { genFile :: String,
    patterns :: [FilePattern]
  }
  deriving (Generic, Show, Eq)

instance Hashable GetGenId

instance NFData GetGenId

instance Binary GetGenId

type instance RuleResult GetGenId = UUID.UUID

-- | The per-file maspsx flags (gp-externs + extras). These are computed in
-- Haskell and passed as a @cmd_@ argument, which Shake can't see change — so a
-- stale @.s@ would survive a gp-list edit (matchdiff then shows absolute
-- @lui/lw@ instead of the gp @lw@). Exposing them through an oracle makes each
-- @.s@ rule depend on the flag VALUE: edit the list → the oracle answer changes
-- → only the affected @.s@ regenerates. (Proper fix for what a @shakeVersion@
-- bump would otherwise hammer with a full rebuild.)
newtype GpFlags = GpFlags FilePath
  deriving (Generic, Show, Eq)

instance Hashable GpFlags

instance NFData GpFlags

instance Binary GpFlags

type instance RuleResult GpFlags = [String]

data GenData = GenData
  { lastRunId :: UUID.UUID,
    -- TODO: I think we should not only store paths of all the files but also
    -- hashes. If someone tries to change a generated file by hand, we should
    -- retrigger!
    generatedFiles :: [FilePath]
  }
  deriving
    (Generic)

instance A.ToJSON GenData where
  toEncoding = A.genericToEncoding A.defaultOptions

instance A.FromJSON GenData

runIdOracle :: GetGenId -> IO UUID.UUID
runIdOracle genId =
  IO.doesFileExist logFile >>= \case
    True ->
      A.decodeFileStrict' logFile >>= \case
        Just GenData {lastRunId, generatedFiles} -> do
          filesOnDisk <- getDirectoryFilesIO "" (patterns genId)
          if filesOnDisk /= generatedFiles
            then nextRandom
            else pure lastRunId
        Nothing -> nextRandom
    False -> nextRandom
  where
    logFile = genFile genId

recordGeneratedFiles :: UUID.UUID -> FilePath -> [FilePattern] -> IO ()
recordGeneratedFiles runId out patterns = do
  filesCreated <- getDirectoryFilesIO "" patterns
  A.encodeFile out $ GenData {lastRunId = runId, generatedFiles = filesCreated}

generator :: FilePath -> [FilePattern] -> Action () -> Rules ()
generator out generatedPatterns generationCmd = do
  generatedPatterns |%> \_ -> need [out]
  out %> \_ -> do
    runId <- askOracle $ SplatGenId {genFile = out, patterns = generatedPatterns}
    liftIO $ removeFiles "" generatedPatterns
    generationCmd
    liftIO $ recordGeneratedFiles runId out generatedPatterns

getGeneratedFiles :: FilePath -> Action [FilePath]
getGeneratedFiles out = do
  need [out]
  liftIO $
    A.decodeFileStrict' out >>= \case
      Just GenData {generatedFiles} -> pure generatedFiles
      Nothing ->
        fail "GenData decode failure in getGeneratedFiles"

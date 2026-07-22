{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE LambdaCase #-}
{-# LANGUAGE NamedFieldPuns #-}
{-# LANGUAGE TypeFamilies #-}

import Control.Monad (when)
import qualified Data.Aeson as A
import Data.Binary (Binary)
import Data.Functor ((<&>))
import Data.Hashable (Hashable)
import Data.List (intercalate, isPrefixOf, sort)
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
-- (kept as plain constants: `targets` is defined below and Haskell has no
-- forward-use problem, but these two are referenced in comments/tools too.)

configDir :: FilePath
configDir = "config"

ramLayoutHeader, ramLayoutTool :: FilePath
ramLayoutHeader = srcDir </> "main.exe" </> "ram_layout.h"
ramLayoutTool = "tools" </> "ram_layout.py"

ramLayoutValue :: String -> Action String
ramLayoutValue field = do
  need [ramLayoutTool, ramLayoutHeader]
  StdoutTrim value <- cmd "python3" ramLayoutTool field
  pure value

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

-- | Opt-in source-level debugging for VSCode/gdb. Each matched C file is
-- recompiled with @-gstabs@ (byte-identical @.text@) and filtered to the
-- line-only stabs modern gdb can read (tools/stabs_lines.py). GNU ld collapses
-- N_FUN records when merging hundreds of @.stab@ sections, so instead of one
-- merged debug ELF we emit a gdb script that @add-symbol-file@s each per-object
-- debug object at its address in the launched layout (tools/gen_debug_gdb.py).
-- The debug objects are byte-identical in @.text@; only the resolved addresses
-- differ per layout, so one object set serves exact/mod/relink. Deliberately
-- outside buildDir so its @*.c.o@ rule cannot collide with the generic
-- @buildDir//*.c.o@ object rule. See docs/debugging-vscode.md.
debugObjDir :: FilePath
debugObjDir = shakeDir </> "main.exe-dbg"

-- | Per-layout gdb source-line scripts, sourced by .vscode/launch.json.
mainDebugGdb, mainRelinkDebugGdb :: FilePath
mainDebugGdb = mainExe <.> "debug.gdb"
mainRelinkDebugGdb = mainRelinkExe <.> "debug.gdb"

-- | Opt-in proof link where the 555 game inputs own their public symbols.
-- It is retail-sized and byte-exact; SDK/header/BSS relocation is later work.
mainRelocGameExe, mainRelocGameElf, mainRelocGameMap :: FilePath
mainRelocGameExe = buildDir </> "tenchu" </> "main_reloc_game.exe"
mainRelocGameElf = mainRelocGameExe <.> "elf"
mainRelocGameMap = buildDir </> "tenchu" </> "main_reloc_game.exe.map"

relocGameDir, relocGameLinker, relocGameSymbols :: FilePath
relocGameDir = buildDir </> "reloc-game"
relocGameLinker = relocGameDir </> "main.exe.ld"
relocGameSymbols = relocGameDir </> "symbols.main.exe.txt"

-- | The two allocator functions retain matching raw constants in their one C
-- source. The normal lane compiles that same source into this isolated
-- directory, then applies a bounded LUI/ORI-to-symbolic-LUI/ADDIU assembly
-- transform. Ordinary exact symbolic objects are audited alongside it. No
-- source-level build define or per-function compiler flag is involved.
relocCLiteralDir :: FilePath
relocCLiteralDir = shakeDir </> "reloc-c-literals"

relocCLiteralLinker :: FilePath
relocCLiteralLinker = relocCLiteralDir </> "main.exe.ld"

mainRelocCLiteralExe, mainRelocCLiteralElf, mainRelocCLiteralMap :: FilePath
mainRelocCLiteralExe = buildDir </> "tenchu" </> "main_reloc_c_literals.exe"
mainRelocCLiteralElf = mainRelocCLiteralExe <.> "elf"
mainRelocCLiteralMap = mainRelocCLiteralExe <.> "map"

relocCLiteralNames :: [String]
relocCLiteralNames =
  [ "valloc",
    "vinit"
  ]

relocAllocatorLiteralNames :: [String]
relocAllocatorLiteralNames = ["valloc", "vinit"]

ordinaryRelocCLiteralNames :: [String]
ordinaryRelocCLiteralNames =
  [ "ActivateHumans",
    "SelectCameraOwnerOption",
    "FileOption",
    "ProcItemShinsoku"
  ]

relocCLiteralAuditNames :: [String]
relocCLiteralAuditNames = relocCLiteralNames <> ordinaryRelocCLiteralNames

relocCLiteralPreprocessed, relocCLiteralAssembly, relocCLiteralObject :: String -> FilePath
relocCLiteralPreprocessed name = relocCLiteralDir </> name <.> "i"
relocCLiteralAssembly name = relocCLiteralDir </> name <.> "s"
relocCLiteralObject name = relocCLiteralDir </> name <.> "o"

relocCLiteralReferenceObject :: String -> FilePath
relocCLiteralReferenceObject name = buildDir </> "main.exe" </> name <.> "c.o"

relocCLiteralAuditObject :: String -> FilePath
relocCLiteralAuditObject name
  | name `elem` ordinaryRelocCLiteralNames = relocCLiteralReferenceObject name
  | otherwise = relocCLiteralObject name

relocCLiteralObjects :: [FilePath]
relocCLiteralObjects = map relocCLiteralObject relocCLiteralNames

relocCLiteralReferenceObjects :: [FilePath]
relocCLiteralReferenceObjects = map relocCLiteralReferenceObject relocCLiteralNames

relocCLiteralAuditObjects :: [FilePath]
relocCLiteralAuditObjects = map relocCLiteralAuditObject relocCLiteralAuditNames

-- | User function overrides for the normal relink lane.  A file
-- @src/mod-relink/main.exe/\<Name\>.c@ replaces the matched translation unit
-- of the same name in @./Build relink@ only: it is compiled by the identical
-- cpp|cc1|maspsx|as pipeline into an isolated directory, and every generated
-- linker reference to the original object is rewritten to the override, so
-- the function grows or shrinks in place in the retail input order.  The
-- exact matching lanes never read these objects, so @./Build check@ stays
-- byte-identical while a mod is present.  Brand-new translation units belong
-- in @src/main.exe/reloc/@ instead.  See docs/modding-and-nonmatching.md.
modRelinkSrcDir :: FilePath
modRelinkSrcDir = srcDir </> "mod-relink" </> "main.exe"

modRelinkDir :: FilePath
modRelinkDir = shakeDir </> "mod-relink"

modRelinkPreprocessed, modRelinkAssembly, modRelinkObject :: String -> FilePath
modRelinkPreprocessed name = modRelinkDir </> name <.> "i"
modRelinkAssembly name = modRelinkDir </> name <.> "s"
modRelinkObject name = modRelinkDir </> name <.> "o"

-- | Committed real-edit regression lane: replay the PadProc+mod_probe grown
-- edit through the override machinery in an isolated composition, without
-- touching src/.  The gate proves the modding contract stays runnable.
realeditFixtureDir :: FilePath
realeditFixtureDir = "tools" </> "fixtures" </> "relink-realedit"

realeditDir :: FilePath
realeditDir = shakeDir </> "relink-realedit"

realeditLayoutDir :: FilePath
realeditLayoutDir = realeditDir </> "layout"

realeditLinker, realeditSymbols, realeditUndefined :: FilePath
realeditLinker = realeditLayoutDir </> "main.exe.ld"
realeditSymbols = realeditLayoutDir </> "symbols.main.exe.txt"
realeditUndefined = realeditLayoutDir </> "undefined_symbols_auto.main.exe.txt"

realeditTailAsm, realeditTailObject :: FilePath
realeditTailAsm = realeditDir </> "generated" </> "75F64.bss.s"
realeditTailObject = realeditDir </> "obj" </> "75F64.bss.s.o"

realeditOverridePreprocessed, realeditOverrideAssembly, realeditOverrideObject :: FilePath
realeditOverridePreprocessed = realeditDir </> "PadProc.i"
realeditOverrideAssembly = realeditDir </> "PadProc.s"
realeditOverrideObject = realeditDir </> "PadProc.o"

realeditExtDir :: FilePath
realeditExtDir = realeditDir </> "ext"

realeditExtPreprocessed, realeditExtAssembly, realeditExtObject :: FilePath
realeditExtPreprocessed = realeditExtDir </> "mod_probe.i"
realeditExtAssembly = realeditExtDir </> "mod_probe.s"
realeditExtObject = realeditExtDir </> "mod_probe.c.o"

realeditElf, realeditMap, realeditLogical, realeditExe :: FilePath
realeditElf = realeditDir </> "main_realedit.exe.elf"
realeditMap = realeditDir </> "main_realedit.exe.map"
realeditLogical = realeditDir </> "main_realedit.logical"
realeditExe = realeditDir </> "main_realedit.exe"

-- | Overriding the allocator sources would silently bypass their reviewed
-- normal-lane relocation transform, so reject those names with guidance.
modRelinkOverrideNames :: Action [String]
modRelinkOverrideNames = do
  sources <- getDirectoryFiles modRelinkSrcDir ["*.c"]
  let names = map dropExtension sources
      forbidden = filter (`elem` relocCLiteralNames) names
  when (not (null forbidden)) $
    fail $
      "src/mod-relink cannot override the allocator sources "
        <> show forbidden
        <> "; their normal-lane objects carry the reviewed pool/capacity "
        <> "relocation transform (docs/relocatable-build.md). Adjust "
        <> "src/main.exe/ram_layout.h policy or edit the originals instead."
  pure names

-- | Bounded second relocation proof.  Splat emits every raw SDK text carve in
-- 0x800601d4..0x80086764 as canonical assembly, preserving the existing C and
-- canonical-object islands between them.  The base link is retail-exact; a
-- controlled +4 link before Exec audits ordinary J/JAL and HI16/LO16 relocation.
mainRelocSdkExe, mainRelocSdkElf, mainRelocSdkMap :: FilePath
mainRelocSdkExe = buildDir </> "tenchu" </> "main_reloc_sdk.exe"
mainRelocSdkElf = mainRelocSdkExe <.> "elf"
mainRelocSdkMap = buildDir </> "tenchu" </> "main_reloc_sdk.exe.map"

mainRelocSdkShiftExe, mainRelocSdkShiftElf, mainRelocSdkShiftMap :: FilePath
mainRelocSdkShiftExe = buildDir </> "tenchu" </> "main_reloc_sdk_shift4.exe"
mainRelocSdkShiftElf = mainRelocSdkShiftExe <.> "elf"
mainRelocSdkShiftMap = buildDir </> "tenchu" </> "main_reloc_sdk_shift4.exe.map"

relocSdkDir, relocSdkBaseDir, relocSdkShiftDir :: FilePath
relocSdkDir = buildDir </> "reloc-sdk"
relocSdkBaseDir = relocSdkDir </> "base"
relocSdkShiftDir = relocSdkDir </> "shift4"

relocSdkBaseLinker, relocSdkBaseSymbols :: FilePath
relocSdkBaseLinker = relocSdkBaseDir </> "main.exe.ld"
relocSdkBaseSymbols = relocSdkBaseDir </> "symbols.main.exe.txt"

relocSdkShiftLinker, relocSdkShiftSymbols :: FilePath
relocSdkShiftLinker = relocSdkShiftDir </> "main.exe.ld"
relocSdkShiftSymbols = relocSdkShiftDir </> "symbols.main.exe.txt"

-- | Opt-in proof link where initialized data, BSS, and the virtual-memory pool
-- are separate linker-owned regions.  The binary output is deliberately named
-- "logical": it stops at initialized data, before the retail PS-X EXE's 0x150
-- bytes of sector padding.  This lane proves ownership/layout only; it is not
-- yet the final size-changing executable path.
mainRelocBssLogical, mainRelocBssExe, mainRelocBssElf, mainRelocBssMap :: FilePath
mainRelocBssLogical = buildDir </> "tenchu" </> "main_reloc_bss.logical"
mainRelocBssExe = buildDir </> "tenchu" </> "main_reloc_bss.exe"
mainRelocBssElf = buildDir </> "tenchu" </> "main_reloc_bss.exe.elf"
mainRelocBssMap = buildDir </> "tenchu" </> "main_reloc_bss.exe.map"

relocBssDir, relocBssLinker, relocBssSymbols, relocBssUndefined :: FilePath
relocBssDir = buildDir </> "reloc-bss"
relocBssLinker = relocBssDir </> "main.exe.ld"
relocBssSymbols = relocBssDir </> "symbols.main.exe.txt"
relocBssUndefined = relocBssDir </> "undefined_symbols_auto.main.exe.txt"

relocBssTailAsm, relocBssTailObject :: FilePath
relocBssTailAsm = relocBssDir </> "generated" </> "75F64.bss.s"
-- Keep this below an extra directory so the ordinary generated-asm wildcard
-- cannot mistake it for a splat-owned target input.
relocBssTailObject = relocBssDir </> "obj" </> "75F64.bss.s.o"

-- Reviewed pointer-bearing data copies used by the composed normal relink.
-- 75F64 is transformed here first, then the BSS lane applies its NOBITS split
-- to that copy so both transformations survive in one object.
relocIntegratedDataDir :: FilePath
relocIntegratedDataDir = relocBssDir </> "data"

relocDataNames :: [String]
relocDataNames =
  [ "E58", "1160", "1490", "207C", "2EB0", "33C4", "37A8", "400C", "4900",
    "75F64"
  ]

relocDataTargetNames :: [String]
relocDataTargetNames = filter (/= "75F64") relocDataNames

relocDataAsm, relocDataObject :: String -> FilePath
relocDataAsm name = relocIntegratedDataDir </> name <.> "data.s"
relocDataObject name = relocBssDir </> "obj" </> name <.> "data.s.o"

relocDataAsms, relocDataObjects :: [FilePath]
relocDataAsms = map relocDataAsm relocDataNames
relocDataObjects = map relocDataObject relocDataTargetNames

relocData75F64Asm :: FilePath
relocData75F64Asm = relocDataAsm "75F64"

-- | Growth-capable composition.  Unlike the retail-exact BSS oracle above,
-- this linker chain consumes the reviewed symbolic allocator objects without
-- pinning the game/SDK boundary.  Canonical SDK text and every later
-- linker-owned boundary are therefore free to follow changed game layout. It
-- remains separate so the ordinary matching artifact stays untouched.
mainRelinkLogical, mainRelinkExe, mainRelinkElf, mainRelinkMap :: FilePath
mainRelinkLogical = buildDir </> "tenchu" </> "main_relink.logical"
mainRelinkExe = buildDir </> "tenchu" </> "main_relink.exe"
mainRelinkElf = buildDir </> "tenchu" </> "main_relink.exe.elf"
mainRelinkMap = buildDir </> "tenchu" </> "main_relink.exe.map"

normalRelinkDir, normalRelinkCLinker, normalRelinkSdkLinker :: FilePath
normalRelinkDir = buildDir </> "relink"
normalRelinkCLinker = normalRelinkDir </> "c" </> "main.exe.ld"
normalRelinkSdkLinker = normalRelinkDir </> "sdk" </> "main.exe.ld"

normalRelinkSdkSymbols, normalRelinkLinker, normalRelinkSymbols :: FilePath
normalRelinkSdkSymbols = normalRelinkDir </> "sdk" </> "symbols.main.exe.txt"
normalRelinkLinker = normalRelinkDir </> "layout" </> "main.exe.ld"
normalRelinkSymbols = normalRelinkDir </> "layout" </> "symbols.main.exe.txt"

normalRelinkUndefined, normalRelinkTailAsm, normalRelinkTailObject :: FilePath
normalRelinkUndefined = normalRelinkDir </> "layout" </> "undefined_symbols_auto.main.exe.txt"
normalRelinkTailAsm = normalRelinkDir </> "layout" </> "75F64.bss.s"
normalRelinkTailObject = normalRelinkDir </> "obj" </> "75F64.bss.s.o"

-- | The modded (non-matching) build: hooked functions patched in place by
-- tools/mkmod.py, so it stays the same size as main.exe (disc rebuild is faithful).
mainModExe :: FilePath
mainModExe = buildDir </> "tenchu" </> "main_mod.exe"

-- | Every executable on the disc. `main.exe` is the one we are decompiling; the
-- rest are still whole -- their splat config (tools/newexe.py) splits each into a
-- single `data` blob and the build reassembles it byte-identically. They cost
-- nothing until you carve a function out of one, at which point the workflow is
-- exactly main.exe's.
data Target = Target
  { tgName :: String,      -- ^ e.g. @"main.exe"@; also the gen/build subdirectory
    tgImage :: FilePath,   -- ^ the original image we must reproduce
    tgSha :: String        -- ^ sha256 of that image, so `check` catches a swap
  }

targets :: [Target]
targets =
  [ Target "main.exe" ("disks" </> "tenchu" </> "main.exe")
      "0690a5c14ff3e975ebcb3de26e196a4dbb6afc992677d0de2cbcf86af9993558",
    Target "menu.exe" ("disks" </> "tenchu" </> "menu.exe")
      "5de759508d87fa3bd3122329eaaf00467ff5f4d964a1673699309f3b6983c5ab",
    Target "ending.exe" ("disks" </> "tenchu" </> "ending.exe")
      "b7ada48dbc9d78098bd2e9fbf8589bdbcb1c64d963abc6282cd65855bf2bb606",
    Target "trial.exe" ("disks" </> "tenchu" </> "trial.exe")
      "f236f35ad8d4bed8c70f6cc1e69b62e7dfd27469e6bf1a838aa65932335d7230",
    Target "run.exe" ("disks" </> "tenchu" </> "run.exe")
      "8b7d7da5be9b78688e468b1319686f1e36a0fe874f897e5608cb2038340d2caf",
    Target "slps_019.01" ("disks" </> "slps_019.01")
      "03f3106094be75b99844f7e111712be4691798fc34fbaf5fd30d1a11e238b332"
  ]

mainTarget :: Target
mainTarget = head targets

tgGenDir, tgGen, tgExe, tgElf, tgSplat, tgSymbols, tgBuildDir, tgSrcDir :: Target -> FilePath
tgGenDir t = genDir </> tgName t
tgGen t = genDir </> tgName t <.> "done"
tgExe t = buildDir </> "tenchu" </> tgName t
tgElf t = tgExe t <.> "elf"
tgSplat t = configDir </> "splat." <> tgName t <> ".yaml"
tgSymbols t = configDir </> "symbols." <> tgName t <> ".txt"
tgBuildDir t = buildDir </> tgName t
tgSrcDir t = srcDir </> tgName t

-- | Bootable disc images repacked from our exe. Matching, same-slot mod, and
-- normal-relink builds get distinct names so each can be a real Shake target.
tenchuBin, tenchuCue, tenchuModBin, tenchuModCue,
  tenchuRelinkBin, tenchuRelinkCue :: FilePath
tenchuBin = buildDir </> "tenchu" </> "tenchu.bin"
tenchuCue = buildDir </> "tenchu" </> "tenchu.cue"
tenchuModBin = buildDir </> "tenchu" </> "tenchu-mod.bin"
tenchuModCue = buildDir </> "tenchu" </> "tenchu-mod.cue"
tenchuRelinkBin = buildDir </> "tenchu" </> "tenchu-relink.bin"
tenchuRelinkCue = buildDir </> "tenchu" </> "tenchu-relink.cue"

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

ccDefault :: FilePath
ccDefault = "cc1-281"

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
    extra "bow_shoot_logic" = ["--expand-div"]
    extra "Think3escape" = ["--expand-div"]
    extra "think_setting_small_rotation_small_steps_" = ["--expand-div"]
    extra "UpdateTexScroll" = ["--expand-div"]
    extra "DrawSprite" = ["--expand-div"]
    extra "DrawSpriteXYZ" = ["--expand-div"]
    extra "ComputeAreaLevel" = ["--expand-div"]
    extra "PadProc" = ["--expand-div"]
    extra "IsVisible" = ["--expand-div"]
    extra "PutLifeBar" = ["--expand-div"]
    extra "SetSmoke" = ["--expand-div"]
    extra "SetBleeds" = ["--expand-div"]
    extra "SetBlood" = ["--expand-div"]
    extra "SetHinoko" = ["--expand-div"]
    extra "SetupFly" = ["--expand-div"]
    extra "SetBleedsDir" = ["--expand-div"]
    extra "FUN_80037e0c" = ["--expand-div"]
    extra "FUN_8003a148" = ["--expand-div"]
    extra "FUN_80039fb0" = ["--expand-div"]
    extra "MoveFly" = ["--expand-div"]
    extra "DrawBleed" = ["--expand-div"]
    extra "DrawFrame" = ["--expand-div"]
    extra "DrawAfterimage" = ["--expand-div"]
    extra "UpdateSplineControl" = ["--expand-div"]
    extra "DrawFlyWire" = ["--expand-div"]
    extra "MakeDifSub" = ["--expand-div"]
    extra "FUN_80039ddc" = ["--expand-div"]
    extra "FUN_8004c59c" = ["--expand-div"]
    extra "FUN_8004d6d4" = ["--expand-div"]
    extra "SuccessionAttack" = ["--expand-div"]
    extra "GetSpline" = ["--expand-div"]
    extra "StateTransition" = ["--expand-div"]
    extra "AttackIndirect" = ["--expand-div"]
    extra "AttackLong" = ["--expand-div"]
    extra "AttackGeneral" = ["--expand-div"]
    extra "AttackShort" = ["--expand-div"]
    extra "DamageControl" = ["--expand-div"]
    extra "ControlHumanoid" = ["--expand-div"]
    extra "ProcItemNapalm" = ["--expand-div"]
    extra "ProcItemFire" = ["--expand-div"]
    extra "ProcItemJirai" = ["--expand-div"]
    extra "DrawBlood" = ["--expand-div"]
    extra "FUN_8003562c" = ["--expand-div"]
    extra "SwimCheck" = ["--expand-div"]
    extra "ProcItemArrow" = ["--expand-div"]
    extra "SetupBG" = ["--expand-div"]
    extra "SetWire" = ["--expand-div"]
    extra "SetLightningI" = ["--expand-div"]
    extra "register_character_death" = ["--expand-div"]
    extra "SweepMotion" = ["--expand-div"]
    extra "Think3attack" = ["--expand-div"]
    extra "DrawSplash" = ["--expand-div"]
    extra "SetFlyWire" = ["--expand-div"]
    extra "FUN_8002fd9c" = ["--expand-div"]
    extra "ArrangeLocalMatrix" = ["--expand-div"]
    extra "FUN_80033bc0" = ["--expand-div"]
    extra "ActDEAD" = ["--expand-div"]
    extra "ActSTICKON" = ["--expand-div"]
    extra "SetSmokeS" = ["--expand-div"]
    extra "FUN_80033f10" = ["--expand-div"]
    extra "DrawSnow" = ["--expand-div"]
    extra "DrawImpact" = ["--expand-div"]
    extra "FUN_80036284" = ["--expand-div"]
    extra "FUN_8003d768" = ["--expand-div"]
    extra _ = []
    -- Think1sleep.c is a fragment of the original think TU, which defines these.
    syms "Think1sleep" = ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "DrawSprite" = ["OTablePt"]
    syms "SetImpact" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "GetAreaMapVector" = ["FieldAttrib", "FieldArea", "FieldIndex"]
    syms "SetSnow" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_80018f00" = ["AccessPower"]
    syms "valloc" = ["virtual_memory_pool"]
    syms "SetupCharacterParameter" = ["NowStage"]
    syms "AdtMessageBox" = ["AdtPadRead", "AdtMessageBoxCount"]
    syms "DrawModelArchive" = ["SkipFrame", "OTablePt"]
    syms "Camera" = ["Projection"]
    syms "vmemoryGC" = ["virtual_memory_pool"]
    syms "ComputeAllConflict" = ["ConflictObjects"]
    syms "PlayVoice" = ["D_80097CA0", "D_80097CA4", "D_80097CA8", "D_80097CAC", "D_80097C9C", "D_80097C98"]
    syms "PutStrain" = ["D_80097F68"]
    syms "Think3hitaway" = ["Distance", "SR", "Me_THINK_C", "Degree", "Attrib"]
    syms "AttackContinuousCheck" = ["dtM", "Me_MOTION_C"]
    syms "Think4abandon" = ["Me_THINK_C", "Attrib", "SR", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "WeaponHitWeapon" = ["Me_MOTION_C", "dtM"]
    syms "dispose_weapon_data_of_char_" = ["Me_MOTION_C", "dtM"]
    syms "SoundEx" = ["StageSE"]
    syms "SetSmoke" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetBleeds" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "EndDrawing" = ["GameClock", "SkipFrame", "DrawingPage", "D_800976B8", "OTablePt", "time"]
    syms "SetBlood" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetHinoko" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetBleedsDir" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_80037e0c" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "ChasetoTarget" = ["Me_THINK_C", "Attrib", "Distance"]
    syms "GetAreaMapPassage" = ["FieldArea", "FieldIndex"]
    syms "PlaySE" = ["voice"]
    syms "SetupTexScroll" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "D_80097F30", "D_80097F32"]
    syms "AVCameraSetup" = ["CVAnow", "CameraTarget"]
    syms "AVCameraControl" = ["CameraPanMode", "D_80097CC8", "CameraTarget"]
    syms "CVAsequence" = ["CVAdata", "CVAnow", "CameraTarget", "D_80097CCC", "D_80097CC0"]
    syms "CVAupdate" = ["CVAnow", "D_80097CCC", "CameraTarget", "CameraPanMode", "D_80097CC8", "CVAdata"]
    syms "CVArun" = ["D_80097CC0", "CVAnow"]
    syms "CVAsetup" = ["CVAdata"]
    syms "CreateHumanoid" = ["Humans"]
    -- PutItemList.c is also part of the info-view TU; it defines/uses both
    -- selected-item-kind smalls gp-relatively.
    syms "PutItemList" = ["SelectedItem", "ItemCursor"]
    -- PutMap.c is also part of the info-view TU; it DEFINES PutMapMode (the
    -- "PutMap latch" DoInfoViewProc.c also references) plus the wipe
    -- position pair.
    syms "PutMap" = ["PutMapMode", "D_80097F6C", "D_80097F70"]
    -- UpdateEvent.c is also part of the original STAGE.C TU.
    syms "UpdateEvent" = ["StageEvent", "StagePlayer"]
    syms "KillHumanoid" = ["Humans"]
    syms "GetNearestHumanoid" = ["Humans"]
    syms "JumpControl" = ["dtL", "dtM", "motID", "Me_MOTION_C", "motMODE", "dtV", "dtPAD"]
    syms "HangCheck" = ["Me_MOTION_C", "motID", "dtL", "dtR", "MotionUpdateMode", "motMODE"]
    syms "SetExplosion" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "Think1random" = ["Me_THINK_C", "Attrib"]
    syms "Think1chase" = ["Me_THINK_C"]
    syms "ItemUse" = ["Me_THINK_C", "Degree"]
    syms "Think4contact" = ["SR", "Attrib", "Me_THINK_C", "Degree"]
    syms "Think4chase" = ["SR", "Attrib", "Me_THINK_C", "Degree"]
    -- ReqItemDrop.c is part of the original item TU, which defines its
    -- round-robin counter (the item TU's .sdata block starts at 0x80097ac8).
    syms "ReqItemDrop" = ["COUNTER_FOR_ITEM_ARRAY_"]
    -- GetFreeItemSlot.c is also part of the original item TU (same counter).
    syms "GetFreeItemSlot" = ["COUNTER_FOR_ITEM_ARRAY_"]
    -- GetAreaMapLevel.c is part of the original area-map TU, which defines
    -- these small globals (.sdata around 0x80097ec0).
    syms "GetAreaMapLevel" = ["FieldIndex", "FieldArea", "D_80097EC0", "FieldAttrib"]
    -- DoInfoViewProc.c is part of the original info-view TU, which defines
    -- these smalls (fInitialize / item cursor / PutMap latch).
    syms "DoInfoViewProc" = ["fInitialize", "ItemCursor", "PutMapMode"]
    -- BriefingAndInventorySelectionScreen.c's original TU defines the one-shot
    -- capacity-cheat latch (.sdata, 0x80097cdc); SkipFrame stays absolute.
    syms "BriefingAndInventorySelectionScreen" = ["CARRY_30_ITEMS_CHEAT_APPLIED"]
    -- LayoutEnemyOption's TU defines this small (debug enemy-layout state).
    syms "LayoutEnemyOption" = ["CurrentEnemyID"]
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
    syms "ReqItemUse" = ["COUNTER_FOR_ITEM_ARRAY_", "SyurikenModel", "sprNapalm"]
    syms "ReqItemLaunch" = ["COUNTER_FOR_ITEM_ARRAY_", "SyurikenModel"]
    syms "ReqItemArrow" = ["COUNTER_FOR_ITEM_ARRAY_", "ArrowModel"]
    syms "ReqItemHappou" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ProcItemHappou" = ["HappouModel"]
    syms "Sound" = ["VoiceMode"]
    syms "DeleteConflict" = ["ConflictObjects"]
    syms "InsertConflict" = ["ConflictObjects"]
    syms "GetConflictResult" = ["ConflictObjects", "ConflictDistance", "ConflictModel"]
    syms "DisposeAreaMap" = ["GlobalAreaMap"]
    syms "NowReturnNormal" = ["Me_MOTION_C", "motID", "motMODE"]
    syms "vinit" = ["virtual_memory_pool"]
    syms "LoadAreaMap" = ["GlobalAreaMap", "FieldIndex", "FieldArea"]
    syms "DrawBG" = ["OTablePt"]
    syms "PrepareAccess" = ["AccessPower"]
    syms "handle_balmer_acm_" = ["GlobalAreaMap", "FieldIndex", "D_800976E8", "FieldArea"]
    syms "FUN_80027304" = ["Me_MOTION_C", "dtL"]
    syms "init_score_stats" = ["StageBosses", "StageEnemies", "Findenemies", "Murders", "Criticals", "FriendHits"]
    syms "is_character_state_present_on_stage_" = ["Humans"]
    syms "Think2contact" = ["Attrib", "Me_THINK_C", "Degree"]
    syms "update_something_for_each_visible_enemy_" = ["VISIBLE_ENEMIES_"]
    syms "turn_towards_player_" = ["Me_THINK_C", "Degree", "Attrib", "D_80097F10"]
    syms "Think1trace" = ["Me_THINK_C", "Degree", "Attrib"]
    syms "Think3chase" = ["Distance", "SR", "EngageLevel", "AttackActionCount", "Degree", "Me_THINK_C"]
    syms "handle_char_state_attacking_SEVEN_" = ["dtM", "Me_MOTION_C", "dtR"]
    syms "bow_shoot_logic" = ["Me_MOTION_C", "dtR"]
    syms "Think1watch" = ["Me_THINK_C"]
    syms "Think3firstattack" = ["Distance", "SR", "Me_THINK_C", "Attrib", "Degree"]
    syms "Think3escape" = ["Distance", "SR", "Degree", "Attrib", "Me_THINK_C"]
    syms "Think1ninja" = ["Me_THINK_C"]
    syms "GetArcData" = ["MODEL_ARCHIVE_PTR"]
    syms "AttackFire" = ["dtM", "Me_MOTION_C", "dtR"]
    syms "ReturnNormal" = ["Me_MOTION_C", "motID", "motMODE"]
    syms "DrawOrnament" = ["OTablePt"]
    syms "FUN_8005fe88" = ["D_80097E98"]
    syms "SetupStageSequence" = ["StageEvent", "StagePlayer"]
    syms "AttackGunControl" = ["dtM", "Me_MOTION_C"]
    syms "ThinkBasicHuman1" = ["Me_THINK_C"]
    syms "StartDrawing" = ["DrawingPage", "OTablePt", "GameClock"]
    syms "AttackPQD" = ["Me_MOTION_C", "dtM"]
    syms "SetupSoundEffect" = ["StageSE"]
    syms "initialise_default_player_cameras_" = ["CAMERA_PTR_ARRAY_START"]
    syms "vgetfreesize" = ["virtual_memory_pool"]
    syms "vgetmaxsize" = ["virtual_memory_pool"]
    syms "InitAccessInfo" = ["AccessPower"]
    syms "GetHumanoid" = ["Humans"]
    syms "AttackAnimal" = ["Me_THINK_C", "Distance", "Degree"]
    syms "StickonCheck" = ["Me_MOTION_C", "dtL", "motID", "motMODE"]
    syms "DeleteCard" = ["TENCHU_ID"]
    syms "LoadCard" = ["TENCHU_ID"]
    syms "SaveCard" = ["TENCHU_ID"]
    syms "SetupAfterimage" = ["AfterIMG"]
    syms "MotionAndMove" = ["MotionUpdateMode", "Me_MOTION_C", "motID", "motMODE"]
    syms "FileRead" = ["AccessPower", "ReadMode", "TotalIO"]
    syms "LoadFromDEVPC" = ["TotalIO", "ReadMode", "MemoryLoadAddress"]
    syms "LoadFromCDROM" = ["TotalIO", "ReadMode", "MemoryLoadAddress"]
    syms "SetFrame" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetSplash" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetBleed" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "AttackBowControl" = ["dtM", "Me_MOTION_C"]
    syms "HumanActionControl" = ["Me_MOTION_C", "dtPAD", "dtCMD", "motMODE", "dtV", "dtL", "dtR", "dtM", "motID"]
    syms "AttackCancelControl" = ["Me_MOTION_C", "dtM"]
    syms "DoItemProc" = ["D_80097AC8"]
    syms "vfree" = ["virtual_memory_pool"]
    syms "DrawModel" = ["OTablePt"]
    syms "FUN_8001b2f4" = ["D_800976F6"]
    syms "LoadMotion" = ["MotionPack"]
    syms "SearchMotion" = ["CommonMotion", "PlayerMotion", "StageMotion"]
    syms "GetSpline" = ["D_80097708", "D_80097EEC", "D_80097EE8"]
    syms "GetImage" = ["D_80097C90"]
    syms "InitConflict" = ["ConflictModel", "ConflictObjects"]
    syms "ControlAllHumanoid" = ["Humans", "VISIBLE_ENEMIES_"]
    syms "ControlHumanoid" = ["VISIBLE_ENEMIES_"]
    syms "SuccessionAttack" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel"]
    syms "Think3callaid" = ["Distance", "SR", "Me_THINK_C", "Degree", "Pad", "Attrib"]
    syms "InitFileSystem" = ["ReadMode", "TotalIO", "D_80097EB8"]
    syms "cbAccess" = ["AccessPower"]
    syms "FUN_8005fe38" = ["D_80097E98"]
    syms "FUN_80056e30" = ["TENCHU_ID"]
    syms "FUN_80038fdc" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_8003944c" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "InitMisc" = ["EFFECT_SPAWNERS_INITIALISED"]
    syms "InitializeItem" = ["SyurikenModel", "ArrowModel", "NingyoModel", "HappouModel", "sprNapalm", "sprNapalm2", "D_80097AC8"]
    syms "ProcItemNapalm" = ["sprNapalm2"]
    syms "DoMiscProc" = ["EFFECT_SPAWNERS_INITIALISED"]
    syms "LoadSI" = ["CID"]
    syms "SaveSI" = ["CID"]
    syms "InitializeInfoView" = ["fInitialize"]
    syms "ActSYURI" = ["dtM", "Me_MOTION_C", "motID", "motMODE"]
    syms "ActHANG" = ["dtV", "dtM", "dtPAD", "dtL", "motID", "motMODE", "Me_MOTION_C"]
    syms "ReqItemGun" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "SetSmokeS" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetupAppearance" = ["NowStage", "PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR", "D_800979A6", "AMD_LOADED_FOR_CHARACTER_KIND"]
    syms "StateTransition" = ["StrainRatio", "Me_THINK_C", "Pad", "Attrib", "D_80097F1C", "ActionHalt", "FRAMES_UNTIL_END_OF_ALERT", "SR", "Distance", "D_80097F10", "D_80097F18", "D_80097F14", "EngageLevel", "Degree"]
    syms "ActATTACK" = ["dtM", "Me_MOTION_C", "motID", "motMODE", "dtR", "dtL", "dtPAD", "MotionUpdateMode", "dtV"]
    syms "AttackIndirect" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "SR"]
    syms "AttackLong" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"]
    syms "AttackGeneral" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"]
    syms "AttackShort" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"]
    syms "DamageControl" = ["Me_MOTION_C", "MotionUpdateMode", "motID", "dtL", "motMODE", "dtM", "dtR", "D_8009770C", "dtV"]
    syms "ProcItemNinken" = ["NINKEN_CHARACTER_PTR"]
    syms "ActKAGI" = ["dtM", "Me_MOTION_C", "dtL", "motMODE", "motID", "dtR", "MotionUpdateMode", "dtV"]
    syms "FUN_8003562c" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "ProcItemHenshin" = ["D_80097AEC", "D_80097AF0"]
    syms "ActJUMP" = ["Me_MOTION_C", "motID", "dtL", "dtM", "dtV", "dtR", "MotionUpdateMode", "motMODE", "dtPAD"]
    syms "SwimCheck" = ["Me_MOTION_C", "motID", "dtM", "dtL", "motMODE", "MotionUpdateMode"]
    syms "ActivateHumans" = ["D_80097F40", "D_80097F44", "D_80097F42", "StageID"]
    syms "ProcItemNingyo" = ["D_80097AE0", "NingyoModel"]
    syms "StartStageSequence" = ["StageEvent", "StageTime", "FriendHits", "Murders", "Findenemies", "Criticals", "D_80097F7C", "StagePlayer", "StageCitizens", "StageEnemies", "StageBosses"]
    syms "StageSequence" = ["StagePlayer", "D_80097F78", "D_80097F7C", "StageTime", "Findenemies", "Murders", "Criticals", "StageEnemies", "StageBosses", "FriendHits", "StageCitizens"]
    syms "AddEnemy" = ["CurrentEnemyID"]
    syms "FUN_8005b17c" = ["D_80097D38", "D_80097D24", "D_80097D3C", "D_80097D40", "D_80097D28", "CardStateFlag"]
    syms "LoadConstruction" = ["D_80097A70", "D_80097A74", "StageID"]
    syms "CreateStage" = ["StageID"]
    syms "SetWire" = ["ModelHook"]
    syms "think_setting_small_rotation_small_steps_" = ["Me_THINK_C", "Attrib", "FRAMES_UNTIL_END_OF_ALERT", "Degree", "Distance"]
    syms "ActCHASE" = ["Me_MOTION_C", "dtM", "dtPAD", "MotionUpdateMode", "motID", "motMODE", "dtL", "dtR", "dtCMD"]
    syms "ActENGAGE" = ["dtM", "dtV", "dtPAD", "motID", "Me_MOTION_C", "motMODE", "dtL", "dtCMD", "dtR"]
    syms "DrawShadow" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "D_80097F34"]
    syms "register_character_death" = ["D_800979DE", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "Think3attack" = ["Me_THINK_C", "SR", "Distance", "Degree", "EngageLevel"]
    syms "SetFlyWire" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_80035f44" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "InitEffect" = ["D_80097F34", "LOCAL_COORDINATES_", "AfterIMG", "ModelHook", "D_80097F30", "D_80097F32"]
    syms "FUN_80033bc0" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "AttackControl" = ["Me_MOTION_C", "dtL", "dtR", "motID", "motMODE", "dtPAD", "dtM"]
    syms "Think1target" = ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "FUN_8005adbc" = ["D_80097D20", "D_80097D24", "D_80097D28"]
    syms "ActDEAD" = ["Me_MOTION_C", "dtM", "dtV", "motID", "dtL", "motMODE", "D_8009770C"]
    syms "ActSTICKON" = ["dtM", "Me_MOTION_C", "dtR", "dtCMD", "motID", "motMODE", "MotionUpdateMode", "dtPAD", "dtV", "D_80097EF0", "dtL"]
    syms "ActACTION" = ["dtM", "Me_MOTION_C", "dtV", "dtPAD", "MotionUpdateMode", "motID", "motMODE"]
    syms "ActDAMAGE" = ["dtM", "Me_MOTION_C", "dtV", "motID", "motMODE", "dtL"]
    syms "ActITEM" = ["dtM", "Me_MOTION_C", "motID", "motMODE"]
    syms "ActMOVE" = ["dtM", "Me_MOTION_C", "dtPAD", "motID", "motMODE", "dtL", "dtR"]
    syms "ActNORMAL" = ["dtM", "dtPAD", "Me_MOTION_C", "motID", "motMODE", "dtR", "dtCMD"]
    syms "ActSQUAT" = ["Me_MOTION_C", "dtM", "dtPAD", "dtR", "motID", "motMODE", "dtV", "dtL", "dtCMD"]
    syms "ActSTATE" = ["dtM", "MotionUpdateMode", "motID", "motMODE", "Me_MOTION_C", "dtV", "dtPAD", "dtR", "dtL"]
    syms "ActSWIM" = ["dtM", "motID", "motMODE", "dtPAD", "Me_MOTION_C", "dtR", "dtV", "dtL"]
    syms "FUN_8005a7a4" = ["D_80097D2E", "D_80097D30", "D_80097D18", "D_80097D32", "CardStateFlag"]
    syms "FUN_8005aba4" = ["CardRetryCount", "CardStateFlag"]
    syms "FallCheck" = ["motID", "Me_MOTION_C", "dtM", "dtL", "motMODE", "MotionUpdateMode"]
    syms "ItemControl" = ["Me_MOTION_C", "motID", "motMODE"]
    syms "character_balma_around_main_routine_" = ["D_800976E8", "GlobalAreaMap", "FieldIndex", "FieldArea"]
    syms "reset_alert_duration" = ["FRAMES_UNTIL_END_OF_ALERT"]
    syms "Think3area" = ["Me_THINK_C", "Distance", "SR", "Attrib", "Degree"]
    syms "create_ninken_character_" = ["NINKEN_CHARACTER_PTR"]
    syms "death_camera_something_" = ["LOCAL_COORDINATES_"]
    syms "debug_output_edit_camera_settings" = ["BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT", "BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT", "DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX", "CAMERA_PTR_ARRAY_START"]
    syms "debug_menu_file_animation_test" = ["CVAdata"]
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
    -- cc1, not cpp, decides whether `abs()` inlines. This lived in cppFlags
    -- for a long time, where it did nothing.
    "-fno-builtin",
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

-- Stock PsyQ library objects were not necessarily built with the game's
-- translation-unit defaults. Our one-function C files are an artificial split,
-- so flags are selected ONLY after mapping a carve back to its original object.
-- Never add a function directly to a compiler-option table.
--
-- LIBMCRD.OBJ spans the whole MemCardStart..MemCardFormat range; only
-- MemCardCallback is C-carved today. GS_107.OBJ contains GsSetFlatLight and the
-- three offset-named pieces below; only its 0x4B8/0x51C leaves are C-carved.
-- Listing every known member makes the policy automatic when another piece is
-- converted, rather than silently turning an object option into function tuning.
--
-- The target itself shows unsplit reusable bases throughout both objects. In
-- GS_107, the target-only 0x0/0x444 portions and both C leaves all materialise
-- their object globals through the same `lui`/`addiu` address form.
libmcrdObjectMembers :: [String]
libmcrdObjectMembers =
  [ "MemCardStart",
    "MemCardStop",
    "MemCardExist",
    "FUN_80080f28",
    "MemCardAccept",
    "FUN_80081164",
    "MemCardOpen",
    "MemCardClose",
    "MemCardReadData",
    "FUN_8008161c",
    "MemCardWriteData",
    "FUN_80081810",
    "MemCardReadFile",
    "FUN_80081a64",
    "MemCardWriteFile",
    "FUN_80081c84",
    "MemCardGetDirentry",
    "MemCardCallback",
    "MemCardSync",
    "MemCardCreateFile",
    "MemCardFormat"
  ]

gs107ObjectMembers :: [String]
gs107ObjectMembers =
  [ "GsSetFlatLight",
    "GS_107_OBJ_444",
    "GS_107_OBJ_4B8",
    "GS_107_OBJ_51C"
  ]

-- GS_105.OBJ is a complete one-public-member object. The natural TMD mapper is
-- byte-invariant across the pinned 2.7.2, 2.8.0, and default 2.8.1 compilers,
-- so it needs neither an older compiler attribution nor exceptional options.
gs105ObjectMembers :: [String]
gs105ObjectMembers =
  [ "GsMapModelingData"
  ]

-- PsyQ's archive table proves GS_106.OBJ has exactly this one public member;
-- wibo-extracting the real object proves its complete text is byte-identical to
-- Tenchu. Its natural wrapper source has the same older epilogue under 2.7.2.
gs106ObjectMembers :: [String]
gs106ObjectMembers =
  [ "GsSetProjection"
  ]

-- These are likewise complete one-public-member objects in the real archive.
-- Their extracted text is byte-identical to Tenchu, and natural SDK C
-- reproduces it under the same released 2.7.2 compiler as GS_106.OBJ.
gs110ObjectMembers :: [String]
gs110ObjectMembers =
  [ "GsSetAmbient"
  ]

gs111ObjectMembers :: [String]
gs111ObjectMembers =
  [ "GsDrawOt"
  ]

gs113ObjectMembers :: [String]
gs113ObjectMembers =
  [ "GsClearOt"
  ]

-- GS_119.OBJ has one public member and one private return label. Its natural
-- row-major Z-rotation source reproduces the complete function with the
-- default 2.8.x compiler profile and no exceptional options.
gs119ObjectMembers :: [String]
gs119ObjectMembers =
  [ "gte_rotate_z_matrix"
  ]

-- GS_121.OBJ has one public member and its complete 0x50-byte text is
-- Tenchu's gte_init slot. The natural initializer uses the same 2.7.2
-- epilogue as the neighbouring proven libgs objects.
gs121ObjectMembers :: [String]
gs121ObjectMembers =
  [ "gte_init"
  ]

-- GS_122.OBJ also has one public member. Its complete 0xF0-byte text,
-- including the local return label and trailing alignment, is Tenchu's slot.
gs122ObjectMembers :: [String]
gs122ObjectMembers =
  [ "GsGetTimInfo"
  ]

-- GS_123.OBJ has one public member. Its private switch labels and jump table
-- remain part of the same object/profile, never function-level tuning.
gs123ObjectMembers :: [String]
gs123ObjectMembers =
  [ "Gssub_make_matrix"
  ]

-- GS_125.OBJ is a complete one-public-member object too. The natural getter
-- is byte-invariant between 2.7.2 and the default 2.8.1, so it needs no older
-- compiler attribution or exceptional options.
gs125ObjectMembers :: [String]
gs125ObjectMembers =
  [ "GsGetWorkBase"
  ]

originalObjectCcFlags :: FilePath -> [String]
originalObjectCcFlags src
  | name `elem` libmcrdObjectMembers = ["-mno-split-addresses"]
  | name `elem` gs105ObjectMembers = []
  | name `elem` gs106ObjectMembers = []
  | name `elem` gs110ObjectMembers = []
  | name `elem` gs111ObjectMembers = []
  | name `elem` gs113ObjectMembers = []
  | name `elem` gs119ObjectMembers = []
  | name `elem` gs121ObjectMembers = []
  | name `elem` gs122ObjectMembers = []
  -- Undo the game-wide -funsigned-char for this complete vendor object. The
  -- source's ordinary char axis is signed under the SDK object's defaults.
  | name `elem` gs123ObjectMembers = ["-fsigned-char"]
  | name `elem` gs125ObjectMembers = []
  | name `elem` gs107ObjectMembers = ["-mno-split-addresses"]
  | name `elem` adtObjectMembers = []
  | otherwise = []
  where
    name = takeBaseName src

-- ADT's eleven contiguous C functions are one reused library object. Every
-- member remains byte-exact under GCC 2.8.0; GCC 2.8.1 changes one reload retype
-- and leaves AdtSelect nine bytes off. Keep the complete object membership here
-- so future tooling cannot turn the compiler attribution into function tuning.
adtObjectMembers :: [String]
adtObjectMembers =
  [ "AdtGetDisp",
    "AdtMessageBox",
    "AdtQuiet",
    "AdtFntOpen",
    "AdtFntLoad",
    "AdtReleaseDisp",
    "AdtDmyPadRead",
    "AdtVsprintf",
    "FUN_8005fe38",
    "FUN_8005fe88",
    "AdtSelect"
  ]

originalObjectCcExecutable :: FilePath -> FilePath
originalObjectCcExecutable src
  | takeBaseName src `elem` gs106ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs110ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs111ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs113ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs121ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs122ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs123ObjectMembers = "cc1-272"
  | takeBaseName src `elem` gs107ObjectMembers = "cc1-281-gs107"
  | takeBaseName src `elem` adtObjectMembers = "cc1-280"
  | otherwise = ccDefault

-- | All varying compiler inputs are one original-object profile. Returning the
-- executable and flags through one oracle makes either change invalidate .s.
originalObjectCcProfile :: FilePath -> (FilePath, [String])
originalObjectCcProfile src =
  (originalObjectCcExecutable src, originalObjectCcFlags src)

cppFlags :: [String]
cppFlags =
  [ "-Iinclude",
    "-undef",
    "-Wall",
    "-lang-c",
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

-- | Matching tools may compile a staged candidate without rewriting the
-- checked-in source file.  The variable is deliberately per function: changing
-- one candidate invalidates only that file's preprocessing rule, rather than
-- making every C object depend on one global override value.
--
-- This is an internal tool interface, e.g.
-- @TENCHU_MATCH_SOURCE_ProcItemFire=.shake/autorules-.../ProcItemFire.c@.
matchingSource :: FilePath -> Action FilePath
matchingSource src = do
  override <- getEnv ("TENCHU_MATCH_SOURCE_" <> takeBaseName src)
  pure (maybe src id override)

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

-- | Read the two transformed and four ordinary objects. The focused oracle
-- pins exact offsets/sizes; the normal-link gate deliberately permits layout
-- changes while retaining relocation, opcode, and raw-literal checks.
verifyRelocCLiteralObjectsWith :: Bool -> Action ()
verifyRelocCLiteralObjectsWith allowLayoutChanges = do
  let tool = "tools" </> "reloc_c_literals.py"
      objectArgs = concatMap
        (\name -> ["--object", name <> "=" <> relocCLiteralAuditObject name])
        relocCLiteralAuditNames
      layoutArgs =
        if allowLayoutChanges then ["--allow-layout-changes"] else []
  need $ [tool, ramLayoutTool, ramLayoutHeader] <> relocCLiteralAuditObjects
  cmd_ "python3" tool (["verify-objects"] <> layoutArgs <> objectArgs)

verifyRelocCLiteralObjects :: Action ()
verifyRelocCLiteralObjects = verifyRelocCLiteralObjectsWith False

verifyRelocCLiteralLink :: Action ()
verifyRelocCLiteralLink = do
  let tool = "tools" </> "reloc_c_literals.py"
      objectArgs = concatMap
        (\name -> ["--object", name <> "=" <> relocCLiteralAuditObject name])
        relocCLiteralAuditNames
  need $ [ tool, ramLayoutTool, ramLayoutHeader, mainRelocGameElf,
           mainRelocCLiteralElf, mainRelocCLiteralExe ] <>
    relocCLiteralAuditObjects
  cmd_ "python3" tool
    ([ "verify-linked",
       "--base-elf", mainRelocGameElf,
       "--variant-elf", mainRelocCLiteralElf
     ] <> objectArgs)

verifyNormalRelink :: Action ()
verifyNormalRelink = do
  let tool = "tools" </> "reloc_c_literals.py"
      objectArgs = concatMap
        (\name -> ["--object", name <> "=" <> relocCLiteralAuditObject name])
        relocCLiteralAuditNames
      referenceArgs = concatMap
        (\name ->
          [ "--reference-object",
            name <> "=" <> relocCLiteralReferenceObject name
          ])
        relocCLiteralNames
  need $ [ tool, ramLayoutTool, ramLayoutHeader, mainRelocBssElf,
           mainRelinkElf, normalRelinkCLinker ] <>
    relocCLiteralAuditObjects <> relocCLiteralReferenceObjects
  cmd_ "python3" tool
    ([ "verify-normal-link",
       "--base-elf", mainRelocBssElf,
       "--variant-elf", mainRelinkElf,
       "--linker", normalRelinkCLinker
     ] <> referenceArgs <> objectArgs)

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
            -- "3": one-time bump so every .s rule recorded the GpFlags oracle
            -- dependency. "4": recompile after adding the LIBMCRD-specific
            -- cc flag; command-line contents are not otherwise tracked.
            -- "6": compiler executable is now part of the per-object profile;
            -- the reused ADT object selects pinned GCC 2.8.0.
            -- "7": the normal-link C replacement inventory became explicit.
            -- "8": the replacement recipe now transforms the two allocator
            -- assembly streams; four exact symbolic objects are audited in place.
            shakeVersion = "8"
          }
  shakeArgs opts rules

rules :: Rules ()
rules = do
  _ <- addOracle (liftIO . runIdOracle)
  _ <- addOracle (\(GpFlags f) -> pure (maspsxGpExterns f))
  _ <- addOracle (\(OriginalObjectCcProfile f) -> pure (originalObjectCcProfile f))
  want [mainExe]
  objRules
  mapM_ exeRules targets
  mainExtraRules
  phonyRules

objRules :: Rules ()
objRules = do
  -- Compile the same preprocessed source used by the matching lane.  The
  -- allocator assembly rule below changes only the two reviewed constant
  -- materialisations; every source-level spelling remains identical.
  relocCLiteralDir </> "*.i" %> \out -> do
    let name = takeBaseName out
        src = srcDir </> "main.exe" </> name <.> "c"
        header = srcDir </> "main.exe" </> "main.exe.h"
    when (name `notElem` relocCLiteralNames) $
      fail $ "unexpected relocatable-C input " <> name
    need [src]
    orderOnly [header]
    liftIO $ IO.createDirectoryIfMissing True relocCLiteralDir
    withTempFile $ \makeOut -> do
      cmd_ cpp
        (cppFlags <>
          [ "-MMD", "-MF", makeOut,
            "-I", takeDirectory header
          ])
        src out
      neededMakefileDependencies makeOut

  relocCLiteralDir </> "*.s" %> \out -> do
    let name = takeBaseName out
        processed = relocCLiteralPreprocessed name
        src = srcDir </> "main.exe" </> name <.> "c"
        tool = "tools" </> "reloc_c_literals.py"
    when (name `notElem` relocCLiteralNames) $
      fail $ "unexpected relocatable-C assembly " <> name
    need [processed, tool, ramLayoutTool, ramLayoutHeader]
    gpFlags <- askOracle (GpFlags src)
    (ccExe, objectCc) <- askOracle (OriginalObjectCcProfile src)
    withTempFile $ \ccOut ->
      withTempFile $ \maspsxOut -> do
        cmd_ (FileStdin processed) (FileStdout ccOut)
          ccExe (ccFlags <> objectCc)
        cmd_ (FileStdin ccOut) (FileStdout maspsxOut)
          maspsx (maspsxFlags <> gpFlags)
        if name `elem` relocAllocatorLiteralNames
          then cmd_ "python3" tool
            [ "relocate-allocator-assembly",
              "--name", name,
              "--input", maspsxOut,
              "--output", out
            ]
          else copyFileChanged maspsxOut out

  relocCLiteralDir </> "*.o" %> \out -> do
    let name = takeBaseName out
        assembly = relocCLiteralAssembly name
    when (name `notElem` relocCLiteralNames) $
      fail $ "unexpected relocatable-C object " <> name
    need [assembly]
    trackAllow ["include/*.inc"]
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, assembly]
      neededAsmDeps depFile

  -- Normal-relink function overrides: the same pipeline as matched game C,
  -- compiled from src/mod-relink/main.exe into an isolated directory so the
  -- exact lanes keep their pristine objects.
  modRelinkDir </> "*.i" %> \out -> do
    let name = takeBaseName out
        src = modRelinkSrcDir </> name <.> "c"
        header = srcDir </> "main.exe" </> "main.exe.h"
    need [src]
    orderOnly [header]
    liftIO $ IO.createDirectoryIfMissing True modRelinkDir
    withTempFile $ \makeOut -> do
      cmd_ cpp
        (cppFlags <>
          [ "-MMD", "-MF", makeOut,
            "-I", takeDirectory header
          ])
        src out
      neededMakefileDependencies makeOut

  modRelinkDir </> "*.s" %> \out -> do
    let name = takeBaseName out
        processed = modRelinkPreprocessed name
        src = modRelinkSrcDir </> name <.> "c"
    need [processed]
    gpFlags <- askOracle (GpFlags src)
    (ccExe, objectCc) <- askOracle (OriginalObjectCcProfile src)
    withTempFile $ \ccOut -> do
      cmd_ (FileStdin processed) (FileStdout ccOut) ccExe (ccFlags <> objectCc)
      cmd_ (FileStdin ccOut) (FileStdout out) maspsx (maspsxFlags <> gpFlags)

  modRelinkDir </> "*.o" %> \out -> do
    let name = takeBaseName out
        assembly = modRelinkAssembly name
    need [assembly]
    trackAllow ["include/*.inc"]
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, assembly]
      neededAsmDeps depFile

  -- Debug-ELF C objects: the same per-file pipeline as the retail object
  -- (same cpp output, gp-extern list, and original-object cc profile), but
  -- cc1 gets -gstabs and the maspsx output is filtered to line-only stabs.
  -- -gstabs does not change code, so these are byte-identical in .text.
  debugObjDir <//> "*.c.o" %> \out -> do
    let cRel = dropExtension (makeRelative debugObjDir out)   -- e.g. ProcItemKusuri.c
        processed = processedDir </> "main.exe" </> cRel      -- cpp output (.c)
        src = srcDir </> "main.exe" </> cRel
        filterTool = "tools" </> "stabs_lines.py"
    need [processed, filterTool]
    gpFlags <- askOracle (GpFlags src)
    (ccExe, objectCc) <- askOracle (OriginalObjectCcProfile src)
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    trackAllow ["include/*.inc"]
    withTempFile $ \ccOut ->
      withTempFile $ \masOut ->
        withTempFile $ \filtered -> do
          cmd_ (FileStdin processed) (FileStdout ccOut) ccExe
            (ccFlags <> objectCc <> ["-gstabs"])
          cmd_ (FileStdin ccOut) (FileStdout masOut) maspsx (maspsxFlags <> gpFlags)
          cmd_ (FileStdin masOut) (FileStdout filtered) "python3" [filterTool]
          withTempFile $ \depFile -> do
            cmd_ (FileStdin filtered) as asFlags ["--MD", depFile, "-o", out]
            neededAsmDeps depFile

  -- Real-edit regression fixture objects: the committed grown PadProc and its
  -- new translation unit, compiled by the identical pipeline into the
  -- isolated realedit lane.
  let realeditCompile srcName preprocessed assembly object = do
        preprocessed %> \out -> do
          let src = realeditFixtureDir </> srcName
              header = srcDir </> "main.exe" </> "main.exe.h"
          need [src]
          orderOnly [header]
          liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
          withTempFile $ \makeOut -> do
            cmd_ cpp
              (cppFlags <>
                [ "-MMD", "-MF", makeOut,
                  "-I", takeDirectory header
                ])
              src out
            neededMakefileDependencies makeOut
        assembly %> \out -> do
          let src = realeditFixtureDir </> srcName
          need [preprocessed]
          gpFlags <- askOracle (GpFlags src)
          (ccExe, objectCc) <- askOracle (OriginalObjectCcProfile src)
          withTempFile $ \ccOut -> do
            cmd_ (FileStdin preprocessed) (FileStdout ccOut) ccExe (ccFlags <> objectCc)
            cmd_ (FileStdin ccOut) (FileStdout out) maspsx (maspsxFlags <> gpFlags)
        object %> \out -> do
          need [assembly]
          trackAllow ["include/*.inc"]
          withTempFile $ \depFile -> do
            cmd_ as asFlags ["--MD", depFile, "-o", out, assembly]
            neededAsmDeps depFile
  realeditCompile "PadProc.c"
    realeditOverridePreprocessed realeditOverrideAssembly realeditOverrideObject
  realeditCompile "mod_probe.c"
    realeditExtPreprocessed realeditExtAssembly realeditExtObject

  [buildDir </> "*" </> "data" <//> "*.s.o", buildDir </> "*" </> "*.s.o"] |%> \out -> do
    let fileComponent = makeRelative buildDir out
        target = takeDirectory1 fileComponent
        src = genDir </> target </> asmDir </> dropDirectory1 (dropExtension fileComponent)
    need [src]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    -- A `textbin` subsegment assembles to `.incbin "<assets>/NNN.textbin.bin"`, and
    -- `as` reads that blob before --MD can tell us it did. neededAsmDeps declares it
    -- afterwards; allow exactly that read, as the .c.o rule does for macro.inc.
    trackAllow [genDir </> target </> assetsDir <//> "*"]
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, src]
      neededAsmDeps depFile

  [processedDir <//> "*.c", processedDir <//> "*.h"] |%> \out -> do
    let fileComponent = makeRelative processedDir out
        target = takeDirectory1 fileComponent
        targetGen = genDir </> target <.> "done"
        file = dropDirectory1 fileComponent
        header = srcDir </> target </> target <.> "h"
        genPath = genDir </> target </> "src" </> file
        srcPath = srcDir </> target </> file
    _generatedFiles <- getGeneratedFiles targetGen

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
    compileSrc <- matchingSource src
    need [targetGen, compileSrc]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    nm <- nonMatchingFlags src
    withTempFile $ \makeOut -> do
      cmd_ cpp (cppFlags <> nm <> ["-MMD", "-MF", makeOut, "-I", takeDirectory header]) compileSrc out
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
    (ccExe, objectCc) <- askOracle (OriginalObjectCcProfile processed)
    withTempFile $ \ccOut -> do
      cmd_ (FileStdin processed) (FileStdout ccOut) ccExe (ccFlags <> objectCc)
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
    -- `as` reads the INCLUDE_ASM'd nonmatchings .s (and include/macro.inc)
    -- before we can know which ones -- that is what --MD is for, and
    -- neededAsmDeps declares them afterwards with `needed`. --lint-fsatrace
    -- still sees a read-before-need, so allow exactly that directory.
    trackAllow [genDir </> target </> asmDir <//> "*", "include/*.inc"]
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

-- | Rules for one executable. Identical for every target: splat -> asm/C -> objects
-- -> elf -> raw binary. `main.exe` is the only one with C sources today; the others
-- are one `data` blob each, and everything below is oblivious to the difference.
exeRules :: Target -> Rules ()
exeRules t = do
  let gen = tgGen t
      genD = tgGenDir t
      exe = tgExe t
      elf = tgElf t
      tBuildDir = tgBuildDir t
      definedSymbols = tgSymbols t

  priority 2 $ generator gen [genD <//> "*"] $ do
    need [tgSplat t, tgImage t, definedSymbols]
    liftIO $ mapM_ (\d -> IO.createDirectoryIfMissing True (genD </> d)) splatDirs
    -- Every config sets `generate_asm_macros_files: no`, so splat writes nothing
    -- outside genD and the six targets can split concurrently. include/*.inc are
    -- checked-in sources; `as --MD` + `neededAsmDeps` picks them up as deps.
    cmd_ "split.py" [tgSplat t]
    trackAllow [genD <//> "*"]
    -- splat bakes `buildDir/<target>/<genDir>/...` into the linker script; we put
    -- objects directly under `buildDir/<target>`.
    let beforeAsm = tBuildDir </> genD </> asmDir
        beforeSrc = tBuildDir </> genD </> srcDir
        beforeAssets = tBuildDir </> genD </> assetsDir
        ldOut = genD </> linkerDir </> tgName t <.> "ld"
    cmd_ "sed" ["-i", "s|" <> beforeAsm <> "|" <> tBuildDir <> "|g", ldOut]
    cmd_ "sed" ["-i", "s|" <> beforeSrc <> "|" <> tBuildDir <> "|g", ldOut]
    cmd_ "sed" ["-i", "s|" <> beforeAssets <> "|" <> tBuildDir <> "|g", ldOut]

  elf %> \out -> do
    _generatedFiles <- getGeneratedFiles gen
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    cFiles <- liftIO $ do
      -- a target nobody has started decompiling has no src/<name>/ at all
      hasSrc <- IO.doesDirectoryExist (tgSrcDir t)
      userFiles <- if hasSrc
        then Set.fromList <$> getDirectoryFilesIO (tgSrcDir t) ["//*.c"]
        else pure Set.empty
      genFiles <- Set.fromList <$> getDirectoryFilesIO (genD </> "src") ["//*.c"]
      pure $ Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        ldFile = genD </> linkerDir </> tgName t <.> "ld"
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto." <> tgName t <> ".txt"
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto." <> tgName t <> ".txt"
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    -- definedSymbols is fed to `ld -T` below, so the link MUST depend on it.
    -- Without it, editing config/symbols.<target>.txt silently has no effect:
    -- splat only re-runs when the change touches something it emits, and if
    -- nothing else is dirty the old .elf is reused. `--lint-fsatrace` catches this.
    need (sFilesExp <> assetFilesExp <> oFiles <> [ldFile, definedSymbols, undefinedSymbols, undefinedFunctions])
    liftIO $ IO.createDirectoryIfMissing True (buildDir </> "tenchu")
    cmd_
      ld
      ldFlags
      [ "-o",
        out,
        "-Map",
        buildDir </> "tenchu" </> tgName t <.> "map",
        "-T",
        ldFile,
        "-T",
        definedSymbols,
        "-T",
        undefinedSymbols,
        "-T",
        undefinedFunctions,
        "--no-check-sections",
        "-nostdlib"
        -- NB: not stripped (`-s`) on purpose -- keeping the symbol table lets the
        -- `mod` target look up function slot addresses via nm. objcopy -O binary
        -- ignores the symbol table, so the exe is unaffected.
      ]

  exe %> \_out -> do
    need [elf]
    cmd_ objcopy objcopyFlags [elf, exe]

-- | Things that only make sense for the executable we are actually decompiling.
mainExtraRules :: Rules ()
mainExtraRules = do
  -- Source-line gdb scripts: add-symbol-file each -gstabs debug object at its
  -- address in the given layout's ELF. All matched debug objects are needed;
  -- gen_debug_gdb.py resolves the per-layout addresses from that ELF.
  let debugGdbRule out elf = do
        let t = mainTarget
            gen = tgGen t
            genD = tgGenDir t
            tool = "tools" </> "gen_debug_gdb.py"
        _generatedFiles <- getGeneratedFiles gen
        cFiles <- liftIO $ do
          hasSrc <- IO.doesDirectoryExist (tgSrcDir t)
          userFiles <- if hasSrc
            then Set.fromList <$> getDirectoryFilesIO (tgSrcDir t) ["//*.c"]
            else pure Set.empty
          genFiles <- Set.fromList <$> getDirectoryFilesIO (genD </> "src") ["//*.c"]
          pure $ Set.toList (userFiles <> genFiles)
        let debugCObjs = map (\f -> debugObjDir </> f <.> "o") cFiles
        need ([elf, tool, "tools" </> "stabs_lines.py"] <> debugCObjs)
        liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
        cmd_ "python3" tool
          [ "--elf", elf,
            "--debug-obj-dir", debugObjDir,
            "--out", out
          ]

  mainDebugGdb %> \out -> debugGdbRule out (mainExe <.> "elf")
  mainRelinkDebugGdb %> \out -> debugGdbRule out mainRelinkElf

  -- `debug-gdb` builds the exact-layout source-line script (retail addresses,
  -- shared by run/run-mod/run-iso*); `debug-gdb-relink` the relink layout.
  phony "debug-gdb" $ need [mainDebugGdb]
  phony "debug-gdb-relink" $ need [mainRelinkDebugGdb]


  -- First normal-link shiftability gate. Splat already places all inputs
  -- sequentially, but config/symbols.main.exe.txt overwrites the game objects'
  -- symbols with retail absolute addresses. Generate alternate scripts which
  -- remove those game-range assignments and add `name = .` before each of the
  -- 555 artificial one-function inputs. The latter also exports original
  -- `static` leaves across our artificial object split without changing C.
  [relocGameLinker, relocGameSymbols] &%> \_outs -> do
    let t = mainTarget
        gen = tgGen t
        baseLinker = tgGenDir t </> linkerDir </> tgName t <.> "ld"
        tool = "tools" </> "reloc_game_lane.py"
    _generatedFiles <- getGeneratedFiles gen
    need [baseLinker, tgSymbols t, tool]
    liftIO $ IO.createDirectoryIfMissing True relocGameDir
    cmd_ "python3" tool
      [ "--linker-in", baseLinker,
        "--symbols-in", tgSymbols t,
        "--linker-out", relocGameLinker,
        "--symbols-out", relocGameSymbols
      ]

  mainRelocGameElf %> \out -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto.main.exe.txt"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    cFiles <- liftIO $ do
      userFiles <- Set.fromList <$> getDirectoryFilesIO (tgSrcDir t) ["//*.c"]
      genFiles <- Set.fromList <$> getDirectoryFilesIO (genD </> srcDir) ["//*.c"]
      pure $ Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    need $ sFilesExp <> assetFilesExp <> oFiles <>
      [relocGameLinker, relocGameSymbols, undefinedSymbols, undefinedFunctions]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ ld ldFlags
      [ "-o", out,
        "-Map", mainRelocGameMap,
        "-T", relocGameLinker,
        "-T", relocGameSymbols,
        "-T", undefinedSymbols,
        "-T", undefinedFunctions,
        "--no-check-sections",
        "-nostdlib"
      ]

  mainRelocGameExe %> \_out -> do
    need [mainRelocGameElf]
    cmd_ objcopy objcopyFlags [mainRelocGameElf, mainRelocGameExe]

  -- Substitute the two exact-sized allocator objects into the linker-owned
  -- game lane.  This focused link proves their symbolic instruction pairs at
  -- retail placement; no compensating boundary pad is needed.
  relocCLiteralLinker %> \out -> do
    let tool = "tools" </> "reloc_c_literals.py"
        objectArgs = concatMap
          (\name -> ["--object", name <> "=" <> relocCLiteralObject name])
          relocCLiteralNames
        referenceArgs = concatMap
          (\name ->
            [ "--reference-object",
              name <> "=" <> relocCLiteralReferenceObject name
            ])
          relocCLiteralNames
    need $ [relocGameLinker, tool, ramLayoutTool, ramLayoutHeader] <>
      relocCLiteralObjects <> relocCLiteralReferenceObjects
    cmd_ "python3" tool
      ([ "generate-linker",
         "--linker-in", relocGameLinker,
         "--linker-out", out
       ] <> referenceArgs <> objectArgs)

  mainRelocCLiteralElf %> \out -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto.main.exe.txt"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    cFiles <- liftIO $ do
      userFiles <- Set.fromList <$> getDirectoryFilesIO (tgSrcDir t) ["//*.c"]
      genFiles <- Set.fromList <$> getDirectoryFilesIO (genD </> srcDir) ["//*.c"]
      pure $ Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    need $ sFilesExp <> assetFilesExp <> oFiles <> relocCLiteralObjects <>
      [ relocCLiteralLinker, relocGameSymbols,
        undefinedSymbols, undefinedFunctions
      ]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ ld ldFlags
      [ "-o", out,
        "-Map", mainRelocCLiteralMap,
        "-T", relocCLiteralLinker,
        "-T", relocGameSymbols,
        "-T", undefinedSymbols,
        "-T", undefinedFunctions,
        "--no-check-sections",
        "-nostdlib"
      ]

  mainRelocCLiteralExe %> \_out -> do
    need [mainRelocCLiteralElf]
    cmd_ objcopy objcopyFlags [mainRelocCLiteralElf, mainRelocCLiteralExe]

  -- The real normal-link composition uses the same transformed allocator
  -- objects, while allowing their compiler-produced layout to follow source
  -- edits. The following SDK transform sees and relocates the
  -- actual layout produced by ordinary source edits, with no artificial game/
  -- SDK boundary restoration.
  normalRelinkCLinker %> \out -> do
    let tool = "tools" </> "reloc_c_literals.py"
        objectArgs = concatMap
          (\name -> ["--object", name <> "=" <> relocCLiteralObject name])
          relocCLiteralNames
        referenceArgs = concatMap
          (\name ->
            [ "--reference-object",
              name <> "=" <> relocCLiteralReferenceObject name
            ])
          relocCLiteralNames
    need $ [relocGameLinker, tool, ramLayoutTool, ramLayoutHeader] <>
      relocCLiteralObjects <> relocCLiteralReferenceObjects
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ "python3" tool
      ([ "generate-linker",
         "--linker-in", relocGameLinker,
         "--linker-out", out,
         "--no-boundary-pad"
       ] <> referenceArgs <> objectArgs)

  [normalRelinkSdkLinker, normalRelinkSdkSymbols] &%> \_outs -> do
    let tool = "tools" </> "reloc_sdk_lane.py"
    need [normalRelinkCLinker, relocGameSymbols, tool]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory normalRelinkSdkLinker)
    cmd_ "python3" tool
      [ "generate",
        "--linker-in", normalRelinkCLinker,
        "--symbols-in", relocGameSymbols,
        "--linker-out", normalRelinkSdkLinker,
        "--symbols-out", normalRelinkSdkSymbols
      ]

  [relocSdkBaseLinker, relocSdkBaseSymbols] &%> \_outs -> do
    let tool = "tools" </> "reloc_sdk_lane.py"
    need [relocGameLinker, relocGameSymbols, tool]
    liftIO $ IO.createDirectoryIfMissing True relocSdkBaseDir
    cmd_ "python3" tool
      [ "generate",
        "--linker-in", relocGameLinker,
        "--symbols-in", relocGameSymbols,
        "--linker-out", relocSdkBaseLinker,
        "--symbols-out", relocSdkBaseSymbols
      ]

  [relocSdkShiftLinker, relocSdkShiftSymbols] &%> \_outs -> do
    let tool = "tools" </> "reloc_sdk_lane.py"
    need [relocGameLinker, relocGameSymbols, tool]
    liftIO $ IO.createDirectoryIfMissing True relocSdkShiftDir
    cmd_ "python3" tool
      [ "generate",
        "--linker-in", relocGameLinker,
        "--symbols-in", relocGameSymbols,
        "--linker-out", relocSdkShiftLinker,
        "--symbols-out", relocSdkShiftSymbols,
        "--pad", "4"
      ]

  [mainRelocSdkElf, mainRelocSdkShiftElf] |%> \out -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto.main.exe.txt"
        shifted = out == mainRelocSdkShiftElf
        linkerScript = if shifted then relocSdkShiftLinker else relocSdkBaseLinker
        symbolScript = if shifted then relocSdkShiftSymbols else relocSdkBaseSymbols
        mapFile = if shifted then mainRelocSdkShiftMap else mainRelocSdkMap
    _generatedFiles <- getGeneratedFiles (tgGen t)
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    cFiles <- liftIO $ do
      userFiles <- Set.fromList <$> getDirectoryFilesIO (tgSrcDir t) ["//*.c"]
      genFiles <- Set.fromList <$> getDirectoryFilesIO (genD </> srcDir) ["//*.c"]
      pure $ Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    need $ sFilesExp <> assetFilesExp <> oFiles <>
      [linkerScript, symbolScript, undefinedSymbols, undefinedFunctions]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ ld ldFlags
      [ "-o", out,
        "-Map", mapFile,
        "-T", linkerScript,
        "-T", symbolScript,
        "-T", undefinedSymbols,
        "-T", undefinedFunctions,
        "--no-check-sections",
        "-nostdlib"
      ]

  [mainRelocSdkExe, mainRelocSdkShiftExe] |%> \out -> do
    let input = if out == mainRelocSdkShiftExe
          then mainRelocSdkShiftElf
          else mainRelocSdkElf
    need [input]
    cmd_ objcopy objcopyFlags [input, out]

  relocDataAsms &%> \_outs -> do
    let t = mainTarget
        dataDir = tgGenDir t </> asmDir </> "data"
        manifest = configDir </> "reloc-data.main.exe.json"
        tool = "tools" </> "reloc_data.py"
        inputs = map (dataDir </>) $ map (<.> "data.s") relocDataNames
    _generatedFiles <- getGeneratedFiles (tgGen t)
    need $ [manifest, tool] <> inputs
    cmd_ "python3" tool
      [ "--manifest", manifest,
        "--input-dir", dataDir,
        "--output-dir", relocIntegratedDataDir
      ]

  relocDataObjects |%> \out -> do
    let name = takeWhile (/= '.') (takeFileName out)
        source = relocDataAsm name
    need [source]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, source]
      neededAsmDeps depFile

  -- Second normal-link proof: turn the zero-filled end of 75F64 into a real
  -- NOBITS input, move every C .bss input into a following NOLOAD output, and
  -- reserve the fixed virtual-memory pool explicitly. Inputs are generated
  -- from the canonical SDK-prefix lane, which already composes with the
  -- linker-owned game lane, so this artifact carries all three proofs.
  [relocBssLinker, relocBssSymbols, relocBssUndefined, relocBssTailAsm] &%> \_outs -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        tailSource = relocData75F64Asm
        oldTailObject = tgBuildDir t </> "data" </> "75F64.data.s.o"
        replacementArgs = concatMap
          (\name ->
            [ "--replace-object",
              (tgBuildDir t </> "data" </> name <.> "data.s.o") <>
                "=" <> relocDataObject name
            ])
          relocDataTargetNames
        tool = "tools" </> "reloc_bss_lane.py"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    need [ relocSdkBaseLinker, relocSdkBaseSymbols, undefinedSymbols,
           tailSource, tool, ramLayoutTool, ramLayoutHeader ]
    liftIO $ IO.createDirectoryIfMissing True relocBssDir
    cmd_ "python3" tool $
      [ "generate",
        "--linker-in", relocSdkBaseLinker,
        "--symbols-in", relocSdkBaseSymbols,
        "--undefined-in", undefinedSymbols,
        "--tail-in", tailSource,
        "--linker-out", relocBssLinker,
        "--symbols-out", relocBssSymbols,
        "--undefined-out", relocBssUndefined,
        "--tail-out", relocBssTailAsm,
        "--old-tail-object", oldTailObject,
        "--new-tail-object", relocBssTailObject,
        "--extension-object-glob", tBuildDir </> "reloc" </> "*.c.o"
      ] <> replacementArgs

  relocBssTailObject %> \out -> do
    need [relocBssTailAsm]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, relocBssTailAsm]
      neededAsmDeps depFile

  mainRelocBssElf %> \out -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto.main.exe.txt"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    userFiles <- Set.fromList <$> getDirectoryFiles (tgSrcDir t) ["//*.c"]
    genFiles <- Set.fromList <$> getDirectoryFiles (genD </> srcDir) ["//*.c"]
    let cFiles = Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        extensionObjects =
          map (\f -> tBuildDir </> f <.> "o") $
            filter (\f -> ["reloc"] `isPrefixOf` splitDirectories f) cFiles
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    need $ sFilesExp <> assetFilesExp <> oFiles <>
      [ relocBssLinker, relocBssSymbols, relocBssUndefined,
        relocBssTailObject, undefinedFunctions
      ] <> relocDataObjects
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    cmd_ ld ldFlags
      [ "-o", out,
        "-Map", mainRelocBssMap,
        "-T", relocBssLinker,
        "-T", relocBssSymbols,
        "-T", relocBssUndefined,
        "-T", undefinedFunctions,
        "--no-check-sections",
        "-nostdlib"
      ] extensionObjects

  mainRelocBssLogical %> \out -> do
    need [mainRelocBssElf]
    cmd_ objcopy objcopyFlags [mainRelocBssElf, out]

  -- Finalize the normal link downstream of ld. Entry and load addresses come
  -- from section-owned ELF symbols; payload size comes from the actual logical
  -- output and is padded to a PS-X sector by the finalizer.
  mainRelocBssExe %> \out -> do
    let tool = "tools" </> "psxexe.py"
    stack <- ramLayoutValue "initial_stack_address"
    need [mainRelocBssLogical, mainRelocBssElf, tool]
    cmd_ "python3" tool
      [ "finalize", mainRelocBssLogical,
        "-o", out,
        "--elf", mainRelocBssElf,
        "--entry-symbol", "__SN_ENTRY_POINT",
        "--load-symbol", "__load_start",
        "--set", "sp=" <> stack,
        "--expect", "gp=0",
        "--expect", "sp=" <> stack
      ]

  -- Compose the no-pad C/SDK chain with reviewed loaded-data relocations and
  -- relative initialized/BSS layout.  This is intentionally a separate output
  -- from main_reloc_bss: the latter remains byte-exact and can keep serving as
  -- the retail structural oracle.
  [ normalRelinkLinker, normalRelinkSymbols,
    normalRelinkUndefined, normalRelinkTailAsm
    ] &%> \_outs -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        oldTailObject = tBuildDir </> "data" </> "75F64.data.s.o"
        replacementArgs = concatMap
          (\name ->
            [ "--replace-object",
              (tBuildDir </> "data" </> name <.> "data.s.o") <>
                "=" <> relocDataObject name
            ])
          relocDataTargetNames
        tool = "tools" </> "reloc_bss_lane.py"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    overrideNames <- modRelinkOverrideNames
    let overrideArgs = concatMap
          (\name ->
            [ "--override-object",
              (tBuildDir </> name <.> "c.o") <> "=" <> modRelinkObject name
            ])
          overrideNames
        overrideSectionArgs = concatMap
          (\name -> ["--ordinary-c-object-glob", modRelinkObject name])
          overrideNames
    need [ normalRelinkSdkLinker, normalRelinkSdkSymbols, undefinedSymbols,
           relocData75F64Asm, tool, ramLayoutTool, ramLayoutHeader ]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory normalRelinkLinker)
    cmd_ "python3" tool $
      [ "generate",
        "--linker-in", normalRelinkSdkLinker,
        "--symbols-in", normalRelinkSdkSymbols,
        "--undefined-in", undefinedSymbols,
        "--tail-in", relocData75F64Asm,
        "--dynamic-pool",
        "--strict-orphans",
        "--linker-out", normalRelinkLinker,
        "--symbols-out", normalRelinkSymbols,
        "--undefined-out", normalRelinkUndefined,
        "--tail-out", normalRelinkTailAsm,
        "--old-tail-object", oldTailObject,
        "--new-tail-object", normalRelinkTailObject,
        "--extension-object-glob", tBuildDir </> "reloc" </> "*.c.o",
        "--ordinary-c-object-glob", tBuildDir </> "*.c.o"
      ] <>
      concatMap
        (\name -> ["--ordinary-c-object-glob", relocCLiteralObject name])
        relocCLiteralNames <>
      overrideSectionArgs <>
      replacementArgs <>
      overrideArgs

  normalRelinkTailObject %> \out -> do
    need [normalRelinkTailAsm]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, normalRelinkTailAsm]
      neededAsmDeps depFile

  [mainRelinkElf, mainRelinkMap] &%> \_outs -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto.main.exe.txt"
        inputAuditTool = "tools" </> "reloc_input_audit.py"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    userFiles <- Set.fromList <$> getDirectoryFiles (tgSrcDir t) ["//*.c"]
    genFiles <- Set.fromList <$> getDirectoryFiles (genD </> srcDir) ["//*.c"]
    let cFiles = Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    overrideNames <- modRelinkOverrideNames
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        extensionObjects =
          map (\f -> tBuildDir </> f <.> "o") $
            filter (\f -> ["reloc"] `isPrefixOf` splitDirectories f) cFiles
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    need $ sFilesExp <> assetFilesExp <> oFiles <> relocCLiteralObjects <>
      map modRelinkObject overrideNames <>
      [ normalRelinkLinker, normalRelinkSymbols, normalRelinkUndefined,
        normalRelinkTailObject, undefinedFunctions, inputAuditTool,
        ramLayoutTool, ramLayoutHeader, tgSymbols mainTarget
      ] <> relocDataObjects
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory mainRelinkElf)
    cmd_ ld ldFlags
      [ "-o", mainRelinkElf,
        "-Map", mainRelinkMap,
        "-T", normalRelinkLinker,
        "-T", normalRelinkSymbols,
        "-T", normalRelinkUndefined,
        "-T", undefinedFunctions,
        "--orphan-handling=error",
        "--no-check-sections",
        "-nostdlib"
      ] extensionObjects
    -- Keep the artifact available to the composed dashboard even when this
    -- diagnostic finds a new blocker. Public `relink` and `check-relink`
    -- immediately rerun the same audit strictly.
    cmd_ "python3" inputAuditTool ["--allow-findings"]

  mainRelinkLogical %> \out -> do
    need [mainRelinkElf]
    cmd_ objcopy objcopyFlags [mainRelinkElf, out]

  mainRelinkExe %> \out -> do
    let tool = "tools" </> "psxexe.py"
    stack <- ramLayoutValue "initial_stack_address"
    need [mainRelinkLogical, mainRelinkElf, tool]
    cmd_ "python3" tool
      [ "finalize", mainRelinkLogical,
        "-o", out,
        "--elf", mainRelinkElf,
        "--entry-symbol", "__SN_ENTRY_POINT",
        "--load-symbol", "__load_start",
        "--set", "sp=" <> stack,
        "--expect", "gp=0",
        "--expect", "sp=" <> stack
      ]

  -- Real-edit regression lane: the same composition as the normal relink,
  -- but with the committed fixture override/extension in place of any user
  -- mods.  The gate proves the grown-function modding contract end to end
  -- without touching src/.
  [realeditLinker, realeditSymbols, realeditUndefined, realeditTailAsm] &%> \_outs -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedSymbols = genD </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        oldTailObject = tBuildDir </> "data" </> "75F64.data.s.o"
        replacementArgs = concatMap
          (\name ->
            [ "--replace-object",
              (tBuildDir </> "data" </> name <.> "data.s.o") <>
                "=" <> relocDataObject name
            ])
          relocDataTargetNames
        tool = "tools" </> "reloc_bss_lane.py"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    need [ normalRelinkSdkLinker, normalRelinkSdkSymbols, undefinedSymbols,
           relocData75F64Asm, tool, ramLayoutTool, ramLayoutHeader ]
    liftIO $ IO.createDirectoryIfMissing True realeditLayoutDir
    cmd_ "python3" tool $
      [ "generate",
        "--linker-in", normalRelinkSdkLinker,
        "--symbols-in", normalRelinkSdkSymbols,
        "--undefined-in", undefinedSymbols,
        "--tail-in", relocData75F64Asm,
        "--dynamic-pool",
        "--strict-orphans",
        "--linker-out", realeditLinker,
        "--symbols-out", realeditSymbols,
        "--undefined-out", realeditUndefined,
        "--tail-out", realeditTailAsm,
        "--old-tail-object", oldTailObject,
        "--new-tail-object", realeditTailObject,
        "--extension-object-glob", realeditExtDir </> "*.c.o",
        "--ordinary-c-object-glob", tBuildDir </> "*.c.o"
      ] <>
      concatMap
        (\name -> ["--ordinary-c-object-glob", relocCLiteralObject name])
        relocCLiteralNames <>
      [ "--ordinary-c-object-glob", realeditOverrideObject ] <>
      replacementArgs <>
      [ "--override-object",
        (tBuildDir </> "PadProc.c.o") <> "=" <> realeditOverrideObject
      ]

  realeditTailObject %> \out -> do
    need [realeditTailAsm]
    liftIO $ IO.createDirectoryIfMissing True (takeDirectory out)
    withTempFile $ \depFile -> do
      cmd_ as asFlags ["--MD", depFile, "-o", out, realeditTailAsm]
      neededAsmDeps depFile

  [realeditElf, realeditMap] &%> \_outs -> do
    let t = mainTarget
        genD = tgGenDir t
        tBuildDir = tgBuildDir t
        undefinedFunctions = genD </> metaDir </> "undefined_functions_auto.main.exe.txt"
    _generatedFiles <- getGeneratedFiles (tgGen t)
    sFiles <- liftIO $ getDirectoryFilesIO (genD </> asmDir) ["*.s", "data/*.s"]
    userFiles <- Set.fromList <$> getDirectoryFiles (tgSrcDir t) ["//*.c"]
    genFiles <- Set.fromList <$> getDirectoryFiles (genD </> srcDir) ["//*.c"]
    let cFiles = Set.toList (userFiles <> genFiles)
    assetFiles <- liftIO $ getDirectoryFilesIO (genD </> assetsDir) ["//*.bin"]
    let oFiles = map (\f -> tBuildDir </> f <.> "o") (cFiles <> sFiles <> assetFiles)
        sFilesExp = map (\f -> genD </> asmDir </> f) sFiles
        assetFilesExp = map (\f -> genD </> assetsDir </> f) assetFiles
    need $ sFilesExp <> assetFilesExp <> oFiles <> relocCLiteralObjects <>
      [ realeditLinker, realeditSymbols, realeditUndefined,
        realeditTailObject, realeditOverrideObject, realeditExtObject,
        undefinedFunctions, tgSymbols mainTarget
      ] <> relocDataObjects
    cmd_ ld ldFlags
      [ "-o", realeditElf,
        "-Map", realeditMap,
        "-T", realeditLinker,
        "-T", realeditSymbols,
        "-T", realeditUndefined,
        "-T", undefinedFunctions,
        "--orphan-handling=error",
        "--no-check-sections",
        "-nostdlib",
        realeditExtObject
      ]

  realeditLogical %> \out -> do
    need [realeditElf]
    cmd_ objcopy objcopyFlags [realeditElf, out]

  realeditExe %> \out -> do
    let tool = "tools" </> "psxexe.py"
    stack <- ramLayoutValue "initial_stack_address"
    need [realeditLogical, realeditElf, tool]
    cmd_ "python3" tool
      [ "finalize", realeditLogical,
        "-o", out,
        "--elf", realeditElf,
        "--entry-symbol", "__SN_ENTRY_POINT",
        "--load-symbol", "__load_start",
        "--set", "sp=" <> stack,
        "--expect", "gp=0",
        "--expect", "sp=" <> stack
      ]

  -- Non-matching build: mkmod patches hooked functions in place. It reads main.exe's
  -- symbol table (via nm on the elf), compiles every src/mod/main.exe/*.c, and aborts
  -- if one outgrows its slot -- so depend on the exe+elf, the mod sources, AND the
  -- tool itself, or an edit to a mod (or to mkmod) wouldn't rebuild and the size
  -- guard wouldn't re-run. See docs/modding-and-nonmatching.md.
  mainModExe %> \_out -> do
    modSrcs <- getDirectoryFiles srcDir ["mod/main.exe/*.c"]
    need $ [mainExe, mainExe <.> "elf", "tools/mkmod.py"]
        <> map (srcDir </>) modSrcs
    cmd_ "tools/mkmod.py"

  -- Bootable CD images (.bin/.cue) with our exe swapped in -- repacked only when the
  -- exe changes, not on every `run-iso`. mkiso.py discovers the *original* disc
  -- dynamically ($TENCHU_CUE / disks/ / ~/tenchu-iso/), so a disc swap isn't a
  -- tracked input: `./Build clean` (or touch the exe) to force a repack then.
  [tenchuBin, tenchuCue] &%> \_out -> do
    let tool = "tools" </> "mkiso.py"
    need [mainExe, tool]
    cmd_ tool

  [tenchuModBin, tenchuModCue] &%> \_out -> do
    let tool = "tools" </> "mkiso.py"
    need [mainModExe, tool]
    cmd_ tool ["--exe", mainModExe,
               "--out", buildDir </> "tenchu" </> "tenchu-mod"]

  [tenchuRelinkBin, tenchuRelinkCue] &%> \_out -> do
    let tool = "tools" </> "mkiso.py"
    need [mainRelinkExe, tool]
    cmd_ tool ["--exe", mainRelinkExe,
               "--out", buildDir </> "tenchu" </> "tenchu-relink"]

  -- Debugger symbol loaders for the run* launchers: <artifact>.symbols.lua
  -- is generated from <artifact>.elf, so names always match the launched
  -- layout.
  buildDir </> "tenchu" </> "*.symbols.lua" %> \out -> do
    let elf = dropExtension (dropExtension out) <.> "elf"
        tool = "tools" </> "pcsx_symbols.py"
    need [elf, tool]
    cmd_ "python3" tool ["--elf", elf, "-o", out]

  -- PCSX-Redux Typed Debugger import files. Function addresses come from the
  -- launched ELF (layout-correct); data types are recovered struct layouts
  -- and are layout-independent. The widget imports them via its own file
  -- dialogs (no Lua/CLI hook), so the build keeps them fresh next to the
  -- artifact and the run launcher prints their paths.
  buildDir </> "tenchu" </> "*.redux_funcs.txt" %> \out -> do
    let elf = dropExtension (dropExtension out) <.> "elf"
        tool = "tools" </> "redux_typed_debugger.py"
    need [elf, tool, "reference" </> "psxsym-protos.h",
          "reference" </> "psxsym-types.h"]
    cmd_ "python3" tool ["--elf", elf, "--funcs-out", out]

  buildDir </> "tenchu" </> "*.redux_data_types.txt" %> \out -> do
    let tool = "tools" </> "redux_typed_debugger.py"
    need [tool, "reference" </> "psxsym-types.h"]
    cmd_ "python3" tool ["--types-out", out]

phonyRules :: Rules ()
phonyRules = do
  let runRelinkGrowthProbe = do
        let tool = "tools" </> "reloc_growth_probe.py"
            undefinedFunctions = tgGenDir mainTarget </> metaDir </>
              "undefined_functions_auto.main.exe.txt"
        need
          [ mainRelinkExe, mainRelinkElf, mainRelinkLogical,
            normalRelinkLinker, normalRelinkSymbols, normalRelinkUndefined,
            undefinedFunctions, configDir </> "reloc-data.main.exe.json",
            tool, "tools" </> "psxexe.py", "tools" </> "reloc_audit.py",
            "tools" </> "reloc_c_literals.py", ramLayoutTool, ramLayoutHeader
          ]
        cmd_ "python3" tool

      runShiftabilityReport = do
        let tool = "tools" </> "shiftability_report.py"
            undefinedFunctions = tgGenDir mainTarget </> metaDir </>
              "undefined_functions_auto.main.exe.txt"
        need $
          [ mainRelinkExe, mainRelinkElf, mainRelinkMap, mainRelinkLogical,
            mainRelocBssElf, normalRelinkLinker, normalRelinkCLinker,
            normalRelinkSymbols, normalRelinkUndefined, undefinedFunctions,
            configDir </> "reloc-data.main.exe.json", tool,
            "tools" </> "reloc_input_audit.py",
            "tools" </> "reloc_audit.py",
            "tools" </> "reloc_c_literals.py",
            "tools" </> "reloc_growth_probe.py",
            "tools" </> "reloc_data.py", "tools" </> "psxexe.py",
            ramLayoutTool, ramLayoutHeader, tgSymbols mainTarget
          ] <> relocCLiteralAuditObjects <> relocCLiteralReferenceObjects
        cmd_ "python3" tool

  phony "clean" $ do
    liftIO $
      removeFiles
        "."
        [genDir, buildDir, processedDir, relocCLiteralDir]

  phony "extract_main.exe" $ do
    need [mainGen]

  -- Each of these is just a `need` on a real file target now, so the exe/mod/iso
  -- are (re)built only when their inputs change — `run-iso` no longer repacks the
  -- ~750 MB image every launch. `mod` -> main_mod.exe; `iso`/`iso-mod` -> the
  -- bootable .bin/.cue (matching vs same-slot mod). See docs/modding-and-nonmatching.md
  -- and docs/building-an-iso.md.
  phony "mod" $ need [mainModExe]
  phony "iso" $ need [tenchuCue]
  phony "iso-mod" $ need [tenchuModCue]
  phony "iso-relink" $ need [tenchuRelinkCue]

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

  phony "run-relink" $ do
    need [mainRelinkExe]
    launchLoadExe [] mainRelinkExe

  -- `run-iso` / `run-iso-mod`: faithful — boot the repacked disc (built by the file
  -- rules above only when the exe changed), so the real SLPS_019.01 -> ... -> MAIN.EXE
  -- chain runs.
  phony "run-iso" $ do
    need [tenchuCue]
    launchIso [] tenchuCue mainExe

  -- main_mod.exe is patched in place (same size as main.exe), so the mod disc keeps
  -- the dumped file layout and is byte-faithful except our function. See
  -- docs/modding-and-nonmatching.md.
  phony "run-iso-mod" $ do
    need [tenchuModCue]
    launchIso [] tenchuModCue mainModExe

  phony "run-iso-relink" $ do
    need [tenchuRelinkCue]
    launchIso [] tenchuRelinkCue mainRelinkExe

  -- Verify a target reproduces its original image byte for byte. The reference sha
  -- is pinned so a swapped/corrupt base image is caught, rather than merely proving
  -- the build is self-consistent with whatever splat happened to extract.
  let checkTarget t = do
        need [tgExe t, tgImage t]
        StdoutTrim ref <- cmd "sha256sum" (tgImage t)
        StdoutTrim ours <- cmd "sha256sum" (tgExe t)
        let refSha = head $ words ref
            ourSha = head $ words ours
        when (refSha /= tgSha t) $
          fail $ unwords ["Reference", tgImage t, "has sha256", refSha,
                          "but expected known-good", tgSha t, "- wrong/corrupt base image?"]
        when (ourSha /= refSha) $
          fail $ unwords ["Expected", tgExe t, "to have sha256 of", refSha,
                          "but it's", ourSha]
        putInfo $ "check: " <> tgName t <> " byte-identical"

  -- The matching gate: main.exe only, so it stays fast (matchdiff runs ./Build once
  -- per function). `check-all` verifies every executable on the disc.
  phony "check" $ checkTarget mainTarget

  -- Normal GNU-ld output. Unlike the same-slot `mod` target this allows sections
  -- to grow and accepts new sources under src/main.exe/reloc/. `check-relink`
  -- runs the structural/header/static-address gates; `check-reloc-bss` remains
  -- the byte-exact retail-layout oracle.
  phony "relink" $ do
    let inputAuditTool = "tools" </> "reloc_input_audit.py"
    need [mainRelinkExe, mainRelinkMap, inputAuditTool, tgSymbols mainTarget]
    cmd_ "python3" inputAuditTool

  -- A user/agent-facing dashboard rather than another independent audit: it
  -- composes the mandatory input scan, final-image scan, compiler-object
  -- contracts, current layout budget, and the real +0x10004 growth proof. It
  -- labels exact-source variants and intentional fixed PS1/game contracts
  -- separately so neither is mistaken for a stale-address blocker.
  phony "shiftability-report" runShiftabilityReport

  phony "check-relink-growth" runRelinkGrowthProbe

  -- Deep-runtime gate: boot the grown auto-LBA disc unattended into a real
  -- mission and require mission construction, a running character
  -- simulation, and sustained CD/XA callbacks.  Opt-in: needs pcsx-redux,
  -- the original disc, and several minutes of wall clock.
  phony "check-relink-gameplay" $ do
    let tool = "tools" </> "pcsx_gameplay.py"
    runRelinkGrowthProbe
    need [ tool, "tools" </> "pcsx-gameplay.lua",
           "tools" </> "pcsx_smoke.py", "tools" </> "psxexe.py" ]
    cmd_ "python3" tool ["--repack"]

  -- Replay the committed grown-PadProc fixture through the override
  -- machinery in an isolated composition and verify code growth, the
  -- relocated call, loaded data, a zero-finding input audit, and (unless
  -- TENCHU_REALEDIT_NO_SMOKE=1) an emulator boot with observed behavior.
  phony "check-relink-realedit" $ do
    let tool = "tools" </> "relink_realedit.py"
    need
      [ realeditExe, realeditElf, realeditMap, realeditLinker,
        mainRelocBssElf, tool,
        "tools" </> "reloc_input_audit.py", "tools" </> "psxexe.py",
        "tools" </> "reloc_c_literals.py",
        "tools" </> "pcsx_smoke.py", "tools" </> "pcsx-smoke.lua"
      ]
    cmd_ "python3" tool

  -- Structural integration gate for the real no-pad artifact.  The compiler
  -- object delta is measured, every unique canonical-SDK symbol must follow
  -- it, and extension/BSS/header bounds are checked without requiring a fixed
  -- extension size. The relocation audit separately rejects any regression to
  -- movable ABS definitions or literal code/data pointers.
  phony "check-relink" $ do
    verifyRelocCLiteralObjectsWith True
    verifyNormalRelink
    let psxExeTool = "tools" </> "psxexe.py"
        auditTool = "tools" </> "reloc_audit.py"
        inputAuditTool = "tools" </> "reloc_input_audit.py"
    stack <- ramLayoutValue "initial_stack_address"
    need
      [ mainRelinkExe, mainRelinkElf, mainRelinkMap, psxExeTool, auditTool,
        inputAuditTool, ramLayoutTool, ramLayoutHeader, tgSymbols mainTarget
      ]
    cmd_ "python3" psxExeTool
      [ "validate", mainRelinkExe,
        "--elf", mainRelinkElf,
        "--entry-symbol", "__SN_ENTRY_POINT",
        "--load-symbol", "__load_start",
        "--expect", "gp=0",
        "--expect", "sp=" <> stack
      ]
    cmd_ "python3" auditTool ["--fail-on-findings"]
    cmd_ "python3" inputAuditTool
    runRelinkGrowthProbe
    putInfo "check-relink: normal C/SDK/data/BSS/header composition is structurally valid"

  -- Focused input gate for six compiler-produced address constructions. Two
  -- allocator objects are transformed after cc1; four byte-exact ordinary
  -- objects are audited directly. All six retain one source spelling.
  phony "check-reloc-c-literals" $ do
    verifyRelocCLiteralObjects
    verifyRelocCLiteralLink
    putInfo "check-reloc-c-literals: two transformed and four ordinary exact objects carry and apply linker relocations"

  -- Opt-in exact-at-retail gate for the normal linker-owned game-symbol lane.
  -- This deliberately does not claim that a grown image is runnable yet: raw
  -- SDK code, the fixed PS-EXE header, and fixed BSS symbols are later stages.
  phony "check-reloc-game" $ do
    verifyRelocCLiteralObjects
    verifyRelocCLiteralLink
    need [mainRelocGameExe, tgImage mainTarget]
    StdoutTrim ref <- cmd "sha256sum" (tgImage mainTarget)
    StdoutTrim ours <- cmd "sha256sum" mainRelocGameExe
    let refSha = head $ words ref
        ourSha = head $ words ours
    when (refSha /= tgSha mainTarget) $
      fail $ unwords ["Reference", tgImage mainTarget, "has sha256", refSha,
                      "but expected known-good", tgSha mainTarget,
                      "- wrong/corrupt base image?"]
    when (ourSha /= refSha) $
      fail $ unwords ["Expected", mainRelocGameExe, "to have sha256 of", refSha,
                      "but it's", ourSha]
    putInfo "check-reloc-game: linker-owned game symbols are retail-exact; symbolic-C inputs verified"

  -- Bounded canonical-assembly proof for the complete SDK/CRT text stream
  -- through UnitVector2. The retail-address link must still match; a controlled
  -- +4 link before Exec audits final linked J/JAL and HI/LO words plus ownership
  -- of every emitted text alias. The composed relink owns later data/BSS and
  -- the PS-X EXE header.
  phony "check-reloc-sdk" $ do
    let tool = "tools" </> "reloc_sdk_lane.py"
        sdkNames =
          [ "LIBAPI_4F9D4",
            "CRT_SDK_4FA48",
            "SDK_TEXT_5492C",
            "SDK_TEXT_54B80",
            "SDK_TEXT_54DE4",
            "SDK_TEXT_5534C",
            "SDK_TEXT_55420",
            "SDK_TEXT_5544C",
            "SDK_TEXT_55478",
            "SDK_TEXT_554DC",
            "SDK_TEXT_55530",
            "SDK_TEXT_55618",
            "SDK_TEXT_556EC",
            "SDK_TEXT_55714",
            "SDK_TEXT_55D68",
            "SDK_TEXT_58164",
            "SDK_TEXT_67B78",
            "SDK_TEXT_71800",
            "SDK_TEXT_722E8",
            "SDK_TEXT_72CD0"
          ]
        sdkObjects = map (\name -> tgBuildDir mainTarget </> name <.> "s" <.> "o") sdkNames
        sdkSources = map (\name -> tgGenDir mainTarget </> asmDir </> name <.> "s") sdkNames
        sdkObjectArgs = concatMap (\object -> ["--sdk-object", object]) sdkObjects
        sdkSourceArgs = concatMap (\source -> ["--sdk-source", source]) sdkSources
    need [ mainRelocSdkExe, mainRelocSdkShiftExe, tgImage mainTarget,
           tool, tgSymbols mainTarget ]
    need $ sdkObjects <> sdkSources
    StdoutTrim ref <- cmd "sha256sum" (tgImage mainTarget)
    StdoutTrim ours <- cmd "sha256sum" mainRelocSdkExe
    let refSha = head $ words ref
        ourSha = head $ words ours
    when (refSha /= tgSha mainTarget) $
      fail $ unwords ["Reference", tgImage mainTarget, "has sha256", refSha,
                      "but expected known-good", tgSha mainTarget,
                      "- wrong/corrupt base image?"]
    when (ourSha /= refSha) $
      fail $ unwords ["Expected", mainRelocSdkExe, "to have sha256 of", refSha,
                      "but it's", ourSha]
    cmd_ "python3" tool $
      [ "verify",
        "--base-elf", mainRelocSdkElf,
        "--shifted-elf", mainRelocSdkShiftElf,
        "--symbols-in", tgSymbols mainTarget,
        "--delta", "4"
      ] <> sdkObjectArgs <> sdkSourceArgs
    putInfo "check-reloc-sdk: canonical SDK text is retail-exact and +4-relocatable"

  -- Opt-in exact-at-retail ownership proof for BSS boundaries and the fixed
  -- virtual-memory pool. objcopy emits only the logical initialized prefix;
  -- the finalizer derives entry/load/size from the link and emits sector
  -- padding, while the validator checks every known BSS symbol and both NOBITS
  -- reservations. It intentionally does not claim a grown image is runnable.
  phony "check-reloc-bss" $ do
    let t = mainTarget
        undefinedSymbols = tgGenDir t </> metaDir </> "undefined_symbols_auto.main.exe.txt"
        tool = "tools" </> "reloc_bss_lane.py"
    need ["check-reloc-data"]
    need [ mainRelocBssLogical, mainRelocBssExe, mainRelocBssElf,
           tgImage t, relocSdkBaseSymbols,
           undefinedSymbols, tool, ramLayoutTool, ramLayoutHeader ]
    StdoutTrim ref <- cmd "sha256sum" (tgImage t)
    StdoutTrim finalized <- cmd "sha256sum" mainRelocBssExe
    let refSha = head $ words ref
        finalizedSha = head $ words finalized
    when (refSha /= tgSha t) $
      fail $ unwords ["Reference", tgImage t, "has sha256", refSha,
                      "but expected known-good", tgSha t,
                      "- wrong/corrupt base image?"]
    when (finalizedSha /= refSha) $
      fail $ unwords ["Expected finalized", mainRelocBssExe,
                      "to have sha256 of", refSha, "but it's", finalizedSha]
    cmd_ "python3" tool
      [ "validate",
        "--logical", mainRelocBssLogical,
        "--reference", tgImage t,
        "--elf", mainRelocBssElf,
        "--symbols-in", relocSdkBaseSymbols,
        "--undefined-in", undefinedSymbols
      ]
    putInfo "check-reloc-bss: linker-owned BSS/pool and finalized PS-X EXE are retail-exact"

  -- Standalone loaded-data proof.  The manifest transformer works on copies of
  -- Splat output, so the matching build remains untouched.  The verifier
  -- requires one R_MIPS_32 at every reviewed source word and links the touched
  -- data inputs at retail, +4, and +0x10004 addresses.
  phony "check-reloc-data" $ do
    let t = mainTarget
        dataDir = tgGenDir t </> asmDir </> "data"
        manifest = configDir </> "reloc-data.main.exe.json"
        transform = "tools" </> "reloc_data.py"
        tool = "tools" </> "reloc_data_lane.py"
        inputs = map (dataDir </>) $ map (<.> "data.s") relocDataNames
    _generatedFiles <- getGeneratedFiles (tgGen t)
    need $ [manifest, transform, tool, "include" </> "macro.inc"] <> inputs
    cmd_ "python3" tool
      [ "--manifest", manifest,
        "--input-dir", dataDir,
        "--work-dir", buildDir </> "reloc-data"
      ]
    putInfo "check-reloc-data: reviewed pointer tables are retail-exact and shift-relocatable"

  -- Optional, evidence-preserving lane for the complete stock GS_107.OBJ.
  -- Nothing proprietary is fetched or tracked: point this one target at a
  -- local PsyQ LIBGS.LIB.  The tool verifies both archive and member hashes,
  -- links the converted member as one relocatable object, preserves its BSS in
  -- a NOLOAD section, then requires the full executable to remain identical.
  -- The ordinary `check` target does not inspect this variable or run the lane.
  phony "check-psyq-gs107" $ do
    archive <- liftIO $ lookupEnv "TENCHU_PSYQ_LIBGS"
    libgs <- case archive of
      Just path -> pure path
      Nothing -> fail $ unlines
        [ "check-psyq-gs107 needs a user-supplied PsyQ LIBGS.LIB.",
          "Set TENCHU_PSYQ_LIBGS=/path/to/LIBGS.LIB; the normal ./Build check does not need it."
        ]
    let manifest = configDir </> "psyq-objects.main.exe.json"
        tool = "tools" </> "psyq_object_lane.py"
        ldFile = tgGenDir mainTarget </> "linker" </> "main.exe.ld"
        undefinedSymbols = tgGenDir mainTarget </> "meta" </> "undefined_symbols_auto.main.exe.txt"
        undefinedFunctions = tgGenDir mainTarget </> "meta" </> "undefined_functions_auto.main.exe.txt"
    need [ mainExe, tgImage mainTarget, libgs, manifest, tool, ldFile,
           tgSymbols mainTarget, undefinedSymbols, undefinedFunctions ]
    cmd_ tool ["--archive", libgs, "--manifest", manifest]

  phony "check-all" $ mapM_ checkTarget targets

  phony "all" $ need (map tgExe targets)

  -- Emit the objdiff progress report decomp.dev ingests (.shake/build/tenchu/report.json).
  -- Build-free: tools/objdiff-report.py derives everything from the committed
  -- config (function inventory + splat/symbols + matched .c files), so this needs
  -- no reassembly. CI uploads it as the `jp_report` artifact. See docs/decomp-dev.md.
  phony "report" $
    cmd_ "tools/objdiff-report.py"

  -- Score how close each NON_MATCHING draft is (config/fuzzy.main.exe.tsv), so
  -- decomp.dev shows partial per-function progress. SLOW: builds every draft
  -- once; run occasionally and commit the result. See docs/decomp-dev.md.
  phony "fuzz-score" $
    cmd_ "tools/fuzz-score.py"

-- | Launch our exe fast: mount the original disc and @-loadexe@ over it (no repack).
-- Paths are absolutised — pcsx-redux resolves them against its own cwd. @extra@ are
-- extra emulator flags (e.g. @-8mb@ for the grown mod, whose region is above 2MB).
launchLoadExe :: [String] -> FilePath -> Action ()
launchLoadExe extra exe = do
  disc <- liftIO $ findDisc >>= IO.makeAbsolute
  exeAbs <- liftIO $ IO.makeAbsolute exe
  symbols <- symbolArgs exe
  runPcsx (["-run", "-iso", disc, "-loadexe", exeAbs] <> symbols <> extra)

-- | Launch a repacked disc image (the faithful full boot). @extra@ as
-- 'launchLoadExe'; @symbolsFor@ names the executable whose debugger symbols
-- the disc's MAIN.EXE slot carries.
launchIso :: [String] -> FilePath -> FilePath -> Action ()
launchIso extra cue symbolsFor = do
  cueAbs <- liftIO $ IO.makeAbsolute cue
  symbols <- symbolArgs symbolsFor
  runPcsx (["-run", "-iso", cueAbs] <> symbols <> extra)

-- | Debugger symbols for a launched executable: a generated Lua loader that
-- PCSX.insertSymbol()s every name from the artifact's own ELF, so call
-- stacks and disassembly are named in every layout (exact, mod, relink).
-- main_mod.exe is patched in place, so it shares main.exe's ELF symbols.
-- The Typed Debugger's import files are generated alongside and their paths
-- printed: the widget imports them through its own file dialogs (it exposes
-- no Lua/CLI hook), so the build keeps them fresh and the user imports once
-- per session.
symbolArgs :: FilePath -> Action [String]
symbolArgs exe = do
  let base = if exe == mainModExe then mainExe else exe
      lua = base <.> "symbols.lua"
      dataTypes = base <.> "redux_data_types.txt"
      funcs = base <.> "redux_funcs.txt"
  need [lua, dataTypes, funcs]
  [luaAbs, dataTypesAbs, funcsAbs] <-
    liftIO $ mapM IO.makeAbsolute [lua, dataTypes, funcs]
  -- When the GDB server is on, build the matching source-line script for this
  -- layout so VSCode's launch.json can source a fresh one — no separate
  -- `./Build debug-gdb` step. mod shares main.exe's addresses.
  gdbEnv <- liftIO $ lookupEnv "TENCHU_GDB"
  gdbInfo <- if gdbEnv == Nothing
    then pure ""
    else do
      let debugGdb = if base == mainRelinkExe then mainRelinkDebugGdb else mainDebugGdb
      need [debugGdb]
      debugGdbAbs <- liftIO $ IO.makeAbsolute debugGdb
      pure $ "\n  source lines (VSCode sources this): " <> debugGdbAbs
  putInfo $
    "Typed Debugger imports (Debug -> Typed Debugger -> Import):\n"
      <> "  data types: " <> dataTypesAbs <> "\n"
      <> "  functions:  " <> funcsAbs
      <> gdbInfo
  pure ["-dofile", luaAbs]

-- | Where a @run*@ target records the resolved emulator argv. The @./Build@
-- wrapper execs it after Shake exits, so the emulator does not run inside the
-- Shake session and the global build lock is released for the whole play
-- session (only the build steps up to the launch hold it).
pendingLaunchFile :: FilePath
pendingLaunchFile = shakeDir </> "pending-launch"

-- | Record a pcsx-redux launch with the given base args plus any
-- @$PCSX_REDUX_ARGS@. It falls back to OpenBIOS when no BIOS is configured, so
-- no BIOS is required.
--
-- We force @-interpreter@ by default: pcsx-redux's x64 dynarec crashes
-- (\"Unrecoverable error while running recompiler\") with OpenBIOS — a regression
-- of grumpycoders/pcsx-redux#695 (that combo was fixed in 2022, re-broke in the
-- memory-system rework since). Since we default to OpenBIOS, the dynarec is
-- unusable here. The user opts back into the dynarec (with a real BIOS) by putting
-- @-dynarec@/@-bios@/@-interpreter@ in @$PCSX_REDUX_ARGS@.
--
-- The argv is written NUL-delimited (disc paths contain spaces) and the
-- launch is deferred to the wrapper rather than run via @cmd_@ here.
runPcsx :: [String] -> Action ()
runPcsx baseArgs = do
  pcsx <- liftIO findPcsx
  extra <- liftIO $ maybe [] words <$> lookupEnv "PCSX_REDUX_ARGS"
  -- TENCHU_GDB[=port] enables pcsx-redux's GDB server so VSCode (or a bare
  -- gdb) can attach; default port 3333. See docs/debugging-vscode.md.
  gdbEnv <- liftIO $ lookupEnv "TENCHU_GDB"
  let userChoseCpu = any (`elem` ["-interpreter", "-dynarec", "-bios"]) extra
      cpu = ["-interpreter" | not userChoseCpu]
      gdbArgs = case gdbEnv of
        Nothing -> []
        Just "" -> ["-gdb", "-gdb-port", "3333"]
        Just "1" -> ["-gdb", "-gdb-port", "3333"]
        Just port -> ["-gdb", "-gdb-port", port]
      argv = pcsx : (baseArgs <> cpu <> gdbArgs <> extra)
  liftIO $ IO.createDirectoryIfMissing True shakeDir
  liftIO $ writeFile pendingLaunchFile (intercalate "\0" argv)
  if null gdbArgs
    then putInfo "launch deferred to the ./Build wrapper (build lock released first)"
    else putInfo "GDB server on; launch deferred to wrapper (attach from VSCode / mips-gdb)"

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

-- | The per-original-object compiler executable plus extra flags. As with
-- GpFlags, the oracle value itself must be a dependency: Shake keys on source
-- content and does not otherwise notice a changed command executable or flag.
-- Keeping both inputs in one result also prevents the build and matching tools
-- from accidentally selecting half of a compiler profile.
newtype OriginalObjectCcProfile = OriginalObjectCcProfile FilePath
  deriving (Generic, Show, Eq)

instance Hashable OriginalObjectCcProfile

instance NFData OriginalObjectCcProfile

instance Binary OriginalObjectCcProfile

type instance RuleResult OriginalObjectCcProfile = (FilePath, [String])

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

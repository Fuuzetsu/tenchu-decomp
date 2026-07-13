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
-- (kept as plain constants: `targets` is defined below and Haskell has no
-- forward-use problem, but these two are referenced in comments/tools too.)

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
    extra "FUN_80034dbc" = ["--expand-div"]
    extra "DrawImpact" = ["--expand-div"]
    extra _ = []
    -- Think1sleep.c is a fragment of the original think TU, which defines these.
    syms "Think1sleep" = ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "DrawSprite" = ["OTablePt"]
    syms "SetImpact" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "GetAreaMapVector" = ["FieldAttrib", "FieldArea", "FieldIndex"]
    syms "FUN_80039160" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_80018f00" = ["AccessPower"]
    syms "valloc" = ["virtual_memory_pool"]
    syms "SetupCharacterParameter" = ["NowStage"]
    syms "AdtMessageBox" = ["AdtReadPadFunc", "D_80097E94"]
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
    syms "SoundEx" = ["STAGE_SOUNDS_POINTER"]
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
    syms "FUN_80032720" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "D_80097F30", "D_80097F32"]
    syms "AVCameraSetup" = ["CHOSEN_EVENT_LIST_THING_LOCATION", "D_80097CC4"]
    syms "AVCameraControl" = ["D_80097CCA", "D_80097CC8", "D_80097CC4"]
    syms "CVAsequence" = ["PERSISTENT_EVENT_LIST_THING", "CHOSEN_EVENT_LIST_THING_LOCATION", "D_80097CC4", "D_80097CCC", "D_80097CC0"]
    syms "CVAupdate" = ["CHOSEN_EVENT_LIST_THING_LOCATION", "D_80097CCC", "D_80097CC4", "D_80097CCA", "D_80097CC8", "PERSISTENT_EVENT_LIST_THING"]
    syms "CVArun" = ["D_80097CC0", "CHOSEN_EVENT_LIST_THING_LOCATION"]
    syms "CVAsetup" = ["PERSISTENT_EVENT_LIST_THING"]
    syms "CreateHumanoid" = ["Humans"]
    -- PutItemList.c is also part of the info-view TU; it defines/uses both
    -- selected-item-kind smalls gp-relatively.
    syms "PutItemList" = ["CURRENTLY_SELECTED_ITEM_KIND_0_", "CURRENTLY_SELECTED_ITEM_KIND_1_"]
    -- PutMap.c is also part of the info-view TU; it DEFINES PutMapMode (the
    -- "PutMap latch" DoInfoViewProc.c also references) plus the wipe
    -- position pair.
    syms "PutMap" = ["PutMapMode", "D_80097F6C", "D_80097F70"]
    -- UpdateEvent.c is also part of the original STAGE.C TU.
    syms "UpdateEvent" = ["StageEvent", "StagePlayer"]
    syms "KillHumanoid" = ["Humans"]
    syms "GetNearestHumanoid" = ["Humans"]
    syms "JumpControl" = ["dtL", "dtM", "motID", "Me_MOTION_C", "D_80097F0E", "dtV", "dtPAD"]
    syms "HangCheck" = ["Me_MOTION_C", "motID", "dtL", "dtR", "MotionUpdateMode", "D_80097F0E"]
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
    syms "DoInfoViewProc" = ["fInitialize", "CURRENTLY_SELECTED_ITEM_KIND_1_", "PutMapMode"]
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
    syms "ReqItemUse" = ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F48", "D_80097F5C"]
    syms "ReqItemLaunch" = ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F48"]
    syms "ReqItemArrow" = ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F4C"]
    syms "ReqItemHappou" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "ProcItemHappou" = ["HAPPOU_SCRATCH_MODEL"]
    syms "Sound" = ["D_80097CB4"]
    syms "DeleteConflict" = ["ConflictObjects"]
    syms "InsertConflict" = ["ConflictObjects"]
    syms "GetConflictResult" = ["ConflictObjects", "ConflictDistance", "ConflictModel"]
    syms "DisposeAreaMap" = ["GlobalAreaMap"]
    syms "NowReturnNormal" = ["Me_MOTION_C", "motID", "D_80097F0E"]
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
    syms "AttackFire" = ["dtM", "Me_MOTION_C", "dtR"]
    syms "bow_shoot_logic" = ["Me_MOTION_C", "dtR"]
    syms "Think1watch" = ["Me_THINK_C"]
    syms "Think3firstattack" = ["Distance", "SR", "Me_THINK_C", "Attrib", "Degree"]
    syms "Think3escape" = ["Distance", "SR", "Degree", "Attrib", "Me_THINK_C"]
    syms "Think1ninja" = ["Me_THINK_C"]
    syms "GetArcData" = ["MODEL_ARCHIVE_PTR"]
    syms "FUN_80027730" = ["dtM", "Me_MOTION_C", "dtR"]
    syms "ReturnNormal" = ["Me_MOTION_C", "motID", "D_80097F0E"]
    syms "DrawOrnament" = ["OTablePt"]
    syms "FUN_8005fe88" = ["D_80097E98"]
    syms "SetupStageSequence" = ["StageEvent", "StagePlayer"]
    syms "FUN_800274e8" = ["dtM", "Me_MOTION_C"]
    syms "ThinkBasicHuman1" = ["Me_THINK_C"]
    syms "StartDrawing" = ["DrawingPage", "OTablePt", "GameClock"]
    syms "AttackPQD" = ["Me_MOTION_C", "dtM"]
    syms "SetupSoundEffect" = ["STAGE_SOUNDS_POINTER"]
    syms "initialise_default_player_cameras_" = ["CAMERA_PTR_ARRAY_START"]
    syms "vgetfreesize" = ["virtual_memory_pool"]
    syms "vgetmaxsize" = ["virtual_memory_pool"]
    syms "InitAccessInfo" = ["AccessPower"]
    syms "GetHumanoid" = ["Humans"]
    syms "AttackAnimal" = ["Me_THINK_C", "Distance", "Degree"]
    syms "StickonCheck" = ["Me_MOTION_C", "dtL", "motID", "D_80097F0E"]
    syms "DeleteCard" = ["CardVolumeIdPtr"]
    syms "LoadCard" = ["CardVolumeIdPtr"]
    syms "SaveCard" = ["CardVolumeIdPtr"]
    syms "SetupAfterimage" = ["D_80097F3C"]
    syms "MotionAndMove" = ["MotionUpdateMode", "Me_MOTION_C", "motID", "D_80097F0E"]
    syms "FileRead" = ["AccessPower", "ReadMode", "TotalIO"]
    syms "LoadFromDEVPC" = ["TotalIO", "ReadMode", "MemoryLoadAddress"]
    syms "LoadFromCDROM" = ["TotalIO", "ReadMode", "MemoryLoadAddress"]
    syms "SetFrame" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetSplash" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetBleed" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "AttackBowControl" = ["dtM", "Me_MOTION_C"]
    syms "HumanActionControl" = ["Me_MOTION_C", "dtPAD", "dtCMD", "D_80097F0E", "dtV", "dtL", "dtR", "dtM", "motID"]
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
    syms "FUN_80056e30" = ["CardVolumeIdPtr"]
    syms "FUN_80038fdc" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_8003944c" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "InitMisc" = ["EFFECT_SPAWNERS_INITIALISED"]
    syms "InitializeItem" = ["D_80097F48", "D_80097F4C", "D_80097F50", "HAPPOU_SCRATCH_MODEL", "D_80097F5C", "D_80097F60", "D_80097AC8"]
    syms "ProcItemNapalm" = ["D_80097F60"]
    syms "DoMiscProc" = ["EFFECT_SPAWNERS_INITIALISED"]
    syms "LoadSI" = ["D_80097D8C"]
    syms "SaveSI" = ["D_80097D8C"]
    syms "InitializeInfoView" = ["fInitialize"]
    syms "ActSYURI" = ["dtM", "Me_MOTION_C", "motID", "D_80097F0E"]
    syms "ActHANG" = ["dtV", "dtM", "dtPAD", "dtL", "motID", "D_80097F0E", "Me_MOTION_C"]
    syms "ReqItemGun" = ["COUNTER_FOR_ITEM_ARRAY_"]
    syms "SetSmokeS" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "SetupAppearance" = ["NowStage", "PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR", "D_800979A6", "AMD_LOADED_FOR_CHARACTER_KIND"]
    syms "StateTransition" = ["StrainRatio", "Me_THINK_C", "Pad", "Attrib", "D_80097F1C", "ActionHalt", "FRAMES_UNTIL_END_OF_ALERT", "SR", "Distance", "D_80097F10", "D_80097F18", "D_80097F14", "EngageLevel", "Degree"]
    syms "ActATTACK" = ["dtM", "Me_MOTION_C", "motID", "D_80097F0E", "dtR", "dtL", "dtPAD", "MotionUpdateMode", "dtV"]
    syms "AttackIndirect" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "SR"]
    syms "AttackLong" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"]
    syms "AttackGeneral" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"]
    syms "AttackShort" = ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"]
    syms "DamageControl" = ["Me_MOTION_C", "MotionUpdateMode", "motID", "dtL", "D_80097F0E", "dtM", "dtR", "D_8009770C", "dtV"]
    syms "ProcItemNinken" = ["NINKEN_CHARACTER_PTR"]
    syms "ActKAGI" = ["dtM", "Me_MOTION_C", "dtL", "D_80097F0E", "motID", "dtR", "MotionUpdateMode", "dtV"]
    syms "FUN_8003562c" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "ProcItemHenshin" = ["D_80097AEC", "D_80097AF0"]
    syms "ActJUMP" = ["Me_MOTION_C", "motID", "dtL", "dtM", "dtV", "dtR", "MotionUpdateMode", "D_80097F0E", "dtPAD"]
    syms "SwimCheck" = ["Me_MOTION_C", "motID", "dtM", "dtL", "D_80097F0E", "MotionUpdateMode"]
    syms "ActivateHumans" = ["D_80097F40", "D_80097F44", "D_80097F42", "StageID"]
    syms "ProcItemNingyo" = ["D_80097AE0", "D_80097F50"]
    syms "StartStageSequence" = ["StageEvent", "StageTime", "FriendHits", "Murders", "Findenemies", "Criticals", "D_80097F7C", "StagePlayer", "StageCitizens", "StageEnemies", "StageBosses"]
    syms "StageSequence" = ["StagePlayer", "D_80097F78", "D_80097F7C", "StageTime", "Findenemies", "Murders", "Criticals", "StageEnemies", "StageBosses", "FriendHits", "StageCitizens"]
    syms "AddEnemy" = ["D_80097D44"]
    syms "FUN_8005b17c" = ["D_80097D38", "D_80097D24", "D_80097D3C", "D_80097D40", "D_80097D28", "D_80097D2C"]
    syms "LoadConstruction" = ["D_80097A70", "D_80097A74", "StageID"]
    syms "CreateStage" = ["StageID"]
    syms "SetWire" = ["D_80097F28"]
    syms "think_setting_small_rotation_small_steps_" = ["Me_THINK_C", "Attrib", "FRAMES_UNTIL_END_OF_ALERT", "Degree", "Distance"]
    syms "ActCHASE" = ["Me_MOTION_C", "dtM", "dtPAD", "MotionUpdateMode", "motID", "D_80097F0E", "dtL", "dtR", "dtCMD"]
    syms "ActENGAGE" = ["dtM", "dtV", "dtPAD", "motID", "Me_MOTION_C", "D_80097F0E", "dtL", "dtCMD", "dtR"]
    syms "DrawShadow" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "D_80097F34"]
    syms "register_character_death" = ["D_800979DE", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "Think3attack" = ["Me_THINK_C", "SR", "Distance", "Degree", "EngageLevel"]
    syms "SetFlyWire" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "FUN_80035f44" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "InitEffect" = ["D_80097F34", "LOCAL_COORDINATES_", "D_80097F3C", "D_80097F28", "D_80097F30", "D_80097F32"]
    syms "FUN_80033bc0" = ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"]
    syms "AttackControl" = ["Me_MOTION_C", "dtL", "dtR", "motID", "D_80097F0E", "dtPAD", "dtM"]
    syms "Think1target" = ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"]
    syms "FUN_8005adbc" = ["D_80097D20", "D_80097D24", "D_80097D28"]
    syms "ActDEAD" = ["Me_MOTION_C", "dtM", "dtV", "motID", "dtL", "D_80097F0E", "D_8009770C"]
    syms "ActSTICKON" = ["dtM", "Me_MOTION_C", "dtR", "dtCMD", "motID", "D_80097F0E", "MotionUpdateMode", "dtPAD", "dtV", "D_80097EF0", "dtL"]
    syms "ActACTION" = ["dtM", "Me_MOTION_C", "dtV", "dtPAD", "MotionUpdateMode", "motID", "D_80097F0E"]
    syms "ActDAMAGE" = ["dtM", "Me_MOTION_C", "dtV", "motID", "D_80097F0E", "dtL"]
    syms "ActITEM" = ["dtM", "Me_MOTION_C", "motID", "D_80097F0E"]
    syms "ActMOVE" = ["dtM", "Me_MOTION_C", "dtPAD", "motID", "D_80097F0E", "dtL", "dtR"]
    syms "ActNORMAL" = ["dtM", "dtPAD", "Me_MOTION_C", "motID", "D_80097F0E", "dtR", "dtCMD"]
    syms "ActSQUAT" = ["Me_MOTION_C", "dtM", "dtPAD", "dtR", "motID", "D_80097F0E", "dtV", "dtL", "dtCMD"]
    syms "ActSTATE" = ["dtM", "MotionUpdateMode", "motID", "D_80097F0E", "Me_MOTION_C", "dtV", "dtPAD", "dtR", "dtL"]
    syms "ActSWIM" = ["dtM", "motID", "D_80097F0E", "dtPAD", "Me_MOTION_C", "dtR", "dtV", "dtL"]
    syms "FUN_8005a7a4" = ["D_80097D2E", "D_80097D30", "D_80097D18", "D_80097D32", "D_80097D2C"]
    syms "FUN_8005aba4" = ["D_80097D34", "D_80097D2C"]
    syms "FallCheck" = ["motID", "Me_MOTION_C", "dtM", "dtL", "D_80097F0E", "MotionUpdateMode"]
    syms "ItemControl" = ["Me_MOTION_C", "motID", "D_80097F0E"]
    syms "character_balma_around_main_routine_" = ["D_800976E8", "GlobalAreaMap", "FieldIndex", "FieldArea"]
    syms "reset_alert_duration" = ["FRAMES_UNTIL_END_OF_ALERT"]
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
  want [mainExe]
  objRules
  mapM_ exeRules targets
  mainExtraRules
  phonyRules

objRules :: Rules ()
objRules = do
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

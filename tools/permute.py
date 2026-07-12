#!/usr/bin/env python3
"""Run decomp-permuter on a split function to find the C that byte-matches.

For a function whose src/main.exe/<name>.c is close-but-not-matching (correct
arithmetic, wrong register allocation / schedule — see get_held_buttons/GetRealPad),
this sets up a permuter working dir and runs it:

  * compile.sh   = this repo's pipeline (cc1 -G8 | maspsx | as) on preprocessed C
  * base.c       = the preprocessed src/main.exe/<name>.c (what gets perturbed)
  * target.o     = the original function assembled from its nonmatching .s

On score 0 the winning source is written under the work dir; drop it into
src/main.exe/<name>.c and confirm with `./Build check`.

Usage (in the nix devShell):  tools/permute.py <name> [-- <permuter args>]
e.g.  tools/permute.py GetRealPad --stop-on-zero -j4


CAVEAT: the permuter's search score ignores stack-frame slot placement. It can
report score 0 ("perfect") for a candidate that still differs from the target only
in stack-slot ordering -- FUN_80018f00 has an 11-byte residual the scorer cannot
see. It also ranks some register-allocation wins that are real byte regressions.
This wrapper therefore full-links every retained output and prints matchdiff's raw
whole-image ranking after the run. Use `<name> --rescore-only` after interrupting
one, and still verify an adopted/cleaned candidate with tools/matchdiff.py.
"""
import argparse, glob, os, re, shutil, subprocess, sys, tempfile

from matchlock import MatchToolBusy, matching_tool_lock
import matchdiff

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Kept in sync with shake/src/Build.hs.
CPP = ("mipsel-unknown-linux-gnu-cpp -Iinclude -undef -Wall -lang-c -fno-builtin "
       "-gstabs -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx "
       "-D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C "
       "-DLANGUAGE_C -DHACKS -I src/main.exe").split()
CC_FLAGS = ("-mcpu=3000 -quiet -G8 -w -O2 -funsigned-char -fpeephole -ffunction-cse "
            "-fpcc-struct-return -fcommon -fverbose-asm -fgnu-linker -mgas "
            "-msoft-float").split()
AS_FLAGS = ("-EL -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections -O1 -G0").split()
LD = "mipsel-unknown-linux-gnu-ld"
OBJCOPY = "mipsel-unknown-linux-gnu-objcopy"
LINKER = ".shake/gen/main.exe/linker/main.exe.ld"
SYMBOLS = "config/symbols.main.exe.txt"
UNDEFINED_SYMBOLS = ".shake/gen/main.exe/meta/undefined_symbols_auto.main.exe.txt"
UNDEFINED_FUNCTIONS = ".shake/gen/main.exe/meta/undefined_functions_auto.main.exe.txt"

# Per-function `--gp-extern SYM` maspsx flags — MUST mirror maspsxGpExterns in
# shake/src/Build.hs (ASPSX gp-addresses only TU-local definitions; these are the
# small globals the function's ORIGINAL translation unit defined).
GP_EXTERNS = {
    "vmemoryGC": ["virtual_memory_pool"],
    "ComputeAllConflict": ["ConflictObjects"],
    "PlayVoice": ["D_80097CA0", "D_80097CA4", "D_80097CA8", "D_80097CAC", "D_80097C9C", "D_80097C98"],
    "PutStrain": ["D_80097F68"],
    "Think3hitaway": ["Distance", "SR", "Me_THINK_C", "Degree", "Attrib"],
    "Camera": ["Projection"],
    "AttackContinuousCheck": ["dtM", "Me_MOTION_C"],
    "Think4abandon": ["Me_THINK_C", "Attrib", "SR", "FRAMES_UNTIL_END_OF_ALERT"],
    "WeaponHitWeapon": ["Me_MOTION_C", "dtM"],
    "DrawModelArchive": ["SkipFrame", "OTablePt"],
    "FUN_80032720": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "D_80097F30", "D_80097F32"],
    "PlaySE": ["voice"],
    "SetBleedsDir": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "ChasetoTarget": ["Me_THINK_C", "Attrib", "Distance"],
    "GetAreaMapPassage": ["FieldArea", "FieldIndex"],
    "SetBlood": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetHinoko": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "EndDrawing": ["GameClock", "SkipFrame", "DrawingPage", "D_800976B8", "OTablePt", "time"],
    "SetSmoke": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetBleeds": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "JumpControl": ["dtL", "dtM", "motID", "Me_MOTION_C", "D_80097F0E", "dtV", "dtPAD"],
    "HangCheck": ["Me_MOTION_C", "motID", "dtL", "dtR", "MotionUpdateMode", "D_80097F0E"],
    "SoundEx": ["STAGE_SOUNDS_POINTER"],
    "dispose_weapon_data_of_char_": ["Me_MOTION_C", "dtM"],
    "AdtMessageBox": ["AdtReadPadFunc", "D_80097E94"],
    "PutItemList": ["CURRENTLY_SELECTED_ITEM_KIND_0_", "CURRENTLY_SELECTED_ITEM_KIND_1_"],
    "PutMap": ["PutMapMode", "D_80097F6C", "D_80097F70"],
    "UpdateEvent": ["StageEvent", "StagePlayer"],
    "CreateHumanoid": ["Humans"],
    "KillHumanoid": ["Humans"],
    "GetNearestHumanoid": ["Humans"],
    "SetupCharacterParameter": ["NowStage"],
    "AVCameraSetup": ["CHOSEN_EVENT_LIST_THING_LOCATION", "D_80097CC4"],
    "AVCameraControl": ["D_80097CCA", "D_80097CC8", "D_80097CC4"],
    "CVAsequence": ["PERSISTENT_EVENT_LIST_THING", "CHOSEN_EVENT_LIST_THING_LOCATION", "D_80097CC4", "D_80097CCC", "D_80097CC0"],
    "CVAupdate": ["CHOSEN_EVENT_LIST_THING_LOCATION", "D_80097CCC", "D_80097CC4", "D_80097CCA", "D_80097CC8", "PERSISTENT_EVENT_LIST_THING"],
    "CVArun": ["D_80097CC0", "CHOSEN_EVENT_LIST_THING_LOCATION"],
    "CVAsetup": ["PERSISTENT_EVENT_LIST_THING"],
    "valloc": ["virtual_memory_pool"],
    "FUN_80018f00": ["AccessPower"],
    "GetAreaMapVector": ["FieldAttrib", "FieldArea", "FieldIndex"],
    "FUN_80039160": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetImpact": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetExplosion": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "Think1random": ["Me_THINK_C", "Attrib"],
    "Think1chase": ["Me_THINK_C"],
    "ItemUse": ["Me_THINK_C", "Degree"],
    "Think4contact": ["SR", "Attrib", "Me_THINK_C", "Degree"],
    "Think4chase": ["SR", "Attrib", "Me_THINK_C", "Degree"],
    "DrawSprite": ["OTablePt"],
    "Think1sleep": ["Me_THINK_C", "SR", "Attrib", "FRAMES_UNTIL_END_OF_ALERT"],
    "ReqItemDrop": ["COUNTER_FOR_ITEM_ARRAY_"],
    "FUN_8004a42c": ["COUNTER_FOR_ITEM_ARRAY_"],
    "GetAreaMapLevel": ["FieldIndex", "FieldArea", "D_80097EC0", "FieldAttrib"],
    "DoInfoViewProc": ["fInitialize", "CURRENTLY_SELECTED_ITEM_KIND_1_", "PutMapMode"],
    "BriefingAndInventorySelectionScreen": ["CARRY_30_ITEMS_CHEAT_APPLIED"],
    "LayoutEnemyOption": ["D_80097D44"],
    "debug_menu_stage_option": ["SystemFlag"],
    "FileOption": ["SystemFlag"],
    "ReqItemJirai": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemDokudango": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemSmoke": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemFire": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemShinsoku": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemNinken": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemKaengeki": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemGoshikimai": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemNemuri": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemKusuri": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemKawarimi": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemGosin": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemHenshin": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemManebue": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemMakibishi": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemLightningBolt": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemNingyo": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ReqItemUse": ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F48", "D_80097F5C"],
    "ReqItemLaunch": ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F48"],
    "ReqItemArrow": ["COUNTER_FOR_ITEM_ARRAY_", "D_80097F4C"],
    "ReqItemHappou": ["COUNTER_FOR_ITEM_ARRAY_"],
    "ProcItemHappou": ["HAPPOU_SCRATCH_MODEL"],
    "Sound": ["D_80097CB4"],
    "DeleteConflict": ["ConflictObjects"],
    "InsertConflict": ["ConflictObjects"],
    "GetConflictResult": ["ConflictObjects", "ConflictDistance", "ConflictModel"],
    "DisposeAreaMap": ["GlobalAreaMap"],
    "NowReturnNormal": ["Me_MOTION_C", "motID", "D_80097F0E"],
    "vinit": ["virtual_memory_pool"],
    "LoadAreaMap": ["GlobalAreaMap", "FieldIndex", "FieldArea"],
    "DrawBG": ["OTablePt"],
    "PrepareAccess": ["AccessPower"],
    "handle_balmer_acm_": ["GlobalAreaMap", "FieldIndex", "D_800976E8", "FieldArea"],
    "FUN_80027304": ["Me_MOTION_C", "dtL"],
    "init_score_stats": ["StageBosses", "StageEnemies", "Findenemies", "Murders", "Criticals", "FriendHits"],
    "is_character_state_present_on_stage_": ["Humans"],
    "think_setting_go_towards_player": ["Attrib", "Me_THINK_C", "Degree"],
    "update_something_for_each_visible_enemy_": ["VISIBLE_ENEMIES_"],
    "turn_towards_player_": ["Me_THINK_C", "Degree", "Attrib", "D_80097F10"],
    "Think1trace": ["Me_THINK_C", "Degree", "Attrib"],
    "Think3chase": ["Distance", "SR", "EngageLevel", "AttackActionCount", "Degree", "Me_THINK_C"],
    "AttackFire": ["dtM", "Me_MOTION_C", "dtR"],
    "bow_shoot_logic": ["Me_MOTION_C", "dtR"],
    "Think1watch": ["Me_THINK_C"],
    "Think3firstattack": ["Distance", "SR", "Me_THINK_C", "Attrib", "Degree"],
    "Think3escape": ["Distance", "SR", "Degree", "Attrib", "Me_THINK_C"],
    "Think1ninja": ["Me_THINK_C"],
    "GetArcData": ["MODEL_ARCHIVE_PTR"],
    "FUN_80027730": ["dtM", "Me_MOTION_C", "dtR"],
    "ReturnNormal": ["Me_MOTION_C", "motID", "D_80097F0E"],
    "DrawOrnament": ["OTablePt"],
    "FUN_8005fe88": ["D_80097E98"],
    "SetupStageSequence": ["StageEvent", "StagePlayer"],
    "FUN_800274e8": ["dtM", "Me_MOTION_C"],
    "ThinkBasicHuman1": ["Me_THINK_C"],
    "StartDrawing": ["DrawingPage", "OTablePt", "GameClock"],
    "AttackPQD": ["Me_MOTION_C", "dtM"],
    "SetupSoundEffect": ["STAGE_SOUNDS_POINTER"],
    "initialise_default_player_cameras_": ["CAMERA_PTR_ARRAY_START"],
    "vgetfreesize": ["virtual_memory_pool"],
    "vgetmaxsize": ["virtual_memory_pool"],
    "InitAccessInfo": ["AccessPower"],
    "GetHumanoid": ["Humans"],
    "AttackAnimal": ["Me_THINK_C", "Distance", "Degree"],
    "StickonCheck": ["Me_MOTION_C", "dtL", "motID", "D_80097F0E"],
    "DeleteCard": ["CardVolumeIdPtr"],
    "SetupAfterimage": ["D_80097F3C"],
    "MotionAndMove": ["MotionUpdateMode", "Me_MOTION_C", "motID", "D_80097F0E"],
    "FileRead": ["AccessPower", "ReadMode", "TotalIO"],
    "LoadFromDEVPC": ["TotalIO", "ReadMode", "MemoryLoadAddress"],
    "LoadFromCDROM": ["TotalIO", "ReadMode", "MemoryLoadAddress"],
    "SetFrame": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetSplash": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetBleed": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "AttackBowControl": ["dtM", "Me_MOTION_C"],
    "HumanActionControl": ["Me_MOTION_C", "dtPAD", "dtCMD", "D_80097F0E", "dtV", "dtL", "dtR", "dtM", "motID"],
    "AttackCancelControl": ["Me_MOTION_C", "dtM"],
    "DoItemProc": ["D_80097AC8"],
    "vfree": ["virtual_memory_pool"],
    "DrawModel": ["OTablePt"],
    "FUN_8001b2f4": ["D_800976F6"],
    "LoadMotion": ["MotionPack"],
    "SearchMotion": ["CommonMotion", "PlayerMotion", "StageMotion"],
    "GetSpline": ["D_80097708", "D_80097EEC", "D_80097EE8"],
    "GetImage": ["D_80097C90"],
    "InitConflict": ["ConflictModel", "ConflictObjects"],
    "ControlAllHumanoid": ["Humans", "VISIBLE_ENEMIES_"],
    "ControlHumanoid": ["VISIBLE_ENEMIES_"],
    "SuccessionAttack": ["Me_THINK_C", "Distance", "Degree", "EngageLevel"],
    "Think3callaid": ["Distance", "SR", "Me_THINK_C", "Degree", "Pad", "Attrib"],
    "InitFileSystem": ["ReadMode", "TotalIO", "D_80097EB8"],
    "cbAccess": ["AccessPower"],
    "FUN_80056e30": ["CardVolumeIdPtr"],
    "FUN_80038fdc": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "FUN_8003944c": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "FUN_8005fe38": ["D_80097E98"],
    "InitMisc": ["EFFECT_SPAWNERS_INITIALISED"],
    "InitializeItem": ["D_80097F48", "D_80097F4C", "D_80097F50", "HAPPOU_SCRATCH_MODEL", "D_80097F5C", "D_80097F60", "D_80097AC8"],
    "DoMiscProc": ["EFFECT_SPAWNERS_INITIALISED"],
    "LoadSI": ["D_80097D8C"],
    "InitializeInfoView": ["fInitialize"],
    "ActSYURI": ["dtM", "Me_MOTION_C", "motID", "D_80097F0E"],
    "ActHANG": ["dtV", "dtM", "dtPAD", "dtL", "motID", "D_80097F0E", "Me_MOTION_C"],
    "item_use_gun": ["COUNTER_FOR_ITEM_ARRAY_"],
    "SetSmokeS": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetupAppearance": ["NowStage", "PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR", "D_800979A6", "AMD_LOADED_FOR_CHARACTER_KIND"],
    "StateTransition": ["StrainRatio", "Me_THINK_C", "Pad", "Attrib", "D_80097F1C", "ActionHalt", "FRAMES_UNTIL_END_OF_ALERT", "SR", "Distance", "D_80097F10", "D_80097F18", "D_80097F14", "EngageLevel", "Degree"],
    "ActATTACK": ["dtM", "Me_MOTION_C", "motID", "D_80097F0E", "dtR", "dtL", "dtPAD", "MotionUpdateMode", "dtV"],
    "AttackIndirect": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "SR"],
    "AttackLong": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"],
    "AttackGeneral": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"],
    "AttackShort": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"],
    "DamageControl": ["Me_MOTION_C", "MotionUpdateMode", "motID", "dtL", "D_80097F0E", "dtM", "dtR", "D_8009770C", "dtV"],
}

# Per-function extra maspsx flags — MUST mirror `extra` in Build.hs
# maspsxGpExterns (e.g. --expand-div for TUs that divide by a variable).
MASPSX_EXTRA = {
    "UpdateTexScroll": ["--expand-div"],
    "MakeDifSub": ["--expand-div"],
    "DrawFlyWire": ["--expand-div"],
    "UpdateSplineControl": ["--expand-div"],
    "DrawBleed": ["--expand-div"],
    "DrawFrame": ["--expand-div"],
    "DrawAfterimage": ["--expand-div"],
    "MoveFly": ["--expand-div"],
    "FUN_80039ddc": ["--expand-div"],
    "FUN_8004c59c": ["--expand-div"],
    "FUN_8004d6d4": ["--expand-div"],
    "FUN_8003a148": ["--expand-div"],
    "FUN_80039fb0": ["--expand-div"],
    "SetBleedsDir": ["--expand-div"],
    "SetBlood": ["--expand-div"],
    "SetHinoko": ["--expand-div"],
    "SetupFly": ["--expand-div"],
    "SetSmoke": ["--expand-div"],
    "SetBleeds": ["--expand-div"],
    "PutLifeBar": ["--expand-div"],
    "IsVisible": ["--expand-div"],
    "ComputeAreaLevel": ["--expand-div"],
    "PadProc": ["--expand-div"],
    "FUN_8003a2a8": ["--expand-div"],
    "DrawSprite": ["--expand-div"],
    "GetAreaMapLevel": ["--expand-div"],
    "bow_shoot_logic": ["--expand-div"],
    "Think3escape": ["--expand-div"],
    "SuccessionAttack": ["--expand-div"],
    "GetSpline": ["--expand-div"],
    "StateTransition": ["--expand-div"],
    "AttackIndirect": ["--expand-div"],
    "AttackLong": ["--expand-div"],
    "AttackGeneral": ["--expand-div"],
    "AttackShort": ["--expand-div"],
    "DamageControl": ["--expand-div"],
    "ControlHumanoid": ["--expand-div"],
}

COMPILE_SH = r"""#!/usr/bin/env bash
# permuter compile: preprocessed C -> object, via this repo's cc1|maspsx|as.
set -e
IN=$(realpath "$1"); OUT=$(realpath -m "$3")
cd {root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cc1-281 {ccflags} < "$IN" > "$TMP/a.s"
maspsx --aspsx-version=2.77 -G8{gpexterns} < "$TMP/a.s" > "$TMP/b.s"
mipsel-unknown-linux-gnu-as {asflags} -o "$OUT" "$TMP/b.s"
"""


def piece_addr(path):
    """VRAM of a nonmatching piece's first instruction (its comment column).

    splat >= 0.4x INDENTS instruction lines, so don't anchor at the line start —
    an anchored regex silently returns the sentinel for every piece and the
    jump-table pieces then sort in file order, which is wrong.
    """
    import re
    for line in open(path):
        m = re.search(r"/\* \w+ ([0-9A-Fa-f]{8}) [0-9A-Fa-f]{8} \*/", line)
        if m:
            return int(m.group(1), 16)
    return 1 << 62


def find_nonmatching_s(name):
    """All split pieces of the function, in ADDRESS order (a jump-table switch
    splits one function into several .s files whose names sort wrong
    lexicographically — caseD_1f before caseD_2, switchD last)."""
    dirs = glob.glob(f".shake/gen/main.exe/asm/nonmatchings/{name}")
    hits = sorted(glob.glob(f"{dirs[0]}/*.s"), key=piece_addr) if dirs else \
        glob.glob(f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s")
    if not hits:
        sys.exit(f"permute: {name}.s not found under .shake/gen — is {name} split "
                 f"and built? run `./Build` (or split it with tools/reverse.py).")
    return hits


def raw_byte_diff(first, second):
    """Raw differing-byte count, including a length delta."""
    return (sum(a != b for a, b in zip(first, second)) +
            abs(len(first) - len(second)))


def rewrite_linker_object(linker, name, candidate):
    """Replace one TU object in a generated splat linker script.

    A candidate must be linked in the exact retail layout before its bytes are
    authoritative: raw relocatable-object bytes do not contain resolved jumps,
    gp-relative offsets, or final local-rodata addresses.  Using a private copy
    of the linker script avoids touching Shake's live object tree.
    """
    original = f".shake/build/main.exe/{name}.c.o"
    if original not in linker:
        raise ValueError(f"{original} is absent from {LINKER}")
    return linker.replace(original, os.path.abspath(candidate))


def object_section_size(path, section=".text"):
    out = subprocess.run(
        ["mipsel-unknown-linux-gnu-objdump", "-h", path],
        capture_output=True, text=True, check=True,
    ).stdout
    pattern = re.compile(rf"^\s*\d+\s+{re.escape(section)}\s+([0-9a-fA-F]+)\b",
                         re.M)
    match = pattern.search(out)
    return int(match.group(1), 16) if match else 0


def candidate_sources(work):
    """The baseline plus every improvement source retained by permuter."""
    paths = [os.path.join(work, "base.c")]
    paths.extend(glob.glob(os.path.join(work, "output-*-*", "source.c")))

    def key(path):
        match = re.search(r"/output-(\d+)-(\d+)/source\.c$", path)
        return (-1, 0) if not match else (int(match.group(1)), int(match.group(2)))

    return sorted((path for path in paths if os.path.exists(path)), key=key)


def _link_candidate(name, csh, source, scratch, ordinal, linker_text,
                    original, addr, extent):
    """Compile and fully link one preprocessed candidate; return raw scores."""
    stem = os.path.join(scratch, f"candidate-{ordinal}")
    obj = stem + ".o"
    compile_result = subprocess.run(
        [csh, source, "-o", obj], stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE, text=True,
    )
    if compile_result.returncode:
        return {"source": source, "error": "compile failed"}

    script = stem + ".ld"
    with open(script, "w") as stream:
        stream.write(rewrite_linker_object(linker_text, name, obj))
    elf, image, map_path = stem + ".elf", stem + ".bin", stem + ".map"
    link_result = subprocess.run(
        [LD, "-EL", "-o", elf, "-Map", map_path,
         "-T", script, "-T", SYMBOLS, "-T", UNDEFINED_SYMBOLS,
         "-T", UNDEFINED_FUNCTIONS, "--no-check-sections", "-nostdlib"],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True,
    )
    if link_result.returncode:
        tail = link_result.stderr.strip().splitlines()
        return {"source": source,
                "error": "link failed" + (f": {tail[-1]}" if tail else "")}
    subprocess.run([OBJCOPY, "-O", "binary", elf, image], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    with open(image, "rb") as stream:
        candidate = stream.read()
    file_offset = matchdiff.FILE_TEXT_OFF + (addr - matchdiff.TEXT_START)
    target_window = original[file_offset:file_offset + extent]
    candidate_window = candidate[file_offset:file_offset + extent]
    return {
        "source": source,
        "whole": raw_byte_diff(original, candidate),
        "window": raw_byte_diff(target_window, candidate_window),
        "text_size": object_section_size(obj),
    }


def authoritative_rescore(name, work, csh):
    """Fully link retained candidates and rank them by matchdiff's objective.

    decomp-permuter's scorer intentionally normalizes several details.  That is
    useful during stochastic search but can rank a real byte regression above a
    candidate that is closer after relocations and final section placement.
    This pass uses a private linker script and compares the complete executable,
    exactly like matchdiff, without replacing source or build-tree objects.
    """
    sources = candidate_sources(work)
    if not sources:
        print("permute: authoritative rescore: no base/output sources found")
        return []
    with open(matchdiff.ORIG, "rb") as stream:
        original = stream.read()
    with open(LINKER) as stream:
        linker_text = stream.read()
    addr, extent = matchdiff.symbol_slot(name)
    results = []
    with tempfile.TemporaryDirectory(prefix=f"tenchu-permute-{name}-") as scratch:
        for ordinal, source in enumerate(sources):
            results.append(_link_candidate(
                name, csh, source, scratch, ordinal, linker_text,
                original, addr, extent,
            ))

    valid = sorted((result for result in results if "whole" in result),
                   key=lambda result: (result["whole"], result["window"],
                                       result["source"]))
    print("\npermute: authoritative full-link rescore "
          "(whole image / function window / .text):")
    for result in valid[:20]:
        label = os.path.relpath(result["source"], work)
        print(f"  {result['whole']:6d} / {result['window']:5d} / "
              f"{result['text_size']:5d}  {label}")
    if len(valid) > 20:
        print(f"  ... {len(valid) - 20} more retained candidates")
    for result in results:
        if "error" in result:
            print(f"  INVALID  {os.path.relpath(result['source'], work)}: "
                  f"{result['error']}")
    if valid:
        best = valid[0]
        label = os.path.relpath(best["source"], work)
        if best["whole"] == 0:
            print(f"permute: ★ AUTHORITATIVE MATCH → {label}")
        else:
            print(f"permute: authoritative best → {label} "
                  f"({best['whole']} whole-image differing bytes)")
    return valid


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rescore-only", action="store_true",
                    help="full-link and rank retained candidates without rerunning permuter")
    ap.add_argument("name")
    ap.add_argument("rest", nargs=argparse.REMAINDER,
                    help="args after `--` are passed to permuter.py")
    args = ap.parse_args()
    # argparse.REMAINDER intentionally hands everything after NAME to the
    # child.  Also accept the natural `NAME --rescore-only` spelling locally.
    if "--rescore-only" in args.rest:
        args.rescore_only = True
        args.rest.remove("--rescore-only")
    name = args.name
    src = f"src/main.exe/{name}.c"
    if not os.path.exists(src):
        sys.exit(f"permute: {src} not found")

    work = os.path.join(".shake", "permuter", name)
    csh = os.path.join(work, "compile.sh")
    if args.rescore_only:
        if not os.path.exists(csh):
            sys.exit(f"permute: {work} has no compile.sh; run a normal bounded "
                     "permuter pass first")
        subprocess.run(["./Build"], stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, check=True)
        authoritative_rescore(name, work, csh)
        return

    subprocess.run(["./Build"], stdout=subprocess.DEVNULL, check=False)
    target_s = find_nonmatching_s(name)

    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)

    # compile.sh
    csh = os.path.join(work, "compile.sh")
    gpx = "".join(f" {f}" for f in MASPSX_EXTRA.get(name, []))
    gpx += "".join(f" --gp-extern {s}" for s in GP_EXTERNS.get(name, []))
    open(csh, "w").write(COMPILE_SH.format(
        root=ROOT, ccflags=" ".join(CC_FLAGS), asflags=" ".join(AS_FLAGS),
        gpexterns=gpx))
    os.chmod(csh, 0o755)

    # base.c = preprocessed src (what the permuter perturbs). -DNON_MATCHING
    # selects the draft over the INCLUDE_ASM stub for partial functions (a no-op
    # for files without the guard), so the permuter perturbs real C either way.
    base = os.path.join(work, "base.c")
    subprocess.run(CPP + ["-DPERMUTER", "-DNON_MATCHING", src, base], check=True)

    # target.o = the original function, assembled from its nonmatching .s
    # (all pieces concatenated for split/jump-table functions)
    tgt_s = os.path.join(work, "target.s")
    open(tgt_s, "w").write('.include "macro.inc"\n'
                           + "".join(open(t).read() for t in target_s))
    subprocess.run(["mipsel-unknown-linux-gnu-as", *AS_FLAGS,
                    "-o", os.path.join(work, "target.o"), tgt_s], check=True)

    # sanity: base.c must compile
    subprocess.run([csh, base, "-o", os.path.join(work, "base.o")], check=True)

    print(f"permute: set up {work} — running permuter…")
    print("permute: BUDGET REMINDER — a <=10-byte register-swap / adjacent-"
          "reorder residual is\n         usually sub-C-level (reload/sched) "
          "and permuter-immune. Give it ONE\n         bounded run (~5-10 min); "
          "if it stays flat, stop and mark NON_MATCHING\n         "
          "(docs/matching-cookbook.md 'sub-C-level residual early-stop').")
    rest = args.rest[1:] if args.rest and args.rest[0] == "--" else args.rest
    if not rest:
        rest = ["--stop-on-zero", "-j4"]
    rc = subprocess.run(["permuter.py", work, *rest]).returncode

    wins = sorted(glob.glob(os.path.join(work, "output-0-*", "source.c")))
    if wins:
        print(f"\npermute: proxy score 0 retained → {wins[0]}\n"
              "         Full-link rescoring below decides whether it really matches.")
    authoritative_rescore(name, work, csh)
    sys.exit(rc)


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("permute", target):
            main()
    except MatchToolBusy as e:
        sys.exit(f"permute: {e}")

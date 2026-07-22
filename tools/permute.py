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

The wrapper refuses an early broad/structurally-wrong draft.  ``--force-early``
is an explicit escape hatch only when RTL has proved one allocation cascade.


CAVEAT: the permuter's search score ignores stack-frame slot placement. It can
report score 0 ("perfect") for a candidate that still differs from the target only
in stack-slot ordering -- FUN_80018f00 has an 11-byte residual the scorer cannot
see. It also ranks some register-allocation wins that are real byte regressions.
This wrapper therefore full-links every retained output and prints matchdiff's raw
whole-image ranking after the run. Use `<name> --rescore-only` after interrupting
one, and still verify an adopted/cleaned candidate with tools/matchdiff.py.

The scorer is BLIND to stack-slot offsets. A residual that is purely
`addiu a0,sp,K` placement (FUN_80018f00's whole 11 bytes) scores base 0 and the
permuter writes no candidate at all — it cannot see the defect, so a bounded run
on that class is pure wasted wall-clock. Check `tools/asmdiff.py` first: if every
differing line is an `sp+K` offset, this is an `assign_stack_local` DECLARATION-ORDER
problem, which is arithmetic (see docs/matching-cookbook.md, "When two constraints
fight over ONE lever"), not a search.
"""
import argparse, difflib, glob, os, re, shutil, subprocess, sys, tempfile
import time

from matchlock import MatchToolBusy, matching_tool_lock
import matchdiff
import proclife

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Kept in sync with shake/src/Build.hs.
CPP = ("mipsel-unknown-linux-gnu-cpp -Iinclude -undef -Wall -lang-c "
       "-gstabs -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx "
       "-D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C "
       "-DLANGUAGE_C -DHACKS -I src/main.exe").split()
# `-fno-builtin` belongs to cc1, NOT cpp -- cc1 decides whether abs() inlines.
# Build.hs already learned this ("This lived in cppFlags for a long time, where it
# did nothing") and moved it; THIS MIRROR NEVER GOT THE FIX, so every permuter run
# compiled a DIFFERENT PROGRAM than the build: builtins enabled here, disabled
# there. Measured on SetWire: same insn count, different callee-saved register
# assignment outright, and permute's own rescore printed `291357 / 1004 / 1492` for
# base.c against a draft that is 1488 bytes at 70 differing. A search from that base
# never explores the real draft's neighbourhood, so "the permuter found nothing" on
# such a function was worth nothing. 82 source files mention a builtin-expandable
# call. test_permute_cc_flags_match_build now pins this.
CC_FLAGS = ("-mcpu=3000 -quiet -fno-builtin -G8 -w -O2 -funsigned-char -fpeephole "
            "-ffunction-cse -fpcc-struct-return -fcommon -fverbose-asm -fgnu-linker "
            "-mgas -msoft-float").split()
CC_DEFAULT = "cc1-281"
# Stock PsyQ objects can use different cc1 defaults from the game TUs. The
# decomp's one-function files are artificial, so the executable and options
# belong to an ORIGINAL object and are inherited by every known member. Most
# members below are target-only today; retaining them is deliberate, because a
# future C carve must receive the complete object profile automatically.
ORIGINAL_OBJECT_MEMBERS = {
    "LIBMCRD.OBJ": (
        "MemCardStart", "MemCardStop", "MemCardExist", "FUN_80080f28",
        "MemCardAccept", "FUN_80081164", "MemCardOpen", "MemCardClose",
        "MemCardReadData", "FUN_8008161c", "MemCardWriteData",
        "FUN_80081810", "MemCardReadFile", "FUN_80081a64",
        "MemCardWriteFile", "FUN_80081c84", "MemCardGetDirentry",
        "MemCardCallback", "MemCardSync", "MemCardCreateFile",
        "MemCardFormat",
    ),
    "GS_105.OBJ": (
        "GsMapModelingData",
    ),
    "GS_106.OBJ": (
        "GsSetProjection",
    ),
    "GS_110.OBJ": (
        "GsSetAmbient",
    ),
    "GS_111.OBJ": (
        "GsDrawOt",
    ),
    "GS_113.OBJ": (
        "GsClearOt",
    ),
    "GS_119.OBJ": (
        "gte_rotate_z_matrix",
    ),
    "GS_121.OBJ": (
        "gte_init",
    ),
    "GS_122.OBJ": (
        "GsGetTimInfo",
    ),
    "GS_123.OBJ": (
        "Gssub_make_matrix",
    ),
    "GS_125.OBJ": (
        "GsGetWorkBase",
    ),
    "GS_107.OBJ": (
        "GsSetFlatLight", "GS_107_OBJ_444", "GS_107_OBJ_4B8",
        "GS_107_OBJ_51C",
    ),
    "ADT.OBJ": (
        "AdtGetDisp", "AdtMessageBox", "AdtQuiet", "AdtFntOpen",
        "AdtFntLoad", "AdtReleaseDisp", "AdtDmyPadRead", "AdtVsprintf",
        "FUN_8005fe38", "FUN_8005fe88", "AdtSelect",
    ),
}
ORIGINAL_OBJECT_CC_FLAGS = {
    "LIBMCRD.OBJ": ("-mno-split-addresses",),
    "GS_105.OBJ": (),
    "GS_106.OBJ": (),
    "GS_110.OBJ": (),
    "GS_111.OBJ": (),
    "GS_113.OBJ": (),
    "GS_119.OBJ": (),
    "GS_121.OBJ": (),
    "GS_122.OBJ": (),
    # Undo the game TU's global option for this complete vendor object.
    "GS_123.OBJ": ("-fsigned-char",),
    "GS_125.OBJ": (),
    "GS_107.OBJ": ("-mno-split-addresses",),
    "ADT.OBJ": (),
}
ORIGINAL_OBJECT_CC_EXECUTABLES = {
    "GS_106.OBJ": "cc1-272",
    "GS_110.OBJ": "cc1-272",
    "GS_111.OBJ": "cc1-272",
    "GS_113.OBJ": "cc1-272",
    "GS_121.OBJ": "cc1-272",
    "GS_122.OBJ": "cc1-272",
    "GS_123.OBJ": "cc1-272",
    "GS_107.OBJ": "cc1-281-gs107",
    "ADT.OBJ": "cc1-280",
}
CC_FLAGS_BY_OBJECT_MEMBER = {
    member: list(ORIGINAL_OBJECT_CC_FLAGS[obj])
    for obj, members in ORIGINAL_OBJECT_MEMBERS.items()
    for member in members
}
CC_EXECUTABLE_BY_OBJECT_MEMBER = {
    member: ORIGINAL_OBJECT_CC_EXECUTABLES.get(obj, CC_DEFAULT)
    for obj, members in ORIGINAL_OBJECT_MEMBERS.items()
    for member in members
}
AS_FLAGS = ("-EL -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections -O1 -G0").split()
LD = "mipsel-unknown-linux-gnu-ld"
OBJCOPY = "mipsel-unknown-linux-gnu-objcopy"
LINKER = ".shake/gen/main.exe/linker/main.exe.ld"
SYMBOLS = "config/symbols.main.exe.txt"
UNDEFINED_SYMBOLS = ".shake/gen/main.exe/meta/undefined_symbols_auto.main.exe.txt"
UNDEFINED_FUNCTIONS = ".shake/gen/main.exe/meta/undefined_functions_auto.main.exe.txt"
ASMDIFF = "tools/asmdiff.py"
ASMDIFF_SUMMARY = re.compile(
    r": (\d+) (?:displayed )?differing lines in (\d+) blocks; "
    r"(?:raw aligned residual \d+ lines in \d+ blocks; )?"
    r"length ours (\d+) vs target (\d+)(?:;[^\]]*)?\]"
)
PREFLIGHT_MAX_LINES = 128
PREFLIGHT_MAX_BLOCKS = 32

# GCC accepts these intrinsic spellings without a declaration, but
# decomp-permuter's C type map does not.  Give only the generated search input
# the declarations it needs; the repository source and compiled semantics stay
# unchanged.
PERMUTER_PARSER_DECLS = {
    "__builtin_abs": "extern int __builtin_abs(int);\n",
}


def cc_flags_for(name):
    """Build-equivalent flags inherited from this carve's original object."""
    return CC_FLAGS + CC_FLAGS_BY_OBJECT_MEMBER.get(name, [])


def cc_executable_for(name):
    """Build-equivalent cc1 executable for an original object member."""
    return CC_EXECUTABLE_BY_OBJECT_MEMBER.get(name, CC_DEFAULT)


def add_permuter_parser_declarations(source):
    """Make GCC intrinsic calls visible to decomp-permuter's type parser."""
    declarations = []
    for identifier, declaration in PERMUTER_PARSER_DECLS.items():
        if identifier in source and declaration.strip() not in source:
            declarations.append(declaration)
    return "".join(declarations) + source


def permuter_readiness(diff_lines, blocks, ours, target):
    """Whether a residual is narrow enough for stochastic register search.

    One instruction of length slack is allowed because a local permutation can
    create/delete a move or scheduling nop.  Broad multi-block residuals are
    still structural even when their total length happens to balance.
    """
    delta = abs(ours - target)
    if delta > 1:
        return False, (f"instruction length differs by {delta} "
                       f"({ours} vs {target})")
    if diff_lines > PREFLIGHT_MAX_LINES or blocks > PREFLIGHT_MAX_BLOCKS:
        # A broad multi-block residual is USUALLY a structurally-wrong draft, even
        # at exact length (accidental balance). But NOT always: a large park whose
        # excess blocks are provably-CLOSED small clusters (e.g. unreachable bank
        # pairs) is a legitimate scheduling/allocation target -- the block count
        # cannot tell those apart from real structural divergence, so this stays a
        # refusal, and `--force-early` is the override for when YOU have established
        # the excess is closed clusters. mission_score_screen (169 B, >32 blocks)
        # found two real wins that way. The message must point at that escape hatch
        # for exact length, else a lane reads "broad" as "hopeless".
        hatch = (" — but length is EXACT, so if you have CONFIRMED the excess "
                 "blocks are closed/unreachable clusters (not structural), "
                 "`--force-early` is the intended override" if delta == 0 else
                 " and length differs by 1 — likely structural, not a search target")
        return False, (f"residual is still broad: {diff_lines} aligned lines in "
                       f"{blocks} blocks (limits {PREFLIGHT_MAX_LINES}/"
                       f"{PREFLIGHT_MAX_BLOCKS}){hatch}")
    return True, (f"near-final residual: {diff_lines} aligned lines in {blocks} "
                  f"blocks, length {ours}/{target}")


def draft_shape(name):
    """Build the guarded C draft and return asmdiff's near-final shape tuple."""
    result = subprocess.run(
        [sys.executable, ASMDIFF, name], capture_output=True, text=True,
    )
    match = ASMDIFF_SUMMARY.search(result.stdout)
    if match is None:
        detail = (result.stderr or result.stdout).strip().splitlines()
        tail = detail[-1] if detail else "no asmdiff summary"
        sys.exit(f"permute: could not assess draft shape: {tail}")
    return tuple(int(value) for value in match.groups())

# Per-function `--gp-extern SYM` maspsx flags — MUST mirror maspsxGpExterns in
# shake/src/Build.hs (ASPSX gp-addresses only TU-local definitions; these are the
# small globals the function's ORIGINAL translation unit defined).
GP_EXTERNS = {
    "vmemoryGC": ["virtual_memory_pool"],
    "ComputeAllConflict": ["ConflictObjects"],
    "PlayVoice": ["VoiceXaName", "VoiceXaNameF", "VoiceXaNameI", "VoiceXaNameJ", "D_80097C9C", "D_80097C98"],
    "PutStrain": ["D_80097F68"],
    "Think3hitaway": ["Distance", "SR", "Me_THINK_C", "Degree", "Attrib"],
    "Camera": ["Projection"],
    "AttackContinuousCheck": ["dtM", "Me_MOTION_C"],
    "Think4abandon": ["Me_THINK_C", "Attrib", "SR", "EmergencyNotice"],
    "WeaponHitWeapon": ["Me_MOTION_C", "dtM"],
    "DrawModelArchive": ["SkipFrame", "OTablePt"],
    "SetupTexScroll": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "D_80097F30", "D_80097F32"],
    "PlaySE": ["voice"],
    "SetBleedsDir": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "FUN_80037e0c": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "ChasetoTarget": ["Me_THINK_C", "Attrib", "Distance"],
    "GetAreaMapPassage": ["FieldArea", "FieldIndex"],
    "SetBlood": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetHinoko": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "EndDrawing": ["GameClock", "SkipFrame", "DrawingPage", "D_800976B8", "OTablePt", "time"],
    "SetSmoke": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetBleeds": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "JumpControl": ["dtL", "dtM", "motID", "Me_MOTION_C", "motMODE", "dtV", "dtPAD"],
    "HangCheck": ["Me_MOTION_C", "motID", "dtL", "dtR", "MotionUpdateMode", "motMODE"],
    "SoundEx": ["StageSE"],
    "dispose_weapon_data_of_char_": ["Me_MOTION_C", "dtM"],
    "AdtMessageBox": ["AdtPadRead", "AdtMessageBoxCount"],
    "PutItemList": ["SelectedItem", "ItemCursor"],
    "PutMap": ["PutMapMode", "x", "y"],
    "UpdateEvent": ["StageEvent", "StagePlayer"],
    "CreateHumanoid": ["Humans"],
    "KillHumanoid": ["Humans"],
    "GetNearestHumanoid": ["Humans"],
    "SetupCharacterParameter": ["NowStage"],
    "AVCameraSetup": ["CVAnow", "CameraTarget"],
    "AVCameraControl": ["CameraPanMode", "CameraSpeed", "CameraTarget"],
    "CVAsequence": ["CVAdata", "CVAnow", "CameraTarget", "D_80097CCC", "CVAtime"],
    "CVAupdate": ["CVAnow", "D_80097CCC", "CameraTarget", "CameraPanMode", "CameraSpeed", "CVAdata"],
    "CVArun": ["CVAtime", "CVAnow"],
    "CVAsetup": ["CVAdata"],
    "valloc": ["virtual_memory_pool"],
    "FUN_80018f00": ["AccessPower"],
    "GetAreaMapVector": ["FieldAttrib", "FieldArea", "FieldIndex"],
    "SetSnow": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetImpact": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetExplosion": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "Think1random": ["Me_THINK_C", "Attrib"],
    "Think1chase": ["Me_THINK_C"],
    "ItemUse": ["Me_THINK_C", "Degree"],
    "Think4contact": ["SR", "Attrib", "Me_THINK_C", "Degree"],
    "Think4chase": ["SR", "Attrib", "Me_THINK_C", "Degree"],
    "DrawSprite": ["OTablePt"],
    "Think1sleep": ["Me_THINK_C", "SR", "Attrib", "EmergencyNotice"],
    "ReqItemDrop": ["ic"],
    "GetFreeItemSlot": ["ic"],
    "GetAreaMapLevel": ["FieldIndex", "FieldArea", "D_80097EC0", "FieldAttrib"],
    "DoInfoViewProc": ["fInitialize", "ItemCursor", "PutMapMode"],
    "BriefingAndInventorySelectionScreen": ["CARRY_30_ITEMS_CHEAT_APPLIED"],
    "LayoutEnemyOption": ["CurrentEnemyID"],
    "debug_menu_stage_option": ["SystemFlag"],
    "FileOption": ["SystemFlag"],
    "ReqItemJirai": ["ic"],
    "ReqItemDokudango": ["ic"],
    "ReqItemSmoke": ["ic"],
    "ReqItemFire": ["ic"],
    "ReqItemShinsoku": ["ic"],
    "ReqItemNinken": ["ic"],
    "ReqItemKaengeki": ["ic"],
    "ReqItemGoshikimai": ["ic"],
    "ReqItemNemuri": ["ic"],
    "ReqItemKusuri": ["ic"],
    "ReqItemKawarimi": ["ic"],
    "ReqItemGosin": ["ic"],
    "ReqItemHenshin": ["ic"],
    "ReqItemManebue": ["ic"],
    "ReqItemMakibishi": ["ic"],
    "ReqItemLightningBolt": ["ic"],
    "ReqItemNingyo": ["ic"],
    "ReqItemUse": ["ic", "SyurikenModel", "sprNapalm"],
    "ReqItemLaunch": ["ic", "SyurikenModel"],
    "ReqItemArrow": ["ic", "ArrowModel"],
    "ReqItemHappou": ["ic"],
    "ProcItemHappou": ["HappouModel"],
    "Sound": ["VoiceMode"],
    "DeleteConflict": ["ConflictObjects"],
    "InsertConflict": ["ConflictObjects"],
    "GetConflictResult": ["ConflictObjects", "ConflictDistance", "ConflictModel"],
    "DisposeAreaMap": ["GlobalAreaMap"],
    "NowReturnNormal": ["Me_MOTION_C", "motID", "motMODE"],
    "vinit": ["virtual_memory_pool"],
    "LoadAreaMap": ["GlobalAreaMap", "FieldIndex", "FieldArea"],
    "DrawBG": ["OTablePt"],
    "PrepareAccess": ["AccessPower"],
    "handle_balmer_acm_": ["GlobalAreaMap", "FieldIndex", "D_800976E8", "FieldArea"],
    "FUN_80027304": ["Me_MOTION_C", "dtL"],
    "init_score_stats": ["StageBosses", "StageEnemies", "Findenemies", "Murders", "Criticals", "FriendHits"],
    "is_character_state_present_on_stage_": ["Humans"],
    "Think2contact": ["Attrib", "Me_THINK_C", "Degree"],
    "update_something_for_each_visible_enemy_": ["VISIBLE_ENEMIES_"],
    "turn_towards_player_": ["Me_THINK_C", "Degree", "Attrib", "D_80097F10"],
    "Think1trace": ["Me_THINK_C", "Degree", "Attrib"],
    "Think3chase": ["Distance", "SR", "EngageLevel", "AttackActionCount", "Degree", "Me_THINK_C"],
    "handle_char_state_attacking_SEVEN_": ["dtM", "Me_MOTION_C", "dtR"],
    "bow_shoot_logic": ["Me_MOTION_C", "dtR"],
    "Think1watch": ["Me_THINK_C"],
    "Think3firstattack": ["Distance", "SR", "Me_THINK_C", "Attrib", "Degree"],
    "Think3escape": ["Distance", "SR", "Degree", "Attrib", "Me_THINK_C"],
    "Think1ninja": ["Me_THINK_C"],
    "GetArcData": ["ArcData"],
    "AttackFire": ["dtM", "Me_MOTION_C", "dtR"],
    "ReturnNormal": ["Me_MOTION_C", "motID", "motMODE"],
    "DrawOrnament": ["OTablePt"],
    "FUN_8005fe88": ["D_80097E98"],
    "SetupStageSequence": ["StageEvent", "StagePlayer"],
    "AttackGunControl": ["dtM", "Me_MOTION_C"],
    "ThinkBasicHuman1": ["Me_THINK_C"],
    "StartDrawing": ["DrawingPage", "OTablePt", "GameClock"],
    "AttackPQD": ["Me_MOTION_C", "dtM"],
    "SetupSoundEffect": ["StageSE"],
    "initialise_default_player_cameras_": ["CAMERA_PTR_ARRAY_START"],
    "vgetfreesize": ["virtual_memory_pool"],
    "vgetmaxsize": ["virtual_memory_pool"],
    "InitAccessInfo": ["AccessPower"],
    "GetHumanoid": ["Humans"],
    "AttackAnimal": ["Me_THINK_C", "Distance", "Degree"],
    "StickonCheck": ["Me_MOTION_C", "dtL", "motID", "motMODE"],
    "DeleteCard": ["TENCHU_ID"],
    "LoadCard": ["TENCHU_ID"],
    "SaveCard": ["TENCHU_ID"],
    "SetupAfterimage": ["AfterIMG"],
    "MotionAndMove": ["MotionUpdateMode", "Me_MOTION_C", "motID", "motMODE"],
    "FileRead": ["AccessPower", "ReadMode", "TotalIO"],
    "LoadFromDEVPC": ["TotalIO", "ReadMode", "MemoryLoadAddress"],
    "LoadFromCDROM": ["TotalIO", "ReadMode", "MemoryLoadAddress"],
    "SetFrame": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetSplash": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetBleed": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "AttackBowControl": ["dtM", "Me_MOTION_C"],
    "HumanActionControl": ["Me_MOTION_C", "dtPAD", "dtCMD", "motMODE", "dtV", "dtL", "dtR", "dtM", "motID"],
    "AttackCancelControl": ["Me_MOTION_C", "dtM"],
    "DoItemProc": ["Item_fInitial"],
    "vfree": ["virtual_memory_pool"],
    "DrawModel": ["OTablePt"],
    "FUN_8001b2f4": ["D_800976F6"],
    "LoadMotion": ["MotionPack"],
    "SearchMotion": ["CommonMotion", "PlayerMotion", "StageMotion"],
    "GetSpline": ["D_80097708", "D_80097EEC", "D_80097EE8"],
    "GetImage": ["Images_fInitialize"],
    "InitConflict": ["ConflictModel", "ConflictObjects"],
    "ControlAllHumanoid": ["Humans", "VISIBLE_ENEMIES_"],
    "ControlHumanoid": ["VISIBLE_ENEMIES_"],
    "SuccessionAttack": ["Me_THINK_C", "Distance", "Degree", "EngageLevel"],
    "Think3callaid": ["Distance", "SR", "Me_THINK_C", "Degree", "Pad", "Attrib"],
    "InitFileSystem": ["ReadMode", "TotalIO", "MDfat"],
    "cbAccess": ["AccessPower"],
    "FUN_80056e30": ["TENCHU_ID"],
    "FUN_80038fdc": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "FUN_8003944c": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "FUN_8005fe38": ["D_80097E98"],
    "InitMisc": ["Misc_fInitial"],
    "InitializeItem": ["SyurikenModel", "ArrowModel", "NingyoModel", "HappouModel", "sprNapalm", "sprNapalm2", "Item_fInitial"],
    "ProcItemNapalm": ["sprNapalm2"],
    "DoMiscProc": ["Misc_fInitial"],
    "LoadSI": ["CID"],
    "SaveSI": ["CID"],
    "InitializeInfoView": ["fInitialize"],
    "ActSYURI": ["dtM", "Me_MOTION_C", "motID", "motMODE"],
    "ActHANG": ["dtV", "dtM", "dtPAD", "dtL", "motID", "motMODE", "Me_MOTION_C"],
    "ReqItemGun": ["ic"],
    "SetSmokeS": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetupAppearance": ["NowStage", "PLAYER_REDUCE_DAMAGE_DUE_TO_ARMOUR", "sstage", "smode"],
    "StateTransition": ["StrainRatio", "Me_THINK_C", "Pad", "Attrib", "D_80097F1C", "ActionHalt", "EmergencyNotice", "SR", "Distance", "D_80097F10", "D_80097F18", "D_80097F14", "EngageLevel", "Degree"],
    "ActATTACK": ["dtM", "Me_MOTION_C", "motID", "motMODE", "dtR", "dtL", "dtPAD", "MotionUpdateMode", "dtV"],
    "AttackIndirect": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "SR"],
    "AttackLong": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"],
    "AttackGeneral": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"],
    "AttackShort": ["Me_THINK_C", "Distance", "Degree", "EngageLevel", "Attrib", "AttackActionCount"],
    "DamageControl": ["Me_MOTION_C", "MotionUpdateMode", "motID", "dtL", "motMODE", "dtM", "dtR", "D_8009770C", "dtV"],
    "ProcItemNinken": ["NINKEN_CHARACTER_PTR"],
    "ActKAGI": ["dtM", "Me_MOTION_C", "dtL", "motMODE", "motID", "dtR", "MotionUpdateMode", "dtV"],
    "DrawGore": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "ProcItemHenshin": ["D_80097AEC", "D_80097AF0"],
    "ActJUMP": ["Me_MOTION_C", "motID", "dtL", "dtM", "dtV", "dtR", "MotionUpdateMode", "motMODE", "dtPAD"],
    "SwimCheck": ["Me_MOTION_C", "motID", "dtM", "dtL", "motMODE", "MotionUpdateMode"],
    "ActivateHumans": ["D_80097F40", "D_80097F44", "D_80097F42", "StageID"],
    "ProcItemNingyo": ["D_80097AE0", "NingyoModel"],
    "StartStageSequence": ["StageEvent", "StageTime", "FriendHits", "Murders", "Findenemies", "Criticals", "Event", "StagePlayer", "StageCitizens", "StageEnemies", "StageBosses"],
    "StageSequence": ["StagePlayer", "Event", "StageTime", "Findenemies", "Murders", "Criticals", "StageEnemies", "StageBosses", "FriendHits", "StageCitizens"],
    "AddEnemy": ["CurrentEnemyID"],
    "FUN_8005b17c": ["D_80097D38", "D_80097D24", "D_80097D3C", "D_80097D40", "D_80097D28", "CardStateFlag"],
    "LoadConstruction": ["mma", "ObjectArc", "StageID"],
    "CreateStage": ["StageID"],
    "SetWire": ["ModelHook"],
    "think_setting_small_rotation_small_steps_": ["Me_THINK_C", "Attrib", "EmergencyNotice", "Degree", "Distance"],
    "ActCHASE": ["Me_MOTION_C", "dtM", "dtPAD", "MotionUpdateMode", "motID", "motMODE", "dtL", "dtR", "dtCMD"],
    "ActENGAGE": ["dtM", "dtV", "dtPAD", "motID", "Me_MOTION_C", "motMODE", "dtL", "dtCMD", "dtR"],
    "DrawShadow": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_", "ShadowMdl"],
    "register_character_death": ["D_800979DE", "EmergencyNotice"],
    "Think3attack": ["Me_THINK_C", "SR", "Distance", "Degree", "EngageLevel"],
    "SetFlyWire": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "SetGore": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "InitEffect": ["ShadowMdl", "LOCAL_COORDINATES_", "AfterIMG", "ModelHook", "D_80097F30", "D_80097F32"],
    "FUN_80033bc0": ["CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_"],
    "AttackControl": ["Me_MOTION_C", "dtL", "dtR", "motID", "motMODE", "dtPAD", "dtM"],
    "Think1target": ["Me_THINK_C", "SR", "Attrib", "EmergencyNotice"],
    "FUN_8005adbc": ["D_80097D20", "D_80097D24", "D_80097D28"],
    "ActDEAD": ["Me_MOTION_C", "dtM", "dtV", "motID", "dtL", "motMODE", "D_8009770C"],
    "ActSTICKON": ["dtM", "Me_MOTION_C", "dtR", "dtCMD", "motID", "motMODE", "MotionUpdateMode", "dtPAD", "dtV", "D_80097EF0", "dtL"],
    "ActACTION": ["dtM", "Me_MOTION_C", "dtV", "dtPAD", "MotionUpdateMode", "motID", "motMODE"],
    "ActDAMAGE": ["dtM", "Me_MOTION_C", "dtV", "motID", "motMODE", "dtL"],
    "ActITEM": ["dtM", "Me_MOTION_C", "motID", "motMODE"],
    "ActMOVE": ["dtM", "Me_MOTION_C", "dtPAD", "motID", "motMODE", "dtL", "dtR"],
    "ActNORMAL": ["dtM", "dtPAD", "Me_MOTION_C", "motID", "motMODE", "dtR", "dtCMD"],
    "ActSQUAT": ["Me_MOTION_C", "dtM", "dtPAD", "dtR", "motID", "motMODE", "dtV", "dtL", "dtCMD"],
    "ActSTATE": ["dtM", "MotionUpdateMode", "motID", "motMODE", "Me_MOTION_C", "dtV", "dtPAD", "dtR", "dtL"],
    "ActSWIM": ["dtM", "motID", "motMODE", "dtPAD", "Me_MOTION_C", "dtR", "dtV", "dtL"],
    "FUN_8005a7a4": ["D_80097D2E", "D_80097D30", "D_80097D18", "D_80097D32", "CardStateFlag"],
    "FUN_8005aba4": ["CardRetryCount", "CardStateFlag"],
    "FallCheck": ["motID", "Me_MOTION_C", "dtM", "dtL", "motMODE", "MotionUpdateMode"],
    "ItemControl": ["Me_MOTION_C", "motID", "motMODE"],
    "character_balma_around_main_routine_": ["D_800976E8", "GlobalAreaMap", "FieldIndex", "FieldArea"],
    "reset_alert_duration": ["EmergencyNotice"],
    "Think3area": ["Me_THINK_C", "Distance", "SR", "Attrib", "Degree"],
    "create_ninken_character_": ["NINKEN_CHARACTER_PTR"],
    "death_camera_something_": ["LOCAL_COORDINATES_"],
    "debug_output_edit_camera_settings": ["BUTTONS_HELD_DURING_EXPANDED_DEBUG_OUTPUT", "BUTTONS_REGISTERED_FOR_ONE_FRAME_DURING_EXPANDED_DEBUG_OUTPUT", "DEBUG_PRINT_CHOSEN_CAMERA_TYPE_INDEX", "CAMERA_PTR_ARRAY_START"],
    "debug_menu_file_animation_test": ["CVAdata"],
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
    "FUN_80037e0c": ["--expand-div"],
    "SetBlood": ["--expand-div"],
    "SetHinoko": ["--expand-div"],
    "SetupFly": ["--expand-div"],
    "SetSmoke": ["--expand-div"],
    "SetBleeds": ["--expand-div"],
    "PutLifeBar": ["--expand-div"],
    "IsVisible": ["--expand-div"],
    "ComputeAreaLevel": ["--expand-div"],
    "PadProc": ["--expand-div"],
    "DrawSpriteXYZ": ["--expand-div"],
    "DrawSprite": ["--expand-div"],
    "GetAreaMapLevel": ["--expand-div"],
    "bow_shoot_logic": ["--expand-div"],
    "Think3escape": ["--expand-div"],
    "think_setting_small_rotation_small_steps_": ["--expand-div"],
    "SuccessionAttack": ["--expand-div"],
    "GetSpline": ["--expand-div"],
    "StateTransition": ["--expand-div"],
    "AttackIndirect": ["--expand-div"],
    "AttackLong": ["--expand-div"],
    "AttackGeneral": ["--expand-div"],
    "AttackShort": ["--expand-div"],
    "DamageControl": ["--expand-div"],
    "ControlHumanoid": ["--expand-div"],
    "ProcItemNapalm": ["--expand-div"],
    "ProcItemFire": ["--expand-div"],
    "ProcItemJirai": ["--expand-div"],
    "DrawBlood": ["--expand-div"],
    "DrawGore": ["--expand-div"],
    "SwimCheck": ["--expand-div"],
    "ProcItemArrow": ["--expand-div"],
    "SetupBG": ["--expand-div"],
    "SetWire": ["--expand-div"],
    "SetLightningI": ["--expand-div"],
    "register_character_death": ["--expand-div"],
    "SweepMotion": ["--expand-div"],
    "Think3attack": ["--expand-div"],
    "DrawSplash": ["--expand-div"],
    "SetFlyWire": ["--expand-div"],
    "FUN_8002fd9c": ["--expand-div"],
    "ArrangeLocalMatrix": ["--expand-div"],
    "FUN_80033bc0": ["--expand-div"],
    "ActDEAD": ["--expand-div"],
    "ActSTICKON": ["--expand-div"],
    "SetSmokeS": ["--expand-div"],
    "FUN_80033f10": ["--expand-div"],
    "DrawSnow": ["--expand-div"],
    "DrawImpact": ["--expand-div"],
    "FUN_80036284": ["--expand-div"],
    "FUN_8003d768": ["--expand-div"],
}

COMPILE_SH = r"""#!/usr/bin/env bash
# permuter compile: preprocessed C -> object, via this repo's cc1|maspsx|as.
set -e
IN=$(realpath "$1"); OUT=$(realpath -m "$3")
cd {root}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
{cc} {ccflags} < "$IN" > "$TMP/a.s"
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


def function_definition_span(text, name):
    """Return the source span of a same-line C function definition.

    Permuter normally retains a complete preprocessed translation unit, but
    ``--no-context-output`` deliberately writes only the changed function.
    Finding that definition lets authoritative rescoring put it back into the
    declarations and typedefs from base.c.  This is intentionally a small C
    scanner rather than a general parser: the decomp sources put the return
    type and function name on one line, while delimiter matching ignores
    comments and quoted strings.
    """
    signature = re.compile(
        rf"^[ \t]*(?!#)[A-Za-z_][\w \t*]*\b{re.escape(name)}[ \t]*\(",
        re.M,
    )

    def matching_delimiter(start, opening, closing):
        depth = 0
        quote = None
        escaped = False
        line_comment = False
        block_comment = False
        i = start
        while i < len(text):
            char = text[i]
            pair = text[i:i + 2]
            if line_comment:
                if char == "\n":
                    line_comment = False
                i += 1
                continue
            if block_comment:
                if pair == "*/":
                    block_comment = False
                    i += 2
                else:
                    i += 1
                continue
            if quote:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == quote:
                    quote = None
                i += 1
                continue
            if pair == "//":
                line_comment = True
                i += 2
                continue
            if pair == "/*":
                block_comment = True
                i += 2
                continue
            if char in "\"'":
                quote = char
                i += 1
                continue
            if char == opening:
                depth += 1
            elif char == closing:
                depth -= 1
                if depth == 0:
                    return i
            i += 1
        return None

    for match in signature.finditer(text):
        paren = text.find("(", match.start(), match.end())
        paren_end = matching_delimiter(paren, "(", ")")
        if paren_end is None:
            continue
        brace = paren_end + 1
        while brace < len(text) and text[brace].isspace():
            brace += 1
        if brace >= len(text) or text[brace] != "{":
            continue
        brace_end = matching_delimiter(brace, "{", "}")
        if brace_end is not None:
            return match.start(), brace_end + 1
    return None


def contextualize_candidate(name, base, candidate):
    """Splice a function-only permuter candidate into preprocessed ``base``."""
    base_span = function_definition_span(base, name)
    candidate_span = function_definition_span(candidate, name)
    if base_span is None or candidate_span is None:
        return None
    blo, bhi = base_span
    clo, chi = candidate_span
    return base[:blo] + candidate[clo:chi] + base[bhi:]


def _link_candidate(name, csh, source, scratch, ordinal, linker_text,
                    original, addr, extent, base_source=None):
    """Compile and fully link one preprocessed candidate; return raw scores."""
    stem = os.path.join(scratch, f"candidate-{ordinal}")
    obj = stem + ".o"
    def compile_source(path):
        return subprocess.run(
            [csh, path, "-o", obj], stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE, text=True,
        )

    compile_result = compile_source(source)
    if compile_result.returncode and base_source and source != base_source:
        with open(base_source) as stream:
            base = stream.read()
        with open(source) as stream:
            candidate = stream.read()
        contextualized = contextualize_candidate(name, base, candidate)
        if contextualized is not None:
            contextual_source = stem + ".context.c"
            with open(contextual_source, "w") as stream:
                stream.write(contextualized)
            compile_result = compile_source(contextual_source)
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
    # The rescore runs one FULL LINK per retained candidate, and it runs AFTER the
    # search's `timeout` has already fired -- so `timeout 300 permute.py` bounds the
    # SEARCH, not the tool. Two lanes were bitten: one watched the process alive at
    # 526 s under a 420 s bound; another had inner bounds of 300 s AND 240 s both blow
    # through the harness's 600 000 ms HARD CAP, reporting "the documented 300/600
    # guidance cannot work". Enforce our own wall-clock deadline over the rescore and
    # report what we have, rather than being killed mid-loop with nothing.
    budget = float(os.environ.get("PERMUTE_RESCORE_SECONDS", "90"))
    started = time.monotonic()
    results, skipped = [], 0
    with tempfile.TemporaryDirectory(prefix=f"tenchu-permute-{name}-") as scratch:
        for ordinal, source in enumerate(sources):
            if ordinal and time.monotonic() - started > budget:
                skipped = len(sources) - ordinal
                break
            results.append(_link_candidate(
                name, csh, source, scratch, ordinal, linker_text,
                original, addr, extent, sources[0],
            ))
    if skipped:
        # Never silently truncate a measurement -- say what was dropped.
        print(f"\npermute: rescore DEADLINE hit ({budget:.0f}s) — {skipped} of "
              f"{len(sources)} candidate(s) NOT rescored.")
        print("  The ones below are real; the skipped ones are unknown, not bad. "
              "Raise with")
        print("  PERMUTE_RESCORE_SECONDS=<n> if you need them all.")

    valid = sorted((result for result in results if "whole" in result),
                   key=lambda result: (result["whole"], result["window"],
                                       result["source"]))
    print("\npermute: authoritative full-link rescore "
          "(whole image / function window / .text):")
    print("permute: (the per-iteration PROXY scores streamed above are a "
          "relocatable-object heuristic and routinely diverge from — usually "
          "far exceed — these authoritative byte counts; trust ONLY this "
          "rescore, never the streamed proxy best.)")
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


RESULT_NAME = "RESULT.md"

# A declaration whose identifier never appears again. Deliberately conservative:
# one leading type word (optionally qualified/`struct`-tagged) plus an optional
# pointer star. Missing an exotic declaration only costs a hint; a false hit
# would send a reader deleting live code.
DECL_RE = re.compile(
    r"^\s*(?:const |volatile |static |unsigned |signed )*"
    r"(?:(?:struct|union|enum) \w+|\w+)\s+\**\s*(\w+)\s*(?:;|=[^=])",
    re.M)


def _normalized_lines(path):
    """One statement per entry, so the permuter's reflow cannot show as a diff.

    Collapsing whitespace per LINE is not enough: the permuter also moves braces
    and re-breaks lines, so `int F(void) {` vs `int F(void)` + `{` would read as
    an edit. Splitting after every `;`/`{`/`}` instead makes layout unrepresentable
    and leaves only real edits.
    """
    text = re.sub(r"\s+", " ", open(path, errors="replace").read())
    return [part.strip() for part in re.split(r"(?<=[;{}])", text) if part.strip()]


def dead_declarations(text):
    """Identifiers declared in `text` but never used again.

    The permuter invents scratch declarations that ride along into a winning
    candidate without contributing to it (start_demo_'s retained candidate
    carried a dead `GsSPRITE *new_var`). Flag them so whoever transcribes the
    win drops the noise instead of copying it into the draft.
    """
    dead = []
    for match in DECL_RE.finditer(text):
        name = match.group(1)
        if len(re.findall(rf"\b{re.escape(name)}\b", text)) == 1:
            dead.append(name)
    return dead


def semantic_delta(base_path, candidate_path, limit=120):
    """The candidate's edits against base.c, minus reformatting noise.

    The permuter rewrites whitespace/parenthesisation wholesale, so a raw diff
    buries the two or three real edits. Normalizing first is what turns a
    retained candidate into something a reader can act on (start_demo_'s two
    real edits had to be extracted by hand from exactly this diff).
    """
    diff = list(difflib.unified_diff(
        _normalized_lines(base_path), _normalized_lines(candidate_path),
        fromfile="base.c", tofile=os.path.basename(os.path.dirname(candidate_path)),
        lineterm="", n=2))
    if len(diff) > limit:
        diff = diff[:limit] + [f"... ({len(diff) - limit} more diff lines)"]
    return diff


def write_result_report(name, work, valid, interrupted=False):
    """Persist the ranking and the best candidate's delta to a known path.

    Bounded `timeout` runs are the contract (.claude/agents/matcher.md), so the
    normal ending is SIGTERM mid-search — and three separate agents lost a
    finished search's best candidate that way (one recovered it only via
    --rescore-only plus a hand diff; two lost it outright). Every exit path
    writes this file, so a killed run still reports what it found.
    """
    path = os.path.join(work, RESULT_NAME)
    base = os.path.join(work, "base.c")
    out = [f"# permute {name}",
           "",
           f"Run ended: {'INTERRUPTED (bounded timeout / signal)' if interrupted else 'search completed'}.",
           "",
           "## Authoritative full-link rescore (whole image / window / .text)",
           ""]
    if not valid:
        out += ["No candidate survived a full link — the search retained nothing usable.", ""]
    else:
        for result in valid[:20]:
            label = os.path.relpath(result["source"], work)
            out.append(f"    {result['whole']:6d} / {result['window']:5d} / "
                       f"{result['text_size']:5d}  {label}")
        best = valid[0]
        label = os.path.relpath(best["source"], work)
        out += ["",
                f"## Best candidate: `{label}` — {best['whole']} whole-image differing bytes",
                ""]
        if os.path.exists(base):
            delta = semantic_delta(base, best["source"])
            out += ["Minimal semantic delta vs base.c (reformatting normalized away):",
                    "", "```diff"] + delta + ["```", ""]
            dead = dead_declarations(open(best["source"], errors="replace").read())
            if dead:
                out += ["DEAD DECLARATIONS in this candidate (permuter noise — do "
                        "not transcribe): " + ", ".join(dead), ""]
        out += ["Re-verify any transcribed edit with tools/matchdiff.py: this "
                "ranking is a full link, but read the diff for relocated `goto`s "
                "landing after an unconditional jump (dead-code moves score "
                "better while changing reachability).", ""]
    with open(path, "w") as stream:
        stream.write("\n".join(out))
    print(f"permute: result report → {path}")
    return path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rescore-only", action="store_true",
                    help="full-link and rank retained candidates without rerunning permuter")
    ap.add_argument("--force-early", action="store_true",
                    help="override the near-final structural preflight")
    ap.add_argument("name")
    ap.add_argument("rest", nargs=argparse.REMAINDER,
                    help="args after `--` are passed to permuter.py")
    args = ap.parse_args()
    # argparse.REMAINDER intentionally hands everything after NAME to the
    # child.  Also accept the natural `NAME --rescore-only` spelling locally.
    if "--rescore-only" in args.rest:
        args.rescore_only = True
        args.rest.remove("--rescore-only")
    if "--force-early" in args.rest:
        args.force_early = True
        args.rest.remove("--force-early")
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

    diff_lines, blocks, ours, target = draft_shape(name)
    ready, reason = permuter_readiness(diff_lines, blocks, ours, target)
    if not ready and not args.force_early:
        sys.exit(
            "permute: refusing an early stochastic search — " + reason + ".\n"
            "         Finish CFG/loop/expression/stack structure with matchdiff, "
            "rtlguide, and autorules first.\n"
            "         Use --force-early only when RTL proves this broad diff is "
            "one allocation cascade."
        )
    print(f"permute: preflight {'OVERRIDDEN — ' if not ready else ''}{reason}")
    target_s = find_nonmatching_s(name)

    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)

    # compile.sh
    csh = os.path.join(work, "compile.sh")
    gpx = "".join(f" {f}" for f in MASPSX_EXTRA.get(name, []))
    gpx += "".join(f" --gp-extern {s}" for s in GP_EXTERNS.get(name, []))
    open(csh, "w").write(COMPILE_SH.format(
        root=ROOT, cc=cc_executable_for(name),
        ccflags=" ".join(cc_flags_for(name)), asflags=" ".join(AS_FLAGS),
        gpexterns=gpx))
    os.chmod(csh, 0o755)

    # base.c = preprocessed src (what the permuter perturbs). -DNON_MATCHING
    # selects the draft over the INCLUDE_ASM stub for partial functions (a no-op
    # for files without the guard), so the permuter perturbs real C either way.
    base = os.path.join(work, "base.c")
    subprocess.run(CPP + ["-DPERMUTER", "-DNON_MATCHING", src, base], check=True)
    with open(base) as stream:
        preprocessed = stream.read()
    # Strip cpp's `# N "file"` line markers from base.c.
    #
    # NOT because they change codegen -- they do not. I claimed they did, from
    # diffing two `.s` files that differ only in `.stabn` directives and
    # -fverbose-asm comments, neither of which reaches the bytes. Compared on
    # INSTRUCTIONS ONLY, marker-stripped and marker-carrying base.c are identical.
    #
    # The real reason is READABILITY, and it is a measured cost: the permuter parses
    # base.c with pycparser and REGENERATES C, and that printer drops every marker
    # and re-splits every declaration. So base.c (29 markers, `long vx, vy, vz;`)
    # against a candidate (0 markers, three separate declarations) produces a diff in
    # which the actual edit is invisible -- a lane reported "521 diff lines for 3 real
    # edits" and hand-built a filter to find them. Stripping markers here removes the
    # largest source of that noise so RESULT.md's "minimal semantic delta" is worth
    # reading.
    #
    # The deeper caveat stands regardless: the permuter's world is pycparser-reprinted
    # and the build's is not, so a candidate's score is a claim about ITS source, not
    # ours. ALWAYS port a win's semantic delta into the real .c and re-measure with
    # matchdiff -- never adopt a score. (SetupTelop: a candidate scored 9 vs base 11,
    # and its only readable edit -- a declaration move -- was a nullcheck NO-OP in the
    # real source.)
    preprocessed = re.sub(r"^# [0-9]+ \"[^\n]*\n", "", preprocessed, flags=re.M)
    with open(base, "w") as stream:
        stream.write(add_permuter_parser_declarations(preprocessed))

    # target.o = the original function, assembled from its nonmatching .s
    # (all pieces concatenated for split/jump-table functions)
    tgt_s = os.path.join(work, "target.s")
    open(tgt_s, "w").write('.include "macro.inc"\n'
                           + "".join(open(t).read() for t in target_s))
    subprocess.run(["mipsel-unknown-linux-gnu-as", *AS_FLAGS,
                    "-o", os.path.join(work, "target.o"), tgt_s], check=True)

    # sanity: base.c must compile
    subprocess.run([csh, base, "-o", os.path.join(work, "base.o")], check=True)

    # The permuter's C parser cannot read the gte.h macro layer's inline asm
    # ("base.c does not contain any function!"), so this whole class is not a
    # "bounded run then park" step — the run is impossible, and an agent that
    # tries it burns a turn on a confusing parser error. Say so and point at the
    # escalation that does work (docs/gte-policy.md, FUN_80058c70).
    if re.search(r"^\s*#\s*include\s+\"gte\.h\"", open(src).read(), re.M):
        sys.exit(
            f"permute: {name} uses the gte.h macro layer — the permuter's C parser\n"
            "         rejects inline asm and cannot search this class at all.\n"
            "         Escalate straight to RTL: tools/rtlguide.py, then\n"
            "         tools/rtldump.py --pass loop/greg (docs/gte-policy.md)."
        )

    print(f"permute: set up {work} — running permuter…")
    print("permute: BUDGET REMINDER — a <=10-byte register-swap / adjacent-"
          "reorder residual is\n         usually sub-C-level (reload/sched) "
          "and permuter-immune. Give it ONE\n         bounded run (~5-10 min); "
          "if it stays flat, stop and mark NON_MATCHING\n         "
          "(docs/matching-cookbook.md 'sub-C-level residual early-stop').")
    rest = args.rest[1:] if args.rest and args.rest[0] == "--" else args.rest
    if not rest:
        rest = ["--stop-on-zero", "-j4"]

    # The contract runs this under `timeout`, so SIGTERM mid-search is the
    # NORMAL ending, not an error path: rescore and report what the search
    # already retained instead of discarding it. The search owns its own
    # session so a kill here cannot leave permuter workers behind.
    interrupted = False
    rc = 0
    # WATCH THE PERMUTER'S OUTPUT FOR A CRASH. The sanity compile above only proves
    # OUR base.c builds; the permuter then PARSES and REGENERATES that C, and its
    # rewritten source can fail to compile even though ours is fine. When that
    # happens the permuter searches ZERO candidates and exits quietly -- and this
    # tool used to sail on and print "authoritative best -> base.c", which reads as
    # "searched and found nothing". SetupSpline's park recorded exactly that as
    # "a further bounded permuter run also plateaued at 12"; it was never a
    # negative result, it was a crash. A false permuter-immune verdict is expensive:
    # it sends the next lane into RTL archaeology on a search that never ran.
    crashes = []
    CRASH_MARKERS = (
        "Unable to compile",              # the permuter's own base compile failed
        "does not contain any function",  # its parser rejected the source outright
        "Not able to compile",
    )
    try:
        with proclife.interruption_handlers("permute"):
            proc = proclife.owned_popen(["permuter.py", work, *rest],
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, text=True)
            try:
                # Echo as it goes -- do not swallow the permuter's own progress.
                for line in proc.stdout:
                    sys.stdout.write(line)
                    sys.stdout.flush()
                    if any(marker in line for marker in CRASH_MARKERS):
                        crashes.append(line.strip())
                rc = proc.wait()
            except BaseException:
                proclife.terminate_process_group(proc)
                raise
    except (InterruptedError, KeyboardInterrupt) as stop:
        interrupted = True
        rc = 143
        print(f"\npermute: {stop} — rescoring what the search already retained…")

    if crashes:
        # Refuse LOUDLY rather than report the base as a result.
        sys.exit(
            f"\npermute: *** THE PERMUTER NEVER SEARCHED {name}. ***\n"
            + "".join(f"         {line}\n" for line in crashes[:4])
            + "         Our base.c compiles (the sanity build above passed) — the\n"
              "         permuter's own parse/REGENERATE round-trip produced C that\n"
              "         does not compile, so it evaluated ZERO candidates.\n"
              "\n"
              "         **THIS IS NOT A NEGATIVE RESULT.** Do NOT record 'the permuter\n"
              "         found nothing' or 'permuter-immune' for this function — the\n"
              "         search did not run. SetupSpline carried exactly that false\n"
              "         verdict in its park header.\n"
              "\n"
              "         Usual cause: a construct the permuter's C parser mangles\n"
              "         (casts to/from struct types, compound literals, inline asm).\n"
              "         Look at the reported line in .shake/permuter/"
            + f"{name}/base.c,\n"
              "         and see docs/gte-policy.md for the gte.h class, which is\n"
              "         refused up front. Escalate to tools/rtldump.py + the\n"
              "         cookbook's RTL escalation method instead.")

    wins = sorted(glob.glob(os.path.join(work, "output-0-*", "source.c")))
    if wins:
        print(f"\npermute: proxy score 0 retained → {wins[0]}\n"
              "         Full-link rescoring below decides whether it really matches.")
    valid = authoritative_rescore(name, work, csh)
    write_result_report(name, work, valid, interrupted=interrupted)
    sys.exit(rc)


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("permute", target):
            main()
    except MatchToolBusy as e:
        sys.exit(f"permute: {e}")

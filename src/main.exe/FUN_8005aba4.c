#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005aba4 (0x8005aba4) — advance the memory-card state/message machine.
 *
 * STATUS: NON_MATCHING — the complete C reconstruction has the target's exact
 * 536-byte / 134-instruction extent, jump-table pool, calls, and linked-image
 * layout. Thirteen bytes remain across six aligned instructions: combine folds
 * two `(u16)next_state << 16` operations for the known value 0x28 into `lui`,
 * while the shared dynamic shift/test has the target's v0/v1 allocation
 * reversed. Keep the stub and its static jump-table pool until those sub-C
 * compiler ties are solved; the guarded draft builds with
 * `NON_MATCHING=FUN_8005aba4 ./Build`.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", FUN_8005aba4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_1e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_25);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_1f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_20);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_23);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_24);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_26);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_5a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_5);

/* jump-table pool @ 0x80013e34 (93 words; tables at 0x80013e34) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 FUN_8005aba4_jtbl[93] = {
    0x8005ABF8, 0x8005AD0C, 0x8005AD0C, 0x8005AC00,
    0x8005AC0C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005ACAC, 0x8005AD84,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005ACB4, 0x8005AD84, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005ACBC, 0x8005ACF4,
    0x8005AD04, 0x8005AD0C, 0x8005AD0C, 0x8005AD14,
    0x8005AD6C, 0x8005ACEC, 0x8005AD74, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD7C, 0x8005AD84,
    0x8005ACEC,
};

#else /* NON_MATCHING */

extern s16 CardStateFlag;
extern s16 CardRetryCount;

extern s16 ChkCard(void);
extern s16 FormatCard(void);
extern s32 MemCardExist(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s32 FUN_8005aba4(s16 *state, u16 *message)
{
    s32 cmd;
    s32 result;
    u32 shifted_state;
    s16 card_status;
    s16 next_state;
    u16 next_message;

    next_state = *state;
    next_message = *message;

    switch (next_state)
    {
    case 0:
        next_message = 0x10;
        goto increment_state;

    case 3:
        CardRetryCount = 0;
        next_state = 4;
        break;

    case 4:
        card_status = ChkCard();
        if (card_status == 2)
        {
            goto card_status_two;
        }
        if (card_status >= 3)
        {
            goto card_status_ge_three;
        }
        switch (card_status)
        {
        default:
            next_state = 0x28;
            break;
        case 1:
            goto card_status_one;
        }
        shifted_state = (u32)(u16)next_state << 16;
        goto card_state_test;

card_status_ge_three:
        switch (card_status)
        {
        default:
            next_state = 0x28;
            break;
        case 4:
            goto card_status_four;
        }
        shifted_state = (u32)(u16)next_state << 16;
        goto card_state_test;

card_status_one:
        next_state = 10;
        goto card_state_shift;
card_status_two:
        next_state = 0x14;
        goto card_state_shift;
card_status_four:
        CardStateFlag = 0;
        next_state = 0x1e;

card_state_shift:
        shifted_state = (u32)(u16)next_state << 16;
card_state_test:
        if (((s32)shifted_state >> 16) != 0x28 && CardRetryCount++ < 3)
        {
            next_state = 4;
        }
        break;

    case 10:
        next_message = 0x12;
        break;

    case 0x14:
        next_message = 2;
        break;

    case 0x1e:
        result = MemCardExist(0);
        MemCardSync(0, &cmd, &result);
        next_message = 3;
        if (result == 0)
        {
            break;
        }
        next_message = 0;
        /* fallthrough */
    case 0x25:
    case 0x5c:
        next_state = 0;
        break;

    case 0x1f:
        next_message = 10;
        CardRetryCount = 0;
        next_state = 0x21;
        break;

    case 0x20:
        next_state = 0x5a;
        break;

    case 1:
    case 2:
    case 0x21:
    case 0x22:
increment_state:
        next_state++;
        break;

    case 0x23:
        next_state = 0x24;
        card_status = FormatCard();
        if (card_status == 0)
        {
            next_state = 0x26;
        }
        if (next_state != 0x26 && CardRetryCount++ < 3)
        {
            next_state = 0x23;
        }
        break;

    case 0x24:
        next_message = 0xc;
        break;

    case 0x26:
        next_message = 0xb;
        break;

    case 0x5a:
        next_message = 0x14;
        break;

    case 0xb:
    case 0x15:
    case 0x5b:
        next_state = -1;
        break;

    default:
        return 0;
    }

    *state = next_state;
    *message = next_message;
    return -1;
}

#endif /* NON_MATCHING */

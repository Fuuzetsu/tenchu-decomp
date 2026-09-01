#ifndef TENCHU_PADCMD_H
#define TENCHU_PADCMD_H

struct PADtype;

enum
{
    BUTTONS_PER_CONTROL_SCHEME = 8,
    N_CONTROL_SCHEMES = 4,
    N_BUTTON_ASSIGNMENTS = N_CONTROL_SCHEMES * BUTTONS_PER_CONTROL_SCHEME
};

/* Command[] tags for SetCommand — each entry streams a canned input
 * sequence into the pad (decoded from the retail table @ 0x8008686c):
 * the low nibble is the direction (1 up, 2 down, 3 left, 4 right), the
 * high nibble the move family. 0x0N double-taps the direction (dash),
 * 0x1N is direction+Circle,Circle,direction+Circle (crouch roll),
 * CMD_LUNGE is Up+Square,Up (the forward lunge attack). */
#define CMD_DASH_FORWARD 0x01
#define CMD_DASH_BACKWARD 0x02
#define CMD_DASH_LEFT 0x03
#define CMD_DASH_RIGHT 0x04
#define CMD_ROLL_FORWARD 0x11
#define CMD_ROLL_BACKWARD 0x12
#define CMD_ROLL_LEFT 0x13
#define CMD_ROLL_RIGHT 0x14
#define CMD_LUNGE 0x21
/* The remaining two command streams (each with two accepted input
 * variants in the retail table): the down-variant Square special that
 * plays attack motion 0x712, and the Cross flip that plays jump motion
 * 0x907 with a forward push. */
#define CMD_LUNGE_BACK 0x22
#define CMD_FLIP 0x31

/* check_cheat_command_ results (the recognized input combos; retail
 * table @ 0x8008eddc). Consumers: the briefing/shop screen (item cap,
 * refill, unlocks, quit, armour) and PauseProc (revive, debug menu —
 * the R2 L2 R1 L1 + L1+R2-chord sequence). CHEAT_QUIT is the plain
 * Start+Select combo sharing the table: it restores the saved loadout
 * and returns to the menu. CHEAT_ARMOUR selects the armour
 * (selItem[ITEM_ARMOUR]) for non-Rikimaru characters. */
/* Each CHEAT_COMMANDS_ stream is {result code, presses NEWEST-first,
 * -1}: pattern[0] compares against the latest press, so the code is
 * ENTERED in reverse of the stored order below (retail data). */
enum
{
    N_CHEAT_COMMANDS = 7,
    N_CHEAT_COMMAND_TABLE_ENTRIES = N_CHEAT_COMMANDS + 1,
    N_CHEAT_HISTORY_ENTRIES = 12
};

extern short *CHEAT_COMMANDS_[N_CHEAT_COMMAND_TABLE_ENTRIES];
extern unsigned short PAD_HISTORY_[N_CHEAT_HISTORY_ENTRIES];

#define CHEAT_ITEM_CAP 0x1    /* Tri+Left Tri+Down Tri+Right Tri+Up
                               * R1+Tri L1+Tri */
#define CHEAT_ITEM_REFILL 0x2 /* Tri+Left Tri+Right Tri+Down Tri+Up
                               * R1+Tri L1+Tri */
#define CHEAT_ITEM_UNLOCK 0x4 /* Squ+Up Squ+Right Squ+Down Squ+Left
                               * R1+Squ L1+Squ */
#define CHEAT_QUIT 0x8        /* the single Select+Start chord */
#define CHEAT_REVIVE 0x10     /* Right Left Up Down R1 L1 */
#define CHEAT_ARMOUR 0x20     /* Up Left Down Right R1 L1 */
#define CHEAT_DEBUG_MENU 0x1000 /* R2 L2 R1 L1 then R2+L1 chorded with
                                 * Cir Right Squ Left Cro Down Tri Up */

extern void GetPadXY(short no, short *x, short *y);
extern short GetPad(short no);
extern short GetCommand(struct PADtype *pad);
extern short SetCommand(struct PADtype *pad, short cmd);

#endif

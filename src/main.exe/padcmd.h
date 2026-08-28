#ifndef TENCHU_PADCMD_H
#define TENCHU_PADCMD_H

struct PADtype;

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

/* check_cheat_command_ results (the recognized input combos; retail
 * table @ 0x8008eddc). Consumers: the briefing/shop screen (item cap,
 * refill, unlocks, quit, armour) and PauseProc (revive, debug menu —
 * the R2 L2 R1 L1 + L1+R2-chord sequence). CHEAT_QUIT is the plain
 * Start+Select combo sharing the table: it restores the saved loadout
 * and returns to the menu. CHEAT_ARMOUR selects the armour
 * (selItem[ITEM_ARMOUR]) for non-Rikimaru characters. */
#define CHEAT_ITEM_CAP 0x1
#define CHEAT_ITEM_REFILL 0x2
#define CHEAT_ITEM_UNLOCK 0x4
#define CHEAT_QUIT 0x8
#define CHEAT_REVIVE 0x10
#define CHEAT_ARMOUR 0x20
#define CHEAT_DEBUG_MENU 0x1000

extern void GetPadXY(short no, short *x, short *y);
extern short GetPad(short no);
extern short GetCommand(struct PADtype *pad);
extern short SetCommand(struct PADtype *pad, short cmd);

#endif

#ifndef TENCHU_GAMEFLOW_H
#define TENCHU_GAMEFLOW_H

extern ShopItemDefault SHOP_ITEM_DEFAULTS[];

void DoBriefingAndInventorySelection(void);
void BriefingAndInventorySelectionScreen(void);
void clamp_shop_stock_(TLinkInfo *state);
void briefing_screen_(void);
void SelectStage(TLinkInfo *state);
void StageEndScreen(void);
void game_over_screen_(void);

#endif

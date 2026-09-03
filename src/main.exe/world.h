#ifndef TENCHU_WORLD_H
#define TENCHU_WORLD_H

void CreateStage(stage_id stage, int character);
int IsVisible(s32 x, s32 y, s32 z, s32 radius);
void ActivateHumans(void);
void DrawConstruction(void);
void DisposeOrnamentArchive(OrnamentArchiveType *archive);

#endif

#ifndef TENCHU_MODEL_H
#define TENCHU_MODEL_H

/* Coordinate, transform, collision, and attribute state shared by models,
 * model archives, and Sprite3D's ModelType-compatible prefix. */
#define INITIALIZE_MODEL_STATE(model_, parent_)                              \
    GsInitCoordinate2((parent_), &(model_)->locate);                          \
    (model_)->locate.coord.t[0] = 0;                                         \
    (model_)->locate.coord.t[1] = 0;                                         \
    (model_)->locate.coord.t[2] = 0;                                         \
    (model_)->rotate.vx = 0;                                                 \
    (model_)->rotate.vy = 0;                                                 \
    (model_)->rotate.vz = 0;                                                 \
    (model_)->clip.vx = 0;                                                   \
    (model_)->clip.vy = 0;                                                   \
    (model_)->clip.vz = 0;                                                   \
    RotMatrixYXZ(&(model_)->rotate, &(model_)->locate.coord);                 \
    (model_)->locate.flg = 0;                                                \
    (model_)->id = CONFLICT_NONE;                                            \
    (model_)->attribute = 0

/* Common constructor body used by 3DCTRL.C's loaded and cloned objects. */
#define INITIALIZE_MODEL_INSTANCE(model_, parent_)                           \
    {                                                                         \
        (model_)->object.coord2 = &(model_)->locate;                          \
        (model_)->object.attribute = 0;                                       \
        INITIALIZE_MODEL_STATE(model_, parent_);                             \
    }

#define INITIALIZE_ORNAMENT_INSTANCE(ornament_, parent_)                     \
    {                                                                         \
        (ornament_)->object.coord2 = &(ornament_)->locate;                    \
        (ornament_)->object.attribute = 0;                                   \
        GsInitCoordinate2((parent_), &(ornament_)->locate);                   \
        (ornament_)->locate.coord.t[0] = 0;                                  \
        (ornament_)->locate.coord.t[1] = 0;                                  \
        (ornament_)->locate.coord.t[2] = 0;                                  \
        RotMatrixYXZ(&UnitVector, &(ornament_)->locate.coord);                \
        (ornament_)->locate.flg = 0;                                         \
    }

short DrawModel(ModelType *model);
ModelType *LoadModel(u_long *data);
void DisposeModel(ModelType *model);
ModelType *CreateCloneModel(ModelType *model);

ModelArchiveType *LoadModelArchive(u_long *data, ModelType *parent);
short DrawModelArchive(ModelArchiveType *archive, long gap);
ModelArchiveType *CreateCloneModelArchive(ModelArchiveType *archive);
void DisposeModelArchive(ModelArchiveType *archive);

OrnamentType *LoadOrnament(u_long *data);
short DrawOrnament(OrnamentType *ornament);
void DisposeOrnament(OrnamentType *ornament);
OrnamentType *CreateCloneOrnament(OrnamentType *ornament);
void UpdateOrnament(OrnamentType *ornament, short rotation);

void UpdateCoordinate(ModelType *model);
void UpdateCoordinate2(ModelType *model);
VECTOR *GetAbsolutePosition(ModelType *model, short x, short y, short z);
long DrawClip(ModelType *model, long *xy);

#endif

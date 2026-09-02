#ifndef TENCHU_MODEL_H
#define TENCHU_MODEL_H

/* Common constructor bodies used by 3DCTRL.C's loaded and cloned objects. */
#define INITIALIZE_MODEL_INSTANCE(model_, parent_)                            \
    {                                                                         \
        (model_)->object.coord2 = &(model_)->locate;                          \
        (model_)->object.attribute = 0;                                       \
        GsInitCoordinate2((parent_), &(model_)->locate);                      \
        (model_)->locate.coord.t[0] = 0;                                     \
        (model_)->locate.coord.t[1] = 0;                                     \
        (model_)->locate.coord.t[2] = 0;                                     \
        (model_)->rotate.vx = 0;                                             \
        (model_)->rotate.vy = 0;                                             \
        (model_)->rotate.vz = 0;                                             \
        (model_)->clip.vx = 0;                                               \
        (model_)->clip.vy = 0;                                               \
        (model_)->clip.vz = 0;                                               \
        RotMatrixYXZ(&(model_)->rotate, &(model_)->locate.coord);             \
        (model_)->locate.flg = 0;                                            \
        (model_)->id = CONFLICT_NONE;                                        \
        (model_)->attribute = 0;                                             \
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

#endif

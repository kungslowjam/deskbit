#include "anim_manager.h"
#include "lvgl.h"

static const anim_shape_t my_anim_f0_shapes[] = {
    { SHAPE_ELLIPSE, 46.00f, 117.00f, 193.00f, 183.00f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 246.00f, 130.00f, 192.00f, 161.00f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f1_shapes[] = {
    { SHAPE_ELLIPSE, 43.92f, 134.19f, 173.99f, 161.39f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 262.15f, 143.80f, 173.25f, 145.11f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f2_shapes[] = {
    { SHAPE_ELLIPSE, 41.83f, 151.38f, 154.98f, 139.77f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 278.29f, 157.60f, 154.50f, 129.23f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f3_shapes[] = {
    { SHAPE_ELLIPSE, 39.75f, 168.56f, 135.97f, 118.16f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 294.44f, 171.41f, 135.75f, 113.34f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f4_shapes[] = {
    { SHAPE_ELLIPSE, 38.29f, 180.62f, 122.63f, 102.99f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 305.77f, 181.09f, 122.59f, 102.20f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f5_shapes[] = {
    { SHAPE_ELLIPSE, 40.09f, 165.76f, 139.07f, 121.68f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 291.80f, 169.15f, 138.81f, 115.94f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f6_shapes[] = {
    { SHAPE_ELLIPSE, 41.89f, 150.89f, 155.51f, 140.38f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 277.84f, 157.22f, 155.03f, 129.68f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f7_shapes[] = {
    { SHAPE_ELLIPSE, 43.69f, 136.03f, 171.95f, 159.07f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 263.87f, 145.28f, 171.24f, 143.41f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f8_shapes[] = {
    { SHAPE_ELLIPSE, 45.50f, 121.16f, 188.40f, 177.77f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 249.91f, 133.34f, 187.46f, 157.15f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_vector_frame_t my_anim_frames[] = {
    { my_anim_f0_shapes, 2, 83, 0 },
    { my_anim_f1_shapes, 2, 83, 0 },
    { my_anim_f2_shapes, 2, 83, 0 },
    { my_anim_f3_shapes, 2, 83, 0 },
    { my_anim_f4_shapes, 2, 83, 0 },
    { my_anim_f5_shapes, 2, 83, 0 },
    { my_anim_f6_shapes, 2, 83, 0 },
    { my_anim_f7_shapes, 2, 83, 0 },
    { my_anim_f8_shapes, 2, 83, 0 },
};

const anim_vector_t my_anim_data = {
    .name = "my_anim",
    .frames = my_anim_frames,
    .frame_count = 9
};

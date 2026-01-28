#include "anim_manager.h"
#include "lvgl.h"

static const anim_shape_t my_anim_f0_shapes[] = {
    { SHAPE_ELLIPSE, 38.00f, 148.00f, 188.00f, 135.00f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 237.00f, 139.00f, 191.00f, 144.00f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f1_shapes[] = {
    { SHAPE_ELLIPSE, 51.97f, 152.17f, 175.75f, 128.63f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 239.94f, 146.35f, 178.99f, 132.97f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f2_shapes[] = {
    { SHAPE_ELLIPSE, 65.94f, 156.33f, 163.49f, 122.25f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 242.88f, 153.71f, 166.98f, 121.94f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f3_shapes[] = {
    { SHAPE_ELLIPSE, 79.91f, 160.50f, 151.24f, 115.88f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 245.82f, 161.06f, 154.97f, 110.91f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f4_shapes[] = {
    { SHAPE_ELLIPSE, 93.88f, 164.67f, 138.98f, 109.51f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.76f, 168.41f, 142.96f, 99.88f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f5_shapes[] = {
    { SHAPE_ELLIPSE, 77.52f, 159.79f, 153.33f, 116.97f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 245.32f, 159.80f, 157.03f, 112.80f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f6_shapes[] = {
    { SHAPE_ELLIPSE, 58.52f, 154.12f, 170.00f, 125.64f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 241.32f, 149.80f, 173.36f, 127.80f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f7_shapes[] = {
    { SHAPE_ELLIPSE, 39.52f, 148.45f, 186.67f, 134.31f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 237.32f, 139.80f, 189.69f, 142.80f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
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
};

const anim_vector_t my_anim_data = {
    .name = "my_anim",
    .frames = my_anim_frames,
    .frame_count = 8
};

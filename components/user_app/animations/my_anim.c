#include "anim_manager.h"
#include "lvgl.h"

static const anim_shape_t my_anim_f0_shapes[] = {
    { SHAPE_ELLIPSE, 103.00f, 168.00f, 115.00f, 71.00f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 126.00f, 72.00f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f1_shapes[] = {
    { SHAPE_ELLIPSE, 93.43f, 168.00f, 124.57f, 77.64f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 134.33f, 78.53f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f2_shapes[] = {
    { SHAPE_ELLIPSE, 83.86f, 168.00f, 134.14f, 84.29f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 142.67f, 85.06f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f3_shapes[] = {
    { SHAPE_ELLIPSE, 74.28f, 168.00f, 143.72f, 90.93f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 151.00f, 91.59f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f4_shapes[] = {
    { SHAPE_ELLIPSE, 64.71f, 168.00f, 153.29f, 97.58f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 159.33f, 98.13f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f5_shapes[] = {
    { SHAPE_ELLIPSE, 55.14f, 168.00f, 162.86f, 104.22f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 167.67f, 104.66f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f6_shapes[] = {
    { SHAPE_ELLIPSE, 45.57f, 168.00f, 172.43f, 110.86f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 176.00f, 111.19f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f7_shapes[] = {
    { SHAPE_ELLIPSE, 36.00f, 168.00f, 182.00f, 117.51f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 184.33f, 117.72f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f8_shapes[] = {
    { SHAPE_ELLIPSE, 26.42f, 168.00f, 191.58f, 124.15f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 192.67f, 124.25f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f9_shapes[] = {
    { SHAPE_ELLIPSE, 18.71f, 168.00f, 199.29f, 129.51f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 199.38f, 129.52f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f10_shapes[] = {
    { SHAPE_ELLIPSE, 24.61f, 168.00f, 193.39f, 125.41f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 194.24f, 125.49f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f11_shapes[] = {
    { SHAPE_ELLIPSE, 30.51f, 168.00f, 187.49f, 121.31f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 189.11f, 121.46f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f12_shapes[] = {
    { SHAPE_ELLIPSE, 36.42f, 168.00f, 181.58f, 117.22f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 183.97f, 117.43f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f13_shapes[] = {
    { SHAPE_ELLIPSE, 42.32f, 168.00f, 175.68f, 113.12f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 178.83f, 113.41f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f14_shapes[] = {
    { SHAPE_ELLIPSE, 48.22f, 168.00f, 169.78f, 109.02f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 173.69f, 109.38f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f15_shapes[] = {
    { SHAPE_ELLIPSE, 54.12f, 168.00f, 163.88f, 104.92f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 168.55f, 105.35f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f16_shapes[] = {
    { SHAPE_ELLIPSE, 60.03f, 168.00f, 157.97f, 100.83f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 163.41f, 101.32f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f17_shapes[] = {
    { SHAPE_ELLIPSE, 65.93f, 168.00f, 152.07f, 96.73f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 158.27f, 97.29f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f18_shapes[] = {
    { SHAPE_ELLIPSE, 71.83f, 168.00f, 146.17f, 92.63f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 153.13f, 93.27f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f19_shapes[] = {
    { SHAPE_ELLIPSE, 77.74f, 168.00f, 140.26f, 88.54f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 147.99f, 89.24f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f20_shapes[] = {
    { SHAPE_ELLIPSE, 83.64f, 168.00f, 134.36f, 84.44f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 142.86f, 85.21f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f21_shapes[] = {
    { SHAPE_ELLIPSE, 89.54f, 168.00f, 128.46f, 80.34f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 137.72f, 81.18f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f22_shapes[] = {
    { SHAPE_ELLIPSE, 95.44f, 168.00f, 122.56f, 76.24f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 132.58f, 77.16f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_shape_t my_anim_f23_shapes[] = {
    { SHAPE_ELLIPSE, 101.35f, 168.00f, 116.65f, 72.15f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
    { SHAPE_ELLIPSE, 248.00f, 168.00f, 127.44f, 73.13f, 0.00f, 0x00ffff, 1.00f, 0.00f, 0.00f, NULL, 14 },
};

static const anim_vector_frame_t my_anim_frames[] = {
    { my_anim_f0_shapes, 2, 41, 0 },
    { my_anim_f1_shapes, 2, 41, 0 },
    { my_anim_f2_shapes, 2, 41, 0 },
    { my_anim_f3_shapes, 2, 41, 0 },
    { my_anim_f4_shapes, 2, 41, 0 },
    { my_anim_f5_shapes, 2, 41, 0 },
    { my_anim_f6_shapes, 2, 41, 0 },
    { my_anim_f7_shapes, 2, 41, 0 },
    { my_anim_f8_shapes, 2, 41, 0 },
    { my_anim_f9_shapes, 2, 41, 0 },
    { my_anim_f10_shapes, 2, 41, 0 },
    { my_anim_f11_shapes, 2, 41, 0 },
    { my_anim_f12_shapes, 2, 41, 0 },
    { my_anim_f13_shapes, 2, 41, 0 },
    { my_anim_f14_shapes, 2, 41, 0 },
    { my_anim_f15_shapes, 2, 41, 0 },
    { my_anim_f16_shapes, 2, 41, 0 },
    { my_anim_f17_shapes, 2, 41, 0 },
    { my_anim_f18_shapes, 2, 41, 0 },
    { my_anim_f19_shapes, 2, 41, 0 },
    { my_anim_f20_shapes, 2, 41, 0 },
    { my_anim_f21_shapes, 2, 41, 0 },
    { my_anim_f22_shapes, 2, 41, 0 },
    { my_anim_f23_shapes, 2, 41, 0 },
};

const anim_vector_t my_anim_data = {
    .name = "my_anim",
    .frames = my_anim_frames,
    .frame_count = 24
};

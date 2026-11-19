#pragma once
#include "frame.h"
#include "math_utils.h"
#include "model.h"
#include "scene.h"
#include <cstdint>
#include <vector>

using Fixed = int64_t;
constexpr Fixed SUBPIXEL_BITS = 8;
constexpr Fixed SUBPIXEL_SCALE = 1 << SUBPIXEL_BITS; // 256
constexpr Fixed HALF_PIXEL = SUBPIXEL_SCALE / 2;
constexpr Fixed MIN_AREA2 = 2 * SUBPIXEL_SCALE * SUBPIXEL_SCALE;

struct ScreenPoint {
    Fixed x;
    Fixed y;
};

struct ScreenTriangle {
    ScreenPoint p1;
    ScreenPoint p2;
    ScreenPoint p3;
};

struct NdcTriangle {
    vec4 v1;
    vec4 v2;
    vec4 v3;
};

struct Renderer {
    std::vector<vec4> clipVertices;
    ZBuffer zBuffer;

    void render3DModel(const Model& model, Frame& frame, const mat<4, 4>& viewProj);
    void renderScene(const Scene& scene, Frame& frame);
};

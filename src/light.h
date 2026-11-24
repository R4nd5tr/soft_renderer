#pragma once
#include "math_utils.h"

struct Light {                      // if lightVec.w == 0, it's a directional light
    vec4 lightVec = {0, -1, -1, 0}; // if lightVec.w == 1, it's a point light
    RGB color = {255, 255, 255};
    float intensity = 1.0f;
};
struct AmbientLight {
    RGB color = {255, 255, 255};
    float intensity = 0.05f;
};
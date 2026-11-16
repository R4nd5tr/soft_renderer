#pragma once
#include "math.h"
#include "model.h"

struct Camera {
    vec4 position;
    vec4 target;
    vec4 up;
    float fov = 45.0f;
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
};
struct Light {                     // if lightVec.w == 0, it's a directional light
    vec4 lightVec = {0, 0, -1, 0}; // if lightVec.w == 1, it's a point light
    RGB color = {255, 255, 255};
    float intensity = 1.0f;
};
struct Scene {
    std::vector<Model> models;

    Camera camera;

    Light environmentLight;
    std::vector<Light> lights;
};

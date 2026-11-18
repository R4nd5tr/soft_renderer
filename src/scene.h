#pragma once
#include "math_utils.h"
#include "model.h"

struct Camera {
    vec3 position = {0, 0, 5};
    vec3 target = {0, 0, 0};
    vec3 up = {0, 1, 0};
    float fov = 90.0f;
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    mat<4, 4> lookAt() const {
        const vec3 zaxis = normalized(position - target); // 指向相机后方
        const vec3 xaxis = normalized(cross(up, zaxis));  // 右
        const vec3 yaxis = cross(zaxis, xaxis);           // 上（已正交）

        return mat<4, 4>{vec4{xaxis.x, xaxis.y, xaxis.z, -(xaxis * position)},
                         vec4{yaxis.x, yaxis.y, yaxis.z, -(yaxis * position)},
                         vec4{zaxis.x, zaxis.y, zaxis.z, -(zaxis * position)},
                         vec4{0, 0, 0, 1}};
    }
    mat<4, 4> perspective() const {
        float f = 1.0f / std::tan(fov * 0.5f * 3.14159265358979323846f / 180.0f);
        mat<4, 4> projMatrix;
        projMatrix[0] = {f / aspect, 0, 0, 0};
        projMatrix[1] = {0, f, 0, 0};
        projMatrix[2] = {
            0, 0, (farPlane + nearPlane) / (nearPlane - farPlane), (2 * farPlane * nearPlane) / (nearPlane - farPlane)};
        projMatrix[3] = {0, 0, -1, 0};

        return projMatrix;
    }
    mat<4, 4> viewProjection() const { return perspective() * lookAt(); }
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

    void addModel(const std::filesystem::path& objPath) { models.emplace_back(loadModelFromObj(objPath)); }
    void addModel(const Model& model) { models.push_back(model); }
    void addLight(const Light& light) { lights.push_back(light); }
};

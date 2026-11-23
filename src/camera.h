#pragma once
#include "math_utils.h"
#include <algorithm>

constexpr float CAMERA_MIN_DISTANCE = 0.1f;
struct Camera {
    vec3 position = {0, 1, 3}; // -左+右，-下+上，-前+后
    vec3 target = {0, 1, 0};
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
    float distance() const { return norm(position - target); }
    void move(const vec3& delta) {
        position = position + delta;
        target = target + delta;
    }
    void orbit(vec4 q) {
        vec3 direction = position - target;
        vec3 rotatedDirection = {direction.x * (1 - 2 * (q.y * q.y + q.z * q.z)) + direction.y * (2 * (q.x * q.y - q.z * q.w)) +
                                     direction.z * (2 * (q.x * q.z + q.y * q.w)),
                                 direction.x * (2 * (q.x * q.y + q.z * q.w)) + direction.y * (1 - 2 * (q.x * q.x + q.z * q.z)) +
                                     direction.z * (2 * (q.y * q.z - q.x * q.w)),
                                 direction.x * (2 * (q.x * q.z - q.y * q.w)) + direction.y * (2 * (q.y * q.z + q.x * q.w)) +
                                     direction.z * (1 - 2 * (q.x * q.x + q.y * q.y))};
        position = target + rotatedDirection;
    }
    void orbitYaw(float radians) {
        const float half = radians * 0.5f;
        orbit(vec4{0.0f, std::sin(half), 0.0f, std::cos(half)});
    }
    void dolly(float delta) {
        vec3 direction = normalized(target - position);
        float newDistance = std::max(distance() - delta, CAMERA_MIN_DISTANCE);
        position = target - direction * newDistance;
    }
};

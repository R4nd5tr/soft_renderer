#pragma once
#include "math_utils.h"
#include <algorithm>

constexpr float CAMERA_MIN_DISTANCE = 0.1f;
constexpr float CAMERA_PITCH_LIMIT = 1.55f; // ≈ 88.8°，留余量避免 up×forward 退化

struct Camera {
    // ===== 轨道状态（唯一需要保存的东西）=====
    vec3 target = {0, 0, 0}; // 轨道中心 / pivot -左+右，-下+上，-前+后
    float yaw = 0.0f;        // 绕世界 Y 轴，弧度
    float pitch = 0.0f;      // 仰角，弧度，clamp 在 ±CAMERA_PITCH_LIMIT
    float dist = 3.0f;       // 相机到 pivot 的距离

    // ===== 镜头参数 =====
    float fov = 90.0f;
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;

    // 由 (yaw, pitch, dist) 现算 position：pivot + dir * dist
    vec3 position() const {
        const float cp = std::cos(pitch), sp = std::sin(pitch);
        const float cy = std::cos(yaw), sy = std::sin(yaw);
        const vec3 dir = {cp * sy, sp, cp * cy}; // 从 pivot 指向相机，单位向量
        return target + dir * dist;
    }

    mat<4, 4> lookAt() const {
        const vec3 eye = position();
        const vec3 zaxis = normalized(eye - target);     // 从 target 望向相机 → 相机前向 = -zaxis
        constexpr vec3 up = {0, 1, 0};                   // 固定世界 up（原 up 成员从未被改过）
        const vec3 xaxis = normalized(cross(up, zaxis)); // 相机右
        const vec3 yaxis = cross(zaxis, xaxis);          // 相机上（正交化后）

        return mat<4, 4>{vec4{xaxis.x, xaxis.y, xaxis.z, -(xaxis * eye)},
                         vec4{yaxis.x, yaxis.y, yaxis.z, -(yaxis * eye)},
                         vec4{zaxis.x, zaxis.y, zaxis.z, -(zaxis * eye)},
                         vec4{0, 0, 0, 1}};
    }

    mat<4, 4> perspective() const {
        float f = 1.0f / std::tan(fov * 0.5f * PI / 180.0f);
        mat<4, 4> p;
        p[0] = {f / aspect, 0, 0, 0};
        p[1] = {0, f, 0, 0};
        p[2] = {0, 0, (farPlane + nearPlane) / (nearPlane - farPlane), (2 * farPlane * nearPlane) / (nearPlane - farPlane)};
        p[3] = {0, 0, -1, 0};
        return p;
    }

    mat<4, 4> viewProjection() const { return perspective() * lookAt(); }

    // 这些方法就不再“复合”，只是加减标量
    float distance() const { return dist; }
    void move(const vec3& delta) { target = target + delta; } // pivot 平移，position 跟着变

    // 相机水平朝向（指向 target，已投影到 XZ 平面，不含 pitch）
    vec3 forwardDir() const { return {-std::sin(yaw), 0.0f, -std::cos(yaw)}; }
    // 相机右方向（与 lookAt 的 xaxis 同向）
    vec3 rightDir() const { return {std::cos(yaw), 0.0f, -std::sin(yaw)}; }

    void orbitYaw(float r) {
        yaw += r;
        // 可选：yaw = std::fmod(yaw, 2.0f * PI); // 长时间旋转防精度漂移
    }
    void orbitPitch(float r) {
        // 用 -= 保持和你旧代码“鼠标下拉 → 相机抬到上方俯视”的手感一致
        pitch = std::clamp(pitch - r, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);
    }
    void dolly(float delta) { dist = std::max(dist - delta, CAMERA_MIN_DISTANCE); }
};

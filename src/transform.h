#pragma once
#include "math_utils.h"

// ================= 四元数工具（vec4 = {x, y, z, w}）=================
inline vec4 quatIdentity() {
    return {0, 0, 0, 1};
}

inline vec4 quatFromAxisAngle(const vec3& axis, float radians) {
    const vec3 a = normalized(axis);
    const float h = radians * 0.5f, s = std::sin(h);
    return {a.x * s, a.y * s, a.z * s, std::cos(h)};
}

// 欧拉角（弧度）：先绕 Z(roll)，再绕 X(pitch)，最后绕 Y(yaw)，即 q = qYaw * qPitch * qRoll
inline vec4 quatMul(const vec4& a, const vec4& b);
inline vec4 quatFromEuler(float pitch, float yaw, float roll) {
    const vec4 qx = quatFromAxisAngle({1, 0, 0}, pitch);
    const vec4 qy = quatFromAxisAngle({0, 1, 0}, yaw);
    const vec4 qz = quatFromAxisAngle({0, 0, 1}, roll);
    return quatMul(qy, quatMul(qx, qz));
}

// a * b：表示"先应用 b，再应用 a"
inline vec4 quatMul(const vec4& a, const vec4& b) {
    const vec3 av = a.xyz(), bv = b.xyz();
    const vec3 v = bv * a.w + av * b.w + cross(av, bv);
    return {v.x, v.y, v.z, a.w * b.w - av * bv};
}

inline vec4 quatConjugate(const vec4& q) {
    return {-q.x, -q.y, -q.z, q.w};
} // 单位四元数的逆

inline vec4 quatNormalized(const vec4& q) {
    const float n = norm(q);
    return n > 1e-8f ? q / n : quatIdentity();
}

// 用四元数旋转向量：v' = v + w*t + cross(q.xyz, t)，其中 t = 2 * cross(q.xyz, v)
inline vec3 quatRotate(const vec4& q, const vec3& v) {
    const vec3 qv = q.xyz();
    const vec3 t = cross(qv, v) * 2.0f;
    return v + t * q.w + cross(qv, t);
}

// 从 a 转到 b 的最短旋转
inline vec4 quatFromTo(const vec3& from, const vec3& to) {
    const vec3 a = normalized(from), b = normalized(to);
    const float d = a * b;
    if (d < -0.999999f) { // 反向：任选一根垂直轴转 180°
        vec3 axis = cross(vec3{1, 0, 0}, a);
        if (axis * axis < 1e-6f) axis = cross(vec3{0, 1, 0}, a);
        return quatFromAxisAngle(axis, 3.14159265f);
    }
    const vec3 c = cross(a, b);
    return quatNormalized({c.x, c.y, c.z, 1.0f + d});
}

inline vec4 quatSlerp(vec4 a, vec4 b, float t) {
    float d = a * b;
    if (d < 0) {
        b = -b;
        d = -d;
    } // 走短弧
    if (d > 0.9995f) return quatNormalized(lerp(a, b, t)); // 接近时退化为 nlerp，避免除 0
    const float theta = std::acos(d);
    const float sa = std::sin((1 - t) * theta), sb = std::sin(t * theta), s = std::sin(theta);
    return (a * sa + b * sb) / s;
}

// 四元数转 3x3 旋转（存在 mat<4,4> 左上角）
inline mat<4, 4> quatToMat4(const vec4& q) {
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    return mat<4, 4>{vec4{1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0},
                     vec4{2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0},
                     vec4{2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0},
                     vec4{0, 0, 0, 1}};
}

// ================= Transform =================
struct Transform {
    vec3 position{0, 0, 0};
    vec4 rotation{0, 0, 0, 1}; // 单位四元数
    vec3 scale{1, 1, 1};

    // ---- 设置 ----
    void setEuler(float pitch, float yaw, float roll) { rotation = quatFromEuler(pitch, yaw, roll); }
    void setUniformScale(float s) { scale = {s, s, s}; }

    // ---- 增量操作 ----
    void translate(const vec3& d) { position = position + d; }                            // 世界空间平移
    void translateLocal(const vec3& d) { position = position + quatRotate(rotation, d); } // 沿自身轴平移

    // 绕世界轴旋转（左乘）
    void rotateWorld(const vec3& axis, float radians) {
        rotation = quatNormalized(quatMul(quatFromAxisAngle(axis, radians), rotation));
    }
    // 绕自身轴旋转（右乘）
    void rotateLocal(const vec3& axis, float radians) {
        rotation = quatNormalized(quatMul(rotation, quatFromAxisAngle(axis, radians)));
    }
    // 绕世界空间中某个点旋转（位置和朝向都会变）
    void rotateAround(const vec3& pivot, const vec3& axis, float radians) {
        const vec4 q = quatFromAxisAngle(axis, radians);
        position = pivot + quatRotate(q, position - pivot);
        rotation = quatNormalized(quatMul(q, rotation));
    }
    // 让 -Z（和相机一致的"前方"）指向目标
    void lookAt(const vec3& target) {
        const vec3 dir = target - position;
        if (dir * dir < 1e-12f) return;
        rotation = quatFromTo({0, 0, -1}, dir); // 忽略 roll，保持简单
    }

    // ---- 局部坐标轴（世界空间）----
    vec3 right() const { return quatRotate(rotation, {1, 0, 0}); }
    vec3 up() const { return quatRotate(rotation, {0, 1, 0}); }
    vec3 forward() const { return quatRotate(rotation, {0, 0, -1}); }

    // ---- 矩阵 ----
    // M = T * R * S：第 j 列乘以 scale[j]，再放上平移
    mat<4, 4> matrix() const {
        mat<4, 4> m = quatToMat4(rotation);
        for (int i = 0; i < 3; ++i) {
            m[i][0] *= scale.x;
            m[i][1] *= scale.y;
            m[i][2] *= scale.z;
        }
        m[0][3] = position.x;
        m[1][3] = position.y;
        m[2][3] = position.z;
        return m;
    }
    // M^-1 = S^-1 * R^T * T^-1，直接解析构造，比通用 invert() 快得多
    mat<4, 4> inverseMatrix() const {
        const mat<4, 4> r = quatToMat4(rotation);
        const float s[3] = {scale.x, scale.y, scale.z};
        const float t[3] = {position.x, position.y, position.z};
        mat<4, 4> m;
        for (int i = 0; i < 3; ++i) {
            const float inv = 1.0f / s[i];
            float tr = 0;
            for (int j = 0; j < 3; ++j) {
                m[i][j] = r[j][i] * inv; // R^T 的第 i 行除以 s[i]
                tr += r[j][i] * t[j];
            }
            m[i][3] = -tr * inv;
        }
        m[3][3] = 1;
        return m;
    }
    // 法线矩阵 = (M^-1)^T 的左上 3x3 = R * S^-1（比 invert_transpose 的余子式法快且稳）
    mat<4, 4> normalMatrix() const {
        mat<4, 4> m = quatToMat4(rotation);
        for (int i = 0; i < 3; ++i) {
            m[i][0] /= scale.x;
            m[i][1] /= scale.y;
            m[i][2] /= scale.z;
        }
        return m;
    }

    // ---- 点/方向变换 ----
    vec3 transformPoint(const vec3& p) const { return position + quatRotate(rotation, hadamard(p, scale)); }
    vec3 transformDirection(const vec3& d) const { return quatRotate(rotation, hadamard(d, scale)); }
};

// 父子层级：必须在矩阵层面组合，因为"旋转后的非均匀缩放"再叠加会产生剪切，
// 单个 TRS 表示不了
inline mat<4, 4> combine(const Transform& parent, const Transform& child) {
    return parent.matrix() * child.matrix();
}

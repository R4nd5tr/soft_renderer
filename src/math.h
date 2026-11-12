#pragma once
#include <cmath>

struct Vec2 {
    float x, y;
};
struct Vec3 {
    float x, y, z;
    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float dot(Vec3 o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(Vec3 o) const { return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x}; }
    Vec3 normalize() const {
        float l = std::sqrt(dot(*this));
        return {x / l, y / l, z / l};
    }
};
struct Vec4 {
    float x, y, z, w;
    Vec3 xyz() const { return {x, y, z}; }
};
// 列向量约定：Mat4 是 16 个 float，m[col*4+row]，乘法 M*v
struct Mat4 {
    float m[16];
    static Mat4 identity() {
        Mat4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
        return r;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 r{};
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row) {
                float s = 0;
                for (int k = 0; k < 4; ++k)
                    s += m[k * 4 + row] * o.m[c * 4 + k];
                r.m[c * 4 + row] = s;
            }
        return r;
    }
    Vec4 operator*(const Vec4& v) const {
        return {
            m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
            m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
            m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w,
        };
    }
};

Mat4 translate(Vec3 t) {
    Mat4 r = Mat4::identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}
Mat4 scale(Vec3 s) {
    Mat4 r = Mat4::identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}
Mat4 rotateY(float rad) {
    Mat4 r = Mat4::identity();
    float c = std::cos(rad), s = std::sin(rad);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}
Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = (target - eye).normalize();
    Vec3 s = f.cross(up).normalize();
    Vec3 u = s.cross(f);
    Mat4 r = Mat4::identity();
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -s.dot(eye);
    r.m[13] = -u.dot(eye);
    r.m[14] = f.dot(eye);
    return r;
}
Mat4 perspective(float fovY, float aspect, float n, float f) {
    float t = 1.0f / std::tan(fovY * 0.5f);
    Mat4 r{};
    r.m[0] = t / aspect;
    r.m[5] = t;
    r.m[10] = (f + n) / (n - f);
    r.m[11] = -1.0f;
    r.m[14] = (2 * f * n) / (n - f);
    return r;
}
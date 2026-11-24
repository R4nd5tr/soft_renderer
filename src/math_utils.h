#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

constexpr float EPSILON = 1e-6f;                // 长度阈值
constexpr float EPSILON_SQ = EPSILON * EPSILON; // 长度平方阈值 = 1e-12
constexpr float PI = 3.14159265358979323846f;   // 圆周率

// vectors and matrices from tinyrenderer (https://github.com/ssloy/tinyrenderer)
// ========================================
template <int n> struct vec {
    float data[n] = {0};
    float& operator[](const int i) {
        assert(i >= 0 && i < n);
        return data[i];
    }
    float operator[](const int i) const {
        assert(i >= 0 && i < n);
        return data[i];
    }
};

// dot product of two vectors
template <int n> float operator*(const vec<n>& lhs, const vec<n>& rhs) {
    float ret = 0;
    for (int i = 0; i < n; i++)
        ret += lhs[i] * rhs[i];
    return ret;
}

template <int n> vec<n> operator+(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] += rhs[i])
        ;
    return ret;
}

template <int n> vec<n> operator-(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] -= rhs[i])
        ;
    return ret;
}

template <int n> vec<n> operator*(const vec<n>& lhs, const float rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] *= rhs)
        ;
    return ret;
}

template <int n> vec<n> operator*(const float& lhs, const vec<n>& rhs) {
    return rhs * lhs;
}

template <int n> vec<n> operator/(const vec<n>& lhs, const float& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] /= rhs)
        ;
    return ret;
}

template <int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i = 0; i < n; i++)
        out << v[i] << " ";
    return out;
}

template <> struct vec<2> {
    float x = 0, y = 0;
    float& operator[](const int i) {
        assert(i >= 0 && i < 2);
        return i ? y : x;
    }
    float operator[](const int i) const {
        assert(i >= 0 && i < 2);
        return i ? y : x;
    }
};

template <> struct vec<3> {
    float x = 0, y = 0, z = 0;
    float& operator[](const int i) {
        assert(i >= 0 && i < 3);
        return i ? (1 == i ? y : z) : x;
    }
    float operator[](const int i) const {
        assert(i >= 0 && i < 3);
        return i ? (1 == i ? y : z) : x;
    }
};

template <> struct vec<4> {
    float x = 0, y = 0, z = 0, w = 0;
    float& operator[](const int i) {
        assert(i >= 0 && i < 4);
        return i < 2 ? (i ? y : x) : (2 == i ? z : w);
    }
    float operator[](const int i) const {
        assert(i >= 0 && i < 4);
        return i < 2 ? (i ? y : x) : (2 == i ? z : w);
    }
    vec<2> xy() const { return {x, y}; }
    vec<3> xyz() const { return {x, y, z}; }
};

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;

template <int n> float norm(const vec<n>& v) {
    return std::sqrt(v * v);
}

template <int n> vec<n> normalized(const vec<n>& v) {
    const float inv = 1.0f / std::sqrt(v * v);
    return v * inv;
}

inline vec3 cross(const vec3& v1, const vec3& v2) {
    return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

template <int n> struct dt;

template <int nrows, int ncols> struct mat {
    vec<ncols> rows[nrows] = {{}};

    vec<ncols>& operator[](const int idx) {
        assert(idx >= 0 && idx < nrows);
        return rows[idx];
    }
    const vec<ncols>& operator[](const int idx) const {
        assert(idx >= 0 && idx < nrows);
        return rows[idx];
    }

    float det() const { return dt<ncols>::det(*this); }

    float cofactor(const int row, const int col) const {
        mat<nrows - 1, ncols - 1> submatrix;
        for (int i = nrows - 1; i--;)
            for (int j = ncols - 1; j--; submatrix[i][j] = rows[i + int(i >= row)][j + int(j >= col)])
                ;
        return submatrix.det() * ((row + col) % 2 ? -1 : 1);
    }

    mat<nrows, ncols> invert_transpose() const {
        mat<nrows, ncols> adjugate_transpose; // transpose to ease determinant computation, check the last line
        for (int i = nrows; i--;)
            for (int j = ncols; j--; adjugate_transpose[i][j] = cofactor(i, j))
                ;
        return adjugate_transpose / (adjugate_transpose[0] * rows[0]);
    }

    mat<nrows, ncols> invert() const { return invert_transpose().transpose(); }

    mat<ncols, nrows> transpose() const {
        mat<ncols, nrows> ret;
        for (int i = ncols; i--;)
            for (int j = nrows; j--; ret[i][j] = rows[j][i])
                ;
        return ret;
    }
};

template <int nrows, int ncols> vec<ncols> operator*(const vec<nrows>& lhs, const mat<nrows, ncols>& rhs) {
    return (mat<1, nrows>{{lhs}} * rhs)[0];
}

template <int nrows, int ncols> vec<nrows> operator*(const mat<nrows, ncols>& lhs, const vec<ncols>& rhs) {
    vec<nrows> ret;
    for (int i = nrows; i--; ret[i] = lhs[i] * rhs)
        ;
    return ret;
}

template <int R1, int C1, int C2> mat<R1, C2> operator*(const mat<R1, C1>& lhs, const mat<C1, C2>& rhs) {
    mat<R1, C2> result;
    for (int i = R1; i--;)
        for (int j = C2; j--;)
            for (int k = C1; k--; result[i][j] += lhs[i][k] * rhs[k][j])
                ;
    return result;
}

template <int nrows, int ncols> mat<nrows, ncols> operator*(const mat<nrows, ncols>& lhs, const float& val) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--; result[i] = lhs[i] * val)
        ;
    return result;
}

template <int nrows, int ncols> mat<nrows, ncols> operator/(const mat<nrows, ncols>& lhs, const float& val) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--; result[i] = lhs[i] / val)
        ;
    return result;
}

template <int nrows, int ncols> mat<nrows, ncols> operator+(const mat<nrows, ncols>& lhs, const mat<nrows, ncols>& rhs) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--;)
        for (int j = ncols; j--; result[i][j] = lhs[i][j] + rhs[i][j])
            ;
    return result;
}

template <int nrows, int ncols> mat<nrows, ncols> operator-(const mat<nrows, ncols>& lhs, const mat<nrows, ncols>& rhs) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--;)
        for (int j = ncols; j--; result[i][j] = lhs[i][j] - rhs[i][j])
            ;
    return result;
}

template <int nrows, int ncols> std::ostream& operator<<(std::ostream& out, const mat<nrows, ncols>& m) {
    for (int i = 0; i < nrows; i++)
        out << m[i] << std::endl;
    return out;
}

template <int n> struct dt { // template metaprogramming to compute the determinant recursively
    static float det(const mat<n, n>& src) {
        float ret = 0;
        for (int i = n; i--; ret += src[0][i] * src.cofactor(0, i))
            ;
        return ret;
    }
};

template <> struct dt<1> { // template specialization to stop the recursion
    static float det(const mat<1, 1>& src) { return src[0][0]; }
};
// ========================================

// customized functions

template <typename T> T lerp(const T& a, const T& b, float t) {
    return a * (1.0f - t) + b * t;
}
template <int n> vec<n> clamp01(const vec<n>& v) {
    vec<n> ret;
    for (int i = 0; i < n; i++) {
        ret[i] = std::clamp(v[i], 0.0f, 1.0f);
    }
    return ret;
}
template <int n> vec<n> operator-(const vec<n>& v) {
    vec<n> ret;
    for (int i = n; i--; ret[i] = -v[i])
        ;
    return ret;
}
// 逐分量乘法（Hadamard product）
template <int n> vec<n> hadamard(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret;
    for (int i = n; i--; ret[i] = lhs[i] * rhs[i])
        ;
    return ret;
}
template <int n> vec<n>& operator+=(vec<n>& lhs, const vec<n>& rhs) {
    for (int i = 0; i < n; ++i)
        lhs[i] += rhs[i];
    return lhs;
}
inline float powi(float base, int n) {
    float r = 1.0f;
    while (n) {
        if (n & 1) r *= base;
        base *= base;
        n >>= 1;
    }
    return r;
}

// gamma

inline float srgbToLinear(float c) {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
inline float linearToSrgb(float c) {
    return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

struct GammaLUT {
    float toLinear[256];  // 8 位 sRGB -> 线性 float
    uint8_t toSrgb[4096]; // 线性 [0,1] 量化到 4096 级 -> 8 位 sRGB
    GammaLUT() {
        for (int i = 0; i < 256; ++i)
            toLinear[i] = srgbToLinear(i / 255.0f);
        for (int i = 0; i < 4096; ++i)
            toSrgb[i] = static_cast<uint8_t>(linearToSrgb(i / 4095.0f) * 255.0f + 0.5f);
    }
};
inline const GammaLUT GAMMA_LUT;

// color

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    vec3 toFloat() const { return {r / 255.0f, g / 255.0f, b / 255.0f}; }
    vec3 toLinear() const { return {GAMMA_LUT.toLinear[r], GAMMA_LUT.toLinear[g], GAMMA_LUT.toLinear[b]}; }
};
struct RGBA {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    RGB rgb() const { return {r, g, b}; }
};

constexpr RGB WHITE_RGB = {255, 255, 255};
constexpr RGB BLACK_RGB = {0, 0, 0};
constexpr RGB RED_RGB = {255, 0, 0};
constexpr RGB GREEN_RGB = {0, 255, 0};
constexpr RGB BLUE_RGB = {0, 0, 255};
constexpr RGB YELLOW_RGB = {255, 255, 0};
constexpr RGB CYAN_RGB = {0, 255, 255};
constexpr RGB MAGENTA_RGB = {255, 0, 255};

inline RGBA vec3toRGBA(const vec3& c) {
    const vec3 v = clamp01(c);
    return {static_cast<uint8_t>(v.x * 255.0f), static_cast<uint8_t>(v.y * 255.0f), static_cast<uint8_t>(v.z * 255.0f), 255};
}
inline RGBA linearToRGBA_sRGB(const vec3& c) {
    const vec3 v = clamp01(c);
    return {GAMMA_LUT.toSrgb[static_cast<int>(v.x * 4095.0f + 0.5f)],
            GAMMA_LUT.toSrgb[static_cast<int>(v.y * 4095.0f + 0.5f)],
            GAMMA_LUT.toSrgb[static_cast<int>(v.z * 4095.0f + 0.5f)],
            255};
}

#pragma once
#include "math_utils.h"
#include <vector>

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
#if !defined(FRAME_COLOR_ORDER_BGR) && !defined(FRAME_COLOR_ORDER_RGB)
    #if defined(_WIN32) || defined(__linux__)
        #define FRAME_COLOR_ORDER_BGR
    #else
        #define FRAME_COLOR_ORDER_RGB
    #endif
#endif

#if defined(FRAME_COLOR_ORDER_BGR) && defined(FRAME_COLOR_ORDER_RGB)
    #error "FRAME_COLOR_ORDER_BGR 与 FRAME_COLOR_ORDER_RGB 不能同时定义"
#endif

inline constexpr bool FRAME_IS_BGR =
#if defined(FRAME_COLOR_ORDER_BGR)
    true;
#else
    false;
#endif

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    vec3 toFloat() const { return {r / 255.0f, g / 255.0f, b / 255.0f}; }
    vec3 toLinear() const { return {GAMMA_LUT.toLinear[r], GAMMA_LUT.toLinear[g], GAMMA_LUT.toLinear[b]}; }
};
struct RGBA {
#if defined(FRAME_COLOR_ORDER_BGR)
    uint8_t b, g, r, a; // 内存序 B,G,R,A
#else
    uint8_t r, g, b, a; // 内存序 R,G,B,A
#endif
    RGB rgb() const { return {r, g, b}; } // RGB 字段顺序固定，与帧缓冲布局无关
};
inline RGBA makeRGBA(int r, int g, int b, int a = 255) {
    RGBA c;
    c.r = static_cast<uint8_t>(r);
    c.g = static_cast<uint8_t>(g);
    c.b = static_cast<uint8_t>(b);
    c.a = static_cast<uint8_t>(a);
    return c;
}

inline RGBA vec3toRGBA(const vec3& c) {
    const vec3 v = clamp01(c);
    return makeRGBA(static_cast<int>(v.x * 255.0f), static_cast<int>(v.y * 255.0f), static_cast<int>(v.z * 255.0f));
}
inline RGBA linearToRGBA_sRGB(const vec3& c) {
    const vec3 v = clamp01(c);
    return makeRGBA(GAMMA_LUT.toSrgb[static_cast<int>(v.x * 4095.0f + 0.5f)],
                    GAMMA_LUT.toSrgb[static_cast<int>(v.y * 4095.0f + 0.5f)],
                    GAMMA_LUT.toSrgb[static_cast<int>(v.z * 4095.0f + 0.5f)]);
}

#define BOTTOM_LEFT_ORIGIN

struct Frame {
    std::vector<RGBA> data;
    int width = 0;
    int height = 0;
};

struct PixelCoord {
    int x = 0;
    int y = 0;
};

inline void scaleToFrame(Frame& srcFrame, Frame& dstFrame) {
    for (int y = 0; y < dstFrame.height; y++) {
        for (int x = 0; x < dstFrame.width; x++) {
            int sourceX = x * srcFrame.width / dstFrame.width;
            int sourceY = y * srcFrame.height / dstFrame.height;

            size_t src = (sourceY * srcFrame.width + sourceX);
            size_t dst = (y * dstFrame.width + x);

            dstFrame.data[dst] = srcFrame.data[src];
        }
    }
}

inline void clearFrame(Frame& frame) {
    if (frame.data.empty()) return;
    std::fill(frame.data.begin(), frame.data.end(), RGBA{0});
}

inline void drawPixelUnchecked(Frame& frame, const PixelCoord& p, const RGBA& c) {
#ifdef BOTTOM_LEFT_ORIGIN
    frame.data[(frame.height - 1 - p.y) * frame.width + p.x] = c;
#else
    frame.data[p.y * frame.width + p.x] = c;
#endif
}
inline void drawPixel(Frame& frame, const PixelCoord& p, const RGBA& color) {
    if (frame.data.empty()) return;
    if (p.x >= frame.width || p.x < 0 || p.y >= frame.height || p.y < 0) return;

    drawPixelUnchecked(frame, p, color);
}

bool writeFrameToImage(const Frame& frame, const std::string& filename);

struct ZBuffer {
    std::vector<float> data;
    int width = 0;
    int height = 0;

    void resize(int w, int h) {
        width = w;
        height = h;
        data.resize(w * h, 0.0f);
    }

    void clear() { std::fill(data.begin(), data.end(), 0.0f); }
    bool toImage(const std::string& filename);
};

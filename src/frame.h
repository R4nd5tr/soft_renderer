#pragma once
#include "math_utils.h"
#include <vector>

#define BOTTOM_LEFT_ORIGIN

using Color = RGBA;

struct Frame {
    std::vector<Color> data;
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
    std::fill(frame.data.begin(), frame.data.end(), Color{0});
}

inline void drawPixelUnchecked(Frame& frame, const PixelCoord& p, const Color& c) {
#ifdef BOTTOM_LEFT_ORIGIN
    frame.data[(frame.height - 1 - p.y) * frame.width + p.x] = c;
#else
    frame.data[p.y * frame.width + p.x] = c;
#endif
}
inline void drawPixel(Frame& frame, const PixelCoord& p, const Color& color) {
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
        data.resize(w * h, std::numeric_limits<float>::infinity());
    }

    void clear() { std::fill(data.begin(), data.end(), std::numeric_limits<float>::infinity()); }
    bool toImage(const std::string& filename);
};

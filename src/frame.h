#pragma once
#include "math_utils.h"
#include "stb_image_write.h"
#include <vector>

#define BOTTOM_LEFT_ORIGIN

using Color = RGB;

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
    std::fill(frame.data.begin(), frame.data.end(), Color{0, 0, 0});
}

inline void drawPixel(Frame& frame, const PixelCoord& p, const Color& color) {
    if (frame.data.empty()) return;
    if (p.x >= frame.width || p.x < 0 || p.y >= frame.height || p.y < 0) return;

#ifdef BOTTOM_LEFT_ORIGIN
    frame.data[(frame.height - 1 - p.y) * frame.width + p.x] = color;
#else
    frame.data[p.y * frame.width + p.x] = color;
#endif
}

inline bool writeFrameToImage(const Frame& frame, const std::string& filename) {
    if (frame.data.empty() || frame.width <= 0 || frame.height <= 0) return false;
    return stbi_write_png(
        filename.c_str(), frame.width, frame.height, sizeof(Color), frame.data.data(), frame.width * sizeof(Color));
}

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
    bool toImage(const std::string& filename) {
        // Normalize the ZBuffer values to [0, 255] for visualization
        if (data.empty() || width <= 0 || height <= 0) return false;

        float minZ = std::numeric_limits<float>::infinity();
        float maxZ = -std::numeric_limits<float>::infinity();

        for (float z : data) {
            if (!std::isfinite(z)) continue;
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);
        }
        std::vector<uint8_t> imageData(width * height);
        for (int y = 0; y < height; ++y) {
#ifdef BOTTOM_LEFT_ORIGIN
            const int sourceY = height - 1 - y;
#else
            const int sourceY = y;
#endif

            for (int x = 0; x < width; ++x) {
                float z = data[sourceY * width + x];

                uint8_t normalizedZ = 0;
                if (std::isfinite(z) && maxZ > minZ) {
                    normalizedZ = static_cast<uint8_t>(255.0f * (maxZ - z) / (maxZ - minZ));
                }

                imageData[y * width + x] = normalizedZ;
            }
        }
        int result = stbi_write_png(filename.c_str(), width, height, 1, imageData.data(), width);
        return result != 0;
    }
};

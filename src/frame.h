#pragma once
#include "math_utils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>

using Color = RGB;

struct Frame {
    std::vector<Color> data;
    int width = 0;
    int height = 0;
};

struct ZBuffer {
    std::vector<float> data;
    int width = 0;
    int height = 0;
};

struct PixelCoord {
    int x = 0;
    int y = 0;
};

void scaleToFrame(Frame& srcFrame, Frame& dstFrame) {
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

#define BOTTOM_LEFT_ORIGIN
inline void drawPixel(Frame& frame, const PixelCoord& p, const Color& color) {
    if (frame.data.empty()) return;
    if (p.x >= frame.width || p.x < 0 || p.y >= frame.height || p.y < 0) return;

#ifdef BOTTOM_LEFT_ORIGIN
    frame.data[(frame.height - 1 - p.y) * frame.width + p.x] = color;
#else
    frame.data[p.y * frame.width + p.x] = color;
#endif
}

bool writeFrameToImage(const Frame& frame, const std::string& filename) {
    if (frame.data.empty() || frame.width <= 0 || frame.height <= 0) return false;

    std::vector<uint8_t> imageData(frame.width * frame.height * 3);
    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            const Color& color = frame.data[y * frame.width + x];
            size_t idx = (y * frame.width + x) * 3;
            imageData[idx] = color.r;
            imageData[idx + 1] = color.g;
            imageData[idx + 2] = color.b;
        }
    }

    int result = stbi_write_png(filename.c_str(), frame.width, frame.height, 3, imageData.data(), frame.width * 3);
    return result != 0;
}
bool writeZBufferToImage(const ZBuffer& zbuffer, const std::string& filename) {
    // Normalize the ZBuffer values to [0, 255] for visualization
    if (zbuffer.data.empty() || zbuffer.width <= 0 || zbuffer.height <= 0) return false;

    float minZ = std::numeric_limits<float>::infinity();
    float maxZ = -std::numeric_limits<float>::infinity();

    for (float z : zbuffer.data) {
        if (!std::isfinite(z)) continue;
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);
    }
    std::vector<uint8_t> imageData(zbuffer.width * zbuffer.height);
    for (int y = 0; y < zbuffer.height; ++y) {
#ifdef BOTTOM_LEFT_ORIGIN
        const int sourceY = zbuffer.height - 1 - y;
#else
        const int sourceY = y;
#endif

        for (int x = 0; x < zbuffer.width; ++x) {
            float z = zbuffer.data[sourceY * zbuffer.width + x];

            uint8_t normalizedZ = 0;
            if (std::isfinite(z) && maxZ > minZ) {
                normalizedZ = static_cast<uint8_t>(255.0f * (maxZ - z) / (maxZ - minZ));
            }

            imageData[y * zbuffer.width + x] = normalizedZ;
        }
    }
    int result = stbi_write_png(filename.c_str(), zbuffer.width, zbuffer.height, 1, imageData.data(), zbuffer.width);
    return result != 0;
}
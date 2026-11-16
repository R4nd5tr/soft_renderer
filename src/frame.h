#pragma once
#include "math.h"
#include <vector>

using Color = RGB;

struct Frame {
    std::vector<Color> data;
    int width = 0;
    int height = 0;
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

bool writeFrameToImage(const Frame& frame, const std::string& filename);
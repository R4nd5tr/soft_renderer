#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "frame.h"
#include "stb_image_write.h"

bool writeFrameToImage(const Frame& frame, const std::string& filename) {
    if (frame.data.empty() || frame.width <= 0 || frame.height <= 0) return false;
    return stbi_write_png(
        filename.c_str(), frame.width, frame.height, sizeof(Color), frame.data.data(), frame.width * sizeof(Color));
}

bool ZBuffer::toImage(const std::string& filename) {
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
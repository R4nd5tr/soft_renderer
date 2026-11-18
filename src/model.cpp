#include "model.h"
#include <algorithm>
#include <cmath>

RGBA Texture::sampleColor(vec2 uv) const {
    if (data.empty() || width <= 0 || height <= 0) return {0, 0, 0, 0};
    // wrap uv to [0,1]
    uv.x = uv.x - std::floor(uv.x);
    uv.y = uv.y - std::floor(uv.y);

    int x = static_cast<int>(uv.x * width);
    int y = static_cast<int>(uv.y * height);

    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);

    size_t idx = (y * width + x) * (size_t)format;
    RGBA color{0, 0, 0, 255};
    switch (format) {
    case TextureFormat::GRAY:
        color.r = color.g = color.b = data[idx];
        break;
    case TextureFormat::GRAY_ALPHA:
        color.r = color.g = color.b = data[idx];
        color.a = data[idx + 1];
        break;
    case TextureFormat::RGB:
        color.r = data[idx];
        color.g = data[idx + 1];
        color.b = data[idx + 2];
        break;
    case TextureFormat::RGBA:
        color.r = data[idx];
        color.g = data[idx + 1];
        color.b = data[idx + 2];
        color.a = data[idx + 3];
        break;
    default:
        break;
    }
    return color;
}
vec3 Texture::sampleNormal(vec2 uv) const {
    if (data.empty() || width <= 0 || height <= 0) return {0, 0, 1};
    // wrap uv to [0,1]
    uv.x = uv.x - std::floor(uv.x);
    uv.y = uv.y - std::floor(uv.y);
    int x = static_cast<int>(uv.x * width);
    int y = static_cast<int>(uv.y * height);
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);
    size_t idx = (y * width + x) * (size_t)format;
    vec3 normal{0, 0, 1};
    switch (format) {
    case TextureFormat::GRAY:
        normal.x = normal.y = normal.z = data[idx] / 255.0f;
        break;
    case TextureFormat::GRAY_ALPHA:
        normal.x = normal.y = normal.z = data[idx] / 255.0f;
        break;
    case TextureFormat::RGB:
        normal.x = data[idx] / 255.0f;
        normal.y = data[idx + 1] / 255.0f;
        normal.z = data[idx + 2] / 255.0f;
        break;
    case TextureFormat::RGBA:
        normal.x = data[idx] / 255.0f;
        normal.y = data[idx + 1] / 255.0f;
        normal.z = data[idx + 2] / 255.0f;
        break;
    default:
        break;
    }
    // Convert from [0,1] to [-1,1]
    normal = normalized(normal * 2.0 - vec3{1.0, 1.0, 1.0});
    return normal;
}

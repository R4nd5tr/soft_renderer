#define STB_IMAGE_IMPLEMENTATION
#include "model.h"
#include "stb_image.h"

Texture loadTexture(const std::filesystem::path& path) {
    int width, height, channels;
    Texture texture;

    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (!data) return texture; // failed to load image

    texture.data.assign(data, data + width * height * channels);
    texture.width = width;
    texture.height = height;
    stbi_image_free(data);
    switch (channels) {
    case 1:
        texture.format = TextureFormat::GRAY;
        break;
    case 2:
        texture.format = TextureFormat::GRAY_ALPHA;
        break;
    case 3:
        texture.format = TextureFormat::RGB;
        break;
    case 4:
        texture.format = TextureFormat::RGBA;
        break;
    default:
        texture.format = TextureFormat::NONE;
        break;
    }

    return texture;
}

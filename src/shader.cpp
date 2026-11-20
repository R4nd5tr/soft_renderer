#include "shader.h"

static bool shadeUnlit(const Material& material, const ShadingEnv& env, const Fragment& f, Color& out) {
    if (material.baseColorMap.format != TextureFormat::NONE) {
        out = material.baseColorMap.sampleColor(f.uv).rgb();
    } else {
        out = {static_cast<uint8_t>(material.diffuse.x * 255.0f),
               static_cast<uint8_t>(material.diffuse.y * 255.0f),
               static_cast<uint8_t>(material.diffuse.z * 255.0f)};
    }
    return true;
}
static bool shadePhong(const Material& material, const ShadingEnv& env, const Fragment& f, Color& out) {

    return true;
}
static bool shadeNormal(const Material& material, const ShadingEnv& env, const Fragment& f, Color& out) {
    vec3 normal = normalized(f.worldNormal);
    out = {static_cast<uint8_t>((normal.x * 0.5f + 0.5f) * 255.0f),
           static_cast<uint8_t>((normal.y * 0.5f + 0.5f) * 255.0f),
           static_cast<uint8_t>((normal.z * 0.5f + 0.5f) * 255.0f)};
    return true;
}

static constexpr FragmentShaderPtr kShaders[] = {shadeUnlit, shadePhong, shadeNormal};

FragmentShaderPtr getFragmentShader(ShaderKind kind) {
    return kShaders[static_cast<int>(kind)];
}

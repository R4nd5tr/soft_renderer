#pragma once
#include "frame.h"
#include "math_utils.h"
#include "model.h"
#include "scene.h"

struct Fragment {
    vec3 worldPos;
    vec3 worldNormal;
    vec2 uv;
};

struct ShadingEnv {
    vec3 cameraPos;
    const std::vector<Light>* lights = nullptr;
    vec3 ambient{0.05f, 0.05f, 0.05f};
};

struct ShaderContext {
    const Material& material;
    const ShadingEnv& env;
};

enum class ShaderKind { Unlit, Phong, Normal };
using FragmentShader = bool (*)(const ShaderContext&, const Fragment&, Color&);

inline bool unlitShader(const ShaderContext& ctx, const Fragment& f, Color& out) {
    const Material& material = ctx.material;
    if (material.baseColorMap.format != TextureFormat::NONE) {
        out = material.baseColorMap.sampleColor(f.uv).rgb();
    } else {
        out = {static_cast<uint8_t>(material.diffuse.x * 255.0f),
               static_cast<uint8_t>(material.diffuse.y * 255.0f),
               static_cast<uint8_t>(material.diffuse.z * 255.0f)};
    }
    return true;
}
inline bool normalShader(const ShaderContext& ctx, const Fragment& f, Color& out) {
    vec3 normal = normalized(f.worldNormal);
    out = {static_cast<uint8_t>((normal.x * 0.5f + 0.5f) * 255.0f),
           static_cast<uint8_t>((normal.y * 0.5f + 0.5f) * 255.0f),
           static_cast<uint8_t>((normal.z * 0.5f + 0.5f) * 255.0f)};
    return true;
}

inline bool phongShader(const ShaderContext& ctx, const Fragment& f, Color& out) {

    return true;
}
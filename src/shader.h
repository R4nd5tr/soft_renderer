#pragma once
#include "frame.h"
#include "light.h"
#include "math_utils.h"
#include "model.h"
#include <vector>

struct Fragment {
    vec3 worldPos;
    vec3 worldNormal;
    vec3 faceNormal;
    vec2 uv;
    vec3 worldTangent = {0, 0, 0}; // 世界空间切线
    float handedness = 0;
};

enum class ShaderKind;
enum ShaderOptions : uint32_t { AlbedoTexture = 1 << 0, NormalTexture = 1 << 1 };
struct ShadingEnv {
    vec3 cameraPos;
    const std::vector<Light>& lights;
    vec3 ambient;
    const ShaderKind shaderKind;
    const ShaderOptions options;
};

struct ShaderContext {
    const Material& material; // from model
    const ShadingEnv& env;    // from scene
};

inline vec3 albedo(const ShaderContext& ctx, const Fragment& f) {
    const Material& m = ctx.material;
    bool useTexture = ctx.env.options & ShaderOptions::AlbedoTexture;
    if (useTexture && m.baseColorMap.format != TextureFormat::NONE) return m.baseColorMap.sampleColor(f.uv).rgb().toLinear();
    return m.diffuse;
}
inline vec3 normal(const ShaderContext& ctx, const Fragment& f) {
    const Material& m = ctx.material;
    bool useTexture = ctx.env.options & ShaderOptions::NormalTexture;
    // use normal map
    if (useTexture && m.normalMap.format != TextureFormat::NONE) {
        vec3 textureNormal = m.normalMap.sampleNormal(f.uv);
        vec3 N = normalized(f.worldNormal);
        vec3 T = f.worldTangent;

        T = T - N * (T * N); // Gram-Schmidt
        const float tlen = std::sqrt(T * T);
        if (tlen < 1e-6f) {
            T = normalized(cross(N, std::fabs(N.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 1, 0}));
        } else {
            T = T / tlen;
        }

        const float w = (f.handedness < 0.0f) ? -1.0f : 1.0f;
        const vec3 B = cross(N, T) * w;

        // n = x·T + y·B + z·N
        return normalized(T * textureNormal.x + B * textureNormal.y + N * textureNormal.z);
    }
    // use world normal
    const float len2 = f.worldNormal * f.worldNormal;
    if (len2 > EPSILON_SQ) return f.worldNormal / std::sqrt(len2);
    // use face normal
    return f.faceNormal;
}
inline vec3 blinnPhong(const ShaderContext& ctx, const Fragment& f, const vec3& N) {
    const Material& m = ctx.material;

    vec3 albedoColor = albedo(ctx, f);
    vec3 result = hadamard(ctx.env.ambient, albedoColor);
    if (ctx.env.lights.empty()) return result;

    const vec3 V = normalized(ctx.env.cameraPos - f.worldPos);

    for (const Light& light : ctx.env.lights) {
        vec3 L;
        float attenuation = 1.0f;
        if (light.lightVec.w == 0.0f) // directional light
            L = normalized(-light.lightVec.xyz());
        else { // point light
            const vec3 toLight = light.lightVec.xyz() - f.worldPos;
            const float d2 = (toLight * toLight);
            const float invLen = 1.0f / std::sqrt(d2);
            L = toLight * invLen;
            attenuation = 1.0f / (1.0f + d2);
        }

        float LdotN = L * N;
        if (LdotN <= 0.0f) continue;
        const vec3 lightColor = light.color.toFloat() * light.intensity;

        // diffuse
        result += hadamard(albedoColor, lightColor) * LdotN * attenuation;

        // specular
        const vec3 H = normalized(L + V);
        const float HdotN = std::max(H * N, 0.0f);
        float spec;
        if (m.shininessInt > 0) spec = powi(HdotN, m.shininessInt);
        else
            spec = std::pow(HdotN, m.shininess);
        result += hadamard(m.specular, lightColor) * LdotN * attenuation * spec;
    }
    return result;
}

enum class ShaderKind { Unlit, Normal, FlatBlinnPhong, BlinnPhong };
using FragmentShader = bool (*)(const ShaderContext&, const Fragment&, Color&);

inline bool unlitShader(const ShaderContext& ctx, const Fragment& f, Color& out) {
    out = linearToRGBA_sRGB(albedo(ctx, f));
    return true;
}
inline bool normalShader(const ShaderContext& ctx, const Fragment& f, Color& out) {
    vec3 normalVec = normal(ctx, f);
    out = vec3toRGBA(normalVec * 0.5f + vec3{0.5f, 0.5f, 0.5f});
    return true;
}
inline bool flatBlinnPhongShader(const ShaderContext& ctx, const Fragment& f, Color& out) {
    out = linearToRGBA_sRGB(blinnPhong(ctx, f, f.faceNormal)); // use face normal
    return true;
}
inline bool blinnPhongShader(const ShaderContext& ctx, const Fragment& f, Color& out) {
    out = linearToRGBA_sRGB(blinnPhong(ctx, f, normal(ctx, f))); // use normal
    return true;
}

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

enum class ShaderKind { Unlit, Phong, Normal };
using FragmentShaderPtr = bool (*)(const Material&, const ShadingEnv&, const Fragment&, Color&);

FragmentShaderPtr getFragmentShader(ShaderKind kind);

struct Shader {
    const Material& material;
    const ShadingEnv& env;
    FragmentShaderPtr ptr;
    bool shade(const Fragment& f, Color& out) const { return ptr(material, env, f, out); }
};

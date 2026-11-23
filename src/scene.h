#pragma once
#include "camera.h"
#include "light.h"
#include "math_utils.h"
#include "model.h"
#include "shader.h"
#include <algorithm>

struct Scene {
    std::vector<Model> models;

    Camera camera;

    AmbientLight ambientLight;
    std::vector<Light> lights;

    ShaderKind shaderKind = ShaderKind::Unlit;
    ShaderOptions options = static_cast<ShaderOptions>(0xFFFFFFFF);

    void addModel(const std::filesystem::path& objPath) { models.emplace_back(loadModelFromObj(objPath)); }
    void addModel(const Model& model) { models.push_back(model); }
    void addLight(const Light& light) { lights.push_back(light); }
    void clearOption(ShaderOptions opt) { options = static_cast<ShaderOptions>(options & ~opt); }
    void setOption(ShaderOptions opt) { options = static_cast<ShaderOptions>(options | opt); }
};

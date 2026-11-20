#pragma once
#include "frame.h"
#include "math_utils.h"
#include "model.h"
#include "scene.h"
#include "shader.h"
#include <cstdint>
#include <vector>

using Fixed = int64_t;
constexpr Fixed SUBPIXEL_BITS = 8;
constexpr Fixed SUBPIXEL_SCALE = 1 << SUBPIXEL_BITS; // 256
constexpr Fixed HALF_PIXEL = SUBPIXEL_SCALE / 2;
constexpr Fixed MIN_AREA2 = SUBPIXEL_SCALE * SUBPIXEL_SCALE; // half pixel area

struct ScreenPoint {
    Fixed x;
    Fixed y;
};
struct ScreenTriangle {
    ScreenPoint p1;
    ScreenPoint p2;
    ScreenPoint p3;
};

struct NdcTriangle {
    vec4 v1;
    vec4 v2;
    vec4 v3;
};
struct VertexOut {
    vec4 clipVert = {0, 0, 0, 0}; // 裁剪空间坐标
    // varyings
    vec3 worldPos = {0, 0, 0};    // 世界空间位置（光照要用）
    vec3 worldNormal = {0, 0, 0}; // 世界空间法线
    vec2 uv = {0, 0};             // 纹理坐标（OBJ 原始朝向）
};
struct Primitive {
    VertexOut v[3];
};

struct Renderer {
    std::vector<vec4> clipVertices;
    std::vector<vec3> worldNormals;
    std::vector<vec3> worldVertices;
    ZBuffer zBuffer;

    Primitive getPrimitive(const Model& model, const Mesh& mesh, const SubMesh& sub, size_t triIndex) const;
    void render3DModel(const Model& model, Frame& frame, const mat<4, 4>& viewProj, const ShadingEnv& env);
    void renderScene(const Scene& scene, Frame& frame);
};

// // rasterizer3d.h
// template <typename ShaderT>
// void rasterize3D(Frame&, ZBuffer&, const Primitive&, const ShaderT& shader);

// // render3DModel: 每个 submesh 一次 switch，而不是每像素
// switch (shaderKind) {
// case ShaderKind::Unlit: rasterize3D(frame, zbuf, tri, UnlitShader{material, env}); break;
// case ShaderKind::Phong: rasterize3D(frame, zbuf, tri, PhongShader{material, env}); break;
// case ShaderKind::Normal: rasterize3D(frame, zbuf, tri, NormalShader{material, env}); break;
// }
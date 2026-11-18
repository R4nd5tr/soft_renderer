#pragma once
#include "math_utils.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

enum class TextureFormat { NONE, GRAY, GRAY_ALPHA, RGB, RGBA };
struct Texture {
    std::filesystem::path path;
    TextureFormat format = TextureFormat::NONE;
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data;
    RGBA sampleColor(vec2 uv) const;
    vec3 sampleNormal(vec2 uv) const;
};
enum class AlphaMode {
    Opaque, // 不透明
    Mask,   // 低于阈值就丢弃像素
    Blend   // 半透明混合
};
struct Material {
    std::string name; // newmtl

    vec3 ambient{0.0f, 0.0f, 0.0f};       // Ka
    vec3 diffuse{1.0f, 1.0f, 1.0f};       // Kd
    vec3 specular{0.0f, 0.0f, 0.0f};      // Ks
    vec3 transmittance{0.0f, 0.0f, 0.0f}; // Tf
    vec3 emissive{0.0f, 0.0f, 0.0f};      // Ke

    float shininess = 32.0;        // Ns
    float indexOfRefraction = 1.0; // Ni
    float opacity = 1.0;           // d
    int illuminationModel = 2;     // illum
    AlphaMode alphaMode = AlphaMode::Opaque;

    Texture baseColorMap;   // map_Kd
    Texture opacityMap;     // map_d / map_opacity
    Texture normalMap;      // norm / bump / map_bump
    Texture ambientMap;     // map_Ka
    Texture specularMap;    // map_Ks
    Texture specularExpMap; // map_Ns
    Texture reflectionMap;  // refl
    Texture emissiveMap;    // map_Ke
};
struct VertexIndices {
    int vertexIndex = -1;
    int normalIndex = -1;
    int uvIndex = -1;
};
struct SubMesh {            // a part of a Mesh that uses the same material
    int materialIndex = -1; // Index into the materials vector of the Model struct
    size_t indexOffset = 0; // Start index in the indices vector of the Mesh struct
    size_t indexCount = 0;  // Number of indices in the indices vector of the Mesh struct
};
struct Mesh {         // one shape in the .obj file
    std::string name; // o/g in the .obj file
    std::vector<VertexIndices> indices;
    std::vector<SubMesh> subMeshes; // different materials
};
struct Model { // one .obj file corresponds to one Model
    std::filesystem::path path;

    // global pool of materials, vertices, normals, uvs
    std::vector<Material> materials;
    std::vector<vec4> vertices;
    std::vector<vec4> normals;
    std::vector<vec2> uvs;

    // one .obj file can have multiple shapes, each shape corresponds to one Mesh
    std::vector<Mesh> meshes;

    mat<4, 4> transform = {vec4{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}; // Identity matrix
};

Model loadModelFromObj(const std::filesystem::path& objPath);

inline const Material DEFAULT_MATERIAL = [] {
    Material material{};
    material.name = "DefaultMaterial";
    return material;
}();
inline const Model CUBE_MODEL = {

    {},                 // default path
    {DEFAULT_MATERIAL}, // default material
    {
        // vertices
        {-1, -1, -1, 1},
        {1, -1, -1, 1},
        {1, 1, -1, 1},
        {-1, 1, -1, 1}, // back face z = -1
        {-1, -1, 1, 1},
        {1, -1, 1, 1},
        {1, 1, 1, 1},
        {-1, 1, 1, 1} // front face z = +1
    },
    // normals
    {{0, 0, -1, 0}, {0, 0, 1, 0}, {0, -1, 0, 0}, {0, 1, 0, 0}, {-1, 0, 0, 0}, {1, 0, 0, 0}},
    // uvs：每个面 4 个，按该面逆时针顺序 (0,0)->(1,0)->(1,1)->(0,1)
    {
        // back face  z = -1, normalIndex = 0
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}, // uv 0..3
                      // front face z = +1, normalIndex = 1
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}, // uv 4..7
                      // bottom face y = -1, normalIndex = 2
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}, // uv 8..11
                      // top face y = +1, normalIndex = 3
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}, // uv 12..15
                      // left face x = -1, normalIndex = 4
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}, // uv 16..19
                      // right face x = +1, normalIndex = 5
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f} // uv 20..23
    },
    // meshes
    {
        {"Cube", // name
         {       // VertexIndices: {vertexIndex, normalIndex, uvIndex}

          // back face z = -1, normal (0,0,-1)
          // quad 顺序: 0 -> 3 -> 2 -> 1
          {0, 0, 0},
          {3, 0, 1},
          {2, 0, 2},
          {0, 0, 0},
          {2, 0, 2},
          {1, 0, 3},

          // front face z = +1, normal (0,0,1)
          // quad 顺序: 4 -> 5 -> 6 -> 7
          {4, 1, 4},
          {5, 1, 5},
          {6, 1, 6},
          {4, 1, 4},
          {6, 1, 6},
          {7, 1, 7},

          // bottom face y = -1, normal (0,-1,0)
          // quad 顺序: 0 -> 1 -> 5 -> 4
          {0, 2, 8},
          {1, 2, 9},
          {5, 2, 10},
          {0, 2, 8},
          {5, 2, 10},
          {4, 2, 11},

          // top face y = +1, normal (0,1,0)
          // quad 顺序: 3 -> 7 -> 6 -> 2
          {3, 3, 12},
          {7, 3, 13},
          {6, 3, 14},
          {3, 3, 12},
          {6, 3, 14},
          {2, 3, 15},

          // left face x = -1, normal (-1,0,0)
          // quad 顺序: 0 -> 4 -> 7 -> 3
          {0, 4, 16},
          {4, 4, 17},
          {7, 4, 18},
          {0, 4, 16},
          {7, 4, 18},
          {3, 4, 19},

          // right face x = +1, normal (1,0,0)
          // quad 顺序: 1 -> 2 -> 6 -> 5
          {1, 5, 20},
          {2, 5, 21},
          {6, 5, 22},
          {1, 5, 20},
          {6, 5, 22},
          {5, 5, 23}},
         // subMeshes
         {{0, 0, 36}}}
        // default transform
    }

};

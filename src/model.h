#pragma once
#include "math_utils.h"
#include <cstdint>
#include <filesystem>
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

    double shininess = 32.0;        // Ns
    double indexOfRefraction = 1.0; // Ni
    double opacity = 1.0;           // d
    int illuminationModel = 2;      // illum
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

const Material DEFAULT_MATERIAL = [] {
    Material material{};
    material.name = "DefaultMaterial";
    return material;
}();
const Model CUBE_MODEL = {
    {},                 // default path
    {DEFAULT_MATERIAL}, // default material
    {
        // vertices
        {-1, -1, -1, 1},
        {1, -1, -1, 1},
        {1, 1, -1, 1},
        {-1, 1, -1, 1}, // back face
        {-1, -1, 1, 1},
        {1, -1, 1, 1},
        {1, 1, 1, 1},
        {-1, 1, 1, 1} // front face
    },
    // normals
    {{0, 0, -1, 0}, {0, 0, 1, 0}, {0, -1, 0, 0}, {0, 1, 0, 0}, {-1, 0, 0, 0}, {1, 0, 0, 0}},
    // uvs
    {{0.0f, 0.0f}},
    // meshes
    {{"Cube", // name
      {
          // indices
          {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 0, 0}, {3, 0, 0}, {0, 0, 0}, // back face
          {4, 1, 0}, {5, 1, 0}, {6, 1, 0}, {6, 1, 0}, {7, 1, 0}, {4, 1, 0}, // front face
          {0, 2, 0}, {4, 2, 0}, {5, 2, 0}, {5, 2, 0}, {1, 2, 0}, {0, 2, 0}, // bottom face
          {3, 3, 0}, {2, 3, 0}, {6, 3, 0}, {6, 3, 0}, {7, 3, 0}, {3, 3, 0}, // top face
          {0, 4, 0}, {3, 4, 0}, {7, 4, 0}, {7, 4, 0}, {4, 4, 0}, {0, 4, 0}, // left face
          {1, 5, 0}, {5, 5, 0}, {6, 5, 0}, {6, 5, 0}, {2, 5, 0}, {1, 5, 0}  // right face
      },
      // subMeshes
      {{0, 0, 36}}}}
    // default transform
};

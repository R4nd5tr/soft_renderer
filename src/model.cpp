#include "model.h"
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include <cmath>

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
Model loadModelFromObj(const std::filesystem::path& path) {
    Model model;
    model.path = path;

    tinyobj::ObjReaderConfig cfg;
    cfg.triangulate = true;
    cfg.mtl_search_path = path.parent_path().string();

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path.string(), cfg)) {
        std::cerr << reader.Error() << reader.Warning();
        return model;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    model.vertices.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        model.vertices.emplace_back(vec4{attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2], 1.0});
    }
    model.normals.reserve(attrib.normals.size() / 3);
    for (size_t i = 0; i < attrib.normals.size(); i += 3) {
        model.normals.emplace_back(vec4{attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2], 0.0});
    }
    model.uvs.reserve(attrib.texcoords.size() / 2);
    for (size_t i = 0; i < attrib.texcoords.size(); i += 2) {
        model.uvs.emplace_back(vec2{attrib.texcoords[i], attrib.texcoords[i + 1]});
    }

    // ---- 材质 ----
    for (const auto& mat : materials) {
        Material material;
        material.name = mat.name;
        material.ambient = {mat.ambient[0], mat.ambient[1], mat.ambient[2]};
        material.diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]};
        material.specular = {mat.specular[0], mat.specular[1], mat.specular[2]};
        material.transmittance = {mat.transmittance[0], mat.transmittance[1], mat.transmittance[2]};
        material.emissive = {mat.emission[0], mat.emission[1], mat.emission[2]};
        material.shininess = mat.shininess;
        material.opacity = mat.dissolve;
        material.indexOfRefraction = mat.ior;
        material.illuminationModel = mat.illum;

        if (!mat.diffuse_texname.empty()) material.baseColorMap = loadTexture(path.parent_path() / mat.diffuse_texname);
        if (!mat.specular_texname.empty()) material.specularMap = loadTexture(path.parent_path() / mat.specular_texname);
        if (!mat.normal_texname.empty()) material.normalMap = loadTexture(path.parent_path() / mat.normal_texname);
        if (!mat.alpha_texname.empty()) material.opacityMap = loadTexture(path.parent_path() / mat.alpha_texname);
        if (!mat.ambient_texname.empty()) material.ambientMap = loadTexture(path.parent_path() / mat.ambient_texname);
        if (!mat.specular_highlight_texname.empty())
            material.specularExpMap = loadTexture(path.parent_path() / mat.specular_highlight_texname);
        if (!mat.reflection_texname.empty()) material.reflectionMap = loadTexture(path.parent_path() / mat.reflection_texname);
        if (!mat.emissive_texname.empty()) material.emissiveMap = loadTexture(path.parent_path() / mat.emissive_texname);

        model.materials.emplace_back(std::move(material));
    }

    if (model.materials.empty()) model.materials.emplace_back(DEFAULT_MATERIAL);

    // ---- 每个 shape 生成一个 Mesh，内部按 material_id 合并 SubMesh ----
    for (const auto& shape : shapes) {
        Mesh mesh;
        mesh.name = shape.name;

        const auto& rawIndices = shape.mesh.indices;
        const auto& matIds = shape.mesh.material_ids;
        size_t triCount = matIds.size(); // triangulate=true 后，三角形数 == material_ids.size()

        mesh.indices.reserve(rawIndices.size());
        for (const auto& idx : rawIndices)
            mesh.indices.emplace_back(VertexIndices{idx.vertex_index, idx.normal_index, idx.texcoord_index});

        // 按连续相同 material_id 合并 SubMesh
        size_t triStart = 0;
        int currentMat = matIds.empty() ? -1 : matIds[0];
        for (size_t f = 0; f < triCount; ++f) {
            int mat = matIds[f];
            if (mat != currentMat) {
                SubMesh sm;
                sm.materialIndex = currentMat < 0 ? static_cast<int>(model.materials.size()) : currentMat;
                sm.indexOffset = triStart * 3;
                sm.indexCount = (f - triStart) * 3;
                mesh.subMeshes.push_back(sm);
                currentMat = mat;
                triStart = f;
            }
        }
        // 最后一个
        SubMesh sm;
        sm.materialIndex = currentMat < 0 ? static_cast<int>(model.materials.size()) : currentMat;
        sm.indexOffset = triStart * 3;
        sm.indexCount = (triCount - triStart) * 3;
        mesh.subMeshes.push_back(sm);

        model.meshes.emplace_back(std::move(mesh));
    }

    return model;
}

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

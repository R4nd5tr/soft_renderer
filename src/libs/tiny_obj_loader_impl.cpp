#define TINYOBJLOADER_IMPLEMENTATION
#include "model.h"
#include "tiny_obj_loader.h"
#include <unordered_map>
#include <vector>

Texture loadTexture(const std::filesystem::path& path);
struct TangentAccumulator {
    vec3 tangent = {0.0, 0.0, 0.0};
    vec3 bitangent = {0.0, 0.0, 0.0};
    int normalIndex = 0;
};
std::pair<vec3, vec3> computeTangentFromTriangle(const Model& model, const Mesh& mesh, int triangleIndex) {
    const auto& vi0 = mesh.indices[triangleIndex + 0];
    const auto& vi1 = mesh.indices[triangleIndex + 1];
    const auto& vi2 = mesh.indices[triangleIndex + 2];
    if (vi0.vertexIndex < 0 || vi1.vertexIndex < 0 || vi2.vertexIndex < 0 || vi0.uvIndex < 0 || vi1.uvIndex < 0 ||
        vi2.uvIndex < 0)
        return {vec3{1, 0, 0}, vec3{0, 1, 0}};

    const auto& v0 = model.vertices[vi0.vertexIndex];
    const auto& v1 = model.vertices[vi1.vertexIndex];
    const auto& v2 = model.vertices[vi2.vertexIndex];
    const auto& uv0 = model.uvs[vi0.uvIndex];
    const auto& uv1 = model.uvs[vi1.uvIndex];
    const auto& uv2 = model.uvs[vi2.uvIndex];

    const vec3 edge1 = v1.xyz() - v0.xyz();
    const vec3 edge2 = v2.xyz() - v0.xyz();
    const vec2 deltaUV1 = uv1 - uv0;
    const vec2 deltaUV2 = uv2 - uv0;

    const float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
    if (std::fabs(denom) < EPSILON) return {vec3{1, 0, 0}, vec3{0, 1, 0}};
    const float inv = 1.0f / denom;

    const vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * inv;
    const vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * inv;

    return {tangent, bitangent};
}
void computeTangents(Model& model) {
    if (model.uvs.empty()) return;
    if (model.normals.empty()) return;
    for (auto& mesh : model.meshes) {
        const size_t numIndices = mesh.indices.size();
        mesh.tangents.assign(numIndices, {{1.0, 0.0, 0.0}, 1.0});

        // tangent accumulators
        std::vector<TangentAccumulator> accs;
        accs.reserve(numIndices);
        std::unordered_map<VertexIndices, size_t> vertIdxToAcc;
        std::vector<int> idxToAcc(numIndices);

        // init accumulator
        for (size_t i = 0; i < numIndices; ++i) {
            const auto& vi = mesh.indices[i];
            auto it = vertIdxToAcc.find(vi);
            if (it == vertIdxToAcc.end()) {
                int id = static_cast<int>(accs.size());
                vertIdxToAcc.emplace(vi, id);
                accs.emplace_back(TangentAccumulator{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, vi.normalIndex});
                idxToAcc[i] = id;
            } else {
                idxToAcc[i] = it->second;
            }
        }

        // iterate over triangles to compute tangents
        for (size_t t = 0; t + 2 < numIndices; t += 3) {
            const auto [T, B] = computeTangentFromTriangle(model, mesh, t);
            for (int i = 0; i < 3; ++i) {
                const int id = idxToAcc[t + i];
                accs[id].tangent += T;
                accs[id].bitangent += B;
            }
        }

        // normalize and Gram-Schmidt orthogonalize tangent vectors
        std::vector<Tangent> results;
        results.reserve(accs.size());
        for (auto& acc : accs) {
            vec3 N = (acc.normalIndex >= 0 && acc.normalIndex < static_cast<int>(model.normals.size()))
                         ? normalized(model.normals[acc.normalIndex].xyz())
                         : vec3{0, 1, 0};
            vec3 T = acc.tangent;

            float lenT = norm(T);
            if (lenT < EPSILON) {
                T = normalized(cross(N, std::fabs(N.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 1, 0}));
            } else {
                T = T / lenT;
                T = T - (T * N) * N;
                lenT = norm(T);
                if (lenT < EPSILON) {
                    T = normalized(cross(N, std::fabs(N.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 1, 0}));
                } else {
                    T = T / lenT;
                }
            }

            const vec3 crossB = cross(N, T);
            const float handedness = (crossB * acc.bitangent) < 0.0f ? -1.0f : 1.0f;

            results.emplace_back(Tangent{T, handedness});
        }

        for (size_t i = 0; i < numIndices; ++i) {
            mesh.tangents[i] = results[idxToAcc[i]];
        }
    }
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
        if (std::isfinite(mat.shininess) && std::floor(mat.shininess) == mat.shininess) {
            material.shininessInt = static_cast<int>(mat.shininess);
        } else {
            material.shininessInt = 0;
        }
        material.opacity = mat.dissolve;
        material.indexOfRefraction = mat.ior;
        material.illuminationModel = mat.illum;

        if (!mat.diffuse_texname.empty()) material.baseColorMap = loadTexture(path.parent_path() / mat.diffuse_texname);
        if (!mat.specular_texname.empty()) material.specularMap = loadTexture(path.parent_path() / mat.specular_texname);
        if (!mat.normal_texname.empty()) material.normalMap = loadTexture(path.parent_path() / mat.normal_texname);
        else if (!mat.bump_texname.empty())
            material.normalMap = loadTexture(path.parent_path() / mat.bump_texname);
        if (!mat.alpha_texname.empty()) material.opacityMap = loadTexture(path.parent_path() / mat.alpha_texname);
        if (!mat.ambient_texname.empty()) material.ambientMap = loadTexture(path.parent_path() / mat.ambient_texname);
        if (!mat.specular_highlight_texname.empty())
            material.specularExpMap = loadTexture(path.parent_path() / mat.specular_highlight_texname);
        if (!mat.reflection_texname.empty()) material.reflectionMap = loadTexture(path.parent_path() / mat.reflection_texname);
        if (!mat.emissive_texname.empty()) material.emissiveMap = loadTexture(path.parent_path() / mat.emissive_texname);

        model.materials.emplace_back(std::move(material));
    }

    const int defaultMat = static_cast<int>(model.materials.size());
    model.materials.emplace_back(DEFAULT_MATERIAL);

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
                sm.materialIndex = currentMat < 0 ? defaultMat : currentMat;
                sm.indexOffset = triStart * 3;
                sm.indexCount = (f - triStart) * 3;
                mesh.subMeshes.push_back(sm);
                currentMat = mat;
                triStart = f;
            }
        }
        // 最后一个
        SubMesh sm;
        sm.materialIndex = currentMat < 0 ? defaultMat : currentMat;
        sm.indexOffset = triStart * 3;
        sm.indexCount = (triCount - triStart) * 3;
        mesh.subMeshes.push_back(sm);

        model.meshes.emplace_back(std::move(mesh));
    }

    computeTangents(model);
    return model;
}

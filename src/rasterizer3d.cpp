#include "rasterizer3d.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

int floorDiv(Fixed v, Fixed s) {
    Fixed q = v / s;
    if ((v % s != 0) && ((v < 0) != (s < 0))) --q;
    return static_cast<int>(q);
}
int FixedToPixel(Fixed v) {
    return floorDiv(v, SUBPIXEL_SCALE);
}
Fixed toSubPixel(float ndc, int size) {
    return static_cast<Fixed>(std::llround((ndc + 1.0) * SUBPIXEL_SCALE * size * 0.5));
}

Fixed signedArea2(const ScreenTriangle& t) {
    return (t.p2.x - t.p1.x) * (t.p3.y - t.p1.y) - (t.p3.x - t.p1.x) * (t.p2.y - t.p1.y);
}
bool isTopLeftEdge(const ScreenPoint& a, const ScreenPoint& b) {
    const Fixed dx = b.x - a.x;
    const Fixed dy = b.y - a.y;
#ifdef BOTTOM_LEFT_ORIGIN
    return dy < 0 || (dy == 0 && dx < 0);
#else
    return dy < 0 || (dy == 0 && dx > 0);
#endif
}
NdcTriangle clipToNdc(const Primitive& t) {
    const auto& [v1, v2, v3] = t.v;
    return {v1.clipVert / v1.clipVert.w, v2.clipVert / v2.clipVert.w, v3.clipVert / v3.clipVert.w};
}
ScreenTriangle projectNdcToScreen(const NdcTriangle& t, int width, int height) {
    return {{toSubPixel(t.v1.x, width), toSubPixel(t.v1.y, height)},
            {toSubPixel(t.v2.x, width), toSubPixel(t.v2.y, height)},
            {toSubPixel(t.v3.x, width), toSubPixel(t.v3.y, height)}};
}
void rasterize3D(Frame& frame, ZBuffer& zBuffer, const Primitive& tri, const Shader& shader) {
    const auto& [v1, v2, v3] = tri.v;
    NdcTriangle ndct = clipToNdc(tri);
    ScreenTriangle t = projectNdcToScreen(ndct, frame.width, frame.height);

    Fixed totalArea2 = signedArea2(t);
    if (totalArea2 < MIN_AREA2) return; // backface culling and degenerate triangle check
    const float invArea2 = 1.0f / static_cast<float>(totalArea2);

    // bounding box of the triangle, clamped to the frame boundaries
    int minX = std::max(0, FixedToPixel(std::min(t.p1.x, std::min(t.p2.x, t.p3.x))));
    int maxX = std::min(frame.width - 1, FixedToPixel(std::max(t.p1.x, std::max(t.p2.x, t.p3.x))));
    int minY = std::max(0, FixedToPixel(std::min(t.p1.y, std::min(t.p2.y, t.p3.y))));
    int maxY = std::min(frame.height - 1, FixedToPixel(std::max(t.p1.y, std::max(t.p2.y, t.p3.y))));
    if (minX > maxX || minY > maxY) return; // skip drawing if the triangle is entirely outside the frame

    // precompute per-pixel deltas for efficient barycentric coordinate updates.
    Fixed deltaE1Col = (t.p2.y - t.p3.y) * SUBPIXEL_SCALE;
    Fixed deltaE2Col = (t.p3.y - t.p1.y) * SUBPIXEL_SCALE;
    Fixed deltaE3Col = (t.p1.y - t.p2.y) * SUBPIXEL_SCALE;
    Fixed deltaE1Row = (t.p3.x - t.p2.x) * SUBPIXEL_SCALE;
    Fixed deltaE2Row = (t.p1.x - t.p3.x) * SUBPIXEL_SCALE;
    Fixed deltaE3Row = (t.p2.x - t.p1.x) * SUBPIXEL_SCALE;
    ScreenPoint p0{static_cast<Fixed>(minX) * SUBPIXEL_SCALE + HALF_PIXEL,
                   static_cast<Fixed>(minY) * SUBPIXEL_SCALE + HALF_PIXEL}; // center of the pixel at (minX, minY)
    Fixed e1Row = signedArea2({p0, t.p2, t.p3});
    Fixed e2Row = signedArea2({p0, t.p3, t.p1});
    Fixed e3Row = signedArea2({p0, t.p1, t.p2});

    // precompute perspective-correct barycentric coordinates
    const float invClipW1 = 1.0f / v1.clipVert.w;
    const float invClipW2 = 1.0f / v2.clipVert.w;
    const float invClipW3 = 1.0f / v3.clipVert.w;

    // precompute top-left rule
    bool isTopLeft1 = isTopLeftEdge(t.p2, t.p3);
    bool isTopLeft2 = isTopLeftEdge(t.p3, t.p1);
    bool isTopLeft3 = isTopLeftEdge(t.p1, t.p2);

    // iterate over the bounding box of the triangle per pixel
    for (int y = minY; y <= maxY; y++) {
        Fixed e1 = e1Row, e2 = e2Row, e3 = e3Row; // e = signedArea2 of the sub-triangle, e1 + e2 + e3 = totalArea2
        for (int x = minX; x <= maxX; x++) {
            if (e3 >= 0 && e1 >= 0 && e2 >= 0 && // check if the pixel is inside the triangle
                (e1 != 0 || isTopLeft1) && (e2 != 0 || isTopLeft2) && (e3 != 0 || isTopLeft3)) { // top-left rule
                const float b1 = static_cast<float>(e1) * invArea2, b2 = static_cast<float>(e2) * invArea2,
                            b3 = static_cast<float>(e3) * invArea2; // Normalized barycentric coordinates, b1 + b2 + b3 == 1

                const float depth = ndct.v1.z * b1 + ndct.v2.z * b2 + ndct.v3.z * b3;
                float& z = zBuffer.data[y * frame.width + x];
                if (depth <= z) {
                    const float p1 = b1 * invClipW1, p2 = b2 * invClipW2,
                                p3 = b3 * invClipW3;          // Perspective-correct barycentric coordinates
                    const float invP = 1.0f / (p1 + p2 + p3); // for normalization

                    Fragment f;
                    f.uv = (v1.uv * p1 + v2.uv * p2 + v3.uv * p3) * invP;
                    f.worldPos = (v1.worldPos * p1 + v2.worldPos * p2 + v3.worldPos * p3) * invP;
                    f.worldNormal = normalized((v1.worldNormal * p1 + v2.worldNormal * p2 + v3.worldNormal * p3) * invP);

                    Color color;
                    if (shader.shade(f, color)) {
                        z = depth;
                        drawPixel(frame, {x, y}, color);
                    }
                    // drawPixel(frame,
                    //           {x, y},
                    //           {static_cast<uint8_t>(b1 * 255.0f),
                    //            static_cast<uint8_t>(b2 * 255.0f),
                    //            static_cast<uint8_t>(b3 * 255.0f)});
                }
            }
            e1 += deltaE1Col;
            e2 += deltaE2Col;
            e3 += deltaE3Col;
        }
        e1Row += deltaE1Row;
        e2Row += deltaE2Row;
        e3Row += deltaE3Row;
    }
}
Primitive Renderer::getPrimitive(const Model& model, const Mesh& mesh, const SubMesh& subMesh, size_t triIndex) const {
    Primitive tri;
    for (int i = 0; i < 3; ++i) {
        const VertexIndices& vi = mesh.indices[subMesh.indexOffset + triIndex + i];
        tri.v[i].clipVert = clipVertices[vi.vertexIndex];
        tri.v[i].worldPos = worldVertices[vi.vertexIndex];
        if (vi.normalIndex >= 0) tri.v[i].worldNormal = worldNormals[vi.normalIndex];
        if (vi.uvIndex >= 0) tri.v[i].uv = model.uvs[vi.uvIndex];
    }
    return tri;
}
void Renderer::render3DModel(const Model& model, Frame& frame, const mat<4, 4>& viewProj, const ShadingEnv& env) {
    mat<4, 4> mvp = viewProj * model.transform;
    mat<4, 4> normalMat = model.transform.invert_transpose();

    const size_t numVertices = model.vertices.size();
    clipVertices.resize(numVertices);
    worldNormals.resize(model.normals.size());
    worldVertices.resize(numVertices);

    // Transform all vertices of the model to clip space and world space
    for (size_t i = 0; i < numVertices; ++i) {
        clipVertices[i] = mvp * model.vertices[i];
        worldVertices[i] = (model.transform * model.vertices[i]).xyz();
    }
    // Transform all normals of the model to world space
    for (size_t i = 0; i < model.normals.size(); ++i)
        worldNormals[i] = (normalMat * model.normals[i]).xyz();

    // model
    for (const auto& mesh : model.meshes) {          // mesh
        for (const auto& subMesh : mesh.subMeshes) { // material submesh
            const Material& material = model.materials[subMesh.materialIndex];
            ShaderKind shaderKind = ShaderKind::Unlit;
            Shader shader{material, env, getFragmentShader(shaderKind)};

            for (size_t i = 0; i < subMesh.indexCount; i += 3) { // triangle
                const auto t = getPrimitive(model, mesh, subMesh, i);
                auto& [v1, v2, v3] = t.v;
                if (v1.clipVert.w <= 0 || v2.clipVert.w <= 0 || v3.clipVert.w <= 0) continue; // 简化近平面处理

                rasterize3D(frame, zBuffer, t, shader);
            }
        }
    }
}

void Renderer::renderScene(const Scene& scene, Frame& frame) {
    mat<4, 4> viewProj = scene.camera.viewProjection();
    zBuffer.resize(frame.width, frame.height);
    zBuffer.clear();

    ShadingEnv env = {
        scene.camera.position, &scene.lights, scene.environmentLight.color.toFloat() * scene.environmentLight.intensity};

    for (const auto& model : scene.models)
        render3DModel(model, frame, viewProj, env);
}

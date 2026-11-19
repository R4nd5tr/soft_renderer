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
NdcTriangle clipToNdc(const ClipTriangle& t) {
    auto& [c1, c2, c3] = t.c;
    return {c1.clipVert / c1.clipVert.w, c2.clipVert / c2.clipVert.w, c3.clipVert / c3.clipVert.w};
}
ScreenTriangle projectNdcToScreen(const NdcTriangle& t, int width, int height) {
    return {{toSubPixel(t.v1.x, width), toSubPixel(t.v1.y, height)},
            {toSubPixel(t.v2.x, width), toSubPixel(t.v2.y, height)},
            {toSubPixel(t.v3.x, width), toSubPixel(t.v3.y, height)}};
}
float depthAtBarycentric(const NdcTriangle& tri, float w1, float w2, float w3) {
    return tri.v1.z * w1 + tri.v2.z * w2 + tri.v3.z * w3;
}

void rasterize3D(Frame& frame, ZBuffer& zBuffer, const ClipTriangle& clipt, const Material& material) {
    NdcTriangle ndct = clipToNdc(clipt);
    ScreenTriangle t = projectNdcToScreen(ndct, frame.width, frame.height);

    // bounding box of the triangle, clamped to the frame boundaries
    int minX = std::max(0, FixedToPixel(std::min({t.p1.x, t.p2.x, t.p3.x})));
    int maxX = std::min(frame.width - 1, FixedToPixel(std::max({t.p1.x, t.p2.x, t.p3.x})));
    int minY = std::max(0, FixedToPixel(std::min({t.p1.y, t.p2.y, t.p3.y})));
    int maxY = std::min(frame.height - 1, FixedToPixel(std::max({t.p1.y, t.p2.y, t.p3.y})));
    if (minX > maxX || minY > maxY) return; // skip drawing if the triangle is entirely outside the frame

    Fixed totalArea2 = signedArea2(t);
    if (totalArea2 < MIN_AREA2) return; // backface culling and degenerate triangle check

    // precompute per-pixel deltas for efficient barycentric coordinate updates.
    Fixed deltaW1Col = (t.p1.y - t.p2.y) * SUBPIXEL_SCALE;
    Fixed deltaW2Col = (t.p2.y - t.p3.y) * SUBPIXEL_SCALE;
    Fixed deltaW3Col = (t.p3.y - t.p1.y) * SUBPIXEL_SCALE;
    Fixed deltaW1Row = (t.p2.x - t.p1.x) * SUBPIXEL_SCALE;
    Fixed deltaW2Row = (t.p3.x - t.p2.x) * SUBPIXEL_SCALE;
    Fixed deltaW3Row = (t.p1.x - t.p3.x) * SUBPIXEL_SCALE;
    ScreenPoint p0{static_cast<Fixed>(minX) * SUBPIXEL_SCALE + HALF_PIXEL,
                   static_cast<Fixed>(minY) * SUBPIXEL_SCALE + HALF_PIXEL}; // center of the pixel at (minX, minY)
    Fixed w1Row = signedArea2({p0, t.p1, t.p2});
    Fixed w2Row = signedArea2({p0, t.p2, t.p3});
    Fixed w3Row = signedArea2({p0, t.p3, t.p1});

    // iterate over the bounding box of the triangle per pixel
    for (int y = minY; y <= maxY; y++) {
        Fixed w1 = w1Row, w2 = w2Row, w3 = w3Row;
        for (int x = minX; x <= maxX; x++) {
            bool inside = w1 >= 0 && w2 >= 0 && w3 >= 0;
            bool topLeft = (w1 != 0 || isTopLeftEdge(t.p1, t.p2)) && (w2 != 0 || isTopLeftEdge(t.p2, t.p3)) &&
                           (w3 != 0 || isTopLeftEdge(t.p3, t.p1));

            if (inside && topLeft) {
                float depth = depthAtBarycentric(ndct,
                                                 static_cast<float>(w2) / totalArea2,
                                                 static_cast<float>(w3) / totalArea2,
                                                 static_cast<float>(w1) / totalArea2);

                float& z = zBuffer.data[y * frame.width + x];
                if (depth <= z) {
                    z = depth;
                    drawPixel(frame,
                              {x, y},
                              {static_cast<uint8_t>(w2 * 255 / totalArea2),   // R = v1 权重
                               static_cast<uint8_t>(w3 * 255 / totalArea2),   // G = v2 权重
                               static_cast<uint8_t>(w1 * 255 / totalArea2)}); // B = v3 权重
                }
            }

            w1 += deltaW1Col;
            w2 += deltaW2Col;
            w3 += deltaW3Col;
        }
        w1Row += deltaW1Row;
        w2Row += deltaW2Row;
        w3Row += deltaW3Row;
    }
}
ClipTriangle Renderer::getClipTriangle(const Model& model, const Mesh& mesh, const SubMesh& subMesh, size_t triIndex) const {
    ClipTriangle clipt;
    for (int i = 0; i < 3; ++i) {
        const VertexIndices& vi = mesh.indices[subMesh.indexOffset + triIndex + i];
        clipt.c[i].clipVert = clipVertices[vi.vertexIndex];
        clipt.c[i].worldNormal = worldNormals[vi.normalIndex];
        clipt.c[i].worldVert = worldVertices[vi.vertexIndex];
        clipt.c[i].uv = model.uvs[vi.uvIndex];
    }
    return clipt;
}
void Renderer::render3DModel(const Model& model, Frame& frame, const mat<4, 4>& viewProj) {
    mat<4, 4> mvp = viewProj * model.transform;
    mat<4, 4> normalMat = model.transform.invert_transpose();

    const size_t numVertices = model.vertices.size();
    clipVertices.resize(numVertices);
    worldNormals.resize(numVertices);
    worldVertices.resize(numVertices);

    // Transform all vertices of the model to clip space and world space
    for (size_t i = 0; i < numVertices; ++i) {
        clipVertices[i] = mvp * model.vertices[i];
        worldVertices[i] = (model.transform * model.vertices[i]).xyz();
    }
    // Transform all normals of the model to world space
    for (size_t i = 0; i < model.normals.size(); ++i)
        worldNormals[i] = (normalMat * model.normals[i]).xyz();

    for (const auto& mesh : model.meshes) {
        for (const auto& subMesh : mesh.subMeshes) {
            for (size_t i = 0; i < subMesh.indexCount; i += 3) {
                auto t = getClipTriangle(model, mesh, subMesh, i);
                auto& [c1, c2, c3] = t.c;
                if (c1.clipVert.w <= 0 || c2.clipVert.w <= 0 || c3.clipVert.w <= 0) continue; // 简化近平面处理
                rasterize3D(frame, zBuffer, t, model.materials[subMesh.materialIndex]);
            }
        }
    }
}

void Renderer::renderScene(const Scene& scene, Frame& frame) {
    mat<4, 4> viewProj = scene.camera.viewProjection();
    zBuffer.resize(frame.width, frame.height);
    zBuffer.clear();

    for (const auto& model : scene.models)
        render3DModel(model, frame, viewProj);
}
#pragma once
#include "frame.h"
#include "math_utils.h"
#include "model.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

// using Fixed = int64_t;

// constexpr Fixed SUBPIXEL_SCALE = 256;
// constexpr Fixed HALF_PIXEL = SUBPIXEL_SCALE / 2;

// struct FixedPoint {
//     Fixed x;
//     Fixed y;
// };

// struct FixedTriangle {
//     FixedPoint p1;
//     FixedPoint p2;
//     FixedPoint p3;
// };

struct Point {
    int x;
    int y;
};

struct Triangle {
    Point p1;
    Point p2;
    Point p3;
};

struct Face {
    vec4 v1;
    vec4 v2;
    vec4 v3;
};

inline int signedArea2(const Triangle& t) {
    return (t.p2.x - t.p1.x) * (t.p3.y - t.p1.y) - (t.p3.x - t.p1.x) * (t.p2.y - t.p1.y);
}
inline float signedArea2(const Face& f) {
    return (f.v2.x - f.v1.x) * (f.v3.y - f.v1.y) - (f.v3.x - f.v1.x) * (f.v2.y - f.v1.y);
}
inline bool isTopLeftEdge(const Point& a, const Point& b) {
    const int dx = b.x - a.x;
    const int dy = b.y - a.y;
#ifdef BOTTOM_LEFT_ORIGIN
    return dy < 0 || (dy == 0 && dx < 0);
#else
    return dy < 0 || (dy == 0 && dx > 0);
#endif
}
inline Triangle projectFaceToTriangle(const Face& f, int width, int height) {
    return {{static_cast<int>(std::round((f.v1.x + 1.) * width / 2)), static_cast<int>(std::round((f.v1.y + 1.) * height / 2))},
            {static_cast<int>(std::round((f.v2.x + 1.) * width / 2)), static_cast<int>(std::round((f.v2.y + 1.) * height / 2))},
            {static_cast<int>(std::round((f.v3.x + 1.) * width / 2)), static_cast<int>(std::round((f.v3.y + 1.) * height / 2))}};
}
inline float depthAtBarycentric(const Face& f, float w1, float w2, float w3) {
    return f.v1.z * w1 + f.v2.z * w2 + f.v3.z * w3;
}

void drawTriangle3D(Frame& frame, const Face& f, const Color& color, ZBuffer& zbuffer) {
    Triangle t = projectFaceToTriangle(f, frame.width, frame.height); // only supports [-1,1]^3 model space for now

    // bounding box of the triangle, clamped to the frame boundaries
    int minX = std::max(0, std::min({t.p1.x, t.p2.x, t.p3.x}));
    int maxX = std::min(frame.width - 1, std::max({t.p1.x, t.p2.x, t.p3.x}));
    int minY = std::max(0, std::min({t.p1.y, t.p2.y, t.p3.y}));
    int maxY = std::min(frame.height - 1, std::max({t.p1.y, t.p2.y, t.p3.y}));
    if (minX > maxX || minY > maxY) return; // skip drawing if the triangle is entirely outside the frame

    int totalArea2 = signedArea2(t);
    if (totalArea2 < 2) return; // backface culling and degenerate triangle check

    // precompute deltas for efficient barycentric coordinate updates
    int deltaW1Col = (t.p1.y - t.p2.y);
    int deltaW2Col = (t.p2.y - t.p3.y);
    int deltaW3Col = (t.p3.y - t.p1.y);
    int deltaW1Row = (t.p2.x - t.p1.x);
    int deltaW2Row = (t.p3.x - t.p2.x);
    int deltaW3Row = (t.p1.x - t.p3.x);

    Point p0 = {minX, minY};
    int w1Row = signedArea2({p0, t.p1, t.p2});
    int w2Row = signedArea2({p0, t.p2, t.p3});
    int w3Row = signedArea2({p0, t.p3, t.p1});

    // iterate over the bounding box of the triangle per pixel
    for (int y = minY; y <= maxY; y++) {
        int w1 = w1Row, w2 = w2Row, w3 = w3Row;
        for (int x = minX; x <= maxX; x++) {
            bool inside = w1 >= 0 && w2 >= 0 && w3 >= 0;
            bool topLeft = (w1 != 0 || isTopLeftEdge(t.p1, t.p2)) && (w2 != 0 || isTopLeftEdge(t.p2, t.p3)) &&
                           (w3 != 0 || isTopLeftEdge(t.p3, t.p1));

            if (inside && topLeft) {
                float depth = -depthAtBarycentric(f,
                                                  static_cast<float>(w2) / totalArea2,
                                                  static_cast<float>(w3) / totalArea2,
                                                  static_cast<float>(w1) / totalArea2);

                float& z = zbuffer.data[y * zbuffer.width + x];
                if (depth <= z) {
                    z = depth;
                    drawPixel(frame, {x, y}, color);
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

void render3DModel(const Model& model, Frame& frame) {
    ZBuffer zbuffer{
        std::vector<float>(frame.width * frame.height, std::numeric_limits<float>::infinity()), frame.width, frame.height};

    for (const auto& mesh : model.meshes) {
        for (const auto& subMesh : mesh.subMeshes) {
            for (size_t i = 0; i < subMesh.indexCount; i += 3) {
                const VertexIndices& vi1 = mesh.indices[subMesh.indexOffset + i];
                const VertexIndices& vi2 = mesh.indices[subMesh.indexOffset + i + 1];
                const VertexIndices& vi3 = mesh.indices[subMesh.indexOffset + i + 2];

                Face face{model.transform * model.vertices[vi1.vertexIndex],
                          model.transform * model.vertices[vi2.vertexIndex],
                          model.transform * model.vertices[vi3.vertexIndex]};
                Color color{static_cast<uint8_t>(std::rand() % 256),
                            static_cast<uint8_t>(std::rand() % 256),
                            static_cast<uint8_t>(std::rand() % 256)};

                drawTriangle3D(frame, face, color, zbuffer);
            }
        }
    }

    // if (!writeZBufferToImage(zbuffer, "zbuffer.png")) {
    //     std::cerr << "Failed to write Z-buffer image." << std::endl;
    // }
    // if (!writeFrameToImage(frame, "rendered_frame.png")) {
    //     std::cerr << "Failed to write rendered frame image." << std::endl;
    // }
}

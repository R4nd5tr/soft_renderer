#pragma once
#include "frame.h"
#include "math_utils.h"
#include <algorithm>
#include <cstdint>
#include <vector>

struct Point {
    int x;
    int y;
};

struct Triangle {
    Point p1;
    Point p2;
    Point p3;
};

void drawLine(Frame& frame, const Point& p1, const Point& p2, const Color& color) {
    if (frame.data.empty()) return;

    int dx = std::abs(p2.x - p1.x);
    int dy = std::abs(p2.y - p1.y);
    int sx = (p1.x < p2.x) ? 1 : -1;
    int sy = (p1.y < p2.y) ? 1 : -1;

    int x = p1.x, y = p1.y;
    int err = dx - dy;

    while (true) {
        drawPixel(frame, {x, y}, color);
        if ((x == p2.x && y == p2.y) || x < 0 || y < 0 || x >= frame.width || y >= frame.height) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}
void drawCircle(Frame& frame, const Point& center, int radius, const Color& color) {
    if (frame.data.empty() || radius < 0) return;
    if (radius == 0) {
        drawPixel(frame, {center.x, center.y}, color);
        return;
    }

    int x = 0, y = -radius, e = -radius;
    while (x <= -y) {
        if (e >= 0) {
            y++;
            e += 2 * (x + y) + 1;
        } else {
            e += 2 * x + 1;
        }
        drawPixel(frame, {center.x - x, center.y - y}, color);
        drawPixel(frame, {center.x + x, center.y - y}, color);
        drawPixel(frame, {center.x - x, center.y + y}, color);
        drawPixel(frame, {center.x + x, center.y + y}, color);
        drawPixel(frame, {center.x - y, center.y - x}, color);
        drawPixel(frame, {center.x + y, center.y - x}, color);
        drawPixel(frame, {center.x - y, center.y + x}, color);
        drawPixel(frame, {center.x + y, center.y + x}, color);
        x++;
    }
}
inline int signedArea2(const Triangle& t) {
    return (t.p2.x - t.p1.x) * (t.p3.y - t.p1.y) - (t.p3.x - t.p1.x) * (t.p2.y - t.p1.y);
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
void drawTriangle(Frame& frame, const Triangle& t, const Color& color) {
    // bounding box of the triangle, clamped to the frame boundaries
    int minX = std::max(0, std::min({t.p1.x, t.p2.x, t.p3.x}));
    int maxX = std::min(frame.width - 1, std::max({t.p1.x, t.p2.x, t.p3.x}));
    int minY = std::max(0, std::min({t.p1.y, t.p2.y, t.p3.y}));
    int maxY = std::min(frame.height - 1, std::max({t.p1.y, t.p2.y, t.p3.y}));
    if (minX > maxX || minY > maxY) return; // skip drawing if the triangle is entirely outside the frame

    // backface culling and degenerate triangle check
    int totalArea2 = signedArea2(t);
    if (totalArea2 < 2) return;

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

    for (int y = minY; y <= maxY; y++) {
        int w1 = w1Row, w2 = w2Row, w3 = w3Row;
        for (int x = minX; x <= maxX; x++) {
            // check if the point is inside the triangle using barycentric coordinates
            if ((w1 >= 0 && w2 >= 0 && w3 >= 0) &&
                // top-left rule to avoid double drawing of shared edges
                ((w1 != 0 || isTopLeftEdge(t.p1, t.p2)) && (w2 != 0 || isTopLeftEdge(t.p2, t.p3)) &&
                 (w3 != 0 || isTopLeftEdge(t.p3, t.p1))))
                drawPixel(frame, {x, y}, color);

            w1 += deltaW1Col;
            w2 += deltaW2Col;
            w3 += deltaW3Col;
        }
        w1Row += deltaW1Row;
        w2Row += deltaW2Row;
        w3Row += deltaW3Row;
    }
}
void drawRGBInterpolatedTriangle(Frame& frame, const Triangle& t, float wireframeThreshold = 1.0f) {
    // bounding box of the triangle
    int minX = std::max(0, std::min({t.p1.x, t.p2.x, t.p3.x}));
    int maxX = std::min(frame.width - 1, std::max({t.p1.x, t.p2.x, t.p3.x}));
    int minY = std::max(0, std::min({t.p1.y, t.p2.y, t.p3.y}));
    int maxY = std::min(frame.height - 1, std::max({t.p1.y, t.p2.y, t.p3.y}));
    if (minX > maxX || minY > maxY) return;

    // backface culling and degenerate triangle check
    int totalArea2 = signedArea2(t);
    if (totalArea2 < 2) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Point p = {x, y};
            int w1 = signedArea2({p, t.p1, t.p2});
            int w2 = signedArea2({p, t.p2, t.p3});
            int w3 = signedArea2({p, t.p3, t.p1});
            // check if the point is inside the triangle using barycentric coordinates
            if (w1 < 0 || w2 < 0 || w3 < 0) continue;
            // top-left rule to avoid double drawing of shared edges
            if ((w1 == 0 && !isTopLeftEdge(t.p1, t.p2)) || (w2 == 0 && !isTopLeftEdge(t.p2, t.p3)) ||
                (w3 == 0 && !isTopLeftEdge(t.p3, t.p1)))
                continue;
            // wire frame triangle
            if (static_cast<float>(w1) / totalArea2 >= wireframeThreshold &&
                static_cast<float>(w2) / totalArea2 >= wireframeThreshold &&
                static_cast<float>(w3) / totalArea2 >= wireframeThreshold)
                continue;

            drawPixel(frame,
                      {p.x, p.y},
                      {static_cast<uint8_t>(w1 * 255 / totalArea2),
                       static_cast<uint8_t>(w3 * 255 / totalArea2),
                       static_cast<uint8_t>(w2 * 255 / totalArea2)});
        }
    }
}

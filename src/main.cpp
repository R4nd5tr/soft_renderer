#define RGFW_IMPLEMENTATION
#include "RGFW.h"
#include "math.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

struct Color {
    u8 r;
    u8 g;
    u8 b;
};
static_assert(sizeof(Color) == 3);

struct Point {
    int x;
    int y;
};

struct Triangle {
    Point p1;
    Point p2;
    Point p3;
};

struct Frame {
    std::vector<Color> data;
    int width;
    int height;
};

constexpr Color WHITE_COLOR = {255, 255, 255};
constexpr Color BLACK_COLOR = {0, 0, 0};
constexpr Color RED_COLOR = {255, 0, 0};
constexpr Color GREEN_COLOR = {0, 255, 0};
constexpr Color BLUE_COLOR = {0, 0, 255};
constexpr Color YELLOW_COLOR = {255, 255, 0};
constexpr Color CYAN_COLOR = {0, 255, 255};
constexpr Color MAGENTA_COLOR = {255, 0, 255};

#define BOTTOM_LEFT_ORIGIN
inline void drawPixel(Frame& frame, const Point& p, const Color& color) {
    if (frame.data.empty()) return;
    if (p.x >= frame.width || p.x < 0 || p.y >= frame.height || p.y < 0) return;

#ifdef BOTTOM_LEFT_ORIGIN
    frame.data[(frame.height - 1 - p.y) * frame.width + p.x] = color;
#else
    frame.data[p.y * frame.width + p.x] = color;
#endif
}

constexpr int renderWidth = 60, renderHeight = 60;
constexpr int renderScale = 10;
constexpr int windowWidth = renderWidth * renderScale, windowHeight = renderHeight * renderScale;
Frame renderFrame{std::vector<Color>(renderWidth * renderHeight), renderWidth, renderHeight};

Frame windowFrame{std::vector<Color>(windowWidth * windowHeight), windowWidth, windowHeight};
RGFW_window* win = nullptr;
RGFW_surface* surface = nullptr;

void renderToWindow(Frame& srcFrame, Frame& dstFrame) {
    for (int y = 0; y < dstFrame.height; y++) {
        for (int x = 0; x < dstFrame.width; x++) {
            int sourceX = x * srcFrame.width / dstFrame.width;
            int sourceY = y * srcFrame.height / dstFrame.height;

            size_t src = (sourceY * srcFrame.width + sourceX);
            size_t dst = (y * dstFrame.width + x);

            dstFrame.data[dst] = srcFrame.data[src];
        }
    }
}

inline void clearFrame(Frame& frame) {
    if (frame.data.empty()) return;
    std::fill(frame.data.begin(), frame.data.end(), Color{0, 0, 0});
}

void globalRenderToWindow() { // global function to render the current frame to the window, only for debug
    renderToWindow(renderFrame, windowFrame);
    RGFW_window_blitSurface(win, surface);
}
void globalClearFrame() { // global function to clear both the render frame and the window frame
    clearFrame(renderFrame);
    clearFrame(windowFrame);
}

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
        drawPixel(frame, center, color);
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
    int w1_row = signedArea2({p0, t.p1, t.p2});
    int w2_row = signedArea2({p0, t.p2, t.p3});
    int w3_row = signedArea2({p0, t.p3, t.p1});

    for (int y = minY; y <= maxY; y++) {
        int w1 = w1_row, w2 = w2_row, w3 = w3_row;
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
        w1_row += deltaW1Row;
        w2_row += deltaW2Row;
        w3_row += deltaW3Row;
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
                      p,
                      {static_cast<u8>(w1 * 255 / totalArea2),
                       static_cast<u8>(w3 * 255 / totalArea2),
                       static_cast<u8>(w2 * 255 / totalArea2)});
        }
    }
}

int main() {
    win = RGFW_createWindow("Software Renderer", 0, 0, windowWidth, windowHeight, RGFW_windowCenter | RGFW_windowNoResize);
    if (!win) return -1;
    RGFW_window_setExitKey(win, RGFW_escape);

    surface = RGFW_createSurface(
        reinterpret_cast<u8*>(windowFrame.data.data()), windowFrame.width, windowFrame.height, RGFW_formatRGB8);

    size_t frameNumber = 0;
    while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
        RGFW_pollEvents();
        clearFrame(renderFrame);
        clearFrame(windowFrame);
        // ====================================

        // test top-left rule for shared edges
        Point p1 = {10, 15}, p2 = {15, 45}, p3 = {55, 40}, p4 = {40, 5};
        Triangle t1 = {p1, p3, p2};
        Triangle t2 = {p1, p4, p3};
        if (frameNumber % 2 == 0) {
            drawTriangle(renderFrame, t1, GREEN_COLOR);
            drawTriangle(renderFrame, t2, BLUE_COLOR);
        } else {
            drawTriangle(renderFrame, t2, BLUE_COLOR);
            drawTriangle(renderFrame, t1, GREEN_COLOR);
        }
        frameNumber++;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // ====================================
        renderToWindow(renderFrame, windowFrame);
        RGFW_window_blitSurface(win, surface);
    }

    RGFW_surface_free(surface);
    RGFW_window_close(win);
    return 0;
}

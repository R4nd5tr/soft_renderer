#define RGFW_IMPLEMENTATION
#include "RGFW.h"
#include "frame.h"
#include "math_utils.h"
#include "model.h"
#include "rasterizer3d.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

constexpr int renderWidth = 900, renderHeight = 900;
constexpr int renderScale = 1;
constexpr int windowWidth = renderWidth * renderScale, windowHeight = renderHeight * renderScale;
Frame renderFrame{std::vector<Color>(renderWidth * renderHeight), renderWidth, renderHeight};
Frame windowFrame{std::vector<Color>(windowWidth * windowHeight), windowWidth, windowHeight};

RGFW_window* win = nullptr;
RGFW_surface* surface = nullptr;

void globalRenderToWindow() { // global function to render the current frame to the window, only for debug
    scaleToFrame(renderFrame, windowFrame);
    RGFW_window_blitSurface(win, surface);
}
void globalClearFrame() { // global function to clear both the render frame and the window frame
    clearFrame(renderFrame);
    clearFrame(windowFrame);
}
// render static frame
int main() {
    win = RGFW_createWindow("Software Renderer", 0, 0, windowWidth, windowHeight, RGFW_windowCenter | RGFW_windowNoResize);
    if (!win) return -1;
    RGFW_window_setExitKey(win, RGFW_escape);

    surface = RGFW_createSurface(
        reinterpret_cast<u8*>(windowFrame.data.data()), windowFrame.width, windowFrame.height, RGFW_formatRGB8);
    // ===================================
    clearFrame(renderFrame);
    clearFrame(windowFrame);
    Model model = loadModelFromObj("obj/diablo3_pose/diablo3_pose.obj");
    render3DModel(model, windowFrame);

    // scaleToFrame(renderFrame, windowFrame);
    RGFW_window_blitSurface(win, surface);

    // ===================================
    while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
        RGFW_pollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    RGFW_surface_free(surface);
    RGFW_window_close(win);
    return 0;
}

// render dynamic frame
// int main() {
//     win = RGFW_createWindow("Software Renderer", 0, 0, windowWidth, windowHeight, RGFW_windowCenter | RGFW_windowNoResize);
//     if (!win) return -1;
//     RGFW_window_setExitKey(win, RGFW_escape);

//     surface = RGFW_createSurface(
//         reinterpret_cast<u8*>(windowFrame.data.data()), windowFrame.width, windowFrame.height, RGFW_formatRGB8);

//     size_t frameNumber = 0;
//     while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
//         RGFW_pollEvents();
//         clearFrame(renderFrame);
//         clearFrame(windowFrame);
//         // ====================================

//         // test top-left rule for shared edges
//         Point p1 = {10, 15}, p2 = {15, 45}, p3 = {55, 40}, p4 = {40, 5};
//         Triangle t1 = {p1, p3, p2};
//         Triangle t2 = {p1, p4, p3};
//         if (frameNumber % 2 == 0) {
//             drawTriangle(renderFrame, t1, GREEN_RGB);
//             drawTriangle(renderFrame, t2, BLUE_RGB);
//         } else {
//             drawTriangle(renderFrame, t2, BLUE_RGB);
//             drawTriangle(renderFrame, t1, GREEN_RGB);
//         }
//         frameNumber++;
//         std::this_thread::sleep_for(std::chrono::milliseconds(200));

//         // ====================================
//         scaleToFrame(renderFrame, windowFrame);
//         RGFW_window_blitSurface(win, surface);
//     }

//     RGFW_surface_free(surface);
//     RGFW_window_close(win);
//     return 0;
// }

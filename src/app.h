#pragma once
#include "frame.h"
#include "math_utils.h"
#include "model.h"
#include "rasterizer3d.h"
#include "scene.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

struct RGFW_window;
struct RGFW_surface;

class App {
public:
    App() = default;
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init();
    void run();

    void handleEvents();
    void present();

    Scene loadScene();
    void updateScene(float deltaTime); // seconds

    int renderWidth = 1600, renderHeight = 900;
    int renderScale = 1;
    int windowWidth = renderWidth * renderScale, windowHeight = renderHeight * renderScale;
    Frame renderFrame{std::vector<Color>(renderWidth * renderHeight), renderWidth, renderHeight};
    Frame windowFrame{std::vector<Color>(windowWidth * windowHeight), windowWidth, windowHeight};
    Frame& renderTarget() { return renderScale == 1 ? windowFrame : renderFrame; }

    RGFW_window* win = nullptr;
    RGFW_surface* surface = nullptr;

    Scene scene;
    Renderer renderer;

    bool paused = false;        // pause simulation or not
    bool stepRequested = false; // step to next simulation step
    size_t frameNumber = 0;

    struct InputState {
        bool forward = false, left = false, backward = false, right = false, up = false, down = false;
        bool dragging = false;
        int mouseX = 0, mouseY = 0; // 当前鼠标位置
        int dragDx = 0, dragDy = 0; // 自上次消费以来的拖拽累计量
        float scrollDelta = 0.0f;   // 自上次消费以来的滚轮累计量
    } input;

    float moveSpeed = 3.0f;          // WASD 移动相机距离的倍数
    float mouseSensitivity = 0.005f; // 每格鼠标移动量的旋转角度
    float zoomSpeed = 0.1f;          // 每格滚轮缩放当前距离的比例

    bool updateCamera(float deltaTime);
};

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

    RGFW_window* win = nullptr;
    RGFW_surface* surface = nullptr;

    Scene scene;
    Renderer renderer;

    bool paused = true;
    bool stepRequested = false;
    size_t frameNumber = 0;
};

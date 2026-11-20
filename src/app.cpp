#include "app.h"
#define RGFWDEF
#include "RGFW.h"
#include <chrono>
#include <thread>

static u8 iconRGB[2 * 2 * 4] = {
    0x39,
    0xC5,
    0xBB,
    0x39,
    0xC5,
    0xBB,
    0x39,
    0xC5,
    0xBB,
    0x39,
    0xC5,
    0xBB,
};

App::~App() {
    if (surface) RGFW_surface_free(surface);
    if (win) RGFW_window_close(win);
}

bool App::init() {
    win = RGFW_createWindow("Software Renderer", 0, 0, windowWidth, windowHeight, RGFW_windowCenter | RGFW_windowNoResize);
    if (!win) return false;
    RGFW_window_setIcon(win, iconRGB, 2, 2, RGFW_formatRGB8);
    RGFW_window_setExitKey(win, RGFW_escape);

    clearFrame(renderFrame);
    clearFrame(windowFrame);

    surface = RGFW_createSurface(
        reinterpret_cast<u8*>(windowFrame.data.data()), windowFrame.width, windowFrame.height, RGFW_formatRGB8);
    if (!surface) return false;

    scene = loadScene();
    return true;
}

void App::handleEvents() {
    RGFW_event event;
    while (RGFW_window_checkEvent(win, &event)) {
        if (event.type != RGFW_keyPressed || event.key.repeat) continue;
        switch (event.key.value) {
        case RGFW_p:
            paused = !paused;
            break;
        case RGFW_bracket:
            if (paused) stepRequested = true;
            break;
        }
    }
}

void App::present() {
    scaleToFrame(renderFrame, windowFrame);
    RGFW_window_blitSurface(win, surface);
}

void App::run() {
    // 首帧：只渲染初始状态，不 update
    renderer.renderScene(scene, renderFrame);
    present();

    auto lastFrameTime = std::chrono::steady_clock::now();
    while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
        handleEvents();

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
        lastFrameTime = now;

        if (!paused || stepRequested) {
            clearFrame(renderFrame);
            updateScene(deltaTime);
            renderer.renderScene(scene, renderFrame);
            frameNumber += 1;
            stepRequested = false;
        }

        present();
    }
}

// ============================================================
// 场景定义区：这个文件里唯一需要随场景修改的部分
//   loadScene()   —— 启动时执行一次：加载模型、摆相机、设灯光
//   updateScene() —— 每帧（含单步）执行一次：动画、物理、相机控制
// ============================================================

Scene App::loadScene() {
    Scene scene;
    scene.camera.aspect = static_cast<float>(renderWidth) / renderHeight;
    // ============================================================

    scene.addModel("obj/miku_plush/miku_plush.obj");

    return scene;
}

void App::updateScene(float deltaTime) {
    scene.camera.orbitYaw(deltaTime * 0.5f); // 相机绕目标点旋转
}

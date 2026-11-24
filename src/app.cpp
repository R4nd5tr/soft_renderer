#include "app.h"
#define RGFWDEF
#include "RGFW.h"
#include "icon.h"
#include <chrono>
#include <thread>

using Clock = std::chrono::steady_clock;
auto ms = [](auto d) {
    return std::chrono::duration<double, std::milli>(d).count();
};

App::~App() {
    if (surface) RGFW_surface_free(surface);
    if (win) RGFW_window_close(win);
}

bool App::init() {
    win = RGFW_createWindow("Software Renderer", 0, 0, windowWidth, windowHeight, RGFW_windowCenter | RGFW_windowNoResize);
    if (!win) return false;
    RGFW_window_setIcon(win, (uint8_t*)(icon), icon_width, icon_height, RGFW_formatRGBA8);
    RGFW_window_setExitKey(win, RGFW_escape);

    surface = RGFW_createSurface(
        reinterpret_cast<u8*>(windowFrame.data.data()), windowFrame.width, windowFrame.height, RGFW_formatRGBA8);
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
    if (renderScale != 1) scaleToFrame(renderFrame, windowFrame);
    RGFW_window_blitSurface(win, surface);
}

void App::run() {
    // 首帧：只渲染初始状态，不 update
    Frame& target = renderTarget();
    clearFrame(target);
    renderer.renderScene(scene, target);
    present();

    double sumClear = 0, sumUpdate = 0, sumRender = 0, sumPresent = 0, sumWork = 0, sumFrame = 0;
    size_t measured = 0;

    auto lastFrameTime = Clock::now();
    while (RGFW_window_shouldClose(win) == RGFW_FALSE) { // main loop
        const auto frameStartTime = Clock::now();
        const float deltaTime = std::chrono::duration<float>(frameStartTime - lastFrameTime).count();
        const double frameMs = ms(frameStartTime - lastFrameTime);
        lastFrameTime = frameStartTime;

        handleEvents();

        if (!paused || stepRequested) {
            Frame& target = renderTarget();

            const auto t0 = Clock::now();
            clearFrame(target);
            const auto t1 = Clock::now();
            updateScene(deltaTime);
            const auto t2 = Clock::now();
            renderer.renderScene(scene, target);
            const auto t3 = Clock::now();
            present();
            const auto t4 = Clock::now();

            stepRequested = false;
            ++frameNumber;

            sumClear += ms(t1 - t0);
            sumUpdate += ms(t2 - t1);
            sumRender += ms(t3 - t2);
            sumPresent += ms(t4 - t3);
            sumWork += ms(t4 - t0);
            sumFrame += frameMs;
            ++measured;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (measured == 0) return;
    const double n = (double)measured;
    const double fps = 1000.0 / (sumFrame / n);
    std::printf("\n=== %zu frames ===\n", measured);
    std::printf("clear    %7.3f ms\n", sumClear / n);
    std::printf("update   %7.3f ms\n", sumUpdate / n);
    std::printf("render   %7.3f ms\n", sumRender / n);
    std::printf("present  %7.3f ms\n", sumPresent / n);
    std::printf("work     %7.3f ms   (clear+update+render+present)\n", sumWork / n);
    std::printf("frame   %7.3f ms   => %.1f FPS\n", sumFrame / n, fps);
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
    scene.addLight(Light{}); // 环境光
    scene.shaderKind = ShaderKind::BlinnPhong;
    scene.clearOption(ShaderOptions::NormalTexture);

    // scene.addModel("obj/african_head/african_head.obj");
    // scene.addModel("obj/Counter Srike GO AWP model/Model.obj");
    // scene.addModel("obj/diablo3_pose/diablo3_pose.obj");
    // scene.addModel("obj/boggie/boggie.obj");
    scene.addModel("obj/miku_plush/miku_plush.obj");
    // scene.addModel("obj/teapot.obj");
    // scene.addModel("obj/teto/teto.obj");

    // Too big!
    // scene.addModel("obj/iron_golem/iron_golem.obj");
    // scene.addModel("obj/HatsuneMiku/HatsuneMiku.obj");

    // Too small!
    // scene.addModel("obj/YYB Miku/YYB Miku.obj");
    // scene.addModel("obj/stanford-bunny.obj");

    return scene;
}

void App::updateScene(float deltaTime) {
    scene.camera.orbitYaw(deltaTime * 0.5f); // 相机绕目标点旋转
}

// TODO: 法线贴图，切线空间加载
// TODO: 相机控制功能，2DUI功能
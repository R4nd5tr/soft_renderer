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
        switch (event.type) {
        case RGFW_keyPressed:
        case RGFW_keyReleased: {
            const bool down = event.type == RGFW_keyPressed;
            switch (event.key.value) {
            case RGFW_w:
                input.forward = down;
                break;
            case RGFW_a:
                input.left = down;
                break;
            case RGFW_s:
                input.backward = down;
                break;
            case RGFW_d:
                input.right = down;
                break;
            case RGFW_space:
                input.up = down;
                break;
            case RGFW_controlL:
                input.down = down;
                break;
            default:
                break;
            }
            if (down && !event.key.repeat) {
                switch (event.key.value) {
                case RGFW_p:
                    paused = !paused;
                    break;
                case RGFW_bracket:
                    if (paused) stepRequested = true;
                    break;
                default:
                    break;
                }
            }
            break;
        }
        case RGFW_mouseButtonPressed:
            if (event.button.value == RGFW_mouseLeft) input.dragging = true;
            break;
        case RGFW_mouseButtonReleased:
            if (event.button.value == RGFW_mouseLeft) input.dragging = false;
            break;
        case RGFW_mousePosChanged: {
            const int x = event.mouse.x, y = event.mouse.y;
            if (input.dragging) { // 只累加，不动相机
                input.dragDx += x - input.mouseX;
                input.dragDy += y - input.mouseY;
            }
            input.mouseX = x;
            input.mouseY = y;
            break;
        }
        case RGFW_mouseScroll:
            input.scrollDelta += event.scroll.y; // 向前滚为正
            break;
        default:
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

        Frame& target = renderTarget();

        const auto t0 = Clock::now();
        clearFrame(target);
        const auto t1 = Clock::now();
        const bool cameraChanged = updateCamera(deltaTime);
        const bool sceneChanged = !paused || stepRequested;
        if (sceneChanged) updateScene(deltaTime);
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

bool App::updateCamera(float deltaTime) {
    bool moved = false;
    // 鼠标拖拽：消费累计的位移
    if (input.dragDx != 0 || input.dragDy != 0) {
        scene.camera.orbitYaw(-input.dragDx * mouseSensitivity);
        scene.camera.orbitPitch(-input.dragDy * mouseSensitivity);
        input.dragDx = input.dragDy = 0;
        moved = true;
    }
    // 鼠标滚轮：缩放
    if (input.scrollDelta != 0.0f) {
        const float step = scene.camera.distance() * zoomSpeed * input.scrollDelta;
        scene.camera.dolly(step);
        input.scrollDelta = 0.0f;
        moved = true;
    }
    // WASD + 空格/Ctrl
    const float v = moveSpeed * scene.camera.distance() * deltaTime;
    const float f = (input.forward ? v : 0) - (input.backward ? v : 0);
    const float r = (input.right ? v : 0) - (input.left ? v : 0);
    const float u = (input.up ? v : 0) - (input.down ? v : 0);
    if (f != 0 || r != 0 || u != 0) {
        const vec3 delta = scene.camera.forwardDir() * f + scene.camera.rightDir() * r + vec3{0.0f, u, 0.0f};
        scene.camera.move(delta);
        moved = true;
    }
    return moved;
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
    // scene.addModel("obj/boggie/boggie.obj");
    // scene.addModel("obj/Counter Srike GO AWP model/Model.obj");
    // scene.addModel("obj/diablo3_pose/diablo3_pose.obj");
    // scene.addModel("obj/F-15C_Eagle/F-15C_Eagle.obj");
    // scene.addModel("obj/floor/floor.obj");
    // scene.addModel("obj/miku_plush/miku_plush.obj");
    // scene.addModel("obj/teapot.obj");
    // scene.addModel("obj/teto/teto.obj");

    // Too big!
    // scene.addModel("obj/iron_golem/iron_golem.obj");
    // scene.addModel("obj/HatsuneMiku/HatsuneMiku.obj");

    // Too small!
    // scene.addModel("obj/YYB_Miku/YYB_Miku.obj");
    // scene.addModel("obj/stanford-bunny.obj");

    return scene;
}

void App::updateScene(float deltaTime) {
    for (auto& model : scene.models) { // model rotation
        model.transform.rotateLocal(vec3{0, 1, 0}, 0.5f * deltaTime);
    }
}

// TODO: 法线贴图，切线空间加载, 2DUI功能

#include <filesystem>
#include <string>
#include <vector>
namespace fs = std::filesystem;

// 编译器选择
#if defined(_WIN32)
    #if defined(_MSC_VER) && !defined(__clang__)
        #define CC "cl"
        #define NOB_REBUILD_URSELF(binary_path, source_path) CC, "/EHsc", "/std:c++20", source_path, "/Fe:", binary_path
    #elif defined(__clang__)
        #define CC "clang++"
    #elif defined(__GNUC__)
        #define CC "g++"
    #else
        #error "Unknown bootstrap compiler"
    #endif
#elif defined(__linux__) || defined(__APPLE__)
    #if defined(__clang__)
        #define CC "clang++"
    #elif defined(__GNUC__)
        #define CC "g++"
    #endif
#else
    #error "Unsupported platform"
#endif

#ifndef NOB_REBUILD_URSELF
    #define NOB_REBUILD_URSELF(binary_path, source_path) \
        CC, "-std=c++20", "-D_CRT_SECURE_NO_WARNINGS", "-o", binary_path, source_path
#endif
#define NOB_UNSTRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "external/nob.h"

// 编译选项
#if defined(_MSC_VER) && !defined(__clang__)
    #define FLAGS "/std:c++17", "/W4", "/EHsc", "/Z7", "/DEBUG", "/Od", "/DNOMINMAX"
    #define FLAGS_OPTIMIZED "/std:c++17", "/W4", "/EHsc", "/Z7", "/DNDEBUG", "/O2", "/DNOMINMAX"
    #define HEADER "/Isrc", "/external:Iexternal", "/external:W0"
#else
    #define FLAGS                                                                                                             \
        "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-parameter", "-Wno-missing-field-initializers", \
            "-g", "-O0", "-DNOMINMAX"
    #define FLAGS_OPTIMIZED                                                                                                   \
        "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-parameter", "-Wno-missing-field-initializers", \
            "-g", "-O3", "-march=native", "-fno-math-errno", "-fno-trapping-math", "-DNOMINMAX", "-DNDEBUG"
    #define HEADER "-Isrc", "-Iexternal"
#endif

// 编译器输出文件扩展名
#if defined(_MSC_VER) && !defined(__clang__)
    #define OBJ_EXT ".obj"
    #define OBJ_OUT "/Fo:"
    #define COMPILE_ONLY "/nologo", "/c"
#else
    #define OBJ_EXT ".o"
    #define OBJ_OUT "-o"
    #define COMPILE_ONLY "-c"
#endif

// 库文件
#if defined(_WIN32)
    #define OUTPUT_FILE "soft_renderer.exe"
    #if defined(_MSC_VER) && !defined(__clang__)
        #define LIBS "opengl32.lib", "gdi32.lib"
        #define OUTPUT_FLAG "/Fe:"
    #else
        #define LIBS "-lopengl32", "-lgdi32"
        #define OUTPUT_FLAG "-o"
    #endif
#elif defined(__APPLE__)
    #define LIBS "-framework", "Cocoa"
    #define OUTPUT_FILE "soft_renderer"
    #define OUTPUT_FLAG "-o"
#elif defined(__linux__)
    #define LIBS "-lX11", "-lGL", "-lm", "-lXrandr"
    #define OUTPUT_FILE "soft_renderer"
    #define OUTPUT_FLAG "-o"
#else
    #error "Unsupported platform"
#endif

// 工具函数
std::vector<std::string> collect_source_files(const fs::path& directory) {
    std::vector<std::string> sources;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        const auto extension = entry.path().extension();
        if (extension == ".cpp" || extension == ".cc" || extension == ".c") {
            sources.push_back(entry.path().generic_string());
        }
    }
    return sources;
}
std::vector<std::string> collect_header_files(const fs::path& directory) {
    std::vector<std::string> headers;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename() == "nob.h") continue; // 忽略 nob.h
        const auto extension = entry.path().extension();
        if (extension == ".h" || extension == ".hpp") {
            headers.push_back(entry.path().generic_string());
        }
    }
    return headers;
}

// 用法：使用 C++20 编译器编译此文件以引导生成构建系统，windows下注意输出文件名要改为 nob.exe
// 例如：
//      g++ -std=c++20 -o nob nob.cpp
//      clang++ -std=c++20 -D_CRT_SECURE_NO_WARNINGS -o nob nob.cpp
//      cl /EHsc /std:c++20 /Fe: nob.exe nob.cpp
// 然后 ./nob 或 ./nob.exe 来构建项目，项目使用 C++17
// 构建系统会使用引导的编译器来编译项目

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    bool optimize = false;
    bool rebuild = false;
    bool parallel = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-O") == 0) optimize = true;
        if (strcmp(argv[i], "-r") == 0) rebuild = true;
        if (strcmp(argv[i], "-p") == 0) parallel = true;
        if (strcmp(argv[i], "-h") == 0) {
            nob_log(NOB_INFO, "Usage: nob [-O] [-r] [-p]");
            nob_log(NOB_INFO, "  -O: optimize");
            nob_log(NOB_INFO, "  -r: rebuild");
            nob_log(NOB_INFO, "  -p: parallel");
            return 0;
        }
    }

    // 检查 src/ 和 external/ 目录是否存在
    if (!fs::exists("src")) {
        nob_log(NOB_ERROR, "src/ not found");
        return 1;
    }
    if (!fs::exists("external")) {
        nob_log(NOB_ERROR, "external/ not found");
        return 1;
    }

    // 收集所有源文件
    std::vector<std::string> sources = collect_source_files("src");
    if (sources.empty()) {
        nob_log(NOB_ERROR, "No source files found in src/");
        return 1;
    }
    // 收集所有头文件
    std::vector<std::string> headers = collect_header_files("src");
    if (headers.empty()) {
        nob_log(NOB_ERROR, "No header files found in src/");
        return 1;
    }
    std::vector<std::string> external_headers = collect_header_files("external");
    if (external_headers.empty()) {
        nob_log(NOB_ERROR, "No header files found in external/");
        return 1;
    }
    headers.insert(headers.end(), external_headers.begin(), external_headers.end());

    // 创建 build/ 目录
    const std::string buildDir = optimize ? "build/optimized/" : "build/debug/";
    if (!nob_mkdir_if_not_exists("build/")) return 1;
    if (!nob_mkdir_if_not_exists(buildDir.c_str())) return 1;

    Nob_Cmd cmd = {0};
    Nob_Procs procs = {0};
    std::vector<std::string> objects;

    for (const auto& source : sources) {
        // 生成对应的目标文件名
        std::string obj = buildDir + fs::path(source).stem().string() + OBJ_EXT;
        objects.push_back(obj);

        // 检查是否需要重新编译
        std::vector<const char*> inputs = {source.c_str()};
        for (const auto& header : headers)
            inputs.push_back(header.c_str());

        // 检查目标文件是否需要重新编译
        const int needs = nob_needs_rebuild(obj.c_str(), inputs.data(), inputs.size());
        if (needs < 0) return 1;
        if (needs == 0 && !rebuild) continue; // 没变，跳过

        // 如果当前并行编译的进程数已经达到 CPU 核心数，则等待所有进程结束
        if (procs.count >= static_cast<size_t>(nob_nprocs())) {
            if (!nob_procs_flush(&procs)) return 1;
        }

        // 编译源文件
        nob_log(NOB_INFO, "Compiling %s -> %s", source.c_str(), obj.c_str());
        if (optimize) {
            nob_cmd_append(&cmd, CC, FLAGS_OPTIMIZED, HEADER, COMPILE_ONLY, source.c_str(), OBJ_OUT, obj.c_str());
        } else {
            nob_cmd_append(&cmd, CC, FLAGS, HEADER, COMPILE_ONLY, source.c_str(), OBJ_OUT, obj.c_str());
        }
        if (parallel) {
            if (!nob_cmd_run(&cmd, .async = &procs)) return 1; // 并行，只登记进程，立即返回
        } else {
            if (!nob_cmd_run(&cmd)) return 1; // 同步执行
        }
    }

    // 等所有编译进程结束，任何一个失败都返回 false
    if (!nob_procs_flush(&procs)) return 1;

    // 链接
    nob_cmd_append(&cmd, CC);
#if defined(_MSC_VER) && !defined(__clang__)
    nob_cmd_append(&cmd, "/nologo", "/Z7");
#else
    nob_cmd_append(&cmd, "-g");
#endif
    for (const auto& obj : objects)
        nob_cmd_append(&cmd, obj.c_str());
    nob_cmd_append(&cmd, LIBS, OUTPUT_FLAG, OUTPUT_FILE);
    if (!nob_cmd_run(&cmd)) return 1;

    // 运行生成的可执行文件
    nob_cmd_append(&cmd, "./" OUTPUT_FILE);
    if (!nob_cmd_run(&cmd)) return 1;

    return 0;
}

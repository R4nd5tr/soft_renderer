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
    #define FLAGS "/std:c++17", "/W4", "/EHsc", "/Zi", "/DEBUG", "/Od", "/DNOMINMAX"
    #define HEADER "/Isrc", "/external:Iexternal", "/external:W0"
#else
    #define FLAGS "-std=c++17", "-Wall", "-Wextra", "-Wno-unused-function", "-g", "-O0", "-DNOMINMAX"
    #define HEADER "-Isrc", "-Iexternal"
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

bool is_source_file(const fs::path& path) {
    const auto extension = path.extension();
    return extension == ".cpp" || extension == ".cc" || extension == ".c";
}
std::vector<std::string> collect_source_files(const fs::path& directory) {
    std::vector<std::string> sources;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (is_source_file(entry.path())) {
            sources.push_back(entry.path().generic_string());
        }
    }
    return sources;
}

// 用法：使用 C++20 编译器编译此文件以引导生成构建系统，windows下注意输出文件名要改为 nob.exe
// 例如：
//      g++ -std=c++20 -o nob nob.cpp
//      clang++ -std=c++20 -o nob nob.cpp
//      cl /EHsc /std:c++20 /Fe: nob.exe nob.cpp
// 然后 ./nob 或 ./nob.exe 来构建项目，项目使用 C++17
// 构建系统会使用引导的编译器来编译项目

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    // 收集所有源文件
    if (!fs::exists("src")) {
        nob_log(NOB_ERROR, "src/ not found");
        return 1;
    }
    std::vector<std::string> sources = collect_source_files("src");
    if (sources.empty()) {
        nob_log(NOB_ERROR, "No source files found in src/");
        return 1;
    }

    Nob_Cmd cmd = {0};
    // 构建命令
    nob_cmd_append(&cmd, CC, FLAGS, HEADER);
#if defined(_MSC_VER) && !defined(__clang__)
    std::filesystem::create_directories("build");
    nob_cmd_append(&cmd, "/Fo:build/");
#endif
    // 源文件
    for (const auto& source : sources) {
        nob_cmd_append(&cmd, source.c_str());
    }
    // 库文件
    nob_cmd_append(&cmd, LIBS);
    // 输出文件
    nob_cmd_append(&cmd, OUTPUT_FLAG, OUTPUT_FILE);

    // 执行构建命令
    if (!nob_cmd_run(&cmd)) return 1;

    // 运行生成的可执行文件
    // nob_cmd_append(&cmd, "./" OUTPUT_FILE);
    // if (!nob_cmd_run(&cmd)) return 1;

    return 0;
}

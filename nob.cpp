#include <filesystem>
#include <string>
#include <vector>
namespace fs = std::filesystem;

// 编译器选择
#if defined(_WIN32)
#define BOOTSTRAP_CC "g++"
#define CC "g++"
#elif defined(__APPLE__)
#define BOOTSTRAP_CC "clang++"
#define CC "clang++"
#elif defined(__linux__)
#define BOOTSTRAP_CC "g++"
#define CC "g++"
#else
#error "Unsupported platform"
#endif

#define NOB_REBUILD_URSELF(binary_path, source_path) BOOTSTRAP_CC, "-std=c++17", "-o", binary_path, source_path
#define NOB_UNSTRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "external/nob.h"

// 编译选项
#define FLAGS "-std=c++17", "-Wall", "-Wextra", "-g", "-O0"
#define HEADER "-Isrc", "-Iexternal"
// 库文件
#if defined(_WIN32)
#define LIBS "-lopengl32", "-lgdi32"
#define OUTPUT "soft_renderer.exe"
#elif defined(__APPLE__)
#define LIBS "-framework", "Cocoa"
#define OUTPUT "soft_renderer"
#elif defined(__linux__)
#define LIBS "-lX11", "-lGL", "-lm", "-lXrandr"
#define OUTPUT "soft_renderer"
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
            sources.push_back(entry.path().string());
        }
    }
    return sources;
}

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    // 收集所有源文件
    std::vector<std::string> sources = collect_source_files("src");

    Nob_Cmd cmd = {0};
    // 构建命令
    nob_cmd_append(&cmd, CC, FLAGS, HEADER);
    // 源文件
    for (const auto& source : sources) {
        nob_cmd_append(&cmd, source.c_str());
    }
    // 库文件
    nob_cmd_append(&cmd, LIBS);
    // 输出文件
    nob_cmd_append(&cmd, "-o", OUTPUT);

    // 执行构建命令
    if (!nob_cmd_run(&cmd)) return 1;

    // 运行生成的可执行文件
    // nob_cmd_append(&cmd, "./" OUTPUT);
    // if (!nob_cmd_run(&cmd)) return 1;

    return 0;
}

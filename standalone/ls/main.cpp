#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void parse_entry(const char *s) {
    const fs::path path(s);
    if (!fs::exists(path)) {
        std::cerr << path << ": No such file or directory"
                  << std::endl;
        return;
    };
    if (!fs::is_directory(path)) {
        std::cout << path << std::endl;
        return;
    }
    std::cout << path << ":" << std::endl;
    for (const auto &entry : fs::directory_iterator(s))
        std::cout << entry.path().filename() << std::endl;
}

int main(int argc, const char **argw) {
    for (int i = 1; i < argc; i++) {
        parse_entry(argw[i]);
        std::cout << std::endl;
    }
}

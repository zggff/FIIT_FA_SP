#include <__chrono/duration.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <vector>
#include <iterator>
#include <string>
#include <sstream>
#include <chrono>

namespace fs = std::filesystem;
namespace ranges = std::ranges;
namespace views = ranges::views;

std::string demo_perms(std::filesystem::perms p) {
    using std::filesystem::perms;
    std::stringstream s;
    auto show = [&s, p](char op, perms perm) {
        s << (perms::none == (perm & p) ? '-' : op);
    };
    show('r', perms::owner_read);
    show('w', perms::owner_write);
    show('x', perms::owner_exec);
    show('r', perms::group_read);
    show('w', perms::group_write);
    show('x', perms::group_exec);
    show('r', perms::others_read);
    show('w', perms::others_write);
    show('x', perms::others_exec);
    return s.str();
}

void handle_file(fs::path path) {
    auto status = fs::status(path);
    auto perms = demo_perms(status.permissions());
    auto name = path.filename().string();
    auto size = fs::is_directory(path) ? 0 : fs::file_size(path);
    auto last =
        std::chrono::round<std::chrono::seconds>(fs::last_write_time(path));
    std::cout << std::format("{} {}\t{}\t{}", perms, size, last, name)
              << std::endl;
}

void parse_entry(const fs::path &path, bool full_info = false,
                 bool all_files = false) {

    if (!fs::is_directory(path)) {
        std::cout << path << std::endl;
        return;
    }
    std::cout << path.string() << ":" << std::endl;
    std::vector<fs::path> paths;
    ranges::copy(fs::directory_iterator(path) |
                     views::transform([](const auto &entry) { return entry; }),
                 std::inserter(paths, paths.begin()));
    ranges::sort(paths, [](const std::string &a, const std::string &b) {
        for (size_t c = 0; c < a.size() and c < b.size(); c++) {
            if (std::tolower(a[c]) != std::tolower(b[c]))
                return (std::tolower(a[c]) < std::tolower(b[c]));
        }
        return a.size() < b.size();
    });
    for (const auto &path : paths) {
        if (!all_files && path.filename().string().starts_with("."))
            continue;
        if (full_info)
            handle_file(path);
        else
            std::cout << path.filename().string() << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, const char **argw) {
    bool display_info = false;
    bool list_all = false;
    std::vector<fs::path> paths;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argw[i], "-l") == 0) {
            display_info = true;
            continue;
        }
        if (std::strcmp(argw[i], "-a") == 0) {
            list_all = true;
            continue;
        }
        if (std::strcmp(argw[i], "-la") == 0 ||
            std::strcmp(argw[i], "-al") == 0) {
            list_all = true;
            display_info = true;

            continue;
        }
        paths.push_back(argw[i]);
        if (!fs::exists(paths.back())) {
            std::cerr << paths.back() << ": No such file or directory"
                      << std::endl;
            return -1;
        };
    }
    if (paths.empty())
        paths.push_back(".");
    for (const auto &path : paths)
        parse_entry(path, display_info, list_all);
}

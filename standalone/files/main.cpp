#include <bitset>
#include <cmath>
#include <format>
#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <map>
#include <functional>
#include <not_implemented.h>
#include <regex>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void handle_xor(const std::vector<fs::path> &files, const char *op,
                const char *par) {
    int n = op[3] - '0';
    size_t mask = 1;
    mask = (mask << (1 << n)) - 1;
    if (n == 6)
        mask = SIZE_T_MAX;

    size_t res = 0;
    // std::cout << "mask = " << std::format("{:b}", mask) << std::endl;
    for (auto &f : files) {
        size_t buf = 0;
        std::ifstream in(f, std::ifstream::binary);
        in.read(reinterpret_cast<char *>(&buf), sizeof(buf));
        // std::cout << "buf = " << std::format("{:b}", buf) << std::endl;
        for (int i = 0; i < (1 << (6 - n)); i++) {
            res ^= mask & buf;
            // std::cout << std::format("{:b}", mask & buf) << std::endl;
            buf >>= (1 << n);
        }
        in.close();
    }
    std::cout << std::format("{:b}", res) << std::endl;
}
void handle_mask(const std::vector<fs::path> &files, const char *op,
                 const char *par) {
    if (!par) {
        std::cerr << "mask must be provided" << std::endl;
        return;
    }
    if (!std::regex_match(par, std::regex("[a-fa-f0-9]{1,8}"))) {
        std::cerr << "mask must be a hex number" << std::endl;
        return;
    }
    size_t mask = std::stoull(par, nullptr, 16);
    size_t res = 0;
    for (auto &f : files) {
        std::ifstream in(f);
        if (!in) {
            std::cerr << "ERROR: failed to open file" << std::endl;
            return;
        }
        std::string snum;
        while (std::getline(in, snum, ' ')) {
            if (!std::regex_match(snum, std::regex("\\d+\\s*"))) {
                std::cerr << "invalid number: \'" << snum << "'" << std::endl;
                in.close();
                return;
            }
            size_t num = std::stoull(snum);
            if ((num & mask) == mask)
                res++;
        }
        in.close();
    }
    std::cout << res << std::endl;

    // throw not_implemented("handle_mask", "your code should be here...");
}
void handle_copy(const std::vector<fs::path> &files, const char *op,
                 const char *par) {
    int num = std::stoi(std::string(op).substr(4));

    auto fun = [](fs::path file, int num) {
        auto new_filename = file.stem();
        new_filename += std::to_string(num);
        new_filename += file.extension();

        auto new_file = file;
        new_file.replace_filename(new_filename);
        fs::copy_file(file, new_file, fs::copy_options::overwrite_existing);
    };
    std::vector<std::thread> threads;
    threads.reserve(num * files.size());
    for (const auto &file : files) {
        for (int n = 1; n <= num; n++) {
            threads.emplace_back(std::thread(fun, file, n));
        }
    }
    for (auto &t : threads) {
        t.join();
    }
}
void handle_find(const std::vector<fs::path> &files, const char *op,
                 const char *par) {
    std::map<fs::path, bool> files_to_search;
    if (!par) {
        std::cerr << "ERROR: search pattern must be present" << std::endl;
        return;
    }
    for (auto &f : files) {
        std::ifstream in(f);
        if (!in) {
            std::cerr << "ERROR: failed to open file" << std::endl;
            return;
        }
        std::string file;
        while (std::getline(in, file)) {
            if (!fs::exists(file) || fs::is_directory(file)) {
                std::cerr << "ERROR: file does not exist: \"" << file << "\""
                          << std::endl;
                return;
            }
            files_to_search[file] = false;
        }
    }
    auto fun = [&files_to_search, &par](fs::path path) {
        std::string line;
        std::ifstream in(path);
        while (std::getline(in, line)) {
            if (line.find(par) != std::string::npos) {
                files_to_search[path] = true;
            }
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(files_to_search.size());

    for (auto &[f, v] : files_to_search) {
        threads.emplace_back(std::thread(fun, f));
    }

    for (auto &t : threads) {
        t.join();
    }
    bool found = false;
    for (auto &[f, v] : files_to_search) {
        if (v) {
            found = true;
            std::cout << f << std::endl;
        }
    }
    if (!found) {
        std::cout << "string not found" << std::endl;
    }
}

int main(int argc, const char **argw) {
    std::vector<fs::path> files;
    std::vector<std::pair<std::regex,
                          std::function<void(const std::vector<fs::path> &files,
                                             const char *op, const char *par)>>>
        ops = {{std::regex("xor[2-6]"), handle_xor},
               {std::regex("mask"), handle_mask},
               {std::regex("copy\\d+"), handle_copy},
               {std::regex("find"), handle_find}};
    for (int i = 1; i < argc; i++) {
        for (auto [reg, act] : ops) {
            if (std::regex_match(argw[i], reg)) {
                act(files, argw[i], argw[i + 1]);
                return 0;
            }
        }
        files.push_back(argw[i]);
        if (!fs::exists(files.back()) || fs::is_directory(files.back())) {
            std::cerr << "ERROR: file does not exist: \"" << files.back()
                      << "\"" << std::endl;
            return -1;
        }
    }
    std::cerr << "ERROR: no command";
}

#include <_string.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <exception>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <regex>

namespace fs = std::filesystem;

struct UserInfo {
    int pin;
    int limit;
};

void validate_login(std::string login) {
    if (login.length() > 6) {
        throw std::invalid_argument("login can't be longer than 6 symbols");
    }
    if (std::find_if_not(login.begin(), login.end(), isalnum) != login.end()) {
        throw std::invalid_argument(
            "login can only contain alphanumeric symbols");
    }
}
int validate_pin(std::string pin) {
    if (std::find_if_not(pin.begin(), pin.end(), isnumber) != pin.end()) {
        throw std::invalid_argument("pin must be a positive number");
    }
    int pin_i = std::stoi(pin);
    if (pin_i > 100000) {
        throw std::invalid_argument("pin must be in [0; 100000]");
    }
    return pin_i;
}

class UserList {
  private:
    fs::path path;
    bool updated;
    std::map<std::string, UserInfo> m;

  public:
    UserList(const fs::path &path) : path(path), updated(false) {
        std::ifstream file(path, std::fstream::binary);
        if (!file.is_open()) {
            return;
        }
        size_t n;
        file.read(reinterpret_cast<char *>(&n), sizeof(n));
        for (size_t i = 0; i < n; i++) {
            size_t size;
            file.read(reinterpret_cast<char *>(&size), sizeof(size));
            std::string login;
            login.resize(size);
            file.read(&login[0], size);
            UserInfo info;
            file.read(reinterpret_cast<char *>(&info), sizeof(info));
            m[login] = info;
        }
        file.close();
    }
    void write() {
        if (!updated) {
            return;
        }
        std::ofstream file(path, std::ofstream::binary);
        if (!file.is_open()) {
            std::cerr << "ERROR: failed to open file for writing" << std::endl;
        }
        size_t n = m.size();
        file.write(reinterpret_cast<const char *>(&n), sizeof(n));
        for (const auto &[login, info] : m) {
            size_t size = login.size();
            file.write(reinterpret_cast<const char *>(&size), sizeof(size));
            file.write(login.c_str(), size);
            file.write(reinterpret_cast<const char *>(&info), sizeof(info));
        }
        file.close();
    }

    bool contains(const std::string &login) const {
        return m.contains(login);
    }

    void addUser(const std::string &login, const std::string &pin) {
        validate_login(login);
        int pin_i = validate_pin(pin);
        if (m.contains(login)) {
            throw std::invalid_argument("user already exists");
        }
        UserInfo info = {.pin = pin_i, .limit = -1};
        updated = true;
        m[login] = info;
    }
    void debug() const {
        for (const auto &[login, info] : m) {
            std::cout << login << ": {" << info.pin << ", " << info.limit << "}"
                      << std::endl;
        }
    }
    void setLimit(std::string login, int limit) {
        validate_login(login);
        if (!m.contains(login)) {
            throw std::invalid_argument("no login");
        }
        UserInfo info = m[login];
        info.limit = limit;
        m[login] = info;
        updated = true;
    }

    int login(const std::string &login, const std::string &pin) {
        int pin_i = validate_pin(pin);
        validate_login(login);
        if (!m.contains(login)) {
            throw std::invalid_argument("login does not exist");
        }
        UserInfo info = m[login];
        if (pin_i != info.pin) {
            throw std::invalid_argument("invalid pin");
        }
        return info.limit;
    }
};

std::vector<std::string> split() {
    std::string line;
    std::getline(std::cin, line);
    std::stringstream ss(line);
    std::string tok;
    std::vector<std::string> words;
    while (ss >> tok) {
        words.push_back(tok);
    }
    return words;
}

void main_loop(UserList &users) {
    std::string login;
    std::string pin;
    int limit = -1;
    std::cout << "login: " << std::flush;
    std::getline(std::cin, login);
    std::cout << "pin: " << std::flush;
    std::getline(std::cin, pin);

    try {
        limit = users.login(login, pin);
    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return;
    }

    while (!std::cin.eof() && limit != 0) {
        std::cout << "> " << std::flush;
        auto toks = split();

        // std::cout << "[";
        // for (auto tok: toks)
        //         std::cout << "'" << tok << "', ";
        // std::cout << "]" << std::endl;


        if (toks.empty())
            continue;
        if (toks[0] == "Logout")
            return;


        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto local = std::localtime(&in_time_t);

        if (toks[0] == "Time") {
            std::cout << std::put_time(local, "%X") << std::endl;

        } else if (toks[0] == "Date") {
            std::cout << std::put_time(local, "%d:%m:%Y") << std::endl;
        } else if (toks[0] == "Howmuch") {
            if (toks.size() < 2) {
                std::cout << "date must be provided" << std::endl;
                continue;
            }
            if (!std::regex_match(toks[1],
                                  std::regex("\\d{1,2}:\\d{1,2}:\\d{1,4}"))) {
                std::cout << "date must be in dd:mm:yyyy format" << std::endl;
                continue;
            }
            if (toks.size() < 3) {
                std::cout << "time must be provided" << std::endl;
                continue;
            }
            if (!std::regex_match(toks[2],
                                  std::regex("\\d{1,2}:\\d{1,2}:\\d{1,2}"))) {
                std::cout << "time must be in hh:mm:ss format" << std::endl;
                continue;
            }
            std::stringstream full_time(toks[1] + " " + toks[2]);
            std::tm tm{};
            full_time >> std::get_time(&tm, "%d:%m:%Y %X");
            auto t = std::mktime(&tm);
            auto diff = in_time_t - t; // seconds
            std::cout << diff << std::endl;

            if (toks.size() < 4) {
                std::cout << "output format must be provided" << std::endl;
                continue;
            }

        } else if (toks[0] == "Sanctions") {
            if (toks.size() != 3) {
                std::cout << "both username and limit must be provided"
                          << std::endl;
                continue;
            }
            if (std::find_if_not(toks[2].begin(), toks[2].end(), isnumber) !=
                toks[2].end()) {
                std::cout << "limit must be a number" << std::endl;
                continue;
            }
            int limit = std::stoi(toks[2]);
            auto conf = split();
            if (conf.size() != 1 || conf[0] != "12345") {
                continue;
            }
            try {
                users.setLimit(toks[1], limit);
                users.write();
            } catch (std::exception &e) {
                std::cout << "invalid login" << std::endl;
            }
        } else {
            std::cout << "unknown command" << std::endl;
            continue;
        }
        if (limit > 0)
            limit--;
    }
}

int main(int argc, const char **argw) {
    if (argc < 2) {
        std::cerr << "not enough arguments: user data file must be provided"
                  << std::endl;
        return -1;
    }
    UserList users(argw[1]);

    if (argc > 2 && (strcmp(argw[2], "-r") || strcmp(argw[2], "--register"))) {
        if (argc < 5) {
            std::cerr
                << "not enough arguments: both login and pin must be provided"
                << std::endl;
            return -1;
        }
        if (argc > 5) {
            std::cerr
                << "too many arguments: only login and pin must be provided"
                << std::endl;
            return -1;
        }
        try {
            users.addUser(argw[3], argw[4]);
        } catch (std::exception &e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
            return -1;
        }
        users.write();
    }
    // users.debug();

    while(!std::cin.eof()) {
        main_loop(users);
    }
}

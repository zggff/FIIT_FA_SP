#include <string>
#include <sstream>
// #include <algorithm>
#include <utility>
#include <mutex>
#include "../include/client_logger.h"
#include <not_implemented.h>

static std::mutex global_streams_mutex;

std::unordered_map<std::string, std::pair<size_t, std::ofstream>>
    client_logger::refcounted_stream::_global_streams;

logger &client_logger::log(const std::string &text,
                           logger::severity severity) & {
    std::string formatted = make_format(text, severity);
    for (const auto &[sev, pair] : _output_streams) {
        if (sev != severity)
            continue;
        const auto &[streams, is_console] = pair;

        for (const auto &s : streams) {
            if (s._stream.second != nullptr) {
                *s._stream.second << formatted << std::endl;
            }
        }

        if (is_console) {
            std::cout << formatted << std::endl;
        }
    }

    return *this;
}

std::string client_logger::make_format(const std::string &message,
                                       severity sev) const {
        std::string result;
        std::ostringstream s;
        for (size_t i = 0; i < _format.size(); ++i) {
            if (_format[i] == '%' && i + 1 < _format.size()) {
                flag f = char_to_flag(_format[++i]);
                switch (f) {
                case flag::DATE:
                    s << current_date_to_string();
                    break;
                case flag::TIME:
                    s << current_time_to_string();
                    break;
                case flag::SEVERITY:
                    s << severity_to_string(sev);
                    break;
                case flag::MESSAGE:
                    s << message;
                    break;
                default:
                    s << _format[i];
                    break;
                }
            } else {
                s << _format[i];
            }
        }
        return s.str();
}

void client_logger::refcounted_stream::open() {

    if (_stream.first.empty()) {
        _stream.second = nullptr;
        return;
    }

    std::lock_guard<std::mutex> lock(global_streams_mutex);

    auto it = _global_streams.find(_stream.first);
    if (it == _global_streams.end()) {
        _global_streams[_stream.first] =
            std::make_pair(1, std::ofstream(_stream.first));
        if (!_global_streams[_stream.first].second.is_open()) {
            throw std::runtime_error("Failed to open log file: " +
                                     _stream.first);
        }
        _stream.second = &_global_streams[_stream.first].second;
    } else {
        ++it->second.first;
        _stream.second = &it->second.second;
    }
}

client_logger::client_logger(
    const std::unordered_map<
        logger::severity, std::pair<std::forward_list<refcounted_stream>, bool>>
        &streams,
    std::string format)
    : _output_streams(streams), _format(std::move(format)) {
    for (auto &[sev, streams_pair] : _output_streams) {
        for (auto &stream : streams_pair.first) {
            stream.open();
        }
    }
}

client_logger::flag client_logger::char_to_flag(char c) noexcept {
    switch (c) {
    case 'd':
        return flag::DATE;
    case 't':
        return flag::TIME;
    case 's':
        return flag::SEVERITY;
    case 'm':
        return flag::MESSAGE;
    default:
        return flag::NO_FLAG;
    }
}

client_logger::client_logger(const client_logger &other)
    : _output_streams(other._output_streams), _format(other._format) {

    for (auto &[sev, streams_pair] : _output_streams) {
        for (auto &stream : streams_pair.first) {

            if (!stream._stream.first.empty()) {
                stream.open();
            }
        }
    }
}

client_logger &client_logger::operator=(const client_logger &other) {
    if (this != &other) {
        _output_streams = other._output_streams;
        _format = other._format;
    }
    return *this;
}

client_logger::client_logger(client_logger &&other) noexcept
    : _output_streams(std::move(other._output_streams)),
      _format(std::move(other._format)) {
}

client_logger &client_logger::operator=(client_logger &&other) noexcept {
    if (this != &other) {
        _output_streams = std::move(other._output_streams);
        _format = std::move(other._format);
    }
    return *this;
}

client_logger::~client_logger() noexcept = default;

client_logger::refcounted_stream::refcounted_stream(const std::string &path)
    : _stream(path, nullptr) {

    if (path.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(global_streams_mutex);

    auto it = _global_streams.find(path);
    if (it == _global_streams.end()) {
        _global_streams[path] = std::make_pair(1, std::ofstream(path));
        if (!_global_streams[path].second.is_open()) {
            throw std::runtime_error("Failed to open log file: " + path);
        }
        _stream.second = &_global_streams[path].second;
    } else {
        ++it->second.first;
        _stream.second = &it->second.second;
    }
}

client_logger::refcounted_stream::refcounted_stream(
    const client_logger::refcounted_stream &oth)
    : _stream(oth._stream) {
    if (_stream.second != nullptr && !_stream.first.empty()) {
        std::lock_guard<std::mutex> lock(global_streams_mutex);

        auto it = _global_streams.find(_stream.first);
        if (it == _global_streams.end()) {
            _stream.second = nullptr;
            throw std::runtime_error("Global stream not found for path: " +
                                     _stream.first);
        }
        ++it->second.first;
    }
}

client_logger::refcounted_stream &client_logger::refcounted_stream::operator=(
    const client_logger::refcounted_stream &oth) {
    if (this != &oth) {
        {
            std::lock_guard<std::mutex> lock(global_streams_mutex);

            if (_stream.second != nullptr && !_stream.first.empty()) {
                auto current_it = _global_streams.find(_stream.first);
                if (current_it != _global_streams.end()) {
                    if (--current_it->second.first == 0) {
                        current_it->second.second.close();
                        _global_streams.erase(current_it);
                    }
                }
            }
        }

        _stream.first = oth._stream.first;
        _stream.second = oth._stream.second;

        if (_stream.second != nullptr && !_stream.first.empty()) {
            std::lock_guard<std::mutex> lock(global_streams_mutex);

            auto new_it = _global_streams.find(_stream.first);
            if (new_it != _global_streams.end()) {
                ++new_it->second.first;
            } else {
                _stream.second = nullptr;
                throw std::runtime_error("Global stream not found for path: " +
                                         _stream.first);
            }
        }
    }
    return *this;
}

client_logger::refcounted_stream::refcounted_stream(
    client_logger::refcounted_stream &&oth) noexcept
    : _stream(std::move(oth._stream)) {
    oth._stream = {oth._stream.first, nullptr};
}

client_logger::refcounted_stream &client_logger::refcounted_stream::operator=(
    client_logger::refcounted_stream &&oth) noexcept {
    if (this != &oth) {
        this->~refcounted_stream();
        _stream = std::move(oth._stream);
        oth._stream = {oth._stream.first, nullptr};
    }
    return *this;
}

client_logger::refcounted_stream::~refcounted_stream() {
    if (!_stream.second || _stream.first.empty())
        return;

    std::lock_guard<std::mutex> lock(global_streams_mutex);

    auto it = _global_streams.find(_stream.first);
    if (it != _global_streams.end()) {
        if (--it->second.first == 0) {
            it->second.second.close();
            _global_streams.erase(it);
        }
    }
}

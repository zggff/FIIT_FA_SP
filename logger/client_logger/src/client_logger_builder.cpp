#include <filesystem>
#include <utility>
#include <not_implemented.h>
#include "../include/client_logger_builder.h"
#include <not_implemented.h>

using namespace nlohmann;

logger_builder& client_logger_builder::add_file_stream(
    const std::string &stream_file_path,
    logger::severity severity) &
{
    try {
        if (stream_file_path.empty())
        {
            std::cerr << "Warning: File path is empty, stream not added" << std::endl;
            return *this;
        }
        
        std::filesystem::path path(stream_file_path);
        try {
            if (!path.parent_path().empty() && !std::filesystem::exists(path.parent_path()))
            {
                std::filesystem::create_directories(path.parent_path());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory for log file: " << e.what() << std::endl;
            return *this;
        }

        auto &severity_entry = _output_streams[severity];
        
        
        bool duplicate = false;
        for (const auto &stream : severity_entry.first)
        {
            if (stream._stream.first == stream_file_path)
            {
                
                std::cerr << "Warning: File path '" << stream_file_path << "' already added for this severity level" << std::endl;
                duplicate = true;
                break;
            }
        }
        
        
        // uncomment if want to disallow duplicates
        // if (duplicate) {
        //     return *this;
        // }
        
        
        
        {
            std::ofstream test_file(stream_file_path, std::ios::app);
            if (!test_file.is_open()) {
                std::cerr << "Warning: Cannot open log file for writing: " << stream_file_path << std::endl;
                return *this;
            }
        }
        
        //comment if want to disallow duplicates
        severity_entry.first.emplace_front(client_logger::refcounted_stream(stream_file_path));
        return *this;
    } catch (const std::exception& e) {
        std::cerr << "Error adding file stream: " << e.what() << std::endl;
        return *this;
    }
}

logger_builder& client_logger_builder::add_console_stream(
    logger::severity severity) &
{
    try {
        
        _output_streams[severity].second = true;
        return *this;
    } catch (const std::exception& e) {
        std::cerr << "Error adding console stream: " << e.what() << std::endl;
        return *this;
    }
}

logger_builder& client_logger_builder::transform_with_configuration(
    std::string const &configuration_file_path,
    std::string const &configuration_path) &
{
    try {
        std::ifstream file(configuration_file_path);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open configuration file: " + configuration_file_path);
        }

        json config;
        try {
            file >> config;
        } catch (const json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
        file.close();

        
        auto node = config.contains(configuration_path) ? 
                    config.at(configuration_path) : config;
        
        
        if (node.contains("format"))
        {
            _format = node["format"].get<std::string>();
        }

        
        if (node.contains("severity"))
        {

            for (auto& [severity_name, severity_config] : node["severity"].items())
            {
                logger::severity severity;
                
                
                if (severity_name == "trace") severity = logger::severity::trace;
                else if (severity_name == "debug") severity = logger::severity::debug;
                else if (severity_name == "information") severity = logger::severity::information;
                else if (severity_name == "warning") severity = logger::severity::warning;
                else if (severity_name == "error") severity = logger::severity::error;
                else if (severity_name == "critical") severity = logger::severity::critical;
                else
                {
                    throw std::runtime_error("Unknown severity level: " + severity_name);
                }
                
                
                if (severity_config.contains("console") && severity_config["console"].get<bool>())
                {
                    add_console_stream(severity);
                }
                
                
                if (severity_config.contains("files") && severity_config["files"].is_array())
                {
                    for (const auto& file_path : severity_config["files"])
                    {
                        add_file_stream(file_path.get<std::string>(), severity);
                    }
                }
            }
        }
        
        else if (node.contains("streams") && node["streams"].is_array())
        {
            for (const auto& stream : node["streams"])
            {
                std::string type = stream["type"].get<std::string>();
                std::string severity_str = stream["severity"].get<std::string>();
                
                logger::severity severity;
                if (severity_str == "trace") severity = logger::severity::trace;
                else if (severity_str == "debug") severity = logger::severity::debug;
                else if (severity_str == "information") severity = logger::severity::information;
                else if (severity_str == "warning") severity = logger::severity::warning;
                else if (severity_str == "error") severity = logger::severity::error;
                else if (severity_str == "critical") severity = logger::severity::critical;
                else
                {
                    throw std::runtime_error("Unknown severity level: " + severity_str);
                }

                if (type == "console")
                {
                    add_console_stream(severity);
                }
                else if (type == "file")
                {
                    std::string path = stream["path"].get<std::string>();
                    add_file_stream(path, severity);
                }
                else
                {
                    throw std::runtime_error("Unknown stream type: " + type);
                }
            }
        }
        
        return *this;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in transform_with_configuration: " << e.what() << std::endl;
        
        return *this;
    }
}

logger_builder& client_logger_builder::clear() &
{
    try {
        _output_streams.clear();
        _format = "%m";
        return *this;
    } catch (const std::exception& e) {
        std::cerr << "Error clearing logger builder: " << e.what() << std::endl;
        return *this;
    }
}

logger *client_logger_builder::build() const
{
    try {
        if (_output_streams.empty())
        {
            std::cerr << "Warning: No output streams configured, logs will not be output anywhere" << std::endl;
        }

        if (_format.empty())
        {
            std::cerr << "Warning: Empty format string, using default format \"%m\"" << std::endl;

            return new client_logger(_output_streams, "%m");
        }


        if (_format.find("%m") == std::string::npos)
        {
            std::cerr << "Warning: Format string does not contain message placeholder (%m), messages will not be displayed" << std::endl;
        }

        return new client_logger(_output_streams, _format);
    }
    catch (const std::exception& e) {
        std::cerr << "Error building logger: " << e.what() << std::endl;
        throw; 
    }
}

logger_builder& client_logger_builder::set_format(const std::string &format) &
{
    try {
        if (format.empty())
        {
            std::cerr << "Warning: Empty format string provided, using default \"%m\"" << std::endl;
            _format = "%m";
        }
        else
        {
            _format = format;
            
            
            if (_format.find("%m") == std::string::npos)
            {
                std::cerr << "Warning: Format string does not contain message placeholder (%m), messages will not be displayed" << std::endl;
            }
        }
        return *this;
    } catch (const std::exception& e) {
        std::cerr << "Error setting format: " << e.what() << std::endl;
        return *this;
    }
}

logger_builder& client_logger_builder::set_destination(const std::string &format) &
{
    
    std::cerr << "Warning: set_destination is not applicable for client_logger" << std::endl;
    return *this;
}

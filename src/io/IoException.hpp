#pragma once

#include <exception>
#include <string>
#include <format>

namespace tw::io {

class IoException : public std::exception {
    std::string m_filename;
    std::string m_message;

public:
    IoException(std::string filename, std::string what) :
        m_message(std::format("{}: {}", filename, what)),
        m_filename(filename)
    { }

    const char* what() const noexcept override { return m_message.c_str(); }

    const std::string& get_filename() const { return m_filename; }
};

}

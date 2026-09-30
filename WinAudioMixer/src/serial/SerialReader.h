#pragma once

#include "serial/ISerialReader.h"

#include <deque>
#include <istream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

namespace winaudiomixer {

class SerialReader : public ISerialReader {
public:
    explicit SerialReader(std::istream* input = nullptr);

    bool open(const std::string& port, int baudRate) override;
    bool readValues(std::vector<int>& values) override;
    void close() override;
    bool isOpen() const override;

    void setExpectedValues(std::size_t expectedValues);
    void pushLine(const std::string& line);

private:
    std::istream* input_;
    std::deque<std::string> queuedLines_;
    std::string port_;
    int baudRate_;
    std::size_t expectedValues_;
    bool isOpen_;
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
#endif

    bool readQueuedLine(std::string& line);
};

std::vector<int> parseValues(const std::string& text, std::size_t expectedCount);

} // namespace winaudiomixer

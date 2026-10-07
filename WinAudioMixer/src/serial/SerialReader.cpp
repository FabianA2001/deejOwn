#include "serial/SerialReader.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "util/Logger.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX            // verhindert min/max-Makros von windows.h
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>          // ssize_t
#endif

namespace winaudiomixer {
namespace {

std::string trim(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

bool isValidIntegerToken(const std::string& token)
{
    if (token.empty()) {
        return false;
    }

    const auto start = token.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return false;
    }

    std::size_t index = start;
    if (token[index] == '+' || token[index] == '-') {
        ++index;
    }

    if (index == token.size()) {
        return false;
    }

    for (; index < token.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(token[index]))) {
            return false;
        }
    }

    return true;
}

} // namespace

SerialReader::SerialReader(std::istream* input)
    : input_(input), queuedLines_(), port_(), baudRate_(9600), expectedValues_(5), isOpen_(false)
{
}

bool SerialReader::open(const std::string& port, int baudRate)
{
    port_ = port;
    baudRate_ = baudRate;

    if (input_ != nullptr) {
        isOpen_ = true;
        return true;
    }

    winaudiomixer::Logger::info("Oeffne Port: " + port_ + " mit Budrate " + std::to_string(baudRate_));


#ifdef _WIN32
    std::wstring widePort(port.begin(), port.end());
    if (widePort.rfind(L"\\\\.\\", 0) != 0 && widePort.rfind(L"COM", 0) == 0) {
        widePort = L"\\\\.\\" + widePort;
    }

    handle_ = CreateFileW(widePort.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
        isOpen_ = false;
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle_, &dcb)) {
        close();
        return false;
    }

    dcb.BaudRate = static_cast<DWORD>(baudRate_);
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

    if (!SetCommState(handle_, &dcb)) {
        close();
        return false;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(handle_, &timeouts)) {
        close();
        return false;
    }

    isOpen_ = true;
    return true;
#else
    fd_ = ::open(port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0) {
        isOpen_ = false;
        return false;
    }

    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        close();
        return false;
    }

    cfmakeraw(&tty);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    speed_t speed = B9600;
    switch (baudRate_) {
        case 1200: speed = B1200; break;
        case 2400: speed = B2400; break;
        case 4800: speed = B4800; break;
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default: close(); return false;
    }

    if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0) {
        close();
        return false;
    }

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        close();
        return false;
    }

    isOpen_ = true;
    return true;
#endif
}

bool SerialReader::readValues(std::vector<int>& values)
{
    values.clear();

    std::string line;
    if (!readQueuedLine(line)) {
        if (input_ != nullptr) {
            if (!std::getline(*input_, line)) {
                return false;
            }
        } else if (!isOpen_) {
            return false;
        } else {
#ifdef _WIN32
            std::string buffer;
            char ch = 0;
            DWORD bytesRead = 0;
            while (true) {
                if (!ReadFile(handle_, &ch, 1, &bytesRead, nullptr) || bytesRead != 1) {
                    if (buffer.empty()) {
                        return false;
                    }
                    break;
                }

                if (ch == '\r') {
                    continue;
                }
                if (ch == '\n') {
                    line = buffer;
                    break;
                }

                buffer.push_back(ch);
                if (buffer.size() > 1024) {
                    return false;
                }
            }
#else
            std::string buffer;
            char ch = 0;
            while (true) {
                const ssize_t readCount = ::read(fd_, &ch, 1);
                if (readCount == 0) {
                    if (buffer.empty()) {
                        return false;
                    }
                    break;
                }
                if (readCount < 0) {
                    return false;
                }

                if (ch == '\r') {
                    continue;
                }
                if (ch == '\n') {
                    line = buffer;
                    break;
                }

                buffer.push_back(ch);
                if (buffer.size() > 1024) {
                    return false;
                }
            }
#endif
        }
    }

    const std::string cleaned = trim(line);
    if (cleaned.empty()) {
        return false;
    }

    try {
        values = parseValues(cleaned, expectedValues_ == 0 ? 1 : expectedValues_);
    } catch (const std::exception&) {
        return false;
    }

    return !values.empty();
}

void SerialReader::close()
{
    isOpen_ = false;
    queuedLines_.clear();
#ifdef _WIN32
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

bool SerialReader::isOpen() const
{
    return isOpen_;
}

void SerialReader::setExpectedValues(std::size_t expectedValues)
{
    expectedValues_ = expectedValues == 0 ? 1 : expectedValues;
}

void SerialReader::pushLine(const std::string& line)
{
    queuedLines_.push_back(line);
}

bool SerialReader::readQueuedLine(std::string& line)
{
    if (queuedLines_.empty()) {
        return false;
    }

    line = queuedLines_.front();
    queuedLines_.pop_front();
    return true;
}

std::vector<int> parseValues(const std::string& text, std::size_t expectedCount)
{
    const std::string trimmed = trim(text);
    if (trimmed.empty()) {
        return {};
    }

    std::vector<int> values;
    std::size_t tokenCount = 0;
    std::string token;
    std::stringstream stream(trimmed);
    while (std::getline(stream, token, '|')) {
        const std::string normalized = trim(token);
        if (normalized.empty()) {
            return {};
        }
        if (!isValidIntegerToken(normalized)) {
            return {};
        }
        values.push_back(std::stoi(normalized));
        ++tokenCount;
    }

    if (tokenCount != expectedCount) {
        return {};
    }

    return values;
}

} // namespace winaudiomixer

#pragma once

#include <string>
#include <vector>

namespace winaudiomixer {

class ISerialReader {
public:
    virtual ~ISerialReader() = default;

    virtual bool open(const std::string& port, int baudRate) = 0;
    virtual bool readValues(std::vector<int>& values) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};

} // namespace winaudiomixer

#pragma once

#include <vector>

namespace winaudiomixer {

class IValueFilter {
public:
    virtual ~IValueFilter() = default;

    virtual std::vector<int> process(const std::vector<int>& values) = 0;
};

} // namespace winaudiomixer

#pragma once

#include "filter/IValueFilter.h"

#include <cstddef>
#include <vector>

namespace winaudiomixer {

class MedianFilter : public IValueFilter {
public:
    explicit MedianFilter(std::size_t windowSize = 5);

    std::vector<int> process(const std::vector<int>& values) override;

    std::size_t windowSize() const noexcept;

private:
    std::size_t windowSize_;
    std::vector<std::vector<int>> history_;
};

} // namespace winaudiomixer

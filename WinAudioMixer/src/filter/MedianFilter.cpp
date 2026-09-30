#include "filter/MedianFilter.h"

#include <algorithm>
#include <stdexcept>

namespace winaudiomixer {

MedianFilter::MedianFilter(std::size_t windowSize)
    : windowSize_(windowSize == 0 ? 1 : windowSize)
{
}

std::vector<int> MedianFilter::process(const std::vector<int>& values)
{
    std::vector<int> result;
    result.reserve(values.size());

    for (const int value : values) {
        history_.push_back(value);
        if (history_.size() > windowSize_) {
            history_.erase(history_.begin());
        }

        std::vector<int> sorted = history_;
        std::sort(sorted.begin(), sorted.end());

        int median = sorted[sorted.size() / 2];
        if (sorted.size() % 2 == 0) {
            const int left = sorted[sorted.size() / 2 - 1];
            const int right = sorted[sorted.size() / 2];
            median = (left + right) / 2;
        }

        result.push_back(median);
    }

    return result;
}

std::size_t MedianFilter::windowSize() const noexcept
{
    return windowSize_;
}

} // namespace winaudiomixer

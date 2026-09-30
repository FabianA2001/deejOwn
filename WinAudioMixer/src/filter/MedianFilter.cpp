#include "filter/MedianFilter.h"

#include <algorithm>
#include <stdexcept>

namespace winaudiomixer
{

    MedianFilter::MedianFilter(std::size_t windowSize)
        : windowSize_(windowSize == 0 ? 5 : windowSize)
    {
    }

    std::vector<int> MedianFilter::process(const std::vector<int> &values)
    {
        std::vector<int> result;
        result.reserve(values.size());

        if (values.size() > history_.size())
        {
            history_.resize(values.size());
        }
        for (size_t i = 0; i < values.size(); ++i)
        {
            int value = values.at(i);

            auto &currentHistory = history_.at(i);

            currentHistory.push_back(value);

            if (currentHistory.size() > windowSize_)
            {
                currentHistory.erase(currentHistory.begin());
            }

            std::vector<int> sorted = currentHistory;
            std::sort(sorted.begin(), sorted.end());

            int median = sorted[sorted.size() / 2];

            if (sorted.size() % 2 == 0)
            {
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

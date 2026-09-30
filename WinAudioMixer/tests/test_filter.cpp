#include "filter/MedianFilter.h"

#include <cassert>
#include <vector>

void run_filter_test()
{
    winaudiomixer::MedianFilter filter(5);

    const std::vector<int> input = {500, 503, 499, 900, 501};
    const auto result = filter.process(input);

    assert(result.size() == input.size());
    assert(result[0] == 500);
    assert(result[1] == 501);
    assert(result[2] == 500);
    assert(result[3] == 501);
    assert(result[4] == 501);
}

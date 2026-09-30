#include "config/ConfigParser.h"

#include <cassert>
#include <string>

void run_config_test()
{
    const std::string config = R"(
[Serial]
port=COM5
baudrate=9600
slider_count=5

[Filter]
window_size=5

[DeadZone]
value=5

[Mapping]
slider_0=Spotify
slider_1=Chrome
slider_2=Discord,Teams
slider_3=unassigned
slider_4=unassigned
)";

    const auto parsed = winaudiomixer::ConfigParser::parseString(config);

    assert(parsed.serialPort == "COM5");
    assert(parsed.baudRate == 9600);
    assert(parsed.sliderCount == 5);
    assert(parsed.filterWindowSize == 5);
    assert(parsed.deadZone == 5);
    assert(parsed.sliderAssignments.at(0).size() == 1);
    assert(parsed.sliderAssignments.at(2).size() == 2);
    assert(parsed.sliderAssignments.at(3).size() == 1);
    assert(parsed.sliderAssignments.at(3)[0] == "unassigned");
}

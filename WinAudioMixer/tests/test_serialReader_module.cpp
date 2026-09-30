#include "config/ConfigParser.h"
#include "controller/SliderController.h"
#include "filter/MedianFilter.h"
#include "serial/SerialReader.h"
#ifdef _WIN32
#include "audio/WindowsAudioMixer.h"
#else
#include "audio/NullAudioMixer.h"
#endif
#include "util/Logger.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vector>

const int SLIEDERCOUNT = 2;
const std::string PORT = "/dev/cu.usbmodem1301";
const int BAUDRATE = 9600;
const int FILTERWINDOWSIZE = 20;

int main()
{
    winaudiomixer::SerialReader serialReader;
    serialReader.setExpectedValues(SLIEDERCOUNT);
    if (!serialReader.open(PORT, BAUDRATE))
    {
        winaudiomixer::Logger::error("Could not open serial port");
        return 1;
    }

    winaudiomixer::MedianFilter filter(FILTERWINDOWSIZE);

    while (1)
    {
        // for (int i = 0; i < 10; i++)
        // {
        std::vector<int> values;
        winaudiomixer::Logger::info(serialReader.readValues(values) ? "True" : "False");

        const std::vector<int> filteredValues = filter.process(values);

        std::string output = "";
        for (auto &value : filteredValues)
        {
            output += std::to_string(value);
            output += " ";
        }
        winaudiomixer::Logger::info(output);
    }
    serialReader.close();
}
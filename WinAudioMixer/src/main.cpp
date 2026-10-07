#include "config/ConfigParser.h"
#include "controller/SliderController.h"
#include "filter/MedianFilter.h"
#include "serial/SerialReader.h"
#include "util/PathUtils.h"
#ifdef _WIN32
#include "audio/WindowsAudioMixer.h"
#else
#include "audio/NullAudioMixer.h"
#endif
#include "util/Logger.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vector>

int main()
{
    try
    {
        const std::filesystem::path configPath = winaudiomixer::getExecutableDir() / "config.ini";

        const winaudiomixer::Configuration config =winaudiomixer::ConfigParser::parseFile(configPath);
        winaudiomixer::Logger::info("Configuration loaded");

        winaudiomixer::SerialReader serialReader;
        serialReader.setExpectedValues(config.sliderCount);
        if (!serialReader.open(config.serialPort, config.baudRate))
        {
            winaudiomixer::Logger::error("Could not open serial port");
            return 1;
        }

        winaudiomixer::MedianFilter filter(config.filterWindowSize);
        winaudiomixer::SliderController controller(config.sliderCount);
        controller.setDeadZone(config.deadZone);

        for (const auto &[sliderIndex, programs] : config.sliderAssignments)
        {
            controller.setMapping(sliderIndex, programs);
        }

#ifdef _WIN32
        winaudiomixer::Logger::info("Using Windows Audio Mixer");
        winaudiomixer::WindowsAudioMixer audioMixer;
#else
        winaudiomixer::Logger::info("Using Null Audio Mixer (not on Windows)");
        winaudiomixer::NullAudioMixer audioMixer;
#endif
        int readcounter = 0;
        while (true)
        {
            //std::this_thread::sleep_for(std::chrono::milliseconds(5));
            readcounter++;
            std::vector<int> rawValues;
            if (!serialReader.readValues(rawValues))
            {
                //winaudiomixer::Logger::warning("rawValues is empty");
                continue;
            }

            const std::vector<int> filteredValues = filter.process(rawValues);

            if (readcounter < 10)
                continue;
            readcounter = 0;

            const std::vector<winaudiomixer::ApplicationVolumeChange> changes = controller.process(filteredValues, audioMixer);

            for (const auto &change : changes)
            {
                audioMixer.setApplicationVolume(change.application, change.volume);
            }
        }
    }
    catch (const std::exception &ex)
    {
        winaudiomixer::Logger::error(ex.what());
        return 1;
    }
}

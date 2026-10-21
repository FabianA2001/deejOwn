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
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

int main()
{
    try
    {
        const std::filesystem::path configPath = winaudiomixer::getExecutableDir() / "config.ini";

        const winaudiomixer::Configuration config = winaudiomixer::ConfigParser::parseFile(configPath);
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

        // Neue Audioquellen werden vom Windows-Thread hier abgelegt und im Hauptthread abgeholt.
        // Muss VOR dem AudioMixer deklariert werden, damit es laenger lebt als der Callback.
        std::mutex newApplicationsMutex;
        std::vector<std::string> newApplications;

#ifdef _WIN32
        winaudiomixer::Logger::info("Using Windows Audio Mixer");
        winaudiomixer::WindowsAudioMixer audioMixer;
#else
        winaudiomixer::Logger::info("Using Null Audio Mixer (not on Windows)");
        winaudiomixer::NullAudioMixer audioMixer;
#endif
        audioMixer.setNewApplicationCallback([&](const std::string &application)
                                             {
            std::lock_guard<std::mutex> lock(newApplicationsMutex);
            newApplications.push_back(application); });

        int readcounter = 0;
        while (true)
        {
            // std::this_thread::sleep_for(std::chrono::milliseconds(5));
            readcounter++;
            std::vector<int> rawValues;
            if (!serialReader.readValues(rawValues))
            {
                // winaudiomixer::Logger::warning("rawValues is empty");
                continue;
            }

            const std::vector<int> filteredValues = filter.process(rawValues);

            // Neu aufgetauchte Audioquellen sofort auf den aktuellen Sliderwert setzen
            std::vector<std::string> pendingApplications;
            if (!filteredValues.empty())
            {
                std::lock_guard<std::mutex> lock(newApplicationsMutex);
                pendingApplications.swap(newApplications);
            }

            std::vector<winaudiomixer::ApplicationVolumeChange> changes =
                controller.processNewApplications(pendingApplications, filteredValues);

            // Normale Slider-Aenderungen nur alle 10 Lesevorgaenge
            if (readcounter >= 10)
            {
                readcounter = 0;

                const std::vector<winaudiomixer::ApplicationVolumeChange> sliderChanges =
                    controller.process(filteredValues, audioMixer);

                changes.insert(changes.end(), sliderChanges.begin(), sliderChanges.end());
            }

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

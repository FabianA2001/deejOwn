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


int main(){
    winaudiomixer::SerialReader serialReader;
    serialReader.setExpectedValues(SLIEDERCOUNT);
    if (!serialReader.open(PORT, BAUDRATE)) {
        winaudiomixer::Logger::error("Could not open serial port");
        return 1;
    }

    while (1){
        std::vector<int> values;
        serialReader.readValues(values);
        std::string output = "";
        for(auto & value: values){
            output += std::to_string(value);
            output += " ";
        }
        winaudiomixer::Logger::info(output);
    }
}
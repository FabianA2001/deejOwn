cmake_minimum_required(VERSION 3.20)

project(WinAudioMixer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(win_audio_lib
    src/config/Configuration.h
    src/config/ConfigParser.cpp
    src/filter/MedianFilter.cpp
    src/controller/SliderController.cpp
    src/serial/SerialReader.cpp
    src/audio/NullAudioMixer.cpp
    src/audio/WindowsAudioMixer.cpp
    src/util/Logger.cpp
)

target_include_directories(win_audio_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(WinAudioMixer
    src/main.cpp
)



target_link_libraries(WinAudioMixer PRIVATE win_audio_lib)

add_executable(PrintSerialMonitor
    tests/test_serialReader_module.cpp    
)

target_link_libraries(PrintSerialMonitor PRIVATE win_audio_lib)

add_executable(win_audio_tests
    tests/test_suite.cpp
    tests/test_config.cpp
    tests/test_filter.cpp
    tests/test_deadzone.cpp
    tests/test_mapping.cpp
)

target_link_libraries(win_audio_tests PRIVATE win_audio_lib)

enable_testing()
add_test(NAME win_audio_tests COMMAND win_audio_tests)

target_compile_definitions(win_audio_lib PRIVATE
    $<$<PLATFORM_ID:Windows>:UNICODE>
    $<$<PLATFORM_ID:Windows>:_UNICODE>
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

if (WIN32)
    target_link_libraries(win_audio_lib PRIVATE
        ole32
        uuid
        user32
        gdi32
        comctl32
    )
endif()
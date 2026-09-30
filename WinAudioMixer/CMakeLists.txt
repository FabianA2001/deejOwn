cmake_minimum_required(VERSION 3.20)

project(WinAudioMixer
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(WinAudioMixer
    main.cpp
)

target_compile_definitions(WinAudioMixer PRIVATE
    $<$<PLATFORM_ID:Windows>:UNICODE>
    $<$<PLATFORM_ID:Windows>:_UNICODE>
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

if (WIN32)
    target_link_libraries(WinAudioMixer PRIVATE
        ole32
        uuid
        user32
        gdi32
        comctl32
    )
endif()
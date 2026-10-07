cmake_minimum_required(VERSION 3.20)

project(WinAudioMixer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Option: Einzelne .exe (keine DLLs nötig, nur config.ini daneben)
# ---------------------------------------------------------------------------
option(WINAUDIO_SINGLE_EXE "Runtime statisch linken, damit nur eine .exe + config.ini nötig sind" ON)

# Muss VOR allen add_library/add_executable stehen und gilt für alle Targets
if (MSVC)
    if (WINAUDIO_SINGLE_EXE)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")        # /MT bzw. /MTd
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")     # /MD bzw. /MDd
    endif()
endif()

# ---------------------------------------------------------------------------
# Bibliothek (statisch, wird komplett in die .exe eingebunden)
# ---------------------------------------------------------------------------
add_library(win_audio_lib STATIC
    src/config/Configuration.h
    src/config/ConfigParser.cpp
    src/filter/MedianFilter.cpp
    src/controller/SliderController.cpp
    src/serial/SerialReader.cpp
    src/audio/NullAudioMixer.cpp
    src/audio/WindowsAudioMixer.cpp
    src/util/Logger.cpp
    src/util/PathUtils.cpp
    src/util/PathUtils.h
)

target_include_directories(win_audio_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

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

# ---------------------------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------------------------
add_executable(WinAudioMixer
    src/main.cpp
)

target_link_libraries(WinAudioMixer PRIVATE win_audio_lib)

# config.ini nach jedem Build neben die .exe kopieren
set(WINAUDIO_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config.ini)

if (EXISTS ${WINAUDIO_CONFIG_FILE})
    add_custom_command(TARGET WinAudioMixer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WINAUDIO_CONFIG_FILE}
            $<TARGET_FILE_DIR:WinAudioMixer>/config.ini
    )
else()
    message(WARNING "config.ini nicht gefunden unter ${WINAUDIO_CONFIG_FILE}, wird nicht kopiert.")
endif()

# MinGW/GCC unter Windows: Runtime ebenfalls statisch linken
if (WINAUDIO_SINGLE_EXE AND MINGW)
    target_link_options(WinAudioMixer PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# ---------------------------------------------------------------------------
# Hilfsprogramme und Tests
# ---------------------------------------------------------------------------
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
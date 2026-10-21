#include "controller/SliderController.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include "util/Logger.h"

namespace winaudiomixer {
namespace {

std::string trim(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string toLower(const std::string& value)
{
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lowered;
}

bool isUnassignedProgram(const std::string& program)
{
    return toLower(trim(program)) == "unassigned";
}

// Alle Programme, die explizit (also nicht ueber "unassigned") einem Slider zugeordnet sind
std::set<std::string> collectExplicitlyAssigned(const std::vector<std::vector<std::string>>& assignments)
{
    std::set<std::string> explicitlyAssigned;
    for (const auto& programs : assignments) {
        for (const std::string& program : programs) {
            if (!isUnassignedProgram(program)) {
                explicitlyAssigned.insert(toLower(trim(program)));
            }
        }
    }
    return explicitlyAssigned;
}

} // namespace

SliderController::SliderController(std::size_t sliderCount)
    : sliderCount_(sliderCount), deadZone_(5), lastAppliedValues_(sliderCount, 0), sliderAssignments_(sliderCount)
{
}

void SliderController::setDeadZone(int deadZone)
{
    deadZone_ = deadZone < 0 ? 0 : deadZone;
}

void SliderController::setMapping(int sliderIndex, const std::vector<std::string>& programs)
{
    if (sliderIndex < 0 || static_cast<std::size_t>(sliderIndex) >= sliderCount_) {
        return;
    }

    sliderAssignments_[static_cast<std::size_t>(sliderIndex)] = programs;

    std::string conProgramms;

    for (const auto& program : programs) {
        if (!conProgramms.empty())
            conProgramms += " ";

        conProgramms += program;
    }

    winaudiomixer::Logger::info("Slider " + std::to_string(sliderIndex) + " mapping set to: " + conProgramms);
}

void SliderController::setLastAppliedValue(int sliderIndex, int value)
{
    if (sliderIndex < 0 || static_cast<std::size_t>(sliderIndex) >= sliderCount_) {
        return;
    }

    lastAppliedValues_[static_cast<std::size_t>(sliderIndex)] = value;
}

bool SliderController::shouldApplyChange(int sliderIndex, int newValue) const
{
    if (sliderIndex < 0 || static_cast<std::size_t>(sliderIndex) >= sliderCount_) {
        return false;
    }

    const int lastValue = lastAppliedValues_[static_cast<std::size_t>(sliderIndex)];
    const int delta = std::abs(newValue - lastValue);
    return delta > deadZone_;
}

std::vector<ApplicationVolumeChange> SliderController::process(
    const std::vector<int>& filteredValues,
    const IAudioMixer& audioMixer)
{
    std::vector<ApplicationVolumeChange> changes;
    if (filteredValues.empty()) {
        return changes;
    }

    const std::vector<std::string> knownApplications = audioMixer.getAvailableApplications();

    const std::set<std::string> explicitlyAssigned = collectExplicitlyAssigned(sliderAssignments_);

    for (std::size_t sliderIndex = 0; sliderIndex < sliderCount_ && sliderIndex < filteredValues.size(); ++sliderIndex) {
        if (!shouldApplyChange(static_cast<int>(sliderIndex), filteredValues[sliderIndex])) {
            continue;
        }

        std::vector<std::string> applications;
        const std::vector<std::string>& currentMapping = sliderAssignments_[sliderIndex];
        if (currentMapping.empty()) {
            continue;
        }

        for (const std::string& program : currentMapping) {
            const std::string normalized = trim(program);
            if (normalized.empty()) {
                continue;
            }

            if (isUnassignedProgram(normalized)) {
                for (const std::string& known : knownApplications) {
                    const std::string normalizedKnown = toLower(trim(known));
                    if (normalizedKnown.empty()) {
                        continue;
                    }

                    if (explicitlyAssigned.find(normalizedKnown) != explicitlyAssigned.end()) {
                        continue;
                    }

                    if (std::find(applications.begin(), applications.end(), known) == applications.end()) {
                        applications.push_back(known);
                    }
                }
            } else {
                const std::string lowered = toLower(normalized);
                if (std::find_if(knownApplications.begin(), knownApplications.end(), [&](const std::string& value) {
                        return toLower(trim(value)) == lowered;
                    }) != knownApplications.end()) {
                    if (std::find(applications.begin(), applications.end(), normalized) == applications.end()) {
                        applications.push_back(normalized);
                    }
                }
            }
        }

        const float volume = sliderValueToVolume(filteredValues[sliderIndex]);
        for (const std::string& application : applications) {
            changes.push_back({application, volume});
        }

        lastAppliedValues_[sliderIndex] = filteredValues[sliderIndex];
    }

    return changes;
}

std::vector<ApplicationVolumeChange> SliderController::processNewApplications(
    const std::vector<std::string>& newApplications,
    const std::vector<int>& filteredValues) const
{
    std::vector<ApplicationVolumeChange> changes;
    if (newApplications.empty() || filteredValues.empty()) {
        return changes;
    }

    const std::set<std::string> explicitlyAssigned = collectExplicitlyAssigned(sliderAssignments_);

    for (const std::string& application : newApplications) {
        const std::string loweredApplication = toLower(trim(application));
        if (loweredApplication.empty()) {
            continue;
        }

        const bool isExplicit = explicitlyAssigned.find(loweredApplication) != explicitlyAssigned.end();

        for (std::size_t sliderIndex = 0; sliderIndex < sliderCount_ && sliderIndex < filteredValues.size(); ++sliderIndex) {
            bool matches = false;

            for (const std::string& program : sliderAssignments_[sliderIndex]) {
                const std::string normalized = trim(program);
                if (normalized.empty()) {
                    continue;
                }

                if (isUnassignedProgram(normalized)) {
                    // "unassigned" gilt nur fuer Programme, die nirgends explizit zugeordnet sind
                    if (!isExplicit) {
                        matches = true;
                    }
                } else if (toLower(normalized) == loweredApplication) {
                    matches = true;
                }

                if (matches) {
                    break;
                }
            }

            if (matches) {
                changes.push_back({application, sliderValueToVolume(filteredValues[sliderIndex])});
            }
        }
    }

    return changes;
}

float SliderController::sliderValueToVolume(int sliderValue)
{
    const float normalized = std::clamp(static_cast<float>(sliderValue) / 1023.0f, 0.0f, 1.0f);
    return normalized;
}

} // namespace winaudiomixer

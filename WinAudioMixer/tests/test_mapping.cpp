#include "controller/SliderController.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{

    class TestAudioMixer : public winaudiomixer::IAudioMixer
    {
    public:
        explicit TestAudioMixer(std::vector<std::string> applications)
            : applications_(std::move(applications))
        {
        }

        bool setApplicationVolume(const std::string &, float) override
        {
            return true;
        }

        std::vector<std::string> getAvailableApplications() const override
        {
            return applications_;
        }

    private:
        std::vector<std::string> applications_;
    };

} // namespace

void run_mapping_test()
{
    winaudiomixer::SliderController controller(3);
    controller.setMapping(0, {"Spotify"});
    controller.setMapping(1, {"Chrome", "Discord"});
    controller.setMapping(2, {"unassigned"});

    controller.setLastAppliedValue(0, 0);
    controller.setLastAppliedValue(1, 0);
    controller.setLastAppliedValue(2, 0);

    const TestAudioMixer audioMixer({"Spotify", "Chrome", "Discord", "Teams"});
    const auto changes = controller.process({512, 700, 1000}, audioMixer);

    assert(changes.size() == 4);
    assert(changes[0].application == "Spotify");
    assert(changes[0].volume > 0.0f);
    assert(changes[0].volume <= 1.0f);
    assert(changes[3].application == "Teams");
}

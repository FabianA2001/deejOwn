#include "controller/SliderController.h"

#include <cassert>

void run_deadzone_test()
{
    winaudiomixer::SliderController controller(5);
    controller.setDeadZone(5);
    controller.setLastAppliedValue(0, 500);

    assert(controller.shouldApplyChange(0, 500) == false);
    assert(controller.shouldApplyChange(0, 505) == false);
    assert(controller.shouldApplyChange(0, 506) == true);
    assert(controller.shouldApplyChange(0, 494) == true);
}

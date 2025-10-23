#pragma once

#include "pwm/pwm_device.h"
#include <platform/CHIPDeviceLayer.h>

#include <map>


class PhysicalEmulator
{
public:

    explicit PhysicalEmulator(const pwm_dt_spec *spec);

    CHIP_ERROR Init();
    CHIP_ERROR MoveTo(uint16_t position, uint16_t speed);
    CHIP_ERROR Stop();

private:
    // Timer
    static void TimerTimeoutCallback(chip::System::Layer *systemLayer, void *appState);
    void HandleTimer();
    Nrf::PWMDevice mPhysicalIndicator;
    const pwm_dt_spec *mSpec;
    bool mLatched = false;
    uint16_t mSpeed;
    uint16_t mCurrentPosition;
    uint16_t mTargetPosition;
};

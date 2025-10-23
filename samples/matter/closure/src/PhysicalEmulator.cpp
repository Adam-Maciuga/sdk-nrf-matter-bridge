#include "PhysicalEmulator.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <zephyr/logging/log.h>
#include "ClosureManager.h"

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::DeviceLayer;

static constexpr uint32_t kMoveIntervalMs = 100;

PhysicalEmulator::PhysicalEmulator(const pwm_dt_spec *spec) : mSpec(spec){}

CHIP_ERROR PhysicalEmulator::Init()
{
    if (mPhysicalIndicator.Init(mSpec, 0, 255) != 0) {
        LOG_ERR("Cannot initialize the physical indicator");
        return CHIP_ERROR_INCORRECT_STATE;
    }
    LOG_INF("PhysicalEmulator initialized");
    return CHIP_NO_ERROR;
}

CHIP_ERROR PhysicalEmulator::MoveTo(uint16_t position, uint16_t speed)
{
    LOG_INF("Starting movement");
    mTargetPosition = position;
    mSpeed = speed;

    // Start periodic timer
    (void)SystemLayer().StartTimer(System::Clock::Milliseconds32(kMoveIntervalMs),
                                   TimerTimeoutCallback, this);
    return CHIP_NO_ERROR;
}

CHIP_ERROR PhysicalEmulator::Stop()
{
    SystemLayer().CancelTimer(TimerTimeoutCallback, this);
    LOG_INF("Movement stopped");
    ClosureManager::GetInstance().HandleClosureCallback(chip::app::Clusters::ClosureControl::MainStateEnum::kStopped,mCurrentPosition);
    return CHIP_NO_ERROR;
}

void PhysicalEmulator::TimerTimeoutCallback(System::Layer *systemLayer, void *appState)
{
    auto *self = static_cast<PhysicalEmulator *>(appState);
    VerifyOrReturn(self != nullptr);
    self->HandleTimer();
}

void PhysicalEmulator::HandleTimer()
{
    LOG_DBG("Timer tick - updating position or PWM");
    bool finished = false;
    // Example PWM update
    uint32_t move_per_tick_32 = (static_cast<uint32_t>(mSpeed) * kMoveIntervalMs) / 1000U;
    uint16_t move_per_tick = static_cast<uint16_t>(std::min<uint32_t>(move_per_tick_32, UINT16_MAX));
    if (mCurrentPosition <= mTargetPosition){
        if (mCurrentPosition + move_per_tick >= mTargetPosition){
            finished = true;
            mCurrentPosition = mTargetPosition;
        }
        else{
            mCurrentPosition += move_per_tick;
        }
    }else{
        if(mTargetPosition + move_per_tick >= mCurrentPosition ){
            finished = true;
            mCurrentPosition = mTargetPosition;
        }
        else{
            mCurrentPosition -= move_per_tick;
        }
    }

    uint8_t brightness = static_cast<uint8_t>(
        (static_cast<uint32_t>(mCurrentPosition) * 255U + 5000U) / 10000U
    );

    mPhysicalIndicator.InitiateAction(Nrf::PWMDevice::LEVEL_ACTION, 0, &brightness);

    if(finished){
        Stop();
    }else{
        (void)SystemLayer().StartTimer(System::Clock::Milliseconds32(kMoveIntervalMs),
                                   TimerTimeoutCallback, this);
    }
}

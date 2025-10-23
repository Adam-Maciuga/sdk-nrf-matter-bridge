/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "ClosureManager.h"
#include "ClosureControlEndpoint.h"

#include <app-common/zap-generated/cluster-objects.h>
#include <app/util/attribute-storage.h>
#include <platform/CHIPDeviceLayer.h>
#include <dk_buttons_and_leds.h>

#include <app-common/zap-generated/attributes/Accessors.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <map>

LOG_MODULE_DECLARE(closure_manager, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app;
using namespace chip::DeviceLayer;
using namespace chip::app::Clusters::ClosureControl;
using namespace chip::app::Clusters::Globals;

static const struct pwm_dt_spec sPhysicalIndicatorPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

const static std::map<TargetPositionEnum, uint16_t> sPositionMap = {
    {TargetPositionEnum::kMoveToFullyClosed,10000},
    {TargetPositionEnum::kMoveToFullyOpen,0},
    {TargetPositionEnum::kMoveToPedestrianPosition,0},
    {TargetPositionEnum::kMoveToSignaturePosition, 0},
    {TargetPositionEnum::kMoveToVentilationPosition,7500},
};

const static std::map<CurrentPositionEnum, uint16_t> sPositionMapCurr = {
    {CurrentPositionEnum::kFullyClosed,10000},
    {CurrentPositionEnum::kFullyOpened,0},
    {CurrentPositionEnum::kOpenedForPedestrian,0},
    {CurrentPositionEnum::kOpenedAtSignature, 0},
    {CurrentPositionEnum::kOpenedForVentilation,7500},
};

const static std::map<Clusters::Globals::ThreeLevelAutoEnum,uint16_t> sSpeedMap = {
    {ThreeLevelAutoEnum::kLow, 1000},
    {ThreeLevelAutoEnum::kMedium, 2500},
    {ThreeLevelAutoEnum::kHigh, 5000},
    {ThreeLevelAutoEnum::kAuto, 2500},
};

namespace {
constexpr uint32_t kCountdownTimeSeconds = 10;

// Define the Namespace and Tag for the endpoint
constexpr uint8_t kNamespaceClosure   = 0x44;
constexpr uint8_t kTagClosureGarageDoor = 0x05;

// Define the list of semantic tags for the endpoint
const Clusters::Descriptor::Structs::SemanticTagStruct::Type kClosureControlEndpointTagList[] = {
    { .namespaceID = kNamespaceClosure,
      .tag         = kTagClosureGarageDoor,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("Closure.GarageDoor"_span)) }
};

} // namespace

CurrentPositionEnum ExactPos2Enum(uint16_t currPositionExact){

    for (const auto &entry : sPositionMapCurr)
    {
        const auto &positionEnum = entry.first;
        const uint16_t positionValue = entry.second;

        if (currPositionExact == positionValue)
        {
            return positionEnum;
        }
    }
    return CurrentPositionEnum::kPartiallyOpened;
}

ClosureManager ClosureManager::sClosureMgr;

ClosureManager::ClosureManager()
    : mPhysicalEmulator(&sPhysicalIndicatorPwmDevice)
{
    mMainState = MainStateEnum::kStopped;
    mCurrentState.position = chip::MakeOptional(CurrentPositionEnum::kFullyOpened);
    mCurrentState.speed = chip::MakeOptional(ThreeLevelAutoEnum::kAuto);
    mCurrentState.secureState = chip::MakeOptional(false);
    mTargetState.speed = chip::MakeOptional(ThreeLevelAutoEnum::kAuto);
}

void ClosureManager::Init()
{
    LOG_INF("\nINIT\n");

    mPhysicalEmulator.Init();

    DeviceLayer::PlatformMgr().LockChipStack();

    // Closure endpoints initialization
    ClosureControlEndpoint.Init();

    // Set Taglist for Closure endpoints
    SetTagList(/* endpoint= */ 1, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(kClosureControlEndpointTagList));

    ClosureControlEndpoint.WriteAllAttributes(mMainState,mCurrentState,mTargetState);

    DeviceLayer::PlatformMgr().UnlockChipStack();
}

chip::Protocols::InteractionModel::Status ClosureManager::OnCalibrateCommand()
{
    return chip::Protocols::InteractionModel::Status::UnsupportedCommand;
}

chip::Protocols::InteractionModel::Status ClosureManager::OnStopCommand()
{
    mPhysicalEmulator.Stop();
    return chip::Protocols::InteractionModel::Status::Success;
}
chip::Protocols::InteractionModel::Status
ClosureManager::OnMoveToCommand(const chip::Optional<TargetPositionEnum> position,
                                const chip::Optional<bool> latch,
                                const chip::Optional<ThreeLevelAutoEnum> speed)
{
    if (!position.HasValue()){
        return chip::Protocols::InteractionModel::Status::Failure;
    }
    auto pos = sPositionMap.at(position.Value());
    auto spd = sSpeedMap.at(speed.ValueOr(mCurrentState.speed.Value()));
    mPhysicalEmulator.MoveTo(pos,spd);

    mMainState = MainStateEnum::kMoving;
    mTargetState.position = position;
    mTargetState.speed = speed;

    ClosureControlEndpoint.WriteAllAttributes(mMainState,mCurrentState,mTargetState);

    return chip::Protocols::InteractionModel::Status::Success;
}

void ClosureManager::HandleClosureCallback(MainStateEnum newState, uint16_t currPositionExact)
{
    switch (newState)
    {
    case MainStateEnum::kStopped: {
        mCurrentState.position = MakeOptional(ExactPos2Enum(currPositionExact));
        mCurrentState.speed = mTargetState.speed;
        mMainState = MainStateEnum::kStopped;
        mTargetState.position.ClearValue();
    }
    default:
        ChipLogError(AppServer, "Invalid action received in HandleClosureCallback");
        break;
    }
    ClosureControlEndpoint.WriteAllAttributes(mMainState,mCurrentState,mTargetState);
}

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

#include "ClosureControlEndpoint.h"
#include "ClosureManager.h"
#include <app-common/zap-generated/cluster-enums.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <protocols/interaction_model/StatusCode.h>
#include <app-common/zap-generated/attributes/Accessors.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(closure_cntrl, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app::Clusters::ClosureControl;

using Protocols::InteractionModel::Status;

namespace {

constexpr ElapsedS kDefaultCountdownTime = 30;

} // namespace

Status ClosureControlDelegate::HandleCalibrateCommand()
{
    return ClosureManager::GetInstance().OnCalibrateCommand();
}

Status ClosureControlDelegate::HandleMoveToCommand(const Optional<TargetPositionEnum> & position, const Optional<bool> & latch,
                                                   const Optional<Globals::ThreeLevelAutoEnum> & speed)
{
    return ClosureManager::GetInstance().OnMoveToCommand(position, latch, speed);
}

Status ClosureControlDelegate::HandleStopCommand()
{
    return ClosureManager::GetInstance().OnStopCommand();
}

CHIP_ERROR ClosureControlDelegate::GetCurrentErrorAtIndex(size_t index, ClosureErrorEnum & closureError)
{
    // This function should return the current error at the specified index.
    // For now, we dont have a ErrorList implemented, so will return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED.
    return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
}

bool ClosureControlDelegate::IsReadyToMove()
{
    // This function should return true if the closure is ready to move.
    // For now, we will return true.
    return true;
}

bool ClosureControlDelegate::IsManualLatchingNeeded()
{
    // This function should return true if manual latching is needed.
    // For now, we will return false.
    return false;
}

ElapsedS ClosureControlDelegate::GetCalibrationCountdownTime()
{
    // This function should return the calibration countdown time.
    // For now, we will return kDefaultCountdownTime.
    return kDefaultCountdownTime;
}

ElapsedS ClosureControlDelegate::GetMovingCountdownTime()
{
    // This function should return the moving countdown time.
    // For now, we will return kDefaultCountdownTime.
    return kDefaultCountdownTime;
}

ElapsedS ClosureControlDelegate::GetWaitingForMotionCountdownTime()
{
    // This function should return the waiting for motion countdown time.
    // For now, we will return kDefaultCountdownTime.
    return kDefaultCountdownTime;
}

CHIP_ERROR ClosureControlEndpoint::Init()
{
    ClusterConformance conformance;
    conformance.FeatureMap()
        .Set(Feature::kPositioning)
        .Set(Feature::kSpeed)
        .Set(Feature::kVentilation);
    conformance.OptionalAttributes().Set(OptionalAttributeEnum::kCountdownTime);

    ClusterInitParameters initParams;

    ReturnErrorOnFailure(mLogic.Init(conformance, initParams));
    ReturnErrorOnFailure(mInterface.Init());

    return CHIP_NO_ERROR;
}

void ClosureControlEndpoint::WriteAllAttributes(
    const MainStateEnum& mainState,
    const GenericOverallCurrentState& currentState,
    const GenericOverallTargetState& targetState)
{
    DeviceLayer::PlatformMgr().LockChipStack();
    CHIP_ERROR err = mLogic.SetMainState(mainState);
    if (err != CHIP_NO_ERROR){
        LOG_INF("\n\nThe main state set was not successful: %s\n\n",chip::ErrorStr(err));
    }
    err = mLogic.SetOverallCurrentState(currentState);
    if (err != CHIP_NO_ERROR){
        LOG_INF("\n\nThe current state set was not successful: %s\n\n",chip::ErrorStr(err));
    }
    err = mLogic.SetOverallTargetState(targetState);
    if (err != CHIP_NO_ERROR){
        LOG_INF("\n\nThe target state set was not successful: %s\n\n",chip::ErrorStr(err));
    }
    DeviceLayer::PlatformMgr().UnlockChipStack();
}

void ClosureControlEndpoint::OnStopCalibrateComplete()
{
    LOG_INF("\nOnStopCalibrateActionComplete\n");
}

void ClosureControlEndpoint::OnStopMotionComplete()
{
    LOG_INF("\nOnStopMotionActionComplete\n");
}

void ClosureControlEndpoint::OnCalibrateComplete()
{
    LOG_INF("\nOnCalibrateActionComplete\n");
}

void ClosureControlEndpoint::OnMoveToComplete()
{
    LOG_INF("\nOnMoveToActionComplete\n");
}

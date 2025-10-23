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

/*
 * @class ClosureManager
 * @brief Manages the initialization and operations related to closure and
 *        closure panel endpoints in the application.
 *
 * @note This class is part of the closure application example
 */

#pragma once

#include "ClosureControlEndpoint.h"
#include <lib/core/DataModelTypes.h>
#include <zephyr/kernel.h>
#include "board/board.h"
#include "PhysicalEmulator.h"

class ClosureManager
{
public:


    ClosureManager();
    /**
     * @brief Initializes the ClosureManager and its associated resources.
     *
     * This method performs the following actions:
     * - Initializes closure endpoints (ClosureControlEndpoint).
     * - Initializes Physical Emulator
     * - Sets the semantic tag lists for each closure endpoint.
     */
    void Init();

    /**
     * @brief Returns the singleton instance of the ClosureManager.
     *
     * This static method provides access to the single, global instance of the ClosureManager,
     * ensuring that only one instance exists throughout the application's lifetime.
     *
     * @return Reference to the singleton ClosureManager instance.
     */
    static ClosureManager & GetInstance() { return sClosureMgr; }

    /**
     * @brief Handles the calibration command for the closure.
     *
     * This method initiates the calibration process by setting a countdown timer.
     * It posts a calibration action event to the application task and marks
     * the calibration action as in progress.
     *
     * @return chip::Protocols::InteractionModel::Status
     *         Returns Status::Success if all operations succeed, otherwise Status::Failure.
     */
    chip::Protocols::InteractionModel::Status OnCalibrateCommand();

    /**
     * @brief Handles the MoveTo command for the Closure.
     *
     * This method processes the MoveTo command, which is used to initiate a motion action
     * for a closure.
     *
     * @param position Optional target position for the closure device.
     * @param latch Optional flag indicating whether the closure should latch after moving.
     * @param speed Optional speed setting for the movement, represented as a ThreeLevelAutoEnum.
     * @return chip::Protocols::InteractionModel::Status Status of the command handling operation.
     */
    chip::Protocols::InteractionModel::Status
    OnMoveToCommand(const chip::Optional<chip::app::Clusters::ClosureControl::TargetPositionEnum> position,
                    const chip::Optional<bool> latch, const chip::Optional<chip::app::Clusters::Globals::ThreeLevelAutoEnum> speed);

    /**
     * @brief Handles the Stop command for the Closure.
     *
     * This method processes the Stop command, which is used to stop an action for a closure.
     *
     * @return chip::Protocols::InteractionModel::Status
     *         Returns Status::Success if the Stop command is handled successfully,
     *         or an appropriate error status otherwise.
     */
    chip::Protocols::InteractionModel::Status OnStopCommand();

    void HandleClosureCallback(chip::app::Clusters::ClosureControl::MainStateEnum newState, uint16_t currPositionExact);

private:
    PhysicalEmulator mPhysicalEmulator;
    static ClosureManager sClosureMgr;
    chip::app::Clusters::ClosureControl::MainStateEnum mMainState;
    chip::app::Clusters::ClosureControl::GenericOverallCurrentState mCurrentState;
    chip::app::Clusters::ClosureControl::GenericOverallTargetState mTargetState;

    // Define the endpoint ID for the Closure
    static constexpr chip::EndpointId kClosureEndpoint       = 1;

    chip::app::Clusters::ClosureControl::ClosureControlEndpoint ClosureControlEndpoint{ kClosureEndpoint };
};

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once
#include <cstdint>

class IPhysicalDeviceObserver {
public:
	virtual void OnMovementStopped(uint16_t currentPosition) = 0;
	virtual void OnMovementUpdate(uint16_t currentPosition, uint16_t timeLeft, bool justStarted = false) = 0;
	virtual ~IPhysicalDeviceObserver() = default;
};

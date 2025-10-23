/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once
#include <lib/core/CHIPError.h>

#include "physical_device_observer.h"

class IPhysicalDevice {
public:
	virtual ~IPhysicalDevice() = default;

	IPhysicalDevice() = default;
	IPhysicalDevice(const IPhysicalDevice &) = delete;
	IPhysicalDevice &operator=(const IPhysicalDevice &) = delete;

	void SetObserver(IPhysicalDeviceObserver *observer) { mObserver = observer; }
	virtual CHIP_ERROR Init() = 0;
	virtual CHIP_ERROR MoveTo(uint16_t position, uint16_t speed) = 0;
	virtual CHIP_ERROR Stop() = 0;

protected:
	IPhysicalDeviceObserver *mObserver = nullptr;
};

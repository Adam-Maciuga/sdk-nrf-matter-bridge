/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once
#include "closure_manager.h"
#include "physical_emulator.h"

static constexpr chip::EndpointId kClosureEndpoint = 1;

class AppTask {
public:
	AppTask();
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	}

	CHIP_ERROR Init();
	CHIP_ERROR StartApp();

private:
	PhysicalEmulator mPhysicalDevice;
	ClosureManager mClosureManager;
};

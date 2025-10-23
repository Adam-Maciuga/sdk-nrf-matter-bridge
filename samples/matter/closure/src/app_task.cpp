#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "lib/core/CHIPError.h"
#include "lib/support/CodeUtils.h"
#include "ClosureControlEndpoint.h"
#include "ClosureManager.h"

#include <setup_payload/OnboardingCodesUtil.h>
#include <app/util/attribute-storage.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app/data-model/Nullable.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;
using namespace chip::app::Clusters::Descriptor;

CHIP_ERROR AppTask::Init()
{
    ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

    if (!Nrf::GetBoard().Init()) {
        LOG_ERR("User interface initialization failed.");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

    ReturnErrorOnFailure(Nrf::Matter::StartServer());

    ClosureManager::GetInstance().Init();

    return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
    ReturnErrorOnFailure(Init());

    while (true) {
        Nrf::DispatchNextTask();
    }

    return CHIP_NO_ERROR;
}

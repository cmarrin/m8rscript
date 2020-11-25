
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"
#include "Defines.h"
#include "Mallocator.h"
#include "M8rscript.h"
#include "MStream.h"
#include "SystemInterface.h"
#include "SystemTime.h"

static m8r::Duration MainTaskSleepDuration = 10ms;

m8rscript::M8rscriptScriptingLanguage m8rscriptScriptingLanguage;

extern "C" void app_main()
{
    m8r::Application application(m8r::Application::HeartbeatType::Status, "/sys/bin", 23);
    m8r::system()->registerScriptingLanguage(&m8rscriptScriptingLanguage);

    while(1) {
        application.runOneIteration();
        MainTaskSleepDuration.sleep();
    }
}

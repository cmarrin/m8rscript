
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"
#include "M8rscript.h"

static constexpr const char* WebServerRoot = "/sys/bin";
static m8r::Duration MainTaskSleepDuration = 10ms;

m8r::Vector<const char*> fileList = {
    "scripts/mem.m8r",
    "scripts/mrsh.m8r",
    "scripts/examples/NTPClient.m8r",
    "scripts/examples/NTPClient.m8r",
    "scripts/examples/TimeZoneDBClient.m8r",
    "scripts/simple/basic.m8r",
    "scripts/simple/blink.m8r",
    "scripts/simple/hello.m8r",
    "scripts/simple/simpleFunction.m8r",
    "scripts/simple/simpleTest.m8r",
    "scripts/simple/simpleTest2.m8r",
    "scripts/timing/timing-esp.m8r",
    "scripts/timing/timing.m8r",
    "scripts/tests/TestBase64.m8r",
    "scripts/tests/TestClass.m8r",
    "scripts/tests/TestClosure.m8r",
    "scripts/tests/TestGibberish.m8r",
    "scripts/tests/TestIterator.m8r",
    "scripts/tests/TestLoop.m8r",
    "scripts/tests/TestTCPSocket.m8r",
    "scripts/tests/TestUDPSocket.m8r",
};

m8rscript::M8rscriptScriptingLanguage m8rscriptScriptingLanguage;

void m8rmain()
{
    m8r::Application application(m8r::Application::HeartbeatType::Status, WebServerRoot, 23);

     // Upload files needed by web server
    m8r::Application::uploadFiles(fileList, WebServerRoot);

    m8r::system()->registerScriptingLanguage(&m8rscriptScriptingLanguage);

    application.runAutostartTask("/sys/bin/hello.m8r");

    while(1) {
        application.runOneIteration();
        MainTaskSleepDuration.sleep();
    }
}

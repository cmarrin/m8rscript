/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"

#include "HTTPServer.h"
#include "Marly.h"
#include "MFS.h"
#include "Shell.h"
#include "StringStream.h"
#include "SystemInterface.h"
#include "Telnet.h"
#include <unistd.h>

#ifdef MONITOR_TRAFFIC
#include <string>
#endif
using namespace m8r;

//#define RUN_SAMPLE
//#define ENABLE_WEBSERVER
//#define ENABLE_HEARTBEAT
//#define ENABLE_SHELL

#ifdef RUN_SAMPLE
class Sample : public Executable
{
public:
    virtual CallReturnValue execute() override
    {
        print("***** Hello Native World!!!\n");
        return CallReturnValue(CallReturnValue::Type::Finished);
    }
};
#endif

SystemInterface* Application::_system = nullptr;

Application::Application(uint16_t port)
{
    init(port);
}

void Application::init(uint16_t port)
{
    // Seed the random number generator
    srand(static_cast<unsigned>(Time::now().us()));

    assert(!_system);
    _system = SystemInterface::create();
    
#ifdef ENABLE_HEARTBEAT
    system()->setHeartrate(1s);
#endif
    system()->init();

#ifdef ENABLE_HEARTBEAT
    system()->setHeartrate(3s);
#endif
    
#ifdef ENABLE_WEBSERVER
    // Setup test web server
    _webServer = std::make_unique<HTTPServer>(80, "/sys/bin");
    _webServer->on("/", "index.html");
    _webServer->on("/favicon.ico", "favicon.ico");
#endif
#ifdef ENABLE_WEBSERVER
    _terminal = std::make_unique<Terminal>(port, [this]()
    {
        std::shared_ptr<Task> task = std::make_shared<Task>();
#if M8RSCRIPT_SUPPORT == 1
        task->load(shellName());
#else
        task->load(std::make_shared<Shell>());
#endif
        return task;
    });
#endif
    mountFileSystem();

    // Start things running
    system()->printf(ROMSTR("\n*** m8rscript v%d.%d - %s\n"), MajorVersion, MinorVersion, __TIMESTAMP__);
    system()->printf(ROMSTR("Free heap: %d\n\n"), m8r::Mallocator::shared()->freeSize());

    if (m8r::system()->fileSystem() && m8r::system()->fileSystem()->mounted()) {
        uint32_t totalSize = m8r::system()->fileSystem()->totalSize();
        uint32_t totalUsed = m8r::system()->fileSystem()->totalUsed();
        m8r::system()->printf(ROMSTR("Filesystem - total size:%sB, used:%sB\n"), String::prettySize(totalSize, 1, true).c_str(), String::prettySize(totalUsed, 1, true).c_str());
    }
}

void Application::runAutostartTask()
{
    _autostartTask = std::make_shared<Task>();
    _autostartTask->setConsolePrintFunction([](const String& s) {
        system()->printf(ROMSTR("%s"), s.c_str());
    });
    
    system()->setListenerFunc([this](const char* line) {
        if (!line) {
            return;
        }
        size_t size = strlen(line);
        if (line[size - 1] == '\n') {
            size -= 1;
        }
        _autostartTask->receivedData(String(line, static_cast<uint32_t>(size)), KeyAction::None);
    });
    
#ifdef RUN_SAMPLE
    bool result = _autostartTask->load(std::make_shared<Sample>());
#else
    bool result = _autostartTask->load("/sys/bin/timing.m8r");
#endif

    system()->taskManager()->run(_autostartTask, [this, result](m8r::Task*) {
        m8r::system()->printf(ROMSTR("******* autostart task completed. Result=%d\n"), result);
        _autostartTask.reset();
    });  
}

Application::~Application()
{
    delete _system;
}

Application::NameValidationType Application::validateBonjourName(const char* name)
{
    if (!name || name[0] == '\0') {
        return NameValidationType::BadLength;
    }
    
    for (size_t i = 0; name[i]; i++) {
        if (i >= 31) {
            return NameValidationType::BadLength;
        }
        
        char c = name[i];
        if (c == '-' ||
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z')) {
            continue;
        }
        return NameValidationType::InvalidChar;
    }
    return NameValidationType::Ok;
}

bool Application::mountFileSystem()
{
    if (!system()->fileSystem()) {
        return false;
    }
    if (!system()->fileSystem()->mount()) {
        if (system()->fileSystem()->lastError().code() == Error::Code::FSNotFormatted) {
            m8r::system()->printf(ROMSTR("Filessytem not present, formatting...\n"));
        } else {
            system()->print(Error::formatError(system()->fileSystem()->lastError().code(), ROMSTR("Filesystem mount failed")).c_str());
            return false;
        }
        if (m8r::system()->fileSystem()->format()) {
            m8r::system()->printf(ROMSTR("succeeded.\n"));
        } else {
            m8r::system()->printf(ROMSTR("FAILED.\n"));
        }
    }
    return true;
}

bool Application::runOneIteration()
{
    if (_terminal) {
        _terminal->handleEvents();
    }
    if (_webServer) {
        _webServer->handleEvents();
    }
    return system()->runOneIteration();
}

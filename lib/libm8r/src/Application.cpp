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

#define ENABLE_WEBSERVER
#define ENABLE_HEARTBEAT
#define ENABLE_SHELL

class Sample : public Executable
{
public:
    virtual CallReturnValue execute() override
    {
        print("***** Hello Native World!!!\n");
        return CallReturnValue(CallReturnValue::Type::Finished);
    }
};

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
        SharedPtr<Task> task(new Task());
        task->load(SharedPtr<Shell>(new Shell()));
        return task;
    });
#endif
    mountFileSystem();

    // Start things running
    system()->printf("\n*** m8rscript v%d.%d - %s\n", MajorVersion, MinorVersion, __TIMESTAMP__);
    system()->printf("Free heap: %d\n\n", m8r::Mallocator::shared()->freeSize());

    if (m8r::system()->fileSystem() && m8r::system()->fileSystem()->mounted()) {
        uint32_t totalSize = m8r::system()->fileSystem()->totalSize();
        uint32_t totalUsed = m8r::system()->fileSystem()->totalUsed();
        m8r::system()->printf("Filesystem - total size:%sB, used:%sB\n", String::prettySize(totalSize, 1, true).c_str(), String::prettySize(totalUsed, 1, true).c_str());
    }
}

void Application::runAutostartTask()
{
    _autostartTask = SharedPtr<Task>(new Task());
    _autostartTask->setConsolePrintFunction([](const String& s) {
        system()->printf("%s", s.c_str());
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

    if (!_autostartTask->load("/sys/bin/hello.m8r")) {
        if (!_autostartTask->load("/sys/bin/hello.marly")) {
            _autostartTask->load(SharedPtr<Sample>(new Sample()));
        }
    }
    
    system()->taskManager()->run(_autostartTask, [this](m8r::Task*) {
        m8r::system()->printf("******* autostart task completed.\n");
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
            m8r::system()->printf("Filessytem not present, formatting...\n");
        } else {
            system()->print(Error::formatError(system()->fileSystem()->lastError().code(), "Filesystem mount failed").c_str());
            return false;
        }
        if (m8r::system()->fileSystem()->format()) {
            m8r::system()->printf("succeeded.\n");
        } else {
            m8r::system()->printf("FAILED.\n");
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

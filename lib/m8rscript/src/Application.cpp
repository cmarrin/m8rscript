/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"

#include "SystemInterface.h"
#include "Telnet.h"
#include <unistd.h>

#ifdef MONITOR_TRAFFIC
#include <string>
#endif
using namespace m8r;

SystemInterface* Application::_system = nullptr;


Application::Application(uint16_t port)
{
    // Seed the random number generator
    srand(static_cast<unsigned>(Time::now().us()));

    assert(!_system);
    _system = SystemInterface::create();
    
    system()->setHeartrate(1s);
    
    system()->init();

    _terminal = std::make_unique<Terminal>(port, shellName());

    mountFileSystem();

    // Start things running
    system()->printf(ROMSTR("\n*** m8rscript v%d.%d - %s\n"), MajorVersion, MinorVersion, __TIMESTAMP__);
    system()->printf(ROMSTR("Free heap: %d\n\n"), m8r::Mallocator::shared()->freeSize());

    if (m8r::system()->fileSystem() && m8r::system()->fileSystem()->mounted()) {
        uint32_t totalSize = m8r::system()->fileSystem()->totalSize();
        uint32_t totalUsed = m8r::system()->fileSystem()->totalUsed();
        m8r::system()->printf(ROMSTR("Filesystem - total size:%sB, used:%sB\n"), String::prettySize(totalSize, 1, true).c_str(), String::prettySize(totalUsed, 1, true).c_str());
    }
    
    // If autostart is on, run the main program
    String filename = autostartFilename();
    if (filename) {
        _autostartTask = Task::create();
        _autostartTask->load(filename.c_str());
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
        system()->taskManager()->run(_autostartTask, [this](m8r::TaskBase*) {
            m8r::system()->printf(ROMSTR("******* autostart task completed\n"));
            _autostartTask.reset();
        });    
    }
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

m8r::String Application::autostartFilename() const
{
    // Look for it in config first
    return "/sys/bin/hello.m8r";
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

void Application::runOneIteration()
{
    _terminal->handleEvents();
    system()->runOneIteration();
}

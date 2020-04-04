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
    printf("***** creating Application\n");

    // Seed the random number generator
    srand(static_cast<unsigned>(Time::now().us()));

    if (!_system) {
        _system = SystemInterface::create();
    }

    Mad<TCP> socket = system()->createTCP(port, [this](TCP*, TCP::Event event, int16_t connectionId, const char* data, int16_t length)
    {
        switch(event) {
            case TCP::Event::Connected:
                _shells[connectionId].task = Mad<Task>::create();
                
                // Set the print function to send the printed string out the TCP channel
                _shells[connectionId].task->setConsolePrintFunction([&](const String& s) {
                    // Break it up into lines. We need to insert '\r'
                    Vector<String> v = s.split("\n");
                    for (int i = 0; i < v.size(); ++i) {
                        if (!v[i].empty()) {
                            _shellSocket->send(connectionId, v[i].c_str(), v[i].size());
                        }
                        if (i == v.size() - 1) {
                            break;
                        }
                        _shellSocket->send(connectionId, "\r\n", 2);
                    }
                });
                
                _shells[connectionId].task->init(Application::shellName());
                if (_shells[connectionId].task->error().code() != Error::Code::OK) {
                    Error::printError(_shells[connectionId].task->eu(), _shells[connectionId].task->error().code());
                    _shells[connectionId].task = Mad<Task>();
                    _shellSocket->disconnect(connectionId);
                } else {
                    _shellSocket->send(connectionId, _shells[connectionId].telnet.init().c_str());
                    
                    // Run the task
                    _shells[connectionId].task->run([connectionId, this](TaskBase*)
                    {
                        // On return from finished task, drop the connection
                        _shellSocket->disconnect(connectionId);
                        _shells[connectionId].task.destroy();
                        _shells[connectionId].task = Mad<Task>();
                    });
                }
                break;
            case TCP::Event::Disconnected:
                if (_shells[connectionId].task.valid()) {
                    _shells[connectionId].task->terminate();
                    _shells[connectionId].task.destroy();
                    _shells[connectionId].task = Mad<Task>();
                }
                break;
            case TCP::Event::ReceivedData:
                if (_shells[connectionId].task.valid()) {
                    // Receiving characters. Pass them through Telnet
                    String toChannel, toClient;
                    for (int16_t i = 0; i < length; ++i) {
                        if (!data[i]) {
                            break;
                        }
                        KeyAction action = _shells[connectionId].telnet.receive(data[i], toChannel, toClient);
                    
                        if (!toClient.empty() || action != KeyAction::None) {
                            _shells[connectionId].task->receivedData(toClient, action);
                        }
                        if (!toChannel.empty()) {
                            _shellSocket->send(connectionId, toChannel.c_str(), toChannel.size());
                        }
                    }
                }
                break;
            case TCP::Event::SentData:
                break;
            default:
                break;
        }
    });
    
    _shellSocket = socket;

    mountFileSystem();

    // Start things running
    system()->printf(ROMSTR("\n*** m8rscript v%d.%d - %s\n\n"), MajorVersion, MinorVersion, __TIMESTAMP__);
    
    if (m8r::system()->fileSystem() && m8r::system()->fileSystem()->mounted()) {
        uint32_t totalSize = m8r::system()->fileSystem()->totalSize();
        uint32_t totalUsed = m8r::system()->fileSystem()->totalUsed();
        m8r::system()->printf(ROMSTR("Filesystem - total size:%sB, used:%sB\n"), String::prettySize(totalSize, 1).c_str(), String::prettySize(totalUsed, 1).c_str());
    }
    
    // If autostart is on, run the main program
    String filename = autostartFilename();
    if (filename) {
        _autostartTask = Mad<Task>::create();
        _autostartTask->init(filename.c_str());
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
        _autostartTask->run([this](m8r::TaskBase*) {
            m8r::system()->printf(ROMSTR("******* autostart task completed\n"));
            _autostartTask.destroy();
        });    
    }
}

Application::~Application()
{
    _shellSocket.destroy();
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

String Application::autostartFilename() const
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
            system()->printf(ROMSTR("ERROR: Filesystem mount failed: "));
            
            Error::showError(system()->fileSystem()->lastError().code());
            
            system()->printf(ROMSTR("\n"));
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

void Application::runLoop()
{    
    system()->runLoop();
}

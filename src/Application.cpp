/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"

#include "FS.h"
#include "SystemInterface.h"
#include "Telnet.h"

#ifdef MONITOR_TRAFFIC
#include <string>
#endif
using namespace m8r;

class MyShellSocket : public TCPDelegate {
public:
    MyShellSocket(Application* application, uint16_t port)
        : _tcp(system()->createTCP(this, port))
        , _application(application)
    { }
    
    virtual ~MyShellSocket() { }

    // TCPDelegate
    virtual void TCPevent(m8r::TCP* tcp, m8r::TCPDelegate::Event event, int16_t connectionId, const char* data, int16_t length) override
    {
        switch(event) {
            case m8r::TCPDelegate::Event::Connected:
                _shells[connectionId].task = new Task(Application::shellName());
                if (_shells[connectionId].task->error().code() != Error::Code::OK) {
                    Error::printError(_shells[connectionId].task->error().code());
                    _shells[connectionId].task = nullptr;
                    tcp->disconnect(connectionId);
                } else {
                    // Set the print function to send the printed string out the TCP channel
                    _shells[connectionId].task->setConsolePrintFunction([this, tcp, connectionId](const String& s) {
                        // Break it up into lines. We need to insert '\r'
                        std::vector<String> v = s.split("\n");
                        for (int i = 0; i < v.size(); ++i) {
                            if (!v[i].empty()) {
                                tcp->send(connectionId, v[i].c_str(), v[i].size());
                            }
                            if (i == v.size() - 1) {
                                break;
                            }
                            tcp->send(connectionId, "\r\n", 2);
                        }
                    });
                    
                    tcp->send(connectionId, _shells[connectionId].telnet.init().c_str());
                    
                    // Run the task
                    _shells[connectionId].task->run([tcp, connectionId, this](TaskBase*)
                    {
                        // On return from finished task, drop the connection
                        tcp->disconnect(connectionId);
                        delete _shells[connectionId].task;
                        _shells[connectionId].task = nullptr;
                    });
                }
                break;
            case m8r::TCPDelegate::Event::Disconnected:
                if (_shells[connectionId].task) {
                    _shells[connectionId].task->terminate();
                    _shells[connectionId].task = nullptr;
                }
                break;
            case m8r::TCPDelegate::Event::ReceivedData:
                if (_shells[connectionId].task) {
                    // Receiving characters. Pass them through Telnet
                    String toChannel, toClient;
                    for (int16_t i = 0; i < length; ++i) {
                        if (!data[i]) {
                            break;
                        }
                        Telnet::Action action = _shells[connectionId].telnet.receive(data[i], toChannel, toClient);
                    
                        if (!toClient.empty() || action != Telnet::Action::None) {
                            _shells[connectionId].task->receivedData(toClient, action);
                        }
                        if (!toChannel.empty()) {
                            tcp->send(connectionId, toChannel.c_str(), toChannel.size());
                        }
                    }
                }
                break;
            case m8r::TCPDelegate::Event::SentData:
                break;
            default:
                break;
        }
    }

private:
    std::unique_ptr<m8r::TCP> _tcp;
    Application* _application;
    
    struct ShellEntry
    {
        ShellEntry() { }
        Task* task = nullptr;
        Telnet telnet;
    };
    
    ShellEntry _shells[m8r::TCP::MaxConnections];
};

Application::Application(uint16_t port)
{
    _shellSocket = new MyShellSocket(this, port);
}

Application::~Application()
{
    delete _shellSocket;
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
    return shellName();
}

bool Application::mountFileSystem()
{
    if (!system()->fileSystem()->mount() != 0) {
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

    if (m8r::system()->fileSystem()->mounted()) {
        m8r::system()->printf(ROMSTR("Filesystem - total size:%d, used:%d\n"), m8r::system()->fileSystem()->totalSize(), m8r::system()->fileSystem()->totalUsed());
    }
    return true;
}

void Application::runLoop()
{
    system()->printf(ROMSTR("\n*** m8rscript v%d.%d - %s\n\n"), MajorVersion, MinorVersion, BuildTimeStamp);
    
    // If autostart is on, run the main program
    String filename = autostartFilename();
    if (filename) {
        _autostartTask = std::shared_ptr<Task>(new Task(filename.c_str()));
        _autostartTask->run();
        // FIXME: Create a task and run the autostart file
//        m8r::Error error;
//        if (!load(error, false)) {
//            error.showError();
//        } else if (!program()) {
//            system()->printf(ROMSTR("Error:failed to compile application"));
//        } else {
//            run([this]{
//                m8r::MemoryInfo info;
//                m8r::Object::memoryInfo(info);
//                system()->printf(ROMSTR("***** finished - free ram:%d, num allocations:%d\n"), info.freeSize, info.numAllocations);
//            });
//        }
    }
    
    system()->runLoop();
}

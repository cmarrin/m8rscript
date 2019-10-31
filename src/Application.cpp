/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
	  
    - Redistributions in binary form must reproduce the above copyright 
	  notice, this list of conditions and the following disclaimer in the 
	  documentation and/or other materials provided with the distribution.
	  
    - Neither the name of the <ORGANIZATION> nor the names of its 
	  contributors may be used to endorse or promote products derived from 
	  this software without specific prior written permission.
	  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "Application.h"

#include "FS.h"
#include "SystemInterface.h"

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
                _shells[connectionId] = new Task(Application::shellName());
                if (_shells[connectionId]->error() != Error::Code::OK) {
                    Error::printError(_shells[connectionId]->error().code());
                    _shells[connectionId] = nullptr;
                    tcp->disconnect(connectionId);
                } else {
                    _shells[connectionId]->setConsoleOutputFunction([](const String& s) {
                        tcp->send(connectionId, s.c_str());
                    });
                    _shells[connectionId]->run([tcp, connectionId, this](TaskBase*)
                    {
                        tcp->disconnect(connectionId);
                        delete _shells[connectionId];
                        _shells[connectionId] = nullptr;
                    });
                }
                break;
            case m8r::TCPDelegate::Event::Disconnected:
                if (_shells[connectionId]) {
                    _shells[connectionId]->terminate();
                    _shells[connectionId] = nullptr;
                }
                break;
            case m8r::TCPDelegate::Event::ReceivedData:
                if (_shells[connectionId]) {
                    _shells[connectionId]->receivedData(data, length);
                }
                break;
            case m8r::TCPDelegate::Event::SentData:
                //if (_shells[connectionId]) {
                //    _shells[connectionId]->sendComplete();
                //}
                break;
            default:
                break;
        }
    }

private:
    std::unique_ptr<m8r::TCP> _tcp;
    Application* _application;
    Task* _shells[m8r::TCP::MaxConnections];
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

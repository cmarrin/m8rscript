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

#include "MStream.h"
#include "Shell.h"
#include "SystemInterface.h"

#ifndef NO_PARSER_SUPPORT
#include "Parser.h"
#endif

#ifdef MONITOR_TRAFFIC
#include <string>
#endif
using namespace m8r;

class MyTCP;

class MyShell : public m8r::Shell {
public:
    MyShell(m8r::Application* application, m8r::TCP* tcp, uint16_t connectionId)
        : Shell(application)
        , _tcp(tcp)
        , _connectionId(connectionId)
    { }
    
    // Shell Delegate
    virtual void shellSend(const char* data, uint16_t size)
    {
#ifdef MONITOR_TRAFFIC
    if (!size) {
        size = strlen(data);
    }
    char* s = new char[size + 1];
    memcpy(s, data, size);
    s[size] = '\0';
    debugf("[Shell] >>>> shellSend:'%s'\n", s);
    delete [ ] s;
#endif
        _tcp->send(_connectionId, data, size);
    }

private:
    m8r::TCP* _tcp;
    uint16_t _connectionId;
};

class MyShellSocket : public TCPDelegate {
public:
    MyShellSocket(Application* application, uint16_t port)
        : _tcp(m8r::TCP::create(this, port))
        , _application(application)
    { }
    
    virtual ~MyShellSocket() { delete _tcp; }

    // TCPDelegate
    virtual void TCPevent(m8r::TCP*, m8r::TCPDelegate::Event event, int16_t connectionId, const char* data, int16_t length) override
    {
        switch(event) {
            case m8r::TCPDelegate::Event::Connected:
                _shells[connectionId] = new MyShell(_application, _tcp, connectionId);
                _shells[connectionId]->connected();
                break;
            case m8r::TCPDelegate::Event::Disconnected:
                if (_shells[connectionId]) {
                    _shells[connectionId]->disconnected();
                    delete _shells[connectionId];
                    _shells[connectionId] = nullptr;
                }
                break;
            case m8r::TCPDelegate::Event::ReceivedData:
                if (_shells[connectionId] && !_shells[connectionId]->received(data, length)) {
                    _tcp->disconnect(connectionId);
                }
                break;
            case m8r::TCPDelegate::Event::SentData:
                if (_shells[connectionId]) {
                    _shells[connectionId]->sendComplete();
                }
                break;
            default:
                break;
        }
    }

private:
    m8r::TCP* _tcp;
    Application* _application;
    MyShell* _shells[m8r::TCP::MaxConnections];
};

Application::Application(FS* fs, SystemInterface* system, uint16_t port)
    : _fs(fs)
    , _system(system)
    , _runTask()
    , _heartbeatTask(system)
{
    _shellSocket = new MyShellSocket(this, port);
}

Application::~Application()
{
    delete _shellSocket;
}

bool Application::load(Error& error, bool debug, const char* filename)
{
    stop();
    _program = nullptr;
    _syntaxErrors.clear();
    
    if (filename && validateFileName(filename) == NameValidationType::Ok) {
        FileStream m8rbStream(_fs, filename);
        if (!m8rbStream.loaded()) {
            error.setError(Error::Code::FileNotFound);
            return false;
        }
        
#ifdef NO_PARSER_SUPPORT
        return false;
#else
        // See if we can parse it
        FileStream m8rStream(_fs, filename);
        _system->printf(ROMSTR("Parsing...\n"));
        Parser parser(_system);
        parser.parse(&m8rStream, debug);
        _system->printf(ROMSTR("Finished parsing %s. %d error%s\n\n"), filename, parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
        if (parser.nerrors()) {
            _syntaxErrors.swap(parser.syntaxErrors());
            return false;
        }
        _program = parser.program();
        return true;
#endif
    }
    
    // See if there is a 'main' file (which contains the name of the program to run)
    String name = "main";
    FileStream mainStream(_fs, name.c_str());
    if (mainStream.loaded()) {
        name.clear();
        while (!mainStream.eof()) {
            int c = mainStream.read();
            if (c < 0) { 
                break;
            }
            name += static_cast<char>(c);
        }
    } else {
        _system->printf(ROMSTR("'main' not found in filesystem, trying default...\n"));
    }
    
#ifdef NO_PARSER_SUPPORT
    _system->printf(ROMSTR("File not found, nothing to load\n"));
    return false;
#else
    name = name.slice(0, -1);
    _system->printf(ROMSTR("File not found, trying '%s'...\n"), name.c_str());
    FileStream m8rMainStream(_fs, name.c_str());
    
    if (!m8rMainStream.loaded()) {
        error.setError(Error::Code::FileNotFound);
        return false;
    }

    Parser parser(_system);
    parser.parse(&m8rMainStream, debug);
    _system->printf(ROMSTR("Finished parsing %s. %d error%s\n\n"), name.c_str(), parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
    if (parser.nerrors()) {
        _syntaxErrors.swap(parser.syntaxErrors());
        return false;
    }

    _program = parser.program();
    return true;
#endif
}

void Application::run(std::function<void()> function)
{
    stop();
    _system->printf(ROMSTR("\n***** Start of Program Output *****\n\n"));
    _runTask.run(_program, function);
}

void Application::pause()
{
    _runTask.pause();
}

void Application::stop()
{
    if (_runTask.stop()) {
        _system->printf(ROMSTR("\n***** Program Stopped *****\n\n"));
    }
}

Application::NameValidationType Application::validateFileName(const char* name)
{
    if (!name || name[0] == '\0') {
        return NameValidationType::BadLength;
    }
    
    for (size_t i = 0; name[i]; i++) {
        if (i >= 31) {
            return NameValidationType::BadLength;
        }
        
        char c = name[i];
        if (c == '-' || c == '.' || c == '_' || c == '+' ||
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')) {
            continue;
        }
        return NameValidationType::InvalidChar;
    }
    return NameValidationType::Ok;
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

bool Application::MyRunTask::execute()
{
    if (!_running) {
        return false;
    }
    CallReturnValue returnValue = _eu.continueExecution();
    if (returnValue.isMsDelay()) {
        runOnce(returnValue.msDelay());
    } else if (returnValue.isContinue()) {
        runOnce(0);
    } else if (returnValue.isFinished() || returnValue.isTerminated()) {
        _function();
        _running = false;
    } else if (returnValue.isWaitForEvent()) {
        runOnce(50);
    }
    return true;
}

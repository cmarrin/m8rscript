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

#include "EventManager.h"
#include "MStream.h"
#include "SystemInterface.h"

#ifndef NO_PARSER_SUPPORT
#include "Parser.h"
#endif

using namespace m8r;

Application::Application()
    : _runTask()
    , _heartbeatTask()
{
}

bool Application::load(Error& error, const char* filename)
{
    _program = nullptr;
    
    if (filename && validateFileName(filename) == NameValidationType::Ok) {
        FileStream m8rbStream(filename);
        if (!m8rbStream.loaded()) {
            error.setError(Error::Code::FileNotFound);
            return false;
        }
        
        // Is it a m8rb file?
        _program = new m8r::Program();
        if (_program->deserializeObject(&m8rbStream, error, nullptr, AtomTable(), std::vector<char>())) {
            return true;
        }
        _program = nullptr;
        if (error.code() != Error::Code::SerialHeader) {
            return false;
        }
        
#ifdef NO_PARSER_SUPPORT
        return false;
#else
        // See if we can parse it
        FileStream m8rStream(filename);
        SystemInterface::shared()->printf(ROMSTR("Parsing...\n"));
        Parser parser;
        parser.parse(&m8rStream);
        SystemInterface::shared()->printf(ROMSTR("Finished parsing %s. %d error%s\n\n"), filename, parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
        if (parser.nerrors()) {
            return false;
        }
        _program = parser.program();
        return true;
#endif
    }
    
    // See if there is a 'main' file (which contains the name of the program to run)
    String name = "main";
    FileStream mainStream(name.c_str());
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
        SystemInterface::shared()->printf(ROMSTR("'main' not found in filesystem, trying default...\n"));
    }
    
    name += ".m8rb";
    SystemInterface::shared()->printf(ROMSTR("Opening '%s'...\n"), name.c_str());

    FileStream m8rbMainStream(name.c_str());
    
    if (m8rbMainStream.loaded()) {
        _program = new m8r::Program();
        Global::addObject(_program, true);
        return _program->deserializeObject(&m8rbMainStream, error, nullptr, AtomTable(), std::vector<char>());
     }

#ifdef NO_PARSER_SUPPORT
    SystemInterface::shared()->printf(ROMSTR("File not found, nothing to load\n"));
    return false;
#else
    name = name.slice(0, -1);
    SystemInterface::shared()->printf(ROMSTR("File not found, trying '%s'...\n"), name.c_str());
    FileStream m8rMainStream(name.c_str());
    
    if (!m8rMainStream.loaded()) {
        error.setError(Error::Code::FileNotFound);
        return false;
    }

    Parser parser;
    parser.parse(&m8rMainStream);
    SystemInterface::shared()->printf(ROMSTR("Finished parsing %s. %d error%s\n\n"), name.c_str(), parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
    if (parser.nerrors()) {
        return false;
    }

    _program = parser.program();
    return true;
#endif
}

void Application::run(std::function<void()> function)
{
    stop();
    SystemInterface::shared()->printf(ROMSTR("\n***** Start of Program Output *****\n\n"));
    _runTask.run(_program, function);
}

void Application::pause()
{
    _runTask.pause();
}

void Application::stop()
{
    if (_runTask.stop()) {
        SystemInterface::shared()->printf(ROMSTR("\n***** Program Stopped *****\n\n"));
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

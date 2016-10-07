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

#include "Engine.h"

#import "Application.h"
#import "Parser.h"
#import "CodePrinter.h"
#import "ExecutionUnit.h"
#import "Shell.h"
#import "SystemInterface.h"

#include <chrono>
#include <thread>

using namespace m8r;

class Engine;

class Engine : public SystemInterface, public ShellOutput {
public:
    Engine(void* simulator)
        : _simulator(simulator)
        , _isBuild(true)
        , _eu(this)
        , _shell(this)
    { }

    void importBinary(const char* filename);
    void exportBinary(const char* filename);
    void build(const char* source, const char* name);
    void run();
    void pause();
    void stop();
    void simulate();

    bool canRun() { return _program && !_running; }
    bool canStop() { return _program && _running; }

    // SystemInterface
    virtual void printf(const char* s, ...) const override;
    virtual void updateGPIOState(uint16_t mode, uint16_t state) override
    {
        Simulator_updateGPIOState(_simulator, mode, state);
    }
    virtual int read() const override { return -1; }

    // ShellOutput
    virtual void shellSend(const char* data, uint16_t size = 0) override
    {
    }

private:
    void* _simulator;
    bool _isBuild;
    ExecutionUnit _eu;
    m8r::Program* _program;
    m8r::Application* _application;
    bool _running = false;
    Shell _shell;
};

void Engine::printf(const char* s, ...) const
{
    va_list args;
    va_start(args, s);
    Simulator_vprintf(_simulator, s, args, _isBuild);
}

void Engine::importBinary(const char* filename)
{
    m8r::FileStream stream(filename, "r");
    _program = new Program(this);
    m8r::Error error;
    if (!_program->deserializeObject(&stream, error)) {
        error.showError(this);
    }
}

void Engine::exportBinary(const char* filename)
{
    m8r::FileStream stream(filename, "w");
    if (_program) {
        m8r::Error error;
        if (!_program->serializeObject(&stream, error)) {
            error.showError(this);
        }
    }
}

void Engine::build(const char* source, const char* name)
{
    _program = nullptr;
    _running = false;
    
    _isBuild = true;
    printf("Building %s\n", name);

    m8r::StringStream stream(source);
    m8r::Parser parser(this);
    parser.parse(&stream);
    printf("Parsing finished...\n");

    if (parser.nerrors()) {
        printf("***** %d error%s\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
    } else {
        printf("0 errors. Ready to run\n");
        _program = parser.program();

        m8r::CodePrinter codePrinter(this);
        m8r::String codeString = codePrinter.generateCodeString(_program);
        
        printf("\n*** Start Generated Code ***\n\n");
        printf(codeString.c_str());
        printf("\n*** End of Generated Code ***\n\n");
    }
}

void Engine::run()
{
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    
    _isBuild = false;
    printf("*** Program started...\n\n");
    
    auto start = std::chrono::system_clock::now();
        
    _eu.startExecution(_program);
    while (1) {
        int32_t delay = _eu.continueExecution();
        if (delay < 0) {
            break;
        }
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
    
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;
    printf("\n\n*** Finished (run time:%fms)\n", diff * 1000);
    _running = false;
}

void Engine::pause()
{
}

void Engine::stop()
{
    if (!_running) {
        assert(0);
        return;
    }
    _eu.requestTermination();
    _running = false;
    printf("*** Stopped\n");
}

void Engine::simulate()
{
    _application = new m8r::Application(this);
    _program = _application->program();
    run();
}


void* Engine_createEngine(void* simulator) { return new Engine(simulator); }
void Engine_deleteEngine(void* engine) { delete reinterpret_cast<Engine*>(engine); }
bool Engine_canRun(void* engine) { return reinterpret_cast<Engine*>(engine)->canRun(); }
bool Engine_canStop(void* engine) { return reinterpret_cast<Engine*>(engine)->canStop(); }

void Engine_importBinary(void* engine, const char* filename)
{
    reinterpret_cast<Engine*>(engine)->importBinary(filename);
}

void Engine_exportBinary(void* engine, const char* filename)
{
    reinterpret_cast<Engine*>(engine)->exportBinary(filename);
}

void Engine_build(void* engine, const char* source, const char* name)
{
    reinterpret_cast<Engine*>(engine)->build(source, name);
}

void Engine_run(void* engine) { reinterpret_cast<Engine*>(engine)->run(); }
void Engine_pause(void* engine) { reinterpret_cast<Engine*>(engine)->pause(); }
void Engine_stop(void* engine) { reinterpret_cast<Engine*>(engine)->stop(); }
void Engine_simulate(void* engine) { reinterpret_cast<Engine*>(engine)->simulate(); }

int validateFileName(const char* name) {
    switch(m8r::Application::validateFileName(name)) {
        case m8r::Application::NameValidationType::Ok: return NameValidationOk;
        case m8r::Application::NameValidationType::BadLength: return NameValidationBadLength;
        case m8r::Application::NameValidationType::InvalidChar: return NameValidationInvalidChar;
    }
}
    
int validateBonjourName(const char* name)
{
    switch(m8r::Application::validateBonjourName(name)) {
        case m8r::Application::NameValidationType::Ok: return NameValidationOk;
        case m8r::Application::NameValidationType::BadLength: return NameValidationBadLength;
        case m8r::Application::NameValidationType::InvalidChar: return NameValidationInvalidChar;
    }
}

//
//  Simulator.cpp
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#include "Simulator.h"

#include "Application.h"
#include "CodePrinter.h"
#include "MStream.h"
#include "Parser.h"
#include <chrono>
#include <thread>

Simulator::~Simulator()
{
    if (_program) {
        delete _program;
    }
    if (_application) {
        delete _application;
    }
}

void Simulator::importBinary(const char* filename)
{
    m8r::FileStream stream(filename, "r");
    _program = new m8r::Program(_system);
    m8r::Error error;
    if (!_program->deserializeObject(&stream, error)) {
        error.showError(_system);
    }
}

void Simulator::exportBinary(const char* filename)
{
    m8r::FileStream stream(filename, "w");
    if (_program) {
        m8r::Error error;
        if (!_program->serializeObject(&stream, error)) {
            error.showError(_system);
        }
    }
}

void Simulator::build(const char* source, const char* name)
{
    _program = nullptr;
    _running = false;
    
    _isBuild = true;
    _system->printf(ROMSTR("Building %s\n"), name);

    m8r::StringStream stream(source);
    m8r::Parser parser(_system);
    parser.parse(&stream);
    _system->printf(ROMSTR("Parsing finished...\n"));

    if (parser.nerrors()) {
        _system->printf(ROMSTR("***** %d error%s\n"), parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
    } else {
        _system->printf(ROMSTR("0 errors. Ready to run\n"));
        _program = parser.program();

        m8r::CodePrinter codePrinter(_system);
        m8r::String codeString = codePrinter.generateCodeString(_program);
        
        _system->printf(ROMSTR("\n*** Start Generated Code ***\n\n"));
        _system->printf("%s", codeString.c_str());
        _system->printf(ROMSTR("\n*** End of Generated Code ***\n\n"));
    }
}

void Simulator::run()
{
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    
    _isBuild = false;
    _system->printf(ROMSTR("*** Program started...\n\n"));
    
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
    _system->printf(ROMSTR("\n\n*** Finished (run time:%fms)\n"), diff.count() * 1000);
    _running = false;
}

void Simulator::pause()
{
}

void Simulator::stop()
{
    if (!_running) {
        assert(0);
        return;
    }
    _eu.requestTermination();
    _running = false;
    _system->printf(ROMSTR("*** Stopped\n"));
}

void Simulator::simulate()
{
    _application = new m8r::Application(_system);
    m8r::Error error;
    if (_application->load(error)) {
        error.showError(_system);
        return;
    }
    _program = _application->program();
    run();
}

long Simulator::sendToShell(const void* data, long size)
{
    if (_shell.received(reinterpret_cast<const char*>(data), static_cast<uint16_t>(size))) {
        return size;
    }
    return 0;
}
long Simulator::receiveFromShell(void* data, long size)
{
    if (_receivedString.empty()) {
        return 0;
    }
    if (size >_receivedString.size()) {
        strcpy(reinterpret_cast<char*>(data), _receivedString.c_str());
        _receivedString.clear();
        _shell.sendComplete();
        return _receivedString.size() + 1;
    }
    memcpy(data, _receivedString.c_str(), size);
    _receivedString.erase(0, size);
    _shell.sendComplete();
    return size;
}


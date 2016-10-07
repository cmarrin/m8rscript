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
    _system->printf("Building %s\n", name);

    m8r::StringStream stream(source);
    m8r::Parser parser(_system);
    parser.parse(&stream);
    _system->printf("Parsing finished...\n");

    if (parser.nerrors()) {
        _system->printf("***** %d error%s\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
    } else {
        _system->printf("0 errors. Ready to run\n");
        _program = parser.program();

        m8r::CodePrinter codePrinter(_system);
        m8r::String codeString = codePrinter.generateCodeString(_program);
        
        _system->printf("\n*** Start Generated Code ***\n\n");
        _system->printf("%s", codeString.c_str());
        _system->printf("\n*** End of Generated Code ***\n\n");
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
    _system->printf("*** Program started...\n\n");
    
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
    _system->printf("\n\n*** Finished (run time:%fms)\n", diff.count() * 1000);
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
    _system->printf("*** Stopped\n");
}

void Simulator::simulate()
{
    _application = new m8r::Application(_system);
    _program = _application->program();
    run();
}


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

#include "Simulator.h"

#include "Application.h"
#include "CodePrinter.h"
#include "MStream.h"
#include "Parser.h"
#include <chrono>
#include <thread>

Simulator::~Simulator()
{
}

void Simulator::importBinary(const char* filename)
{
//    m8r::FileStream stream(filename, "r");
//    _program = new m8r::Program(_system);
//    m8r::Error error;
//    if (!_program->deserializeObject(&stream, error)) {
//        error.showError(_system);
//        delete _program;
//        _program = nullptr;
//    }
}

void Simulator::exportBinary(const char* filename)
{
//    m8r::FileStream stream(filename, "w");
//    if (_program) {
//        m8r::Error error;
//        if (!_program->serializeObject(&stream, error)) {
//            error.showError(_system);
//            delete _program;
//            _program = nullptr;
//        }
//    }
}

bool Simulator::exportBinary(std::vector<uint8_t>& vector)
{
    if (!_shell.program()) {
        return false;
    }
    
    m8r::VectorStream stream;
    m8r::Error error;
    if (!_shell.program()->serializeObject(&stream, error, _shell.program())) {
        error.showError(_system);
        return false;
    }
    stream.swap(vector);
    return true;
}

void Simulator::build(const char* name)
{
    _running = false;
    if (_shell.load(name)) {
        _system->printf(ROMSTR("Ready to run\n"));
    }
}

void Simulator::printCode()
{
    m8r::CodePrinter codePrinter(_system);
    m8r::String codeString = codePrinter.generateCodeString(_shell.program());
    
    _system->printf(ROMSTR("\n*** Start Generated Code ***\n\n"));
    _system->printf("%s", codeString.c_str());
    _system->printf(ROMSTR("\n*** End of Generated Code ***\n\n"));
}

void Simulator::run()
{
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    _system->printf(ROMSTR("*** Program started...\n\n"));

    auto start = std::chrono::system_clock::now();
    _shell.run([start, this]{
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = end - start;
        _system->printf(ROMSTR("\n\n*** Finished (run time:%fms)\n"), diff.count() * 1000);
        _running = false;
    });
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
    _shell.stop();
    _running = false;
    _system->printf(ROMSTR("*** Stopped\n"));
}

void Simulator::simulate()
{
    if (!_shell.load(nullptr)) {
        return;
    }
    _shell.run([]{});
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


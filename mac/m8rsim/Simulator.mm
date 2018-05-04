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
#include "MacFS.h"
#include "MStream.h"
#include "Parser.h"
#include <chrono>
#include <thread>

#define PrintCode 1

uint64_t m8r::SystemInterface::currentMicroseconds()
{
    return static_cast<uint64_t>(std::clock() * 1000000 / CLOCKS_PER_SEC);
}

void* m8r::SystemInterface::alloc(MemoryType type, size_t size)
{
    return ::malloc(size);
}

void m8r::SystemInterface::free(MemoryType, void* p)
{
    ::free(p);
}

void m8r::SystemInterface::memoryInfo(MemoryInfo& info)
{
    // FIXME: info.freeSize = g_freeHeapSize;
    // FIXME: info.numAllocations = g_allocCount;
}

class MyLogSocket : public m8r::TCPDelegate {
public:
    MyLogSocket(uint16_t port)
        : _tcp(m8r::TCP::create(this, port))
    { }
    
    virtual ~MyLogSocket() { delete _tcp; }

    virtual void TCPevent(m8r::TCP* tcp, m8r::TCPDelegate::Event event, int16_t connectionId, const char* data, int16_t length) override
    {
        if (event == m8r::TCPDelegate::Event::Connected) {
            tcp->send(connectionId, "Start m8rscript Log\n\n");
        }
    }

    void log(const char* s)
    {
        for (uint16_t connection = 0; connection < m8r::TCP::MaxConnections; ++connection) {
            _tcp->send(connection, s);
        }
    }
private:
    m8r::TCP* _tcp;
};

class MyShell : public m8r::Shell {
    friend class Simulator;
    
public:
    MyShell(m8r::FS* fs, m8r::SystemInterface* system, Simulator* simulator) : _application(fs, system, 22), Shell(&_application), _simulator(simulator) { }
    virtual void shellSend(const char* data, uint16_t size = 0) override { _simulator->shellSend(data, size); }

private:
    m8r::Application _application;
    Simulator* _simulator;
};
    
class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface(uint16_t port, Simulator* simulator) : _gpio(simulator)
    {
        _logSocket = new MyLogSocket(port + 1);
    }
    
    virtual ~MySystemInterface()
    {
        delete _logSocket;
    }
    
    virtual void vprintf(const char* s, va_list args) const override
    {
        NSString* string = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:s] arguments:args];
        _logSocket->log([string UTF8String]);
    }
    
    virtual void setDeviceName(const char*) override { }
    
    virtual m8r::GPIOInterface& gpio() override { return _gpio; }

private:
    class MyGPIOInterface : public m8r::GPIOInterface {
    public:
        MyGPIOInterface(Simulator* simulator) : _simulator(simulator) { }
        virtual ~MyGPIOInterface() { }

        virtual bool setPinMode(uint8_t pin, PinMode mode) override
        {
            if (!GPIOInterface::setPinMode(pin, mode)) {
                return false;
            }
            _pinio = (_pinio & ~(1 << pin)) | ((mode == PinMode::Output) ? (1 << pin) : 0);

            // FIXME: [_simulator updateGPIOState:_pinio withMode:_pinstate];
            return true;
        }
        
        virtual bool digitalRead(uint8_t pin) const override
        {
            return _pinstate & (1 << pin);
        }
        
        virtual void digitalWrite(uint8_t pin, bool level) override
        {
            if (pin > 16) {
                return;
            }
            _pinstate = (_pinstate & ~(1 << pin)) | (level ? (1 << pin) : 0);

            // FIXME: [_simulator updateGPIOState:_pinstate withMode:_pinio];
        }
        
        virtual void onInterrupt(uint8_t pin, Trigger, std::function<void(uint8_t pin)> = { }) override { }
        
    private:
        // 0 = input, 1 = output
        uint32_t _pinio = 0;
        uint32_t _pinstate = 0;
        
        Simulator* _simulator;
    };
    
    MyGPIOInterface _gpio;
    MyLogSocket* _logSocket;
};

Simulator::Simulator(uint32_t port)
{
    _fs.reset(m8r::FS::createFS());
    _system.reset(new MySystemInterface(port, this));
    _application.reset(new m8r::Application(_fs.get(), _system.get(), port));
    _shell.reset(new MyShell(_fs.get(), _system.get(), this));
}

Simulator::~Simulator()
{
}

void Simulator::setFiles(NSURL* files)
{
    NSFileWrapper* wrapper = [[NSFileWrapper alloc] initWithURL:files options:0 error:NULL];
    static_cast<m8r::MacFS*>(_fs.get())->setFiles(wrapper);
}

const m8r::ErrorList* Simulator::build(const char* name, bool debug)
{
    _running = false;
    const m8r::ErrorList* errors = _shell->load(name, debug);
    if (!errors) {
#ifdef PrintCode
        printCode();
#endif
        _shell->program()->system()->printf(ROMSTR("Ready to run\n"));
    }
    return errors;
}

void Simulator::printCode()
{
    m8r::CodePrinter codePrinter;
    m8r::String codeString = codePrinter.generateCodeString(_shell->program());
    
    _shell->program()->system()->printf(ROMSTR("\n*** Start Generated Code ***\n\n"));
    _shell->program()->system()->printf("%s", codeString.c_str());
    _shell->program()->system()->printf(ROMSTR("\n*** End of Generated Code ***\n\n"));
}

void Simulator::run()
{
    if (_running) {
        assert(0);
        return;
    }
    
    _running = true;
    _shell->program()->system()->printf(ROMSTR("*** Program started...\n\n"));

    auto start = std::chrono::system_clock::now();
    _shell->run([start, this]{
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = end - start;
        _shell->program()->system()->printf(ROMSTR("\n\n*** Finished (run time:%fms)\n"), diff.count() * 1000);
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
    _shell->stop();
    _running = false;
    _shell->program()->system()->printf(ROMSTR("*** Stopped\n"));
}

void Simulator::simulate()
{
    if (!_shell->load(nullptr, false)) {
        return;
    }
    _shell->run([]{});
}

long Simulator::sendToShell(const void* data, long size)
{
    if (_shell->received(reinterpret_cast<const char*>(data), static_cast<uint16_t>(size))) {
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
        _shell->sendComplete();
        return _receivedString.size() + 1;
    }
    memcpy(data, _receivedString.c_str(), size);
    _receivedString.erase(0, size);
    _shell->sendComplete();
    return size;
}


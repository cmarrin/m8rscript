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

static constexpr uint32_t StaticPort = 2222;

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

class MySystemInterface : public m8r::SystemInterface
{
public:
    MySystemInterface(uint16_t port, m8r::GPIOInterface* gpio) : _gpio(gpio)
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
    
    virtual m8r::GPIOInterface& gpio() override { return *_gpio; }

private:    
    m8r::GPIOInterface* _gpio;
    MyLogSocket* _logSocket;
};

uint32_t Simulator::_nextLocalPort = StaticPort;


Simulator::Simulator(m8r::GPIOInterface* gpio)
{
    //_fs.reset(m8r::FS::createFS());
    _system.reset(new MySystemInterface(_nextLocalPort, gpio));
    _application.reset(new m8r::Application(_fs.get(), _system.get(), _nextLocalPort));
    _localPort = _nextLocalPort;
    _nextLocalPort += 2;
}

Simulator::~Simulator()
{
}

bool Simulator::setFiles(NSURL* files, NSError** error)
{
    NSFileWrapper* wrapper = nullptr;
    
    if (files) {
        wrapper = [[NSFileWrapper alloc] initWithURL:files options:0 error:error];
    } else {
        wrapper = [[NSFileWrapper alloc] initDirectoryWithFileWrappers:@{ }];
        *error = nil;
    }
    
    if (!wrapper) {
        return false;
    }
    static_cast<m8r::MacFS*>(_fs.get())->setFiles(wrapper);
    return true;
}

NSFileWrapper* Simulator::getFiles() const
{
    return static_cast<m8r::MacFS*>(_fs.get())->getFiles();
}

NSArray* Simulator::listFiles()
{
    m8r::DirectoryEntry* directoryEntry = _fs->directory();
    if (!directoryEntry) {
        return NULL;
    }
    
    NSMutableArray* array = [[NSMutableArray alloc]init];

    while (directoryEntry->valid()) {
        [array addObject:@{ @"name" : [NSString stringWithUTF8String:directoryEntry->name()], 
                            @"size" : [NSNumber numberWithUnsignedInt:directoryEntry->size()] }];
        directoryEntry->next();
    }

    delete directoryEntry;
    return array;
}

NSData* Simulator::getFileData(NSString* name)
{
    m8r::File* file = _fs->open([name UTF8String], "r");
    if (!file) {
        return NULL;
    }
    
    file->seek(0, m8r::File::SeekWhence::End);
    uint32_t size = file->tell();
    file->seek(0, m8r::File::SeekWhence::Set);

    char* buffer = new char[size];
    
    int32_t result = file->read(buffer, size);
    if (result != size) {
        delete file;
        delete [ ] buffer;
        return NULL;
    }
    
    NSData* data = [NSData dataWithBytes:buffer length:size];
    delete file;
    delete [ ] buffer;
    return data;
}

void Simulator::printCode()
{
    m8r::CodePrinter codePrinter;
    m8r::String codeString = codePrinter.generateCodeString(_application->program());
    
    _system->printf(ROMSTR("\n*** Start Generated Code ***\n\n"));
    _system->printf("%s", codeString.c_str());
    _system->printf(ROMSTR("\n*** End of Generated Code ***\n\n"));
}

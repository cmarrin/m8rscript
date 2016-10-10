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

#pragma once

#include "FS.h"
#include "Containers.h"

namespace m8r {

class ShellOutput {
public:
    virtual void shellSend(const char* data, uint16_t size = 0) = 0;
    virtual void setDeviceName(const char* name) { }
};

class Shell {
public:
    static const uint16_t BufferSize = 76;
    static const uint16_t Base64MaxSize = (BufferSize - 3) / 4 * 3;
    static const uint32_t StackAllocLimit = 33; // This must be divisible by 4/3
    static_assert (StackAllocLimit < Base64MaxSize, "BufferSize too big");
    
    enum class State { Init, NeedPrompt, ShowingPrompt, ListFiles, GetFile, PutFile };
    
    enum class ErrorCode { Ok = 0,
        ReadFailed,
        BinaryPutTooLarge,
        BinaryEncodeFailed,
        BinaryDecodeFailed,
        FileNameRequired,
        DeviceNameRequired,
        GetOpenFailed,
        PutOpenFailed,
        RemoveFailed,
        BadDeviceNameLength,
        IllegalDeviceName,
    };
    
    Shell(ShellOutput* output)
        : _output(output)
    { }
    
    void connected();
    void disconnected() { }
    bool received(const char* data, uint16_t size);
    void sendComplete();
    void init();
    
    long send(const void* data, long size);
    long receive(void* data, long size);
        
private:
    bool executeCommand(const std::vector<m8r::String>& array);
    void showError(ErrorCode);
    void sendString(const char* s);

    ShellOutput* _output = nullptr;
    m8r::DirectoryEntry* _directoryEntry = nullptr;
    State _state = State::Init;
    bool _binary = true;
    m8r::File* _file = nullptr;
    
    char _buffer[BufferSize];
};

}

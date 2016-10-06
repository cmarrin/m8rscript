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

#include "Shell.h"

#include "Application.h"
#include "base64.h"
#include <cstdarg>

using namespace m8r;

void Shell::connected()
{
    _state = State::Init;
    sendComplete();
}

bool Shell::received(const char* data, uint16_t size)
{
    if (_state == State::PutFile) {
        if (size == 0 || data[0] == '\04') {
            delete _file;
            _file = nullptr;
            _state = State::NeedPrompt;
            sendComplete();
            return true;
        }

        if (_binary) {
            // If incoming buffer is too large, error
            if (size > BufferSize) {
                showError(ErrorCode::BinaryPutTooLarge);
            }

            int length = base64_decode(size, data, BufferSize, reinterpret_cast<uint8_t*>(_buffer));
            if (length < 0) {
                delete _file;
                _file = nullptr;
                showError(ErrorCode::BinaryDecodeFailed);
                return false;
            }
            _file->write(_buffer, length);
        } else {
            _file->write(data, size);
        }
        return true;
    }
    std::vector<m8r::String> array = m8r::String(data).trim().split(" ", true);
    return executeCommand(array);
}

void Shell::sendComplete()
{
    switch(_state) {
        case State::Init:
            sendString(ROMSTR("\nWelcome to m8rscript\n"));
            _state = State::NeedPrompt;
            break;
        case State::NeedPrompt:
            _output->shellSend("\n>");
            _state = State::ShowingPrompt;
            break;
        case State::ShowingPrompt:
            break;
        case State::ListFiles:
            if (_directoryEntry && _directoryEntry->valid()) {
                if (_binary) {
                    sprintf(_buffer, "file:%s:%d\n", _directoryEntry->name(), _directoryEntry->size());
                } else {
                    sprintf(_buffer, "    %-32s %d\n", _directoryEntry->name(), _directoryEntry->size());
                }
                _output->shellSend(_buffer);
                _directoryEntry->next();
            } else {
                if (_directoryEntry) {
                    delete _directoryEntry;
                    _directoryEntry = nullptr;
                }
                _state = State::NeedPrompt;
                sendComplete();
            }
            break;
        case State::GetFile: {
            if (!_file) {
                _state = State::NeedPrompt;
                _output->shellSend("\04");
                break;
            }
            if (_binary) {
                char binaryBuffer[StackAllocLimit];
                int32_t result = _file->read(binaryBuffer, StackAllocLimit);
                if (result < 0) {
                    showError(ErrorCode::ReadFailed);
                    break;
                }
                if (result < StackAllocLimit) {
                    delete _file;
                    _file = nullptr;
                }
                int length = base64_encode(result, reinterpret_cast<uint8_t*>(binaryBuffer), BufferSize, _buffer);
                if (length < 0) {
                showError(ErrorCode::BinaryEncodeFailed);
                    delete _file;
                    _file = nullptr;
                    break;
                }
                _buffer[length++] = '\r';
                _buffer[length++] = '\n';
                _output->shellSend(_buffer, length);
            } else {
                int32_t result = _file->read(_buffer, BufferSize);
                if (result < 0) {
                    showError(ErrorCode::ReadFailed);
                    break;
                }
                if (result < BufferSize) {
                    delete _file;
                    _file = nullptr;
                }
                _output->shellSend(_buffer, result);
            }
            break;
        }
        case State::PutFile: {
            break;
        }
    }
}

bool Shell::executeCommand(const std::vector<m8r::String>& array)
{
    if (array.size() == 0) {
        return true;
    }
    if (array[0] == "ls") {
        _directoryEntry = m8r::FS::sharedFS()->directory();
        _state = State::ListFiles;
        sendComplete();
    } else if (array[0] == "t") {
        _binary = false;
        _state = State::NeedPrompt;
        sendString(ROMSTR("text: Setting text transfer mode\n"));
    } else if (array[0] == "b") {
        _binary = true;
        _state = State::NeedPrompt;
        sendString(ROMSTR("binary: Setting binary transfer mode\n"));
    } else if (array[0] == "get") {
        if (array.size() < 2) {
            showError(ErrorCode::FileNameRequired);
        } else {
            _file = m8r::FS::sharedFS()->open(array[1].c_str(), "r");
            if (!_file) {
                showError(ErrorCode::GetOpenFailed);
            } else {
                _state = State::GetFile;
                sendComplete();
            }
        }
    } else if (array[0] == "put") {
        if (array.size() < 2) {
            showError(ErrorCode::FileNameRequired);
        } else {
            _file = m8r::FS::sharedFS()->open(array[1].c_str(), "w");
            if (!_file) {
                showError(ErrorCode::PutOpenFailed);
            } else {
                _state = State::PutFile;
            }
        }
    } else if (array[0] == "rm") {
        if (array.size() < 2) {
            showError(ErrorCode::FileNameRequired);
        } else {
            if (!m8r::FS::sharedFS()->remove(array[1].c_str())) {
                showError(ErrorCode::RemoveFailed);
            } else {
                _state = State::NeedPrompt;
                sendString(ROMSTR("removed file\n"));
            }
        }
    } else if (array[0] == "dev") {
        if (array.size() < 2) {
            showError(ErrorCode::DeviceNameRequired);
        } else {
            Application::NameValidationType type = Application::validateBonjourName(array[1].c_str());

            if (type == Application::NameValidationType::BadLength) {
                showError(ErrorCode::BadDeviceNameLength);
            } else if (type == Application::NameValidationType::InvalidChar) {
                showError(ErrorCode::IllegalDeviceName);
            } else {
                _output->setDeviceName(array[1].c_str());
                _state = State::NeedPrompt;
                sendString(ROMSTR("set dev name\n"));
            }
        }
    } else if (array[0] == "format") {
        m8r::FS::sharedFS()->format();
        _state = State::NeedPrompt;
        sendString(ROMSTR("formatted FS\n"));
    } else if (array[0] == "erase") {
        m8r::FS* fs = m8r::FS::sharedFS();
        m8r::DirectoryEntry* dir = fs->directory();
        while (dir->valid()) {
            if (strcmp(dir->name(), ".userdata") != 0) {
                fs->remove(dir->name());
            }
            dir->next();
        }
        _state = State::NeedPrompt;
        sendString(ROMSTR("erased all files\n"));
    } else if (array[0] == "quit") {
        return false;
    }
    return true;
}

static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR ReadFailed[ ] = "file read failed";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR BinaryPutTooLarge[ ] = "binary 'put' too large";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR BinaryEncodeFailed[ ] = "binary encode failed";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR BinaryDecodeFailed[ ] = "binary decode failed";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR FileNameRequired[ ] = "filename required";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR DeviceNameRequired[ ] = "device name required";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR GetOpenFailed[ ] = "could not open file for 'get'";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR PutOpenFailed[ ] = "could not open file for 'put'";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR RemoveFailed[ ] = "could not remove file";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR BadDeviceNameLength[ ] = "device name must be between 1 and 31 characters";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR IllegalDeviceName[ ] = "illegal character (only numbers, lowercase letters and hyphen)";
static const char ICACHE_RODATA_ATTR ICACHE_STORE_ATTR E[ ] = "error:";

void Shell::showError(ErrorCode code)
{
    const char* msg = nullptr;
    
    switch(code) {
        case ErrorCode::ReadFailed: {
            msg = ReadFailed;
            break;
        }
        case ErrorCode::BinaryPutTooLarge: {
            msg = BinaryPutTooLarge;
            break;
        }
        case ErrorCode::BinaryEncodeFailed: {
            msg = BinaryEncodeFailed;
            break;
        }
        case ErrorCode::BinaryDecodeFailed: {
            msg = BinaryDecodeFailed;
            break;
        }
        case ErrorCode::FileNameRequired: {
            msg = FileNameRequired;
            break;
        }
        case ErrorCode::DeviceNameRequired: {
            msg = DeviceNameRequired;
            break;
        }
        case ErrorCode::GetOpenFailed: {
            msg = GetOpenFailed;
            break;
        }
        case ErrorCode::PutOpenFailed: {
            msg = PutOpenFailed;
            break;
        }
        case ErrorCode::RemoveFailed: {
            msg = RemoveFailed;
            break;
        }
        case ErrorCode::BadDeviceNameLength: {
            msg = BadDeviceNameLength;
            break;
        }
        case ErrorCode::IllegalDeviceName: {
            msg = IllegalDeviceName;
            break;
        }
        default: return;
    }
    
    _state = State::NeedPrompt;
    
    char* p = strcpy_rom(_buffer, E);
    p = strcpy_rom(p, msg);
    p = strcpy_rom(p, "\n");
    
    _output->shellSend(_buffer);
}

void Shell::sendString(const char* s)
{
    strcpy_rom(_buffer, s);
    _output->shellSend(_buffer);
}


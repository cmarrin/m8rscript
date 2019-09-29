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
#include "Base64.h"
#include "Error.h"
#include <cstdarg>
#include <string>

using namespace m8r;

#define Prompt "\n>"

void Shell::connected()
{
    init();
}

void Shell::init()
{
    if (_directoryEntry) {
        delete _directoryEntry;
        _directoryEntry = nullptr;
    }
    _state = State::Init;
    _binary = true;
    if (_file) {
        delete _file;
        _file = nullptr;
    }
    sendComplete();
}

const ErrorList* Shell::load(const char* filename, bool debug)
{
    Error error;
    if (!_application->load(error, debug, filename)) {
        error.showError(_application->system());
        return _application->syntaxErrors();
    }
    if (!_application->program()) {
        showMessage(MessageType::Error, ROMSTR("failed to compile application"));
        return _application->syntaxErrors();
    }
    return nullptr;
}

void Shell::run(std::function<void()> function)
{
    if (!_application->program()) {
        showMessage(MessageType::Error, ROMSTR("no application to run"));
        return;
    }
    _application->run(function);
}

void Shell::send(const char* data, uint16_t size)
{
    shellSend(data, size);
}

bool Shell::received(const char* data, uint16_t size)
{
#ifdef MONITOR_TRAFFIC
    if (!size) {
        size = strlen(data);
    }
    char* s = new char[size + 1];
    memcpy(s, data, size);
    s[size] = '\0';
    debugf("[Shell] <<<< received:'%s'\n", s);
    delete [ ] s;
#endif
    if (_state == State::PutFile) {
        if (_binary) {
            // Process line by line
            const char* p = data;
            uint16_t remainingSize = size;
            
            while (1) {
                if (remainingSize >= 1 && *p == '\04') {
                    delete _file;
                    _file = nullptr;
                    _state = State::NeedPrompt;
                    sendComplete();
                    _application->system()->printf(ROMSTR("done\n"));
                    return true;
                }
                
                _application->system()->printf(ROMSTR("."));

                const char* next = reinterpret_cast<const char*>(memchr(p, '\n', remainingSize));
                if (!next) {
                    // Incomplete line. Save it for next time
                    assert(remainingSize < BufferSize);
                    memcpy(_buffer, p, remainingSize);
                    _remainingReceivedDataSize = remainingSize;
                    return true;
                }
                
                size_t lineSize = next - p + 1;
                
                char* prependedBuffer = nullptr;
                size_t prependedLineSize = 0;
                
                if (_remainingReceivedDataSize) {
                    // Prepend data from previous buffer
                    prependedLineSize = lineSize + _remainingReceivedDataSize;
                    prependedBuffer = new char[prependedLineSize];
                    memcpy(prependedBuffer, _buffer, _remainingReceivedDataSize);
                    memcpy(prependedBuffer + _remainingReceivedDataSize, p, lineSize);
                    _remainingReceivedDataSize = 0;
                }
            
                // If line is too large, error
                if (lineSize > BufferSize) {
                    showMessage(MessageType::Error, ROMSTR("binary 'put' too large"));
                }

                int length = Base64::decode(prependedBuffer ? prependedLineSize : lineSize, prependedBuffer ?: p, BufferSize, reinterpret_cast<uint8_t*>(_buffer));
                if (prependedBuffer) {
                    delete [ ] prependedBuffer;
                }
                
                if (length < 0) {
                    delete _file;
                    _file = nullptr;
                    showMessage(MessageType::Error, ROMSTR("binary decode failed"));
                    return false;
                }
                _file->write(_buffer, length);
                if (!next) {
                    break;
                }
                p += lineSize;
                remainingSize -= lineSize;
            }
        } else {
            if (size <= 1) {
                delete _file;
                _file = nullptr;
                _state = State::NeedPrompt;
                sendComplete();
                return true;
            }
            _file->write(data, size);
        }
        return true;
    }
    std::vector<m8r::String> array = m8r::String(data, size).trim().split(" ", true);
    return executeCommand(array);
}

void Shell::sendComplete()
{
    switch(_state) {
        case State::Init:
            _state = State::NeedPrompt;
            showMessage(MessageType::Info, ROMSTR("\nWelcome to m8rscript\n"));
            break;
        case State::NeedPrompt:
            _state = State::ShowingPrompt;
            showMessage(MessageType::Info, ROMSTR(Prompt));
            break;
        case State::ShowingPrompt:
            break;
        case State::ListFiles:
            if (_directoryEntry && _directoryEntry->valid()) {
                if (_binary) {
                    ROMsnprintf(_buffer, BufferSize - 1, ROMSTR("file:%s:%d\n"), _directoryEntry->name(), _directoryEntry->size());
                } else {
                    ROMsnprintf(_buffer, BufferSize - 1, ROMSTR("    %-32s %d\n"), _directoryEntry->name(), _directoryEntry->size());
                }
                _directoryEntry->next();
                send(_buffer);
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
                send("\04");
                break;
            }
            if (_binary) {
                char binaryBuffer[StackAllocLimit];
                int32_t result = _file->read(binaryBuffer, StackAllocLimit);
                if (result < 0) {
                    showMessage(MessageType::Error, ROMSTR("file read failed (%d)"), result);
                    delete _file;
                    _file = nullptr;
                    break;
                }
                if (result < StackAllocLimit) {
                    delete _file;
                    _file = nullptr;
                }
                int length = Base64::encode(result, reinterpret_cast<uint8_t*>(binaryBuffer), BufferSize, _buffer);
                if (length < 0) {
                    showMessage(MessageType::Error, ROMSTR("binary encode failed"));
                    delete _file;
                    _file = nullptr;
                    break;
                }
                _buffer[length++] = '\r';
                _buffer[length++] = '\n';
                send(_buffer, length);
            } else {
                int32_t result = _file->read(_buffer, BufferSize);
                if (result < 0) {
                    showMessage(MessageType::Error, ROMSTR("file read failed (%d)"), result);
                    break;
                }
                if (result < BufferSize) {
                    delete _file;
                    _file = nullptr;
                }
                send(_buffer, result);
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
        _directoryEntry = _application->fileSystem()->directory();
        _state = State::ListFiles;
        sendComplete();
    } else if (array[0] == "t") {
        _binary = false;
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("text: Setting text transfer mode\n"));
    } else if (array[0] == "b") {
        _binary = true;
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("binary: Setting binary transfer mode\n"));
    } else if (array[0] == "get") {
        if (array.size() < 2) {
            showMessage(MessageType::Error, ROMSTR("filename required"));
        } else {
            _file = _application->fileSystem()->open(array[1].c_str(), "r");
            if (!_file) {
                showMessage(MessageType::Error, ROMSTR("could not open file for 'get'"));
            } else {
                _state = State::GetFile;
                sendComplete();
            }
        }
    } else if (array[0] == "put") {
        if (array.size() < 2) {
            showMessage(MessageType::Error, ROMSTR("filename required"));
        } else {
            _file = _application->fileSystem()->open(array[1].c_str(), "w");
            if (!_file) {
                showMessage(MessageType::Error, ROMSTR("could not open file for 'put'"));
            } else {
                _application->system()->printf(ROMSTR("Put '%s'"), array[1].c_str());
                _state = State::PutFile;
            }
        }
    } else if (array[0] == "rm") {
        if (array.size() < 2) {
            showMessage(MessageType::Error, ROMSTR("filename required"));
        } else {
            if (!_application->fileSystem()->remove(array[1].c_str())) {
                showMessage(MessageType::Error, ROMSTR("could not remove file"));
            } else {
                _state = State::NeedPrompt;
                showMessage(MessageType::Info, ROMSTR("removed file\n"));
            }
        }
    } else if (array[0] == "mv") {
        if (array.size() < 3) {
            showMessage(MessageType::Error, ROMSTR("source and destination filenames required"));
        } else {
            if (!_application->fileSystem()->rename(array[1].c_str(), array[2].c_str())) {
                showMessage(MessageType::Error, ROMSTR("could not rename file"));
            } else {
                _state = State::NeedPrompt;
                showMessage(MessageType::Info, ROMSTR("renamed file\n"));
            }
        }
    } else if (array[0] == "heap") {
        _state = State::NeedPrompt;
        MemoryInfo info;
        Object::memoryInfo(info);
        
        uint32_t numOth = info.numAllocations;
        uint32_t numObj = info.numAllocationsByType[static_cast<uint32_t>(MemoryType::Object)];
        uint32_t numStr = info.numAllocationsByType[static_cast<uint32_t>(MemoryType::String)];
        numOth -= numObj + numStr;
        
        if (_binary) {
            showMessage(MessageType::Info, ROMSTR("heap:%d:%d:%d:%d\n"), info.freeSize, numObj, numStr, numOth);
        } else {
            showMessage(MessageType::Info, ROMSTR("heap size: %d, numObj: %d, numStr: %d, numOth: %d\n"), info.freeSize, numObj, numStr, numOth);
        }
    } else if (array[0] == "dev") {
        if (array.size() < 2) {
            showMessage(MessageType::Error, ROMSTR("device name required"));
        } else {
            Application::NameValidationType type = Application::validateBonjourName(array[1].c_str());

            if (type == Application::NameValidationType::BadLength) {
                showMessage(MessageType::Error, ROMSTR("device name must be between 1 and 31 characters"));
            } else if (type == Application::NameValidationType::InvalidChar) {
                showMessage(MessageType::Error, ROMSTR("illegal character (only numbers, lowercase letters and hyphen)"));
            } else {
                _application->system()->setDeviceName(array[1].c_str());
                _state = State::NeedPrompt;
                showMessage(MessageType::Info, ROMSTR("set dev name\n"));
            }
        }
    } else if (array[0] == "format") {
        _application->fileSystem()->format();
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("formatted FS\n"));
    } else if (array[0] == "erase") {
        m8r::DirectoryEntry* dir = _application->fileSystem()->directory();
        while (dir->valid()) {
            if (strcmp(dir->name(), ".userdata") != 0) {
                _application->fileSystem()->remove(dir->name());
            }
            dir->next();
        }
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("erased all files\n"));
    } else if (array[0] == "run") {
        load((array.size() < 2) ? nullptr : array[1].c_str(), _debug);
        m8r::Application* app = _application;
        run([app]{
            app->system()->printf(ROMSTR("\n***** Program Finished *****\n\n"));
        });
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("Program started...\n"));
    } else if (array[0] == "stop") {
        stop();
        _state = State::NeedPrompt;
    } else if (array[0] == "build") {
        const m8r::ErrorList* errors = load((array.size() < 2) ? nullptr : array[1].c_str(), _debug);
        if (errors) {
            for (const auto& error : *errors) {
                showMessage(MessageType::Info, ROMSTR("%s::%d::%d::%d\n"), error._description, error._lineno, error._charno, error._length);
            }
        }
        _state = State::NeedPrompt;
    } else if (array[0] == "debug") {
        _debug = true;
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("Debug true\n"));
    } else if (array[0] == "nodebug") {
        _debug = false;
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("Debug false\n"));
    } else if (array[0] == "clear") {
        _application->clear();
        _state = State::NeedPrompt;
        showMessage(MessageType::Info, ROMSTR("Application cleared\n"));
    } else if (array[0] == "quit") {
        return false;
    } else {
        _state = State::NeedPrompt;
        showMessage(MessageType::Error, ROMSTR("unrecognized command"));
    }
    return true;
}

void Shell::showMessage(MessageType type, const char* msg, ...)
{
    if (type == MessageType::Error) {
        _state = State::NeedPrompt;
    }
    
    size_t size = BufferSize - 10;
    char* p = _buffer;
    if (type == MessageType::Error) {
        p = ROMCopyString(p, ROMSTR("Error:"));
    }
    
    va_list args;
    va_start(args, msg);
    
    int result = ROMvsnprintf(p, size, msg, args);
    if (result < 0 || result >= size) {
        return;
    }
    
    size -= static_cast<size_t>(result);
    p += result;
    
    if (type == MessageType::Error) {
        ROMCopyString(p, "\n");
    }
    
    send(_buffer);
}

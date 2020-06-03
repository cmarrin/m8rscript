/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "ExecutionUnit.h"
#include "SystemInterface.h"
#include "Task.h"
#include "Terminal.h"

namespace m8r {

class Program;
class Error;
class ExecutionUnit;
class FS;

class Application {
public:
    Application(uint16_t port);
    ~Application();
        
    bool runOneIteration();
    String autostartFilename() const;
    
    void receivedData(const String& data, KeyAction action)
    {
        if (_autostartTask) {
            _autostartTask->receivedData(data, action);
        }
    }

    enum class NameValidationType { Ok, BadLength, InvalidChar };
    NameValidationType validateBonjourName(const char* name);
    
    const char* shellName() { return "/sys/bin/mrsh"; }
    
    bool mountFileSystem();
    
    static SystemInterface* system() { assert(_system); return _system; }

private:
    std::shared_ptr<Task> _autostartTask;
    std::unique_ptr<Terminal> _terminal;

    static SystemInterface* _system;
};
    
}

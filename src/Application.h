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
//#include "Heartbeat.h"
#include "SystemInterface.h"
#include "Task.h"
#include "Telnet.h"
#include <functional>

namespace m8r {

class Program;
class Error;
class ExecutionUnit;
class FS;
class Shell;
class TCP;

class Application {
public:
    Application(uint16_t port);
    ~Application();
        
    void runLoop();
    String autostartFilename() const;

    enum class NameValidationType { Ok, BadLength, InvalidChar };
    NameValidationType validateBonjourName(const char* name);
    
    const char* shellName() { return "/sys/bin/mrsh"; }
    
    bool mountFileSystem();
    
    static SystemInterface* system() { assert(_system); return _system; }

private:
    Mad<TCP> _shellSocket;
    
    //Heartbeat _heartbeat;
    Mad<Task> _autostartTask;

    struct ShellEntry
    {
        ShellEntry() { }
        Mad<Task> task;
        Telnet telnet;
    };
    
    ShellEntry _shells[TCP::MaxConnections];
    
    static SystemInterface* _system;
};
    
}

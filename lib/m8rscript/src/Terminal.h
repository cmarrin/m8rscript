/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Task.h"
#include "Telnet.h"

namespace m8r {

class Terminal {
public:
    Terminal(uint16_t port, const char* command);
    ~Terminal();

private:
    Mad<TCP> _socket;

    struct Entry
    {
        Entry() { }
        Mad<Task> task;
        Telnet telnet;
    };
    
    Entry _shells[TCP::MaxConnections];
    String _command;
};
    
}

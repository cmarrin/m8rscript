/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Base64.h"
#include "MFS.h"
#include "GPIO.h"
#include "Iterator.h"
#include "JSON.h"
#include "Object.h"
#include "SystemTime.h"
#include "Task.h"
#include "Timer.h"
#include "TCP.h"

namespace m8r {

class Function;

class Global : public StaticObject {
public:
    Global();

    static Base64 _base64;
    static GPIO _gpio;
    static JSON _json;
    static TCPProto _tcp;
    static UDPProto _udp;
    static IPAddrProto _ipAddr;
    static Iterator _iterator;
    static TaskProto _task;
    static TimerProto _timer;
    static FSProto _fs;
    static FileProto _file;
    static DirectoryProto _directory;

    static CallReturnValue currentTime(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue delay(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue print(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue printf(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue println(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toFloat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue toUInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue arguments(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue import(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue importString(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue waitForEvent(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue meminfo(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    static const Global* shared() { return &_global; }
    
private:
    static Global _global;
};
    
}

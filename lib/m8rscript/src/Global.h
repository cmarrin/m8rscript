/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "FSProto.h"
#include "GPIO.h"
#include "IPAddrProto.h"
#include "Iterator.h"
#include "JSONProto.h"
#include "Object.h"
#include "SystemTime.h"
#include "TaskProto.h"
#include "TimerProto.h"
#include "TCPProto.h"

namespace m8rscript {

class Function;

class Global : public StaticObject {
public:
    Global();

    static GPIO _gpio;
    static JSONProto _json;
    static TCPProto _tcp;
    static IPAddrProto _ipAddr;
    static Iterator _iterator;
    static TaskProto _task;
    static TimerProto _timer;
    static FSProto _fs;
    static FileProto _file;
    static DirectoryProto _directory;

    static m8r::CallReturnValue currentTime(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue delay(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue print(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue println(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue toFloat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue toInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue toUInt(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue arguments(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue import(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue importString(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue waitForEvent(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue meminfo(ExecutionUnit*, Value thisValue, uint32_t nparams);
    
    static const Global* shared() { return &_global; }
    
private:
    static Global _global;
};
    
}

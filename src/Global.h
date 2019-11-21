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
#include "FS.h"
#include "GPIO.h"
#include "Iterator.h"
#include "JSON.h"
#include "Object.h"
#include "SystemTime.h"
#include "Task.h"
#include "TCP.h"

namespace m8r {

class Function;

class Global : public ObjectFactory {
public:
    Global(Mad<Program>);
    virtual ~Global();

private:
    MaterObject _object;
    MaterObject _array;
    
    Base64 _base64;
    GPIO _gpio;
    JSON _json;
    TCPProto _tcp;
    UDPProto _udp;
    IPAddrProto _ipAddr;
    Iterator _iterator;
    TaskProto _task;
    FSProto _fs;
    FileProto _file;
    DirectoryProto _directory;

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
};
    
}

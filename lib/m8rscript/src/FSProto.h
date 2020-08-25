/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Object.h"

namespace m8rscript {

class FileProto : public StaticObject {
public:
    FileProto();

    static m8r::CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue close(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue read(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue write(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue seek(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue tell(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue eof(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue type(ExecutionUnit*, Value thisValue, uint32_t nparams);
};    

class DirectoryProto : public StaticObject {
public:
    DirectoryProto();

    static m8r::CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue name(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue size(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue type(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue next(ExecutionUnit*, Value thisValue, uint32_t nparams);
};    

class FSProto : public StaticObject {
public:
    FSProto();
        
    static m8r::CallReturnValue mount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue mounted(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue unmount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue format(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue open(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue openDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue makeDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue remove(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue rename(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue stat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue lastError(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue errorString(ExecutionUnit*, Value thisValue, uint32_t nparams);

    static m8r::String findPath(ExecutionUnit*, const m8r::String& filename, const m8r::Mad<Object>& env);
};

}

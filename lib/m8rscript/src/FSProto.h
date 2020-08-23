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

namespace m8r {

class FileProto : public StaticObject {
public:
    FileProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue close(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue read(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue write(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue seek(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue tell(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue eof(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue type(ExecutionUnit*, Value thisValue, uint32_t nparams);
};    

class DirectoryProto : public StaticObject {
public:
    DirectoryProto();

    static CallReturnValue constructor(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue name(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue size(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue type(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue valid(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue error(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue next(ExecutionUnit*, Value thisValue, uint32_t nparams);
};    

class FSProto : public StaticObject {
public:
    FSProto();
        
    static CallReturnValue mount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue mounted(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static CallReturnValue unmount(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue format(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue open(ExecutionUnit* eu, Value thisValue, uint32_t nparams);
    static CallReturnValue openDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue makeDirectory(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue remove(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue rename(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stat(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue lastError(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue errorString(ExecutionUnit*, Value thisValue, uint32_t nparams);

    static String findPath(ExecutionUnit*, const String& filename, const Mad<Object>& env);
};

}

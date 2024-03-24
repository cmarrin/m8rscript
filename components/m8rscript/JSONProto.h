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
    class Scanner;
}

namespace m8rscript {

class JSONProto : public StaticObject {
public:
    JSONProto();
    
    static Value parse(ExecutionUnit* eu, const m8r::String& json);
    static m8r::String stringify(ExecutionUnit* eu, const Value);

    static Value value(ExecutionUnit* eu, m8r::Scanner& scanner);
    static bool propertyAssignment(ExecutionUnit* eu, m8r::Scanner&, Value& key, Value& value);

    static m8r::CallReturnValue parseFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static m8r::CallReturnValue stringifyFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}

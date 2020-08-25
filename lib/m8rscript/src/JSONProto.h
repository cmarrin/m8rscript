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
    
    static Value parse(ExecutionUnit* eu, const String& json);
    static String stringify(ExecutionUnit* eu, const Value);

    static Value value(ExecutionUnit* eu, Scanner& scanner);
    static bool propertyAssignment(ExecutionUnit* eu, Scanner&, Value& key, Value& value);

    static CallReturnValue parseFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue stringifyFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
};

}

/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "StreamProto.h"

using namespace m8r;

StreamProto::StreamProto(Program* program, ObjectFactory* parent)
    : ObjectFactory(program, SA::Iterator, parent, constructor)
{
    addProperty(program, SA::eof, eof);
    addProperty(program, SA::read, read);
    addProperty(program, SA::write, write);
}

CallReturnValue StreamProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // If there are 2 params, they are a filename and open mode (r, r+, w, w+, a, a+)
    // for a FileStream
    // If there is one param, it is a string for a StringStream.
    if (nparams < 1) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue StreamProto::eof(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue StreamProto::read(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue StreamProto::write(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

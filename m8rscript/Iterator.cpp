/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
	  
    - Redistributions in binary form must reproduce the above copyright 
	  notice, this list of conditions and the following disclaimer in the 
	  documentation and/or other materials provided with the distribution.
	  
    - Neither the name of the <ORGANIZATION> nor the names of its 
	  contributors may be used to endorse or promote products derived from 
	  this software without specific prior written permission.
	  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "Iterator.h"

#include "ExecutionUnit.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

Iterator::Iterator(Program* program)
    : _next(next)
{
    _nextAtom = program->atomizeString(ROMSTR("next"));    
    _endAtom = program->atomizeString(ROMSTR("end"));    
    _valueAtom = program->atomizeString(ROMSTR("value"));    
    
    program->addObject(this, false);
    program->addObject(&_next, false);
}

CallReturnValue Iterator::call(ExecutionUnit* eu, uint32_t nparams)
{
    _object = (nparams >= 1) ? eu->stack().top(1 - nparams) : Value();
    _index = 0;
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue Iterator::next(ExecutionUnit* eu, uint32_t nparams)
{
    // FIXME: Implement
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

const Value Iterator::property(ExecutionUnit* eu, const Atom& prop) const
{
    if (prop == _nextAtom) {
        return Value(_next.objectId());
    }
    if (prop == _endAtom) {
        Object* obj = eu->program()->obj(_object);
        
        // FIXME: Implement
        return Value(obj ? true : true);
    }
    if (prop == _valueAtom) {
        Object* obj = eu->program()->obj(_object);
        
        // FIXME: Implement
        return Value(obj ? 0 : 1);
    }
    return Value();
}


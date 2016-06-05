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

#include "ExecutionUnit.h"
#include <cassert>

using namespace m8r;

uint32_t ExecutionUnit::_nextID = 1;

inline uint8_t byteFromInt(uint32_t value, uint32_t index)
{
    assert(index < 4);
    return (reinterpret_cast<uint8_t*>(&value))[index];
}

void ExecutionUnit::addCode(const char* s)
{
    uint32_t len = static_cast<uint32_t>(strlen(s)) + 1;
    if (len < 16) {
        _code.push_back(static_cast<uint8_t>(Op::PUSHS) | static_cast<uint8_t>(len));
    } else if (len < 256) {
        _code.push_back(static_cast<uint8_t>(Op::PUSHSX1));
        _code.push_back(len);
    } else {
        assert(len < 65536);
        _code.push_back(static_cast<uint8_t>(Op::PUSHSX2));
        _code.push_back(byteFromInt(len, 1));
        _code.push_back(byteFromInt(len, 0));
    }
    for (const char* p = s; *p != '\0'; ++p) {
        _code.push_back(*p);
    }
    _code.push_back('\0');
}

void ExecutionUnit::addCode(uint32_t)
{
}

void ExecutionUnit::addCode(float)
{
}

void ExecutionUnit::addCode(const Atom&)
{
}

void ExecutionUnit::addCode(Op)
{
}

void ExecutionUnit::addCode(Op, uint32_t)
{
}

void ExecutionUnit::addCode(ExecutionUnit*)
{
}


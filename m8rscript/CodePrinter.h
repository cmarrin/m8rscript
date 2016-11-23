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

#pragma once

#include <cstdint>

#include "Atom.h"
#include "Function.h"
#include "Program.h"

namespace m8r {

class Parser;
class Function;
class Program;
class SystemInterface;

class CodePrinter {
public:
    CodePrinter(SystemInterface* system = nullptr) : _system(system) { }
    
    m8r::String generateCodeString(const Program* program) const;
    
private:    
    Value* valueFromId(Atom, const Object*) const;
    Value deref(Program*, Object*, const Value&);
    bool deref(Program*, Value&, const Value&);
    Atom propertyNameFromValue(Program*, const Value&);

    m8r::String generateCodeString(const Program*, const Object*, const char* functionName, uint32_t nestingLevel) const;

    struct Annotation {
        uint32_t addr;
        uint32_t uniqueID;
    };
    typedef std::vector<Annotation> Annotations;

    uint32_t findAnnotation(uint32_t addr) const;
    void preamble(m8r::String& s, uint32_t addr) const;
    static const char* stringFromOp(Op op);
    void indentCode(m8r::String&) const;
    mutable uint32_t _nestingLevel = 0;
    mutable Annotations annotations;
    
    mutable uint32_t _nerrors = 0;
    SystemInterface* _system;
};
    
}

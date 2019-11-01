/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>

#include "Atom.h"
#include "Value.h"

namespace m8r {

class Object;
class Parser;
class Function;
class Program;

class CodePrinter {
public:
    CodePrinter() { }
    
    m8r::String generateCodeString(const Program* program) const;
    
private:   
    String regString(const Program*, const Function*, uint32_t reg, bool up = false) const;
    void generateXXX(m8r::String&, uint32_t addr, Op op) const;
    void generateRXX(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, uint32_t d) const;
    void generateRRX(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, uint32_t d, uint32_t s) const;
    void generateRUX(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, uint32_t d, uint32_t s) const;
    void generateURX(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, uint32_t d, uint32_t s) const;
    void generateRRR(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, uint32_t d, uint32_t s1, uint32_t s2) const;
    void generateXN(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, int32_t n) const;
    void generateRN(const Program*, const Function*, m8r::String&, uint32_t addr, Op op, uint32_t d, int32_t n) const;
    void generateCall(const Program*, const Function*, m8r::String& str, uint32_t addr, Op op, uint32_t rcall, uint32_t rthis, int32_t nparams) const;
 
    void showConstant(const Program*, m8r::String&, const Value&, bool abbreviated = false) const;
    Value* valueFromId(Atom, const Object*) const;
    Value deref(Program*, Object*, const Value&);
    bool deref(Program*, Value&, const Value&);
    Atom propertyNameFromValue(Program*, const Value&);

    m8r::String generateCodeString(const Program*, const Object*, const char* functionName, uint32_t nestingLevel) const;

    struct Annotation {
        uint32_t addr;
        uint32_t uniqueID;
    };
    using Annotations = std::vector<Annotation>;

    uint32_t findAnnotation(uint32_t addr) const;
    void preamble(m8r::String& s, uint32_t addr) const;
    static const char* stringFromOp(Op op);
    void indentCode(m8r::String&) const;
    mutable uint32_t _nestingLevel = 0;
    mutable Annotations annotations;
    
    mutable uint32_t _nerrors = 0;
    
    mutable int32_t _lineno = -1;
    mutable int32_t _emittedLineNumber = -1;
};
    
}

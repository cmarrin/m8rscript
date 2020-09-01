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
#include "MachineCode.h"
#include "Value.h"

namespace m8rscript {

class Object;
class Parser;
class Function;
class Program;

class CodePrinter {
public:
    CodePrinter() { }
    
    m8r::String generateCodeString(const ExecutionUnit*) const;
    
    bool hasErrors() const { return _nerrors > 0; }
    
private:
    using EnumerationFunction = std::function<void(Op, uint8_t imm, uint32_t pc)>;
    
    uint8_t regOrConst(const m8r::Mad<Object> func, const uint8_t*& code, Value& constant) const;
    m8r::String regString(const ExecutionUnit*, const m8r::Mad<Object>, const uint8_t*& code, bool up = false) const;
    m8r::String regString(const ExecutionUnit*, const m8r::Mad<Object>, uint8_t r, const Value& constant, bool up = false) const;
 
    void showConstant(const ExecutionUnit*, m8r::String&, const Value&, bool abbreviated = false) const;
    Value* valueFromId(m8r::Atom, const m8r::Mad<Object>) const;
    Value deref(m8r::Mad<Program>, m8r::Mad<Object>, const Value&);
    bool deref(m8r::Mad<Program>, Value&, const Value&);
    m8r::Atom propertyNameFromValue(m8r::Mad<Program>, const Value&);

    m8r::String generateCodeString(const ExecutionUnit*, const m8r::Mad<Function>, const char* functionName) const;
    bool enumerateCode(const ExecutionUnit*, const m8r::Mad<Function>, EnumerationFunction) const;
    
    void advanceAddr(Op, const uint8_t*& code) const;

    struct Annotation {
        uint32_t addr;
        uint32_t uniqueID;
    };
    
    using Annotations = m8r::Vector<Annotation>;

    uint32_t findAnnotation(uint32_t addr) const;
    void preamble(m8r::String& s, uint32_t addr, bool indent = true) const;
    static const char* stringFromOp(Op op);
    void indentCode(m8r::String&) const;
    
    mutable uint32_t _nestingLevel = 0;
    mutable Annotations _annotations;
    mutable uint32_t _nerrors = 0;
    mutable int32_t _lineno = -1;
    mutable int32_t _emittedLineNumber = -1;
};
    
}

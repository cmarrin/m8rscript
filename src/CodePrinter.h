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
    
    m8r::String generateCodeString(const Mad<Program> program) const;
    
private:
    uint8_t regOrConst(const Mad<Object> func, const uint8_t*& code, Value& constant) const;
    String regString(const Mad<Program>, const Mad<Object>, const uint8_t*& code, bool up = false) const;
    String regString(const Mad<Program>, const Mad<Object>, uint8_t r, const Value& constant, bool up = false) const;
    void generateXXX(m8r::String&, uint32_t addr, Op op) const;
    void generateRXX(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateRRX(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateRUX(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateURX(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateRRR(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateRParams(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateRRParams(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateJumpAddr(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
    void generateRJumpAddr(const Mad<Program>, const Mad<Object>, m8r::String&, uint32_t addr, Op op, const uint8_t*& code) const;
 
    void showConstant(const Mad<Program>, m8r::String&, const Value&, bool abbreviated = false) const;
    Value* valueFromId(Atom, const Mad<Object>) const;
    Value deref(Mad<Program>, Mad<Object>, const Value&);
    bool deref(Mad<Program>, Value&, const Value&);
    Atom propertyNameFromValue(Mad<Program>, const Value&);

    m8r::String generateCodeString(const Mad<Program>, const Mad<Function>, const char* functionName, uint32_t nestingLevel) const;

    struct Annotation {
        uint32_t addr;
        uint32_t uniqueID;
    };
    using Annotations = Vector<Annotation>;

    uint32_t findAnnotation(uint32_t addr) const;
    void preamble(m8r::String& s, uint32_t addr, bool indent = true) const;
    static const char* stringFromOp(Op op);
    void indentCode(m8r::String&) const;
    mutable uint32_t _nestingLevel = 0;
    mutable Annotations annotations;
    
    mutable uint32_t _nerrors = 0;
    
    mutable int32_t _lineno = -1;
    mutable int32_t _emittedLineNumber = -1;
};
    
}

/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "ExecutionUnit.h"
#include "Scanner.h"
#include "SystemInterface.h"

namespace m8rscript {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Parser
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Parser  {
    friend class ParseEngine;
    
public:
    Parser(m8r::Mad<Program> = m8r::Mad<Program>());
    
    ~Parser();
        
    enum class Debug { None, Full };
    // Debugging variable set according to NDEBUG compile time flag
    #ifndef NDEBUG
        static constexpr Debug debug = Parser::Debug::Full;
    #else
        static constexpr Debug debug = Parser::Debug::None;
    #endif
    
    m8r::Mad<Function> parse(const m8r::Stream& stream, ExecutionUnit*, Debug, m8r::Mad<Function> parent = m8r::Mad<Function>());

	void recordError(const char* format, ...);
    m8r::ParseErrorList& syntaxErrors() { return _syntaxErrors; }

    uint32_t nerrors() const { return static_cast<uint32_t>(_syntaxErrors.size()); }
    m8r::Mad<Program> program() const { return _program; }
    
    m8r::String stringFromAtom(const m8r::Atom& atom) const { return _program->stringFromAtom(atom); }
    m8r::Atom atomizeString(const char* s) const { return _program->atomizeString(s); }

    StringLiteral startString() { return _program->startStringLiteral(); }
    void addToString(char c) { _program->addToStringLiteral(c); }
    void endString() { _program->endStringLiteral(); }
    
private:
    class RegOrConst
    {
    public:
        enum class Type { Reg, Constant };
        
        RegOrConst() { }
        explicit RegOrConst(uint8_t reg) : _reg(reg), _type(Type::Reg) { assert(reg <= MaxRegister); }
        explicit RegOrConst(ConstantId id, m8r::Atom atom = m8r::Atom()) : _reg(id.raw()), _type(Type::Constant), _atom(atom) { assert(id.raw() <= MaxRegister); }
        
        bool operator==(const RegOrConst& other) { return _reg == other._reg && _type == other._type && _atom == other._atom; }
        
        bool isReg() const { return _type == Type::Reg; }
        uint8_t index() const { return isReg() ? _reg : (_reg + MaxRegister + 1); }
        bool isShortAtom() const { return !isReg() && static_cast<BuiltinConstants>(_reg) == BuiltinConstants::AtomShort; }
        bool isLongAtom() const { return !isReg() && static_cast<BuiltinConstants>(_reg) == BuiltinConstants::AtomLong; }
        m8r::Atom atom() const { return _atom; }

        void push(m8r::Vector<uint8_t>* vec)
        {
            vec->push_back(index());
            if (isShortAtom()) {
                uint16_t a = atom().raw();
                assert(a < 256);
                vec->push_back(static_cast<uint8_t>(a));
            } else if (isLongAtom()) {
                uint16_t a = atom().raw();
                vec->push_back(static_cast<uint8_t>(a >> 8));
                vec->push_back(static_cast<uint8_t>(a));
            }
        }

    private:
        uint8_t _reg = static_cast<uint8_t>(BuiltinConstants::Undefined);
        Type _type = Type::Constant;
        m8r::Atom _atom;
    };

    // The next 3 functions work together:
    //
    // Label has a current location which is filled in by the label() call,
    // and a match location which is filled in by the addMatchedJump() function.
    // addMatchedJump also adds the passed Op (which can be JMP, JT or JF)
    // with an empty jump address, to be filled in my matchJump().
    // 
    // When matchJump() is called it adds a JMP to the current location in
    // the Label and then fixed up the match location with the location just
    // past the JMP
    //
    struct Label {
        int32_t label : 20;
        uint32_t uniqueID : 12;
        int32_t matchedAddr : 20;
    };
        
    Label label();
    void addMatchedJump(Op op, Label&);
    void matchJump(const Label& matchLabel)
    {
        int32_t jumpAddr = static_cast<int16_t>(_deferred ? _deferredCode.size() : currentCode().size()) - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }

    void matchJump(const Label& matchLabel, const Label& dstLabel)
    {
        int32_t jumpAddr = dstLabel.label - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }
    
    void matchJump(const Label& matchLabel, int16_t dstAddr)
    {
        int32_t jumpAddr = dstAddr - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }
    
    void doMatchJump(int32_t matchAddr, int32_t jumpAddr);
    void jumpToLabel(Op op, Label&);
    
    int32_t startDeferred()
    {
        assert(!_deferred);
        _deferred = true;
        _deferredCodeBlocks.push_back(_deferredCode.size());
        return static_cast<int32_t>(_deferredCode.size());
    }
    
    int32_t resumeDeferred()
    {
        assert(!_deferred);
        _deferred = true;
        return static_cast<int32_t>(_deferredCode.size());
    }
    
    void endDeferred() { assert(_deferred); _deferred = false; }
    int32_t emitDeferred();

    void functionAddParam(const m8r::Atom& atom);
    void functionStart(bool ctor);
    void functionParamsEnd();
    bool functionIsCtor() const { return _functions.back()._ctor; }
    m8r::Mad<Function> functionEnd();
    m8r::Mad<Function> currentFunction() const { assert(_functions.size()); return _functions.back()._function; }
    m8r::Vector<uint8_t>& currentCode() { assert(_functions.size()); return _functions.back()._code; }
    m8r::Vector<Value>& currentConstants() { assert(_functions.size()); return _functions.back()._constants; }

    void classStart() { _classes.push_back(Object::create<MaterObject>()); }
    void classEnd() { pushK(Value(static_cast<m8r::Mad<Object>>(_classes.back()))); _classes.pop_back(); }
    m8r::Mad<MaterObject> currentClass() const { assert(_classes.size()); return _classes.back(); }
        
    void pushK(const char* value);
    void pushK(const Value& value);
    void pushThis();

    void addNamedFunction(m8r::Mad<Function>, const m8r::Atom&);
    
    void pushTmp();
    
    enum class IdType : uint8_t { MustBeLocal, MightBeLocal, NotLocal };
    void emitId(const m8r::Atom& value, IdType);
    void emitId(const char*, IdType);

    void emitDup();
    void emitMove();
    
    enum class DerefType { Prop, Elt };
    RegOrConst emitDeref(DerefType);
    void emitAppendElt();
    void emitAppendProp();
    void emitUnOp(Op op);
    void emitBinOp(Op op);
    void emitCaseTest();
    void emitLoadLit(bool array);
    void emitPush();
    void emitPop();
    void emitEnd();
    
    void emitLineNumber()
    {
        if (_debug == Debug::None) {
            return;
        }
        uint16_t lineno = _scanner.lineno();
        if (lineno == _emittedLineNumber) {
            return;
        }
        _emittedLineNumber = lineno;
        emitCode(Op::LINENO, lineno);
    }
    
    void emitCallRet(Op value, RegOrConst thisReg, uint8_t params);
    void addVar(const m8r::Atom& name) { _functions.back().addLocal(name); }
    
    void discardResult();
    
    void retireToken() { _scanner.retireToken(); }
    
    RegOrConst addConstant(const Value& v);

    
    void addCode(Op, RegOrConst, RegOrConst, RegOrConst, uint16_t n);
    
    void emitCode(Op op, RegOrConst ra, RegOrConst rb, RegOrConst rc)   { addCode(op, ra, rb, rc, 0); }
    void emitCode(Op op, RegOrConst ra, RegOrConst rb, uint8_t nparams) { addCode(op, ra, rb, RegOrConst(), nparams); }
    void emitCode(Op op, RegOrConst ra, RegOrConst rb)                  { addCode(op, ra, rb, RegOrConst(), 0); }
    void emitCode(Op op, RegOrConst ra, uint8_t nparams)                { addCode(op, ra, RegOrConst(), RegOrConst(), nparams); }
    void emitCode(Op op, RegOrConst ra)                                 { addCode(op, ra, RegOrConst(), RegOrConst(), 0); }
    void emitCode(Op op, uint8_t nparams)                               { addCode(op, RegOrConst(), RegOrConst(), RegOrConst(), nparams); }
    void emitCode(Op op, RegOrConst ra, int16_t n)                      { addCode(op, ra, RegOrConst(), RegOrConst(), n); }
    void emitCode(Op op, int16_t n)                                     { addCode(op, RegOrConst(), RegOrConst(), RegOrConst(), n); }
    void emitCode(Op op, uint16_t n)                                    { addCode(op, RegOrConst(), RegOrConst(), RegOrConst(), n); }
    void emitCode(Op op)                                                { addCode(op, RegOrConst(), RegOrConst(), RegOrConst(), 0); }

    void reconcileRegisters(uint16_t localCount);
    
    class ParseStack {
    public:
        enum class Type { Unknown, Local, Constant, Register, RefK, PropRef, EltRef, This, UpValue };
        
        ParseStack(Parser* parser) : _parser(parser) { }
        
        void push(Type, RegOrConst reg);
        RegOrConst pushRegister();
        void pushConstant(RegOrConst reg) { assert(!reg.isReg()); push(Type::Constant, reg); }
        void setIsValue(bool b) { _stack.top()._isValue = b; }

        void pop();
        void swap();
        
        Type topType() const { return empty() ? Type::Unknown : _stack.top()._type; }
        RegOrConst topReg() const { return empty() ? RegOrConst() : _stack.top()._reg; }
        RegOrConst topDerefReg() const { return empty() ? RegOrConst() : _stack.top()._derefReg; }
        bool topIsValue() const { return empty() ? false : _stack.top()._isValue; }
        bool empty() const { return _stack.empty(); }
        void clear() { _stack.clear(); }
        
        RegOrConst bake(bool makeClosure = false);
        bool needsBaking() const { return _stack.top()._type == Type::PropRef || _stack.top()._type == Type::EltRef || _stack.top()._type == Type::RefK; }
        void replaceTop(Type, RegOrConst reg, RegOrConst derefReg);
        void dup() {
            Entry entry = _stack.top();
            _stack.push(entry);
        }
        void propRefToReg();
        
    private:
        struct Entry {
            
            Entry() { }
            
            Entry(Type type, RegOrConst reg, RegOrConst derefReg = RegOrConst())
                : _type(type)
                , _reg(reg)
                , _derefReg(derefReg)
            {
            }
            
            Type _type = Type::Unknown;
            RegOrConst _reg;
            RegOrConst _derefReg;
            bool _isValue = false;
        };
        
        m8r::Stack<Entry> _stack;
        Parser* _parser;
    };
    
    ParseStack _parseStack;

    struct FunctionEntry {
        FunctionEntry() { }
        FunctionEntry(m8r::Mad<Function> function, bool ctor) : _function(function), _ctor(ctor) { }
        m8r::Vector<uint8_t> _code;
        m8r::Vector<Value> _constants;
        m8r::Vector<m8r::Atom> _locals;
        m8r::Mad<Function> _function;
        uint8_t _nextReg = MaxRegister;
        uint8_t _minReg = MaxRegister + 1;
        bool _ctor = false;

        int16_t addLocal(const m8r::Atom& atom)
        {
            for (auto name : _locals) {
                if (name == atom) {
                    return -1;
                }
            }
            _locals.push_back(atom);
            return static_cast<int16_t>(_locals.size()) - 1;
        }

        int32_t localIndex(const m8r::Atom& name) const
        {
            for (int16_t i = 0; i < static_cast<int16_t>(_locals.size()); ++i) {
                if (_locals[i] == name) {
                    return i;
                }
            }
            return -1;
        }
        
        void markParamEnd() { _function->setFormalParamCount(static_cast<uint16_t>(_locals.size())); }
    };
        
    using FunctionEntryVector = m8r::Vector<FunctionEntry>;

    FunctionEntryVector _functions;
    
    m8r::Vector<m8r::Mad<MaterObject>> _classes;

    m8r::Scanner _scanner;
    m8r::Mad<Program> _program;
    ExecutionUnit* _eu = nullptr;
    m8r::Vector<size_t> _deferredCodeBlocks;
    m8r::Vector<uint8_t> _deferredCode;
    bool _deferred = false;
    int32_t _emittedLineNumber = -1;
    Debug _debug;

    static uint32_t _nextLabelId;

    m8r::ParseErrorList _syntaxErrors;
};

}

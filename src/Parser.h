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

namespace m8r {

struct ErrorEntry {
    ErrorEntry() { }
    ErrorEntry(const char* description, uint32_t lineno, uint16_t charno = 1, uint16_t length = 1)
        : _description(description)
        , _lineno(lineno)
        , _charno(charno)
        , _length(length)
    {
    }
    
    ErrorEntry(const ErrorEntry& other)
        : _description(other._description)
        , _lineno(other._lineno)
        , _charno(other._charno)
        , _length(other._length)
    {
    }
    
    ~ErrorEntry() { }
    
    String _description;
    uint32_t _lineno = 0;
    uint16_t _charno = 0;
    uint16_t _length = 0;
};

using ErrorList = Vector<ErrorEntry>;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Parser
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Parser  {
    friend class ParseEngine;
    friend class ParseStack;
    
public:
    Parser(Mad<Program> = Mad<Program>());
    
    ~Parser();
        
    enum class Debug { None, Full };
    // Debugging variable set according to NDEBUG compile time flag
    #ifndef NDEBUG
        static constexpr Debug debug = Parser::Debug::Full;
    #else
        static constexpr Debug debug = Parser::Debug::None;
    #endif
    
    Mad<Function> parse(const m8r::Stream& stream, ExecutionUnit*, Debug, Mad<Function> parent = Mad<Function>());

	void printError(ROMString format, ...);
    void expectedError(Token token, const char* = nullptr);
    void unknownError(Token token);
    
    ErrorList& syntaxErrors() { return _syntaxErrors; }

    uint32_t nerrors() const { return _nerrors; }
    Mad<Program> program() const { return _program; }
    
    m8r::String stringFromAtom(const Atom& atom) const { return _program->stringFromAtom(atom); }
    Atom atomizeString(const char* s) const { return _program->atomizeString(s); }

    StringLiteral startString() { return _program->startStringLiteral(); }
    void addToString(char c) { _program->addToStringLiteral(c); }
    void endString() { _program->endStringLiteral(); }
    
private:    
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
    Label label();
    void addMatchedJump(Op op, Label&);
    void matchJump(const Label& matchLabel)
    {
        int16_t jumpAddr = static_cast<int16_t>(_deferred ? _deferredCode.size() : currentCode().size()) - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }

    void matchJump(const Label& matchLabel, const Label& dstLabel)
    {
        int16_t jumpAddr = dstLabel.label - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }
    
    void matchJump(const Label& matchLabel, int16_t dstAddr)
    {
        int16_t jumpAddr = dstAddr - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }
    
    void doMatchJump(int16_t matchAddr, int16_t jumpAddr);
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

    void functionAddParam(const Atom& atom);
    void functionStart(bool ctor);
    void functionParamsEnd();
    bool functionIsCtor() const { return _functions.back()._ctor; }
    Mad<Function> functionEnd();
    Mad<Function> currentFunction() const { assert(_functions.size()); return _functions.back()._function; }
    Vector<uint8_t>& currentCode() { assert(_functions.size()); return _functions.back()._code; }
    Vector<Value>& currentConstants() { assert(_functions.size()); return _functions.back()._constants; }

    void classStart() { _classes.push_back(Object::create<MaterObject>()); }
    void classEnd() { pushK(Value(_classes.back())); _classes.pop_back(); }
    Mad<MaterObject> currentClass() const { assert(_classes.size()); return _classes.back(); }
        
    void pushK(StringLiteral::Raw value);
    void pushK(const char* value);
    void pushK(const Value& value);
    void pushThis();

    void addNamedFunction(Mad<Function>, const Atom&);
    
    void pushTmp();
    
    enum class IdType : uint8_t { MustBeLocal, MightBeLocal, NotLocal };
    void emitId(const Atom& value, IdType);
    void emitId(const char*, IdType);

    void emitDup();
    void emitMove();
    
    enum class DerefType { Prop, Elt };
    uint32_t emitDeref(DerefType);
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
        addCode(Instruction(Op::LINENO, lineno));
    }
    
    void emitCallRet(Op value, int32_t thisReg, uint32_t count);
    void addVar(const Atom& name) { currentFunction()->addLocal(name); }
    
    void discardResult();
    
    Token getToken() { return _scanner.getToken(); }
    const Scanner::TokenType& getTokenValue() { return _scanner.getTokenValue(); }
    void retireToken() { _scanner.retireToken(); }
    
    void addCode(Instruction);
    ConstantId addConstant(const Value& v);

    
    // Parse Stack manipulation and code generation
    
    void emitCodeRRR(Op, uint8_t ra, uint8_t rb, uint8_t rc);
    void emitCodeRR(Op, uint8_t ra, uint8_t rb);
    void emitCodeR(Op, uint8_t rn);
    void emitCodeRET(uint8_t nparams);
    void emitCodeRSN(Op, uint8_t rn, int16_t n);
    void emitCodeSN(Op, int16_t n);
    void emitCodeUN(Op, uint16_t n);
    void emitCode(Op);

    void reconcileRegisters(Mad<Function>);

    class ParseStack {
    public:
        enum class Type { Unknown, Local, Constant, Register, RefK, PropRef, EltRef, This, UpValue };
        
        ParseStack(Parser* parser) : _parser(parser) { }
        
        uint32_t push(Type, uint32_t reg);
        uint32_t pushRegister();
        void pushConstant(uint32_t reg) { push(Type::Constant, reg); }
        void setIsValue(bool b) { _stack.top()._isValue = b; }

        void pop();
        void swap();
        
        Type topType() const { return empty() ? Type::Unknown : _stack.top()._type; }
        uint32_t topReg() const { return empty() ? 0 : _stack.top()._reg; }
        uint32_t topDerefReg() const { return empty() ? 0 : _stack.top()._derefReg; }
        bool topIsValue() const { return empty() ? false : _stack.top()._isValue; }
        bool empty() const { return _stack.empty(); }
        void clear() { _stack.clear(); }
        
        uint32_t bake(bool makeClosure = false);
        bool needsBaking() const { return _stack.top()._type == Type::PropRef || _stack.top()._type == Type::EltRef || _stack.top()._type == Type::RefK; }
        void replaceTop(Type, uint32_t reg, uint32_t derefReg);
        void dup() {
            Entry entry = _stack.top();
            _stack.push(entry);
        }
        void propRefToReg();
        
    private:
        struct Entry {
            
            Entry() { }
            Entry(Type type, uint32_t reg, uint32_t derefReg = 0, bool isValue = false)
                : _type(type)
                , _reg(reg)
                , _derefReg(derefReg)
                , _isValue(isValue)
            {
                if (_type == Type::Constant || _type == Type::RefK) {
                    _reg += MaxRegister + 1;
                }
            }
            
            Type _type;
            uint32_t _reg;
            uint32_t _derefReg;
            bool _isValue;
        };
        
        Stack<Entry> _stack;
        Parser* _parser;
    };
    
    ParseStack _parseStack;

    struct FunctionEntry {
        FunctionEntry() { }
        FunctionEntry(Mad<Function> function, bool ctor) : _function(function), _ctor(ctor) { }
        Vector<uint8_t> _code;
        Vector<Value> _constants;
        Mad<Function> _function;
        uint8_t _nextReg = MaxRegister;
        uint8_t _minReg = MaxRegister + 1;
        bool _ctor = false;
    };
        
    using FunctionEntryVector = Vector<FunctionEntry>;

    FunctionEntryVector _functions;
    
    Vector<Mad<Object>> _classes;

    Scanner _scanner;
    Mad<Program> _program;
    ExecutionUnit* _eu = nullptr;
    uint32_t _nerrors = 0;
    Vector<size_t> _deferredCodeBlocks;
    Vector<uint8_t> _deferredCode;
    bool _deferred = false;
    int32_t _emittedLineNumber = -1;
    Debug _debug;

    static uint32_t _nextLabelId;

    ErrorList _syntaxErrors;
};

}

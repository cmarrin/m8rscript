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
    
    ParseErrorList& syntaxErrors() { return _syntaxErrors; }

    uint32_t nerrors() const { return static_cast<uint32_t>(_syntaxErrors.size()); }
    Mad<Program> program() const { return _program; }
    
    m8r::String stringFromAtom(const Atom& atom) const { return _program->stringFromAtom(atom); }
    Atom atomizeString(const char* s) const { return _program->atomizeString(s); }

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
        explicit RegOrConst(ConstantId id, Atom atom = Atom()) : _reg(id.raw()), _atom(atom), _type(Type::Constant) { assert(id.raw() <= MaxRegister); }
        
        bool operator==(const RegOrConst& other) { return _reg == other._reg && _type == other._type && _atom == other._atom; }
        
        bool isReg() const { return _type == Type::Reg; }
        uint8_t index() const { return isReg() ? _reg : (_reg + MaxRegister + 1); }
        bool isShortAtom() const { return !isReg() && static_cast<Function::BuiltinConstants>(_reg) == Function::BuiltinConstants::AtomShort; }
        bool isLongAtom() const { return !isReg() && static_cast<Function::BuiltinConstants>(_reg) == Function::BuiltinConstants::AtomLong; }
        Atom atom() const { return _atom; }

        void push(Vector<uint8_t>* vec)
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
        uint8_t _reg = static_cast<uint8_t>(Function::BuiltinConstants::Undefined);
        Type _type = Type::Constant;
        Atom _atom;
    };

    class Instruction {
    public:
        Instruction() { }
        Instruction(Op op) { assert(OpInfo::size(op) == 0); init(op); }
        Instruction(Op op, RegOrConst ra) { init(op, ra); }
        Instruction(Op op, RegOrConst ra, RegOrConst rb) { assert(OpInfo::size(op) == 2); init(op, ra, rb); }
        Instruction(Op op, RegOrConst ra, RegOrConst rb, RegOrConst rc) { assert(OpInfo::size(op) == 3); init(op, ra, rb, rc); }
        Instruction(Op op, uint8_t params) { assert(OpInfo::size(op) == 2); init(op, static_cast<uint16_t>(params)); _haveParams = true; }
        Instruction(Op op, RegOrConst ra, uint8_t params) { assert(OpInfo::size(op) == 2); init(op, ra, static_cast<uint16_t>(params)); _haveParams = true; }
        Instruction(Op op, RegOrConst ra, RegOrConst rb, uint8_t params) { assert(OpInfo::size(op) == 3); init(op, ra, rb, static_cast<uint16_t>(params)); _haveParams = true; }
        Instruction(Op op, RegOrConst ra, int16_t sn) { assert(OpInfo::size(op) == 3); init(op, ra, static_cast<uint16_t>(sn)); _haveSN = true; }
        Instruction(Op op, int16_t sn) { assert(OpInfo::size(op) == 2); init(op, static_cast<uint16_t>(sn)); _haveSN = true; }
        Instruction(Op op, uint16_t un) { assert(OpInfo::size(op) == 2); init(op, un); _haveUN = true; }
        
        bool haveRa() const { return _haveRa; }
        bool haveRb() const { return _haveRb; }
        bool haveRc() const { return _haveRc; }
        bool haveUN()  const { return _haveUN; }
        bool haveSN()  const { return _haveSN; }
        bool haveParams()  const { return _haveParams; }

        Op op() const { return _op; }
        RegOrConst ra() const { return _ra; }
        RegOrConst rb() const { return _rb; }
        RegOrConst rc() const { return _rc; }
        uint16_t n() const { return _n; }

    private:
        void init(Op op) { _op = op; }
        void init(Op op, RegOrConst ra, RegOrConst rb) {init(op, ra); _haveRb = true; _rb = rb; }
        void init(Op op, RegOrConst ra, RegOrConst rb, RegOrConst rc) { init(op, ra, rb); _haveRc = true; _rc = rc; }
        void init(Op op, RegOrConst ra, RegOrConst rb, uint16_t n) { init(op, ra, rb); _n = n; }
        void init(Op op, RegOrConst ra, uint16_t n) { init(op, ra); _n = n; }
        void init(Op op, uint16_t n) {init(op); _n = n; }

        void init(Op op, RegOrConst ra)
        {
            // The op might be immediate
            if (OpInfo::imm(op)) {
                assert(ra.isReg() && ra.index() <= 3);
                init(static_cast<Op>(byteFromOp(op, ra.index())));
            } else {
                init(op);
                _haveRa = true;
                _ra = ra;
            }
        }

        Op _op;
        RegOrConst _ra;
        RegOrConst _rb;
        RegOrConst _rc;
        uint16_t _n;
        
        bool _haveRa = false;
        bool _haveRb = false;
        bool _haveRc = false;
        bool _haveUN = false;
        bool _haveSN = false;
        bool _haveParams = false;
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

    void functionAddParam(const Atom& atom);
    void functionStart(bool ctor);
    void functionParamsEnd();
    bool functionIsCtor() const { return _functions.back()._ctor; }
    Mad<Function> functionEnd();
    Mad<Function> currentFunction() const { assert(_functions.size()); return _functions.back()._function; }
    Vector<uint8_t>& currentCode() { assert(_functions.size()); return _functions.back()._code; }
    Vector<Value>& currentConstants() { assert(_functions.size()); return _functions.back()._constants; }

    void classStart() { _classes.push_back(Object::create<MaterObject>()); }
    void classEnd() { pushK(Value(static_cast<Mad<Object>>(_classes.back()))); _classes.pop_back(); }
    Mad<MaterObject> currentClass() const { assert(_classes.size()); return _classes.back(); }
        
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
        addCode(Instruction(Op::LINENO, lineno));
    }
    
    void emitCallRet(Op value, RegOrConst thisReg, uint8_t params);
    void addVar(const Atom& name) { _functions.back().addLocal(name); }
    
    void discardResult();
    
    Token getToken() { return _scanner.getToken(); }
    const Scanner::TokenType& getTokenValue() { return _scanner.getTokenValue(); }
    void retireToken() { _scanner.retireToken(); }
    
    void addCode(Instruction);
    RegOrConst addConstant(const Value& v);

    
    // Parse Stack manipulation and code generation
    
    void emitCodeRRR(Op, RegOrConst ra, RegOrConst rb, RegOrConst rc);
    void emitCodeRRNParams(Op, RegOrConst ra, RegOrConst rb, uint8_t nparams);
    void emitCodeRR(Op, RegOrConst ra, RegOrConst rb);
    void emitCodeRNParams(Op, RegOrConst ra, uint8_t nparams);
    void emitCodeR(Op, RegOrConst rn);
    void emitCodeRET(uint8_t nparams);
    void emitCodeRSN(Op, RegOrConst rn, int16_t n);
    void emitCodeSN(Op, int16_t n);
    void emitCodeUN(Op, uint16_t n);
    void emitCode(Op);

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
        
        Stack<Entry> _stack;
        Parser* _parser;
    };
    
    ParseStack _parseStack;

    struct FunctionEntry {
        FunctionEntry() { }
        FunctionEntry(Mad<Function> function, bool ctor) : _function(function), _ctor(ctor) { }
        Vector<uint8_t> _code;
        Vector<Value> _constants;
        Vector<Atom> _locals;
        Mad<Function> _function;
        uint8_t _nextReg = MaxRegister;
        uint8_t _minReg = MaxRegister + 1;
        bool _ctor = false;

        int16_t addLocal(const Atom& atom)
        {
            for (auto name : _locals) {
                if (name == atom) {
                    return -1;
                }
            }
            _locals.push_back(atom);
            return static_cast<int16_t>(_locals.size()) - 1;
        }

        int32_t localIndex(const Atom& name) const
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
        
    using FunctionEntryVector = Vector<FunctionEntry>;

    FunctionEntryVector _functions;
    
    Vector<Mad<MaterObject>> _classes;

    Scanner _scanner;
    Mad<Program> _program;
    ExecutionUnit* _eu = nullptr;
    Vector<size_t> _deferredCodeBlocks;
    Vector<uint8_t> _deferredCode;
    bool _deferred = false;
    int32_t _emittedLineNumber = -1;
    Debug _debug;

    static uint32_t _nextLabelId;

    ParseErrorList _syntaxErrors;
};

}

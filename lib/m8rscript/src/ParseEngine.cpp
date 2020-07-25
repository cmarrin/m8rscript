/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Defines.h"
#if M8RSCRIPT_SUPPORT == 1

#include "ParseEngine.h"

using namespace m8r;

static const char _keywordString[] ROMSTR_ATTR =
    "\x01" "break"
    "\x02" "case"
    "\x03" "class"
    "\x04" "constructor"
    "\x05" "continue"
    "\x06" "default"
    "\x07" "delete"
    "\x08" "do"
    "\x09" "else"
    "\x0a" "for"
    "\x0b" "function"
    "\x0c" "if"
    "\x0d" "new"
    "\x0e" "return"
    "\x0f" "switch"
    "\x10" "this"
    "\x11" "var"
    "\x12" "while"
;

static ROMString keywordString(_keywordString);

// If the word is a keyword, return the enum for it, otherwise return Unknown
ParseEngine::Keyword ParseEngine::scanKeyword()
{
    const char* s = getTokenValue().str;
    int32_t len = static_cast<int32_t>(strlen(s));
    ROMString result = ROMString::strstr(keywordString, s);
    if (!result.valid() || ROMString::readByte(result + len) >= 0x20 || ROMString::readByte(result - 1) >= 0x20) {
        return Keyword::Unknown;
    }
    
    return static_cast<Keyword>(ROMString::readByte(result - 1));
}

ParseEngine::Keyword ParseEngine::tokenToKeyword()
{
    if (getToken() != Token::Identifier) {
        return Keyword::Unknown;
    }
    
    return scanKeyword();
}

ParseEngine::OperatorInfo ParseEngine::_opInfos[ ] = {
    { Token::STO,         1,  OperatorInfo::Assoc::Right, false, Op::MOVE },
    { Token::ADDSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::ADD  },
    { Token::SUBSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::SUB  },
    { Token::MULSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::MUL  },
    { Token::DIVSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::DIV  },
    { Token::MODSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::MOD  },
    { Token::SHLSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::SHL  },
    { Token::SHRSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::SHR  },
    { Token::SARSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::SAR  },
    { Token::ANDSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::AND  },
    { Token::ORSTO,       1,  OperatorInfo::Assoc::Right, true,  Op::OR   },
    { Token::XORSTO,      1,  OperatorInfo::Assoc::Right, true,  Op::XOR  },
    { Token::LOR,         6,  OperatorInfo::Assoc::Left,  false, Op::LOR  },
    { Token::LAND,        7,  OperatorInfo::Assoc::Left,  false, Op::LAND },
    { Token::OR,          8,  OperatorInfo::Assoc::Left,  false, Op::OR   },
    { Token::XOR,         9,  OperatorInfo::Assoc::Left,  false, Op::XOR  },
    { Token::Ampersand,   10, OperatorInfo::Assoc::Left,  false, Op::OR   },
    { Token::EQ,          11, OperatorInfo::Assoc::Left,  false, Op::EQ   },
    { Token::NE,          11, OperatorInfo::Assoc::Left,  false, Op::NE   },
    { Token::LT,          12, OperatorInfo::Assoc::Left,  false, Op::LT   },
    { Token::GT,          12, OperatorInfo::Assoc::Left,  false, Op::GT   },
    { Token::GE,          12, OperatorInfo::Assoc::Left,  false, Op::GE   },
    { Token::LE,          12, OperatorInfo::Assoc::Left,  false, Op::LE   },
    { Token::SHL,         13, OperatorInfo::Assoc::Left,  false, Op::SHL  },
    { Token::SHR,         13, OperatorInfo::Assoc::Left,  false, Op::SHR  },
    { Token::SAR,         13, OperatorInfo::Assoc::Left,  false, Op::SAR  },
    { Token::Plus,        14, OperatorInfo::Assoc::Left,  false, Op::ADD  },
    { Token::Minus,       14, OperatorInfo::Assoc::Left,  false, Op::SUB  },
    { Token::Star,        15, OperatorInfo::Assoc::Left,  false, Op::MUL  },
    { Token::Slash,       15, OperatorInfo::Assoc::Left,  false, Op::DIV  },
    { Token::Percent,     15, OperatorInfo::Assoc::Left,  false, Op::MOD  },
};

ParseEngine::ParseEngine(Parser* parser)
    : _parser(parser)
{
}

bool ParseEngine::expect(Token token)
{
    if (getToken() != token) {
        _parser->expectedError(token);
        return false;
    }
    retireToken();
    return true;
}

bool ParseEngine::expect(Parser::Expect expect, bool expected, const char* s)
{
    if (!expected) {
        _parser->expectedError(expect, s);
    }
    return expected;
}

void ParseEngine::program()
{
    while(getToken() != Token::EndOfFile) {
        if (!expect(Parser::Expect::Statement, statement())) {
            break;
        }
    }
}

bool ParseEngine::statement()
{
    if (functionStatement()) return true;
    if (classStatement()) return true;
    if (compoundStatement()) return true;
    if (selectionStatement()) return true;
    if (switchStatement()) return true;
    if (iterationStatement()) return true;
    if (jumpStatement()) return true;
    if (varStatement()) return true;
    if (expressionStatement()) return true;
    return false;
}

bool ParseEngine::functionStatement()
{
    if (tokenToKeyword() != Keyword::Function) {
        return false;
    }
    retireToken();
    Atom name = _parser->atomizeString(getTokenValue().str);
    expect(Token::Identifier);
    Mad<Function> f = functionExpression(false);
    _parser->addNamedFunction(f, name);
    return true;
}

bool ParseEngine::classStatement()
{
    if (tokenToKeyword() != Keyword::Class) {
        return false;
    }
    retireToken();
    Atom name = _parser->atomizeString(getTokenValue().str);
    _parser->addVar(name);
    _parser->emitId(name, Parser::IdType::MustBeLocal);

    expect(Token::Identifier);
    
    if (!expect(Parser::Expect::Expr, classExpression(), "class")) {
        return false;
    }
    _parser->emitMove();
    _parser->discardResult();
    return true;
}

bool ParseEngine::compoundStatement()
{
    if (getToken() != Token::LBrace) {
        return false;
    }
    retireToken();
    while (statement()) ;
    expect(Token::RBrace);
    return true;
}

bool ParseEngine::selectionStatement()
{
    if (tokenToKeyword() != Keyword::If) {
        return false;
    }
    retireToken();
    expect(Token::LParen);
    commaExpression();
    
    Label ifLabel = _parser->label();
    Label elseLabel = _parser->label();
    _parser->addMatchedJump(m8r::Op::JF, elseLabel);

    expect(Token::RParen);
    statement();

    if (tokenToKeyword() == Keyword::Else) {
        retireToken();
        _parser->addMatchedJump(m8r::Op::JMP, ifLabel);
        _parser->matchJump(elseLabel);
        statement();
        _parser->matchJump(ifLabel);
    } else {
        _parser->matchJump(elseLabel);
    }
    return true;
}

bool ParseEngine::switchStatement()
{
    if (tokenToKeyword() != Keyword::Switch) {
        return false;
    }
    retireToken();
    expect(Token::LParen);
    commaExpression();
    expect(Token::RParen);
    expect(Token::LBrace);
    
    // This pushes a deferral block onto the deferred stack.
    // We use resumeDeferred()/endDeferred() for each statement block
    int32_t deferredStatementStart = _parser->startDeferred();
    _parser->endDeferred();
    
    Vector<CaseEntry> cases;
    int32_t defaultStatement = 0;
    Label defaultFromStatementLabel;
    bool haveDefault = false;
    
    while (true) {
        if (!caseClause(cases, defaultStatement, defaultFromStatementLabel, haveDefault)) {
            break;
        }
    }
    
    expect(Token::RBrace);
    
    // We need a JMP statement here. It will either jump after all the case
    // statements or to the default statement
    Label endJumpLabel = _parser->label();
    _parser->addMatchedJump(Op::JMP, endJumpLabel);
    
    int32_t statementStart = _parser->emitDeferred();
    Label afterStatementsLabel = _parser->label();
    
    if (haveDefault) {
        _parser->matchJump(endJumpLabel, defaultStatement - deferredStatementStart + statementStart);

        // Adjust the matchedAddr in the defaultFromStatementLabel into the code space it got copied to
        defaultFromStatementLabel.matchedAddr += statementStart - deferredStatementStart;
        _parser->matchJump(defaultFromStatementLabel, afterStatementsLabel);
    } else {
        _parser->matchJump(endJumpLabel, afterStatementsLabel);
    }

    for (auto it : cases) {
        _parser->matchJump(it.toStatement, it.statementAddr - deferredStatementStart + statementStart);
        
        if (it.fromStatement.label >= 0) {
            // Adjust the matchedAddr in the fromStatement into the code space it got copied to
            it.fromStatement.matchedAddr += statementStart - deferredStatementStart;
            _parser->matchJump(it.fromStatement, afterStatementsLabel);
        }
    }
    
    _parser->discardResult();
    return true;
}

bool ParseEngine::iterationStatement()
{
    Keyword keyword = tokenToKeyword();
    if (keyword != Keyword::While && keyword != Keyword::Do && keyword != Keyword::For) {
        return false;
    }
    
    retireToken();
    
    _breakStack.emplace_back();
    _continueStack.emplace_back();
    if (keyword == Keyword::While) {
        expect(Token::LParen);
        Label label = _parser->label();
        commaExpression();
        _parser->addMatchedJump(m8r::Op::JF, label);
        expect(Token::RParen);
        statement();
        
        // resolve the continue statements
        for (auto it : _continueStack.back()) {
            _parser->matchJump(it);
        }
        
        _parser->jumpToLabel(Op::JMP, label);
        _parser->matchJump(label);
    } else if (keyword == Keyword::Do) {
        Label label = _parser->label();
        statement();

        // resolve the continue statements
        for (auto it : _continueStack.back()) {
            _parser->matchJump(it);
        }

        expect(Parser::Expect::While);
        expect(Token::LParen);
        commaExpression();
        _parser->jumpToLabel(m8r::Op::JT, label);
        expect(Token::RParen);
        expect(Token::Semicolon);
    } else if (keyword == Keyword::For) {
        expect(Token::LParen);
        if (tokenToKeyword() == Keyword::Var) {
            retireToken();
            
            // Hang onto the identifier. If this is a for..in we need to know it
            Atom name;
            if (getToken() == Token::Identifier) {
                name = _parser->atomizeString(getTokenValue().str);
            }
            
            uint32_t count = variableDeclarationList();
            expect(Parser::Expect::MissingVarDecl, count  > 0);
            if (getToken() == Token::Colon) {
                // for-in case with var
                expect(Parser::Expect::OneVarDeclAllowed, count == 1);
                retireToken();
                forIteration(name);
            } else {
                expect(Parser::Expect::MissingVarDecl, count > 0);
                forLoopCondAndIt();
            }
        } else {
            if (commaExpression()) {
                if (getToken() == Token::Colon) {
                    // for-in case with left hand expr
                    retireToken();
                    forIteration(Atom());
                } else {
                    forLoopCondAndIt();
                }
            }
        }
    }

    // resolve the break statements
    for (auto it : _breakStack.back()) {
        _parser->matchJump(it);
    }
    
    _breakStack.pop_back();
    _continueStack.pop_back();
    
    return true;
}

bool ParseEngine::jumpStatement()
{
    Keyword keyword = tokenToKeyword();
    if (keyword == Keyword::Break || keyword == Keyword::Continue) {
        bool isBreak = keyword == Keyword::Break;
        retireToken();
        expect(Token::Semicolon);
        
        // Add a JMP which will get resolved by the enclosing iteration statement
        Label label = _parser->label();
        _parser->addMatchedJump(Op::JMP, label);
        if (isBreak) {
            _breakStack.back().push_back(label);
        } else {
            _continueStack.back().push_back(label);
        }
        return true;
    }
    if (keyword == Keyword::Return) {
        retireToken();
        uint8_t count = 0;
        if (commaExpression()) {
            count = 1;
        }
        
        // If this is a ctor, we need to return this if we're not returning anything else
        if (!count && _parser->functionIsCtor()) {
            _parser->pushThis();
            count = 1;
        }
        
        _parser->emitCallRet(m8r::Op::RET, Parser::RegOrConst(), count);
        expect(Token::Semicolon);
        return true;
    }
    return false;
}

bool ParseEngine::varStatement()
{
    if (tokenToKeyword() != Keyword::Var) {
        return false;
    }
    
    retireToken();
    expect(Parser::Expect::MissingVarDecl, variableDeclarationList() > 0);
    expect(Token::Semicolon);
    return true;
}

bool ParseEngine::expressionStatement()
{
    if (!commaExpression()) {
        return false;
    }
    
    _parser->discardResult();
    expect(Token::Semicolon);
    return true;
}

bool ParseEngine::classContents()
{
    if (getToken() == Token::EndOfFile) {
        return false;
    }

    Keyword keyword = tokenToKeyword();

    if (keyword == Keyword::Function) {
        retireToken();
        Atom name = _parser->atomizeString(getTokenValue().str);
        expect(Token::Identifier);
        Mad<Function> f = functionExpression(false);
        _parser->currentClass()->setProperty(name, Value(f));
        return true;
    }
    if (keyword == Keyword::Constructor) {
        retireToken();
        Mad<Function> f = functionExpression(true);
        if (!f.valid()) {
            return false;
        }
        _parser->currentClass()->setProperty(Atom(SA::constructor), Value(f));
        return true;
    }
    if (keyword == Keyword::Var) {
        retireToken();

        while (1) {
            if (getToken() != Token::Identifier) {
                return false;
            }
            Atom name = _parser->atomizeString(getTokenValue().str);
            retireToken();
            Value v = Value::NullValue();
            if (getToken() == Token::STO) {
                retireToken();
                
                switch(getToken()) {
                    case Token::Float: v = Value(Float(getTokenValue().number)); retireToken(); break;
                    case Token::Integer: v = Value(getTokenValue().integer); retireToken(); break;
                    case Token::String: v = Value(_parser->program()->addStringLiteral(getTokenValue().str)); retireToken(); break;
                    case Token::True: v = Value(true); retireToken(); break;
                    case Token::False: v = Value(false); retireToken(); break;
                    case Token::Null: v = Value::NullValue(); retireToken(); break;
                    case Token::Undefined: v = Value(); retireToken(); break;
                    default: expect(Parser::Expect::ConstantValueRequired);
                }
            }
            _parser->currentClass()->setProperty(name, v);
            if (getToken() != Token::Comma) {
                break;
            }
            retireToken();
        }
        expect(Token::Semicolon);
        return true;
    }
    return false;
}

bool ParseEngine::caseClause(Vector<CaseEntry>& cases, 
                             int32_t &defaultStatement, 
                             Label& defaultFromStatementLabel, 
                             bool& haveDefault)
{
    Keyword keyword = tokenToKeyword();
    if (keyword == Keyword::Case || keyword == Keyword::Default) {
        bool isDefault = keyword == Keyword::Default;
        retireToken();

        if (isDefault) {
            expect(Parser::Expect::DuplicateDefault, !haveDefault);
            haveDefault = true;
        } else {
            commaExpression();
            _parser->emitCaseTest();
        }
        
        expect(Token::Colon);
        
        if (isDefault) {
            defaultStatement = _parser->resumeDeferred();
            statement();
            defaultFromStatementLabel = _parser->label();
            _parser->addMatchedJump(Op::JMP, defaultFromStatementLabel);
            _parser->endDeferred();
        } else {
            CaseEntry entry;
            entry.toStatement = _parser->label();
            _parser->addMatchedJump(Op::JT, entry.toStatement);
            entry.statementAddr = _parser->resumeDeferred();
            if (statement()) {
                entry.fromStatement = _parser->label();
                _parser->addMatchedJump(Op::JMP, entry.fromStatement);
            } else {
                entry.fromStatement.label = -1;
            }
            _parser->endDeferred();
            cases.push_back(entry);
        }
        return true;
    }
    return false;
}

void ParseEngine::forLoopCondAndIt()
{
    // On entry, we are at the semicolon before the cond expr
    expect(Token::Semicolon);
    Label label = _parser->label();
    commaExpression(); // cond expr
    _parser->addMatchedJump(m8r::Op::JF, label);
    _parser->startDeferred();
    expect(Token::Semicolon);
    commaExpression(); // iterator
    _parser->discardResult();
    _parser->endDeferred();
    expect(Token::RParen);
    statement();

    // resolve the continue statements
    for (auto it : _continueStack.back()) {
        _parser->matchJump(it);
    }

    _parser->emitDeferred();
    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

void ParseEngine::forIteration(Atom iteratorName)
{
    // On entry we have the name of the iterator variable and the colon has been parsed.
    // We need to parse the obj expression and then generate the equivalent of the following:
    //
    //      for (var it = new obj.iterator(obj); !it.done; it.next()) ...
    //
    if (iteratorName) {
        _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    }
    commaExpression();
    expect(Token::RParen);

    _parser->emitDup();
    _parser->emitPush();
    _parser->emitId(Atom(SA::iterator), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::NEW, Parser::RegOrConst(), 1);
    _parser->emitMove();
    _parser->discardResult();
    
    Label label = _parser->label();
    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    _parser->emitId(Atom(SA::done), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::CALL, Parser::RegOrConst(), 0);

    _parser->addMatchedJump(m8r::Op::JT, label);

    statement();

    // resolve the continue statements
    for (auto it : _continueStack.back()) {
        _parser->matchJump(it);
    }

    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    _parser->emitId(Atom(SA::next), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::CALL, Parser::RegOrConst(), 0);
    _parser->discardResult();

    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

uint32_t ParseEngine::variableDeclarationList()
{
    uint32_t count = 0;
    while (variableDeclaration()) {
        ++count;
        if (getToken() != Token::Comma) {
            break;
        }
        retireToken();
    }
    return count;
}

bool ParseEngine::variableDeclaration()
{
    if (getToken() != Token::Identifier) {
        return false;
    }
    Atom name = _parser->atomizeString(getTokenValue().str);
    _parser->addVar(name);
    retireToken();
    if (getToken() != Token::STO) {
        return true;
    }
    retireToken();
    _parser->emitId(name, Parser::IdType::MustBeLocal);

    if (!expect(Parser::Expect::Expr, arithmeticExpression(), "variable")) {
        return false;
    }

    _parser->emitMove();
    _parser->discardResult();
    return true;
}

uint32_t ParseEngine::argumentList()
{
    uint32_t i = 0;
    
    if (!arithmeticExpression()) {
        return i;
    }
    _parser->emitPush();
    ++i;
    while (getToken() == Token::Comma) {
        retireToken();
        arithmeticExpression();
        _parser->emitPush();
        ++i;
    }
    return i;
}

bool ParseEngine::propertyAssignment()
{
    if (!propertyName()) {
        return false;
    }
    return expect(Token::Colon) && expect(Parser::Expect::Expr, arithmeticExpression());
}

bool ParseEngine::propertyName()
{
    switch(getToken()) {
        case Token::Identifier: _parser->emitId(_parser->atomizeString(getTokenValue().str), Parser::IdType::NotLocal); retireToken(); return true;
        case Token::String: _parser->pushK(getTokenValue().str); retireToken(); return true;
        case Token::Float: _parser->pushK(Value(Float(getTokenValue().number))); retireToken(); return true;
        case Token::Integer:
            _parser->pushK(Value(getTokenValue().integer));
            retireToken();
            return true;
        default: return false;
    }
}

void ParseEngine::formalParameterList()
{
    if (getToken() != Token::Identifier) {
        return;
    }
    while (1) {
        _parser->functionAddParam(_parser->atomizeString(getTokenValue().str));
        retireToken();
        if (getToken() != Token::Comma) {
            return;
        }
        retireToken();
        if (getToken() != Token::Identifier) {
            _parser->expectedError(Token::Identifier);
            return;
        }
    }
}

bool ParseEngine::primaryExpression()
{
    if (getToken() == Token::LParen) {
        retireToken();
        commaExpression();
        expect(Token::RParen);
        return true;
    }

    switch(getToken()) {
        case Token::Identifier: _parser->emitId(_parser->atomizeString(getTokenValue().str), Parser::IdType::MightBeLocal); retireToken(); break;
        case Token::Float: _parser->pushK(Value(Float(getTokenValue().number))); retireToken(); break;
        case Token::Integer: _parser->pushK(Value(getTokenValue().integer)); retireToken(); break;
        case Token::String: _parser->pushK(getTokenValue().str); retireToken(); break;
        case Token::True: _parser->pushK(Value(true)); retireToken(); break;
        case Token::False: _parser->pushK(Value(false)); retireToken(); break;
        case Token::Null: _parser->pushK(Value::NullValue()); retireToken(); break;
        case Token::Undefined: _parser->pushK(Value()); retireToken(); break;
        case Token::LBracket:
            retireToken();
            _parser->emitLoadLit(true);
            if (arithmeticExpression()) {
                _parser->emitAppendElt();
                while (getToken() == Token::Comma) {
                    retireToken();
                    if (!expect(Parser::Expect::Expr, arithmeticExpression(), "array element")) {
                        break;
                    }
                    _parser->emitAppendElt();
                }
            }
            expect(Token::RBracket);
            break;
        case Token::LBrace:
            retireToken();
            _parser->emitLoadLit(false);
            if (propertyAssignment()) {
                _parser->emitAppendProp();
                while (getToken() == Token::Comma) {
                    retireToken();
                    if (!expect(Parser::Expect::PropertyAssignment, propertyAssignment())) {
                        break;
                    }
                    _parser->emitAppendProp();
                }
            }
            expect(Token::RBrace);
            break;
            
            break;
        
        default:
            // Check for 'this'
            if (tokenToKeyword() == Keyword::This) {
                _parser->pushThis(); retireToken();
                break;
            }
            return false;
    }
    return true;
}

Mad<Function> ParseEngine::functionExpression(bool ctor)
{
    expect(Token::LParen);
    _parser->functionStart(ctor);
    formalParameterList();
    _parser->functionParamsEnd();
    expect(Token::RParen);
    expect(Token::LBrace);
    while(statement()) { }
    expect(Token::RBrace);
    return _parser->functionEnd();
}

bool ParseEngine::classExpression()
{
    _parser->classStart();
    expect(Token::LBrace);
    while(classContents()) { }
    expect(Token::RBrace);
    _parser->classEnd();
    return true;
}

bool ParseEngine::objectExpression()
{
    Keyword keyword = tokenToKeyword();
    if (keyword == Keyword::New) {
        retireToken();
        primaryExpression();
        uint32_t argCount = 0;
        if (getToken() == Token::LParen) {
            retireToken();
            argCount = argumentList();
            expect(Token::RParen);
        }
        _parser->emitCallRet(m8r::Op::NEW, Parser::RegOrConst(), argCount);
        return true;
    }
    
    if (keyword == Keyword::Delete) {
        retireToken();
        unaryExpression();
        return true;
    } 
    
    if (keyword == Keyword::Function) {
        retireToken();
        Mad<Function> f = functionExpression(false);
        if (!f.valid()) {
            return false;
        }
        _parser->pushK(Value(f));
        return true;
    }
    if (keyword == Keyword::Class) {
        retireToken();
        classExpression();
        return true;
    }
    return false;
}

bool ParseEngine::postfixExpression()
{
    if (!primaryExpression()) {
        return false;
    }

    Parser::RegOrConst objectReg;
    while(1) {
        Token token = getToken();
        
        if (token == Token::INCR || token == Token::DECR) {
            retireToken();
            _parser->emitUnOp((token == Token::INCR) ? Op::POSTINC : Op::POSTDEC);
        } else if (getToken() == Token::LParen) {
            retireToken();
            uint32_t argCount = argumentList();
            expect(Token::RParen);
            _parser->emitCallRet(m8r::Op::CALL, objectReg, argCount);
            objectReg = Parser::RegOrConst();
        } else if (getToken() == Token::LBracket) {
            retireToken();
            commaExpression();
            expect(Token::RBracket);
            objectReg = _parser->emitDeref(Parser::DerefType::Elt);
        } else if (getToken() == Token::Period) {
            retireToken();
            Atom name = _parser->atomizeString(getTokenValue().str);
            expect(Token::Identifier);
            _parser->emitId(name, Parser::IdType::NotLocal);
            objectReg = _parser->emitDeref(Parser::DerefType::Prop);
        } else {
            return true;
        }
    }
}

bool ParseEngine::unaryExpression()
{
    if (objectExpression()) {
        return true;
    }
    
    if (postfixExpression()) {
        return true;
    }
    
    Op op;
    switch(getToken()) {
        case Token::INCR: op = Op::PREINC; break;
        case Token::DECR: op = Op::PREDEC; break;
        case Token::Minus: op = Op::UMINUS; break;
        case Token::Twiddle: op = Op::UNOT; break;
        case Token::Bang: op = Op::UNEG; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op == Op::UNKNOWN) {
        return false;
    }
    
    retireToken();
    unaryExpression();
    _parser->emitUnOp(op);
    return true;
}

bool ParseEngine::arithmeticExpression(uint8_t minPrec)
{
    if (!unaryExpression()) {
        return false;
    }
    
    if (getToken() == Token::Question) {
        // Test the value on TOS. If true leave the next value on the stack, otherwise leave the one after that
        retireToken();

        Label ifLabel = _parser->label();
        Label elseLabel = _parser->label();
        _parser->addMatchedJump(m8r::Op::JF, elseLabel);
        _parser->pushTmp();
        commaExpression();
        _parser->emitMove();
        expect(Token::Colon);
        _parser->addMatchedJump(m8r::Op::JMP, ifLabel);
        _parser->matchJump(elseLabel);
        arithmeticExpression();
        _parser->emitMove();
        _parser->matchJump(ifLabel);
    }
    
    while(1) {
        OperatorInfo* endit = _opInfos + sizeof(_opInfos) / sizeof(OperatorInfo);
        OperatorInfo* it = std::find(_opInfos, endit, getToken());
        if (it == endit || it->prec() < minPrec) {
            break;
        }
        uint8_t nextMinPrec = (it->assoc() == OperatorInfo::Assoc::Left) ? (it->prec() + 1) : it->prec();
        retireToken();
        if (it->sto()) {
            _parser->emitDup();
        }
    
        // If the op is LAND or LOR we want to short circuit. Add logic
        // here to jump over the next expression if TOS is false in the
        // case of LAND or true in the case of LOR
        if (it->op() == Op::LAND || it->op() == Op::LOR) {
            Label passLabel = _parser->label();
            Label skipLabel1 = _parser->label();
            Label skipLabel2 = _parser->label();
            bool skipResult = it->op() != Op::LAND;
            
            // If the TOS is false (if LAND) or true (if LOR) jump to the skip label
            _parser->addMatchedJump(skipResult ? Op::JT : Op::JF, skipLabel1);
            
            if (!expect(Parser::Expect::Expr, arithmeticExpression(nextMinPrec), "right-hand side")) {
                return false;
            }
            
            // If the TOS is false (if LAND) or true (if LOR) jump to the skip label            
            _parser->addMatchedJump(skipResult ? Op::JT : Op::JF, skipLabel2);

            // Test passed. We need to leave a true on the stack
            _parser->pushTmp();
            _parser->pushK(Value(1));
            _parser->emitMove();
            
            _parser->addMatchedJump(Op::JMP, passLabel);
            _parser->matchJump(skipLabel1);
            _parser->matchJump(skipLabel2);
            _parser->pushK(Value(0));
            _parser->emitMove();
            _parser->matchJump(passLabel);
        } else {
            if (!expect(Parser::Expect::Expr, arithmeticExpression(nextMinPrec), "right-hand side")) {
                return false;
            }
            _parser->emitBinOp(it->op());
        }
        
        if (it->sto()) {
            _parser->emitMove();
        }
    }
    return true;
}

bool ParseEngine::commaExpression()
{
    if (!arithmeticExpression()) {
        return false;
    }
    while (getToken() == Token::Comma) {
        if (!expect(Parser::Expect::Expr, arithmeticExpression(), "expression")) {
            return false;
        }
    }
    return true;
}

#endif

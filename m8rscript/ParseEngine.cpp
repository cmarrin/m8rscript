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

#include "ParseEngine.h"

using namespace m8r;

Map<Token, ParseEngine::OpInfo, ParseEngine::CompareTokens> ParseEngine::_opInfo;

ParseEngine::ParseEngine(Parser* parser)
    : _parser(parser)
{
    if (!_opInfo.size()) {
        _opInfo.emplace(Token::STO,         { 1, OpInfo::RightAssoc, false, Op::MOVE });
        _opInfo.emplace(Token::ADDSTO,      { 2, OpInfo::RightAssoc, true, Op::ADD });
        _opInfo.emplace(Token::SUBSTO,      { 2, OpInfo::RightAssoc, true, Op::SUB });
        _opInfo.emplace(Token::MULSTO,      { 3, OpInfo::RightAssoc, true, Op::MUL });
        _opInfo.emplace(Token::DIVSTO,      { 3, OpInfo::RightAssoc, true, Op::DIV });
        _opInfo.emplace(Token::MODSTO,      { 3, OpInfo::RightAssoc, true, Op::MOD });
        _opInfo.emplace(Token::SHLSTO,      { 4, OpInfo::RightAssoc, true, Op::SHL });
        _opInfo.emplace(Token::SHRSTO,      { 4, OpInfo::RightAssoc, true, Op::SHR });
        _opInfo.emplace(Token::SARSTO,      { 4, OpInfo::RightAssoc, true, Op::SAR });
        _opInfo.emplace(Token::ANDSTO,      { 5, OpInfo::RightAssoc, true, Op::AND });
        _opInfo.emplace(Token::ORSTO,       { 5, OpInfo::RightAssoc, true, Op::OR });
        _opInfo.emplace(Token::XORSTO,      { 5, OpInfo::RightAssoc, true, Op::XOR });
        _opInfo.emplace(Token::LOR,         { 6, OpInfo::LeftAssoc, false, Op::LOR });
        _opInfo.emplace(Token::LAND,        { 7, OpInfo::LeftAssoc, false, Op::LAND });
        _opInfo.emplace(Token::OR,          { 8, OpInfo::LeftAssoc, false, Op::OR });
        _opInfo.emplace(Token::XOR,         { 9, OpInfo::LeftAssoc, false, Op::XOR });
        _opInfo.emplace(Token::Ampersand,   { 10, OpInfo::LeftAssoc, false, Op::AND });
        _opInfo.emplace(Token::EQ,          { 11, OpInfo::LeftAssoc, false, Op::EQ });
        _opInfo.emplace(Token::NE,          { 11, OpInfo::LeftAssoc, false, Op::NE });
        _opInfo.emplace(Token::LT,          { 12, OpInfo::LeftAssoc, false, Op::LT });
        _opInfo.emplace(Token::GT,          { 12, OpInfo::LeftAssoc, false, Op::GT });
        _opInfo.emplace(Token::GE,          { 12, OpInfo::LeftAssoc, false, Op::GE });
        _opInfo.emplace(Token::LE,          { 12, OpInfo::LeftAssoc, false, Op::LE });
        _opInfo.emplace(Token::SHL,         { 13, OpInfo::LeftAssoc, false, Op::SHL });
        _opInfo.emplace(Token::SHR,         { 13, OpInfo::LeftAssoc, false, Op::SHR });
        _opInfo.emplace(Token::SAR,         { 13, OpInfo::LeftAssoc, false, Op::SAR });
        _opInfo.emplace(Token::Plus,        { 14, OpInfo::LeftAssoc, false, Op::ADD });
        _opInfo.emplace(Token::Minus,       { 14, OpInfo::LeftAssoc, false, Op::SUB });
        _opInfo.emplace(Token::Star,        { 15, OpInfo::LeftAssoc, false, Op::MUL });
        _opInfo.emplace(Token::Slash,       { 15, OpInfo::LeftAssoc, false, Op::DIV });
        _opInfo.emplace(Token::Percent,     { 15, OpInfo::LeftAssoc, false, Op::MOD });
    }
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

bool ParseEngine::expect(Token token, bool expected)
{
    if (!expected) {
        _parser->expectedError(token);
    }
    return expected;
}

bool ParseEngine::statement()
{
    while (1) {
        if (getToken() == Token::EndOfFile) {
            return false;
        }
        if (getToken() == Token::Semicolon) {
            retireToken();
            return true;
        }
        if (functionStatement()) {
            return true;
        }
        if (classStatement()) {
            return true;
        }
        if (compoundStatement() || selectionStatement() ||
            switchStatement() || iterationStatement() || jumpStatement()) {
            return true;
        }
        if (getToken() == Token::Var) {
            retireToken();
            expect(Token::MissingVarDecl, variableDeclarationList() > 0);
            expect(Token::Semicolon);
            return true;
        } else if (getToken() == Token::Delete) {
            retireToken();
            leftHandSideExpression();
            expect(Token::Semicolon);
            return true;
        } else if (expression()) {
            _parser->discardResult();
            expect(Token::Semicolon);
            return true;
        } else {
            return false;
        }
    }
}

bool ParseEngine::classContentsStatement()
{
    if (getToken() == Token::EndOfFile) {
        return false;
    }
    if (getToken() == Token::Function) {
        retireToken();
        Atom name = _parser->atomizeString(getTokenValue().str);
        expect(Token::Identifier);
        Function* f = functionExpression(false);
        _parser->currentClass()->setProperty(name, Value(f));
        return true;
    }
    if (getToken() == Token::Constructor) {
        retireToken();
        Function* f = functionExpression(true);
        if (!f) {
            return false;
        }
        _parser->currentClass()->setProperty(ATOM(_parser->program(), constructor), Value(f));
        return true;
    }
    if (getToken() == Token::Var) {
        retireToken();

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
                default: expect(Token::ConstantValueRequired);
            }
        }
        _parser->currentClass()->setProperty(name, v);
        expect(Token::Semicolon);
        return true;
    }
    return false;
}

bool ParseEngine::functionStatement()
{
    if (getToken() != Token::Function) {
        return false;
    }
    retireToken();
    Atom name = _parser->atomizeString(getTokenValue().str);
    expect(Token::Identifier);
    Function* f = functionExpression(false);
    _parser->addNamedFunction(f, name);
    return true;
}

bool ParseEngine::classStatement()
{
    if (getToken() != Token::Class) {
        return false;
    }
    retireToken();
    Atom name = _parser->atomizeString(getTokenValue().str);
    _parser->addVar(name);
    _parser->emitId(name, Parser::IdType::MustBeLocal);

    expect(Token::Identifier);
    
    if (!expect(Token::Expr, classExpression())) {
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
    if (getToken() != Token::If) {
        return false;
    }
    retireToken();
    expect(Token::LParen);
    expression();
    
    Label ifLabel = _parser->label();
    Label elseLabel = _parser->label();
    _parser->addMatchedJump(m8r::Op::JF, elseLabel);

    expect(Token::RParen);
    statement();

    if (getToken() == Token::Else) {
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
    if (getToken() != Token::Switch) {
        return false;
    }
    retireToken();
    expect(Token::LParen);
    expression();
    expect(Token::RParen);
    expect(Token::LBrace);

    typedef struct { Label toStatement; Label fromStatement; int32_t statementAddr; } CaseEntry;

    std::vector<CaseEntry> cases;
    
    // This pushes a deferral block onto the deferred stack.
    // We use resumeDeferred()/endDeferred() for each statement block
    int32_t deferredStatementStart = _parser->startDeferred();
    _parser->endDeferred();
    
    int32_t defaultStatement = 0;
    Label defaultFromStatementLabel;
    bool haveDefault = false;
    
    while (true) {
        if (getToken() == Token::Case || getToken() == Token::Default) {
            bool isDefault = getToken() == Token::Default;
            retireToken();

            if (isDefault) {
                expect(Token::DuplicateDefault, !haveDefault);
                haveDefault = true;
            } else {
                expression();
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
        } else {
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

void ParseEngine::forLoopCondAndIt()
{
    // On entry, we are at the semicolon before the cond expr
    expect(Token::Semicolon);
    Label label = _parser->label();
    expression(); // cond expr
    _parser->addMatchedJump(m8r::Op::JF, label);
    _parser->startDeferred();
    expect(Token::Semicolon);
    expression(); // iterator
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

void ParseEngine::forVarIteration(Atom iteratorName)
{
    // On entry we just parsed the colon.

    // Stack now has the result of the object expression (obj). We also have the name of 
    // the identifier (it) that will receive the next iteration on each pass.
    // We need to generate the equivalent of the following:
    //
    //      for (var it = new obj.meta.iterator(obj); !it.done; it.next()) ...
    //
    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    leftHandSideExpression();
    expect(Token::RParen);

    _parser->emitDup();
    _parser->emitPush();
    _parser->emitId(ATOM(_parser->program(), meta), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitId(ATOM(_parser->program(), iterator), Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::NEW, -1, 1);
    _parser->emitMove();
    _parser->discardResult();
    
    Label label = _parser->label();
    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    _parser->emitId("end", Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);

    _parser->addMatchedJump(m8r::Op::JT, label);

    statement();

    // resolve the continue statements
    for (auto it : _continueStack.back()) {
        _parser->matchJump(it);
    }

    _parser->emitId(iteratorName, Parser::IdType::MightBeLocal);
    _parser->emitId("next", Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::CALL, -1, 0);
    _parser->discardResult();

    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

void ParseEngine::forIteration()
{
    // On entry we just parsed the colon.

    // Stack now has the result of the object expression. We also have the name of 
    // the identifier that will receive the next iteration on each pass.
    // We need to generate the equivalent of the following:
    //
    //      for (iteratorName = new Iterator(tos()); !iteratorName.end; iteratorName.next()) ...

    leftHandSideExpression();
    expect(Token::RParen);

    _parser->emitPush();
    _parser->emitId("Iterator", Parser::IdType::MightBeLocal);
    _parser->emitCallRet(Op::NEW, -1, 1);
    _parser->emitMove();
    
    Label label = _parser->label();
    _parser->emitDup();
    _parser->emitId("end", Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);

    _parser->addMatchedJump(m8r::Op::JT, label);

    statement();

    // resolve the continue statements
    for (auto it : _continueStack.back()) {
        _parser->matchJump(it);
    }

    _parser->emitDup();
    _parser->emitId("next", Parser::IdType::NotLocal);
    _parser->emitDeref(Parser::DerefType::Prop);
    _parser->emitCallRet(Op::CALL, -1, 0);
    _parser->discardResult();

    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

bool ParseEngine::iterationStatement()
{
    Token type = getToken();
    if (type != Token::While && type != Token::Do && type != Token::For) {
        return false;
    }
    
    retireToken();
    
    _breakStack.emplace_back();
    _continueStack.emplace_back();
    if (type == Token::While) {
        expect(Token::LParen);
        Label label = _parser->label();
        expression();
        _parser->addMatchedJump(m8r::Op::JF, label);
        expect(Token::RParen);
        statement();
        
        // resolve the continue statements
        for (auto it : _continueStack.back()) {
            _parser->matchJump(it);
        }
        
        _parser->jumpToLabel(Op::JMP, label);
        _parser->matchJump(label);
    } else if (type == Token::Do) {
        Label label = _parser->label();
        statement();

        // resolve the continue statements
        for (auto it : _continueStack.back()) {
            _parser->matchJump(it);
        }

        expect(Token::While);
        expect(Token::LParen);
        expression();
        _parser->jumpToLabel(m8r::Op::JT, label);
        expect(Token::RParen);
        expect(Token::Semicolon);
    } else if (type == Token::For) {
        expect(Token::LParen);
        if (getToken() == Token::Var) {
            retireToken();
            
            // Hang onto the identifier. If this is a for..in we need to know it
            Atom name;
            if (getToken() == Token::Identifier) {
                name = _parser->atomizeString(getTokenValue().str);
            }
            
            uint32_t count = variableDeclarationList();
            expect(Token::MissingVarDecl, count  > 0);
            if (getToken() == Token::Colon) {
                // for-in case with var
                expect(Token::OneVarDeclAllowed, count == 1);
                retireToken();
                forVarIteration(name);
            } else {
                expect(Token::MissingVarDecl, count > 0);
                forLoopCondAndIt();
            }
        } else {
            if (expression()) {
                if (getToken() == Token::Colon) {
                    // for-in case with left hand expr
                    retireToken();
                    forIteration();
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
    if (getToken() == Token::Break || getToken() == Token::Continue) {
        bool isBreak = getToken() == Token::Break;
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
    if (getToken() == Token::Return) {
        retireToken();
        uint32_t count = 0;
        if (expression()) {
            count = 1;
        }
        
        // If this is a ctor, we need to return this if we're not returning anything else
        if (!count && _parser->functionIsCtor()) {
            _parser->pushThis();
            count = 1;
        }
        
        _parser->emitCallRet(m8r::Op::RET, -1, count);
        expect(Token::Semicolon);
        return true;
    }
    return false;
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

    if (!expect(Token::Expr, expression())) {
        return false;
    }

    _parser->emitMove();
    _parser->discardResult();
    return true;
}

bool ParseEngine::arithmeticPrimary()
{
    if (getToken() == Token::LParen) {
        retireToken();
        expression();
        expect(Token::RParen);
        return true;
    }
    
    Op op;
    switch(getToken()) {
        case Token::INC: op = Op::PREINC; break;
        case Token::DEC: op = Op::PREDEC; break;
        case Token::Minus: op = Op::UMINUS; break;
        case Token::Twiddle: op = Op::UNOT; break;
        case Token::Bang: op = Op::UNEG; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        retireToken();
        arithmeticPrimary();
        _parser->emitUnOp(op);
        return true;
    }
    
    if (!leftHandSideExpression()) {
        return false;
    }
    switch(getToken()) {
        case Token::INC: op = Op::POSTINC; break;
        case Token::DEC: op = Op::POSTDEC; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        retireToken();
        _parser->emitUnOp(op);
    }
    return true;
}

bool ParseEngine::expression(uint8_t minPrec)
{
    if (!arithmeticPrimary()) {
        return false;
    }
    
    if (getToken() == Token::Question) {
        // Test the value on TOS. If true leave the next value on the stack, otherwise leave the one after that
        retireToken();

        Label ifLabel = _parser->label();
        Label elseLabel = _parser->label();
        _parser->addMatchedJump(m8r::Op::JF, elseLabel);
        _parser->pushTmp();
        expression();
        _parser->emitMove();
        expect(Token::Colon);
        _parser->addMatchedJump(m8r::Op::JMP, ifLabel);
        _parser->matchJump(elseLabel);
        expression();
        _parser->emitMove();
        _parser->matchJump(ifLabel);
    }
    
    while(1) {
        auto it = _opInfo.find(getToken());
        if (it == _opInfo.end() || it->value.prec < minPrec) {
            break;
        }
        uint8_t nextMinPrec = (it->value.assoc == OpInfo::LeftAssoc) ? (it->value.prec + 1) : it->value.prec;
        retireToken();
        if (it->value.sto) {
            _parser->emitDup();
        }
    
        expression(nextMinPrec);
        _parser->emitBinOp(it->value.op);
        if (it->value.sto) {
            _parser->emitMove();
        }
    }
    return true;
}

bool ParseEngine::leftHandSideExpression()
{
    if (!memberExpression()) {
        return false;
    }
    
    int32_t objectReg = -1;
    while(1) {
        if (getToken() == Token::LParen) {
            retireToken();
            uint32_t argCount = argumentList();
            expect(Token::RParen);
            _parser->emitCallRet(m8r::Op::CALL, objectReg, argCount);
            objectReg = -1;
        } else if (getToken() == Token::LBracket) {
            retireToken();
            expression();
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

bool ParseEngine::memberExpression()
{
    if (getToken() == Token::New) {
        retireToken();
        memberExpression();
        uint32_t argCount = 0;
        if (getToken() == Token::LParen) {
            retireToken();
            argCount = argumentList();
            expect(Token::RParen);
        }
        _parser->emitCallRet(m8r::Op::NEW, -1, argCount);
        return true;
    }
    
    if (getToken() == Token::Function) {
        retireToken();
        Function* f = functionExpression(false);
        _parser->pushK(f);
        return true;
    }
    if (getToken() == Token::Class) {
        retireToken();
        classExpression();
        return true;
    }
    return primaryExpression();
}

uint32_t ParseEngine::argumentList()
{
    uint32_t i = 0;
    
    if (!expression()) {
        return i;
    }
    _parser->emitPush();
    ++i;
    while (getToken() == Token::Comma) {
        retireToken();
        expression();
        _parser->emitPush();
        ++i;
    }
    return i;
}

bool ParseEngine::primaryExpression()
{
    switch(getToken()) {
        case Token::Identifier: _parser->emitId(_parser->atomizeString(getTokenValue().str), Parser::IdType::MightBeLocal); retireToken(); break;
        case Token::This: _parser->pushThis(); retireToken(); break;
        case Token::Float: _parser->pushK(Float(getTokenValue().number)); retireToken(); break;
        case Token::Integer: _parser->pushK(getTokenValue().integer); retireToken(); break;
        case Token::String: _parser->pushK(getTokenValue().str); retireToken(); break;
        case Token::True: _parser->pushK(true); retireToken(); break;
        case Token::False: _parser->pushK(false); retireToken(); break;
        case Token::Null: _parser->pushK(); retireToken(); break;
        case Token::LBracket:
            retireToken();
            _parser->emitLoadLit(true);
            if (expression()) {
                _parser->emitAppendElt();
                while (getToken() == Token::Comma) {
                    retireToken();
                    if (!expect(Token::Expr, expression())) {
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
                    if (!expect(Token::PropertyAssignment, propertyAssignment())) {
                        break;
                    }
                    _parser->emitAppendProp();
                }
            }
            expect(Token::RBrace);
            break;
            
            break;
        
        default: return false;
    }
    return true;
}

bool ParseEngine::propertyAssignment()
{
    if (!propertyName()) {
        return false;
    }
    return expect(Token::Colon) && expect(Token::Expr, expression());
}

bool ParseEngine::propertyName()
{
    switch(getToken()) {
        case Token::Identifier: _parser->emitId(_parser->atomizeString(getTokenValue().str), Parser::IdType::NotLocal); retireToken(); return true;
        case Token::String: _parser->pushK(getTokenValue().str); retireToken(); return true;
        case Token::Float: _parser->pushK(Float(getTokenValue().number)); retireToken(); return true;
        case Token::Integer: _parser->pushK(getTokenValue().integer); retireToken(); return true;
        default: return false;
    }
}

Function* ParseEngine::functionExpression(bool ctor)
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
    while(classContentsStatement()) { }
    expect(Token::RBrace);
    _parser->classEnd();
    return true;
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


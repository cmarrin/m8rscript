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

#include "Parser.h"

using namespace m8r;

Map<Token, ParseEngine::OpInfo, ParseEngine::CompareTokens> ParseEngine::_opInfo;

ParseEngine::ParseEngine(Parser* parser)
    : _parser(parser)
{
    if (!_opInfo.size()) {
        _opInfo.emplace(Token::STO,         { 1, OpInfo::Assoc::Right, Op::STO });
        _opInfo.emplace(Token::ADDSTO,      { 2, OpInfo::Assoc::Right, Op::STOADD });
        _opInfo.emplace(Token::SUBSTO,      { 2, OpInfo::Assoc::Right, Op::STOSUB });
        _opInfo.emplace(Token::MULSTO,      { 3, OpInfo::Assoc::Right, Op::STOMUL });
        _opInfo.emplace(Token::DIVSTO,      { 3, OpInfo::Assoc::Right, Op::STODIV });
        _opInfo.emplace(Token::MODSTO,      { 3, OpInfo::Assoc::Right, Op::STOMOD });
        _opInfo.emplace(Token::SHLSTO,      { 4, OpInfo::Assoc::Right, Op::STOSHL });
        _opInfo.emplace(Token::SHRSTO,      { 4, OpInfo::Assoc::Right, Op::STOSHR });
        _opInfo.emplace(Token::SARSTO,      { 4, OpInfo::Assoc::Right, Op::STOSAR });
        _opInfo.emplace(Token::ANDSTO,      { 5, OpInfo::Assoc::Right, Op::STOAND });
        _opInfo.emplace(Token::ORSTO,       { 5, OpInfo::Assoc::Right, Op::STOOR });
        _opInfo.emplace(Token::XORSTO,      { 5, OpInfo::Assoc::Right, Op::STOXOR });
        _opInfo.emplace(Token::LOR,         { 6, OpInfo::Assoc::Left, Op::LOR });
        _opInfo.emplace(Token::LAND,        { 7, OpInfo::Assoc::Left, Op::LAND });
        _opInfo.emplace(Token::OR,          { 8, OpInfo::Assoc::Left, Op::OR });
        _opInfo.emplace(Token::XOR,         { 9, OpInfo::Assoc::Left, Op::XOR });
        _opInfo.emplace(Token::Ampersand,   { 10, OpInfo::Assoc::Left, Op::AND });
        _opInfo.emplace(Token::EQ,          { 11, OpInfo::Assoc::Left, Op::EQ });
        _opInfo.emplace(Token::NE,          { 11, OpInfo::Assoc::Left, Op::NE });
        _opInfo.emplace(Token::LT,          { 12, OpInfo::Assoc::Left, Op::LT });
        _opInfo.emplace(Token::GT,          { 12, OpInfo::Assoc::Left, Op::GT });
        _opInfo.emplace(Token::GE,          { 12, OpInfo::Assoc::Left, Op::GE });
        _opInfo.emplace(Token::LE,          { 12, OpInfo::Assoc::Left, Op::LE });
        _opInfo.emplace(Token::SHL,         { 13, OpInfo::Assoc::Left, Op::SHL });
        _opInfo.emplace(Token::SHR,         { 13, OpInfo::Assoc::Left, Op::SHR });
        _opInfo.emplace(Token::SAR,         { 13, OpInfo::Assoc::Left, Op::SAR });
        _opInfo.emplace(Token::Plus,        { 14, OpInfo::Assoc::Left, Op::ADD });
        _opInfo.emplace(Token::Minus,       { 14, OpInfo::Assoc::Left, Op::SUB });
        _opInfo.emplace(Token::Star,        { 15, OpInfo::Assoc::Left, Op::MUL });
        _opInfo.emplace(Token::Slash,       { 15, OpInfo::Assoc::Left, Op::DIV });
        _opInfo.emplace(Token::Percent,     { 15, OpInfo::Assoc::Left, Op::MOD });
    }
}

void ParseEngine::syntaxError(Error error, Token token)
{
    String s;
    uint8_t c = static_cast<uint8_t>(token);
    
    switch(error) {
        case Error::Expected:
            s = "syntax error: expected ";
            if (c < 0x80) {
                s += '\'';
                s += c;
                s += '\'';
            } else {
                const char* errorString;
                switch(token) {
                    case Token::Expr: errorString = "expression"; break;
                    case Token::PropertyAssignment: errorString = "property assignment"; break;
                    case Token::Identifier: errorString = "identifier"; break;
                    default: errorString = "*** UNKNOWN TOKEN ***"; break;
                }
                s += errorString;
            }
            break;
    }
    
    _parser->printError(s.c_str());
}

bool ParseEngine::expect(Token token)
{
    if (_token != token) {
        syntaxError(Error::Expected, token);
        return false;
    } else {
        popToken();
        return true;
    }
}

bool ParseEngine::expect(Token token, bool expected)
{
    if (!expected) {
        syntaxError(Error::Expected, token);
    }
    return expected;
}

void ParseEngine::program()
{
    // Prime the pump
    popToken();
    
    sourceElements();
    _parser->programEnd();
}

bool ParseEngine::sourceElements()
{
    while(sourceElement()) ;
    return false;
}

bool ParseEngine::sourceElement()
{
    return functionDeclaration() || statement();
}

bool ParseEngine::statement()
{
    if (_token == Token::EndOfFile) {
        return false;
    }
    if (_token == Token::Semicolon) {
        popToken();
        return true;
    }
    if (compoundStatement() || selectionStatement() || 
        switchStatement() || iterationStatement() || jumpStatement()) {
        return true;
    }
    if (_token == Token::Var) {
        popToken();
        variableDeclarationList();
        expect(Token::Semicolon);
        return true;
    } else if (_token == Token::Delete) {
        popToken();
        leftHandSideExpression();
        expect(Token::Semicolon);
        return true;
    } else if (expression()) {
        _parser->emit(m8r::Op::POP);
        expect(Token::Semicolon);
        return true;
    }
    return false;
}

bool ParseEngine::functionDeclaration()
{
    if (_token != Token::Function) {
        return false;
    }
    popToken();
    Atom name = _tokenValue.atom;
    expect(Token::Identifier);
    Function* f = function();
    _parser->addNamedFunction(f, name);
    return true;
}

bool ParseEngine::compoundStatement()
{
    if (_token != Token::LBrace) {
        return false;
    }
    popToken();
    while (statement()) ;
    expect(Token::RBrace);
    return true;
}

bool ParseEngine::selectionStatement()
{
    if (_token != Token::If) {
        return false;
    }
    popToken();
    expect(Token::LParen);
    expression();
    
    Label ifLabel = _parser->label();
    Label elseLabel = _parser->label();
    _parser->addMatchedJump(m8r::Op::JF, elseLabel);

    expect(Token::RParen);
    statement();

    if (_token == Token::Else) {
        popToken();
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
    // FIXME: Implement
    return false;
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
    _parser->emit(m8r::Op::POP);
    _parser->endDeferred();
    expect(Token::RParen);
    statement();
    _parser->emitDeferred();
    _parser->jumpToLabel(Op::JMP, label);
    _parser->matchJump(label);
}

bool ParseEngine::iterationStatement()
{
    Token type = _token;
    if (_token != Token::While && _token != Token::Do && _token != Token::For) {
        return false;
    }
    
    popToken();
    expect(Token::LParen);
    if (type == Token::While) {
        Label label = _parser->label();
        expression();
        _parser->addMatchedJump(m8r::Op::JF, label);
        expect(Token::RParen);
        statement();
        _parser->jumpToLabel(Op::JMP, label);
        _parser->matchJump(label);
    } else if (type == Token::Do) {
        Label label = _parser->label();
        statement();
        expect(Token::While);
        expect(Token::LParen);
        expression();
        _parser->jumpToLabel(m8r::Op::JT, label);
        expect(Token::RParen);
        expect(Token::Semicolon);
    } else if (type == Token::For) {
        if (_token == Token::Var) {
            popToken();
            variableDeclarationList();
            if (_token == Token::Colon) {
                // for-in case with var
                //FIXME: implement
                // FIXME: We need a way to know if the above decl is a legit variable for for-in
                popToken();
                leftHandSideExpression();
            } else {
                forLoopCondAndIt();
            }
        } else {
            if (expression()) {
                if (_token == Token::Colon) {
                    // for-in case with left hand expr
                    // FIXME: We need a way to know if the above expression is a legit left hand expr
                    // FIXME: implement
                    popToken();
                    leftHandSideExpression();
                } else {
                    forLoopCondAndIt();
                }
            }
        }
    }
    
    // We should be at the closing paren
    return true;
}

bool ParseEngine::jumpStatement()
{
    if (_token == Token::Break || _token == Token::Continue) {
        popToken();
        expect(Token::Semicolon);
        return true;
    }
    if (_token == Token::Return) {
        popToken();
        uint32_t count = 0;
        if (expression()) {
            count = 1;
        }
        _parser->emitWithCount(m8r::Op::RET, count);
        expect(Token::Semicolon);
    }
    return false;
}

bool ParseEngine::variableDeclarationList()
{
    bool haveOne = false;
    while (variableDeclaration()) {
        if (_token != Token::Comma) {
            return true;
        }
        haveOne = true;
        popToken();
    }
    return haveOne;
}

bool ParseEngine::variableDeclaration()
{
    if (_token != Token::Identifier) {
        return false;
    }
    Atom name = _tokenValue.atom;
    _parser->addVar(name);
    popToken();
    if (_token != Token::STO) {
        return true;
    }
    popToken();
    _parser->emitId(name, Parser::IdType::MustBeLocal);
    if (!expect(Token::Expr, expression())) {
        return false;
    }
        
    _parser->emit(m8r::Op::STOPOP);
    return true;
}

bool ParseEngine::arithmeticPrimary()
{
    if (_token == Token::LParen) {
        popToken();
        expression();
        expect(Token::RParen);
        return true;
    }
    
    Op op;
    switch(_token) {
        case Token::INC: op = Op::PREINC; break;
        case Token::DEC: op = Op::PREDEC; break;
        case Token::Plus: op = Op::UPLUS; break;
        case Token::Minus: op = Op::UMINUS; break;
        case Token::Twiddle: op = Op::UNEG; break;
        case Token::Bang: op = Op::UNOT; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        popToken();
        arithmeticPrimary();
        if (op != Op::UPLUS) {
            _parser->emit(op);
        }
        return true;
    }
    
    if (!leftHandSideExpression()) {
        return false;
    }
    switch(_token) {
        case Token::INC: op = Op::POSTINC; break;
        case Token::DEC: op = Op::POSTDEC; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        popToken();
        _parser->emit(op);
    }
    return true;
}

bool ParseEngine::expression(uint8_t minPrec)
{
    if (!arithmeticPrimary()) {
        return false;
    }

    while(1) {
        OpInfo opInfo;            
        if (!_opInfo.find(_token, opInfo) || opInfo.prec < minPrec) {
            break;
        }
        uint8_t nextMinPrec = (opInfo.assoc == OpInfo::Assoc::Left) ? (opInfo.prec + 1) : opInfo.prec;
        popToken();
        expression(nextMinPrec);
        _parser->emit(opInfo.op);
    }
    return true;
}

bool ParseEngine::leftHandSideExpression()
{
    if (_token == Token::New) {
        popToken();
        leftHandSideExpression();
        return true;
    }
    
    if (_token == Token::Function) {
        popToken();
        Function* f = function();
        _parser->emit(f);
    }
    
    if (!primaryExpression()) {
        return false;
    }
    while(1) {
        if (_token == Token::LParen) {
            popToken();
            uint32_t argCount = argumentList();
            expect(Token::RParen);
            _parser->emitWithCount(m8r::Op::CALL, argCount);
        } else if (_token == Token::LBracket) {
            popToken();
            expression();
            expect(Token::RBracket);
            _parser->emit(m8r::Op::DEREF);
        } else if (_token == Token::Period) {
            popToken();
            Atom name = _tokenValue.atom;
            if (expect(Token::Identifier)) {
                _parser->emitId(name, Parser::IdType::NotLocal);
                _parser->emit(m8r::Op::DEREF);
            }
        } else {
            return true;
        }
    }
}

uint32_t ParseEngine::argumentList()
{
    uint32_t i = 0;
    
    if (!expression()) {
        return i;
    }
    ++i;
    while (_token == Token::Comma) {
        popToken();
        expression();
        ++i;
    }
    return i;
}

bool ParseEngine::primaryExpression()
{
    switch(_token) {
        case Token::Identifier: _parser->emitId(_tokenValue.atom, Parser::IdType::MightBeLocal); popToken(); break;
        case Token::Float: _parser->emit(_tokenValue.number); popToken(); break;
        case Token::Integer: _parser->emit(_tokenValue.integer); popToken(); break;
        case Token::String: _parser->emit(_tokenValue.string); popToken(); break;
        case Token::LBracket:
            popToken();
            _parser->emit(Op::PUSHLITA);
            if (expression()) {
                _parser->emit(m8r::Op::STOA);
                while (_token == Token::Comma) {
                    popToken();
                    if (!expect(Token::Expr, expression())) {
                        break;
                    }
                    _parser->emit(m8r::Op::STOA);
                }
            }
            expect(Token::RBracket);
            break;
        case Token::LBrace:
            popToken();
            _parser->emit(Op::PUSHLITO);
            if (propertyAssignment()) {
                _parser->emit(m8r::Op::STOO);
                while (_token == Token::Comma) {
                    popToken();
                    if (!expect(Token::PropertyAssignment, propertyAssignment())) {
                        break;
                    }
                    _parser->emit(m8r::Op::STOO);
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
    switch(_token) {
        case Token::Identifier: _parser->emitId(_tokenValue.atom, Parser::IdType::NotLocal); popToken(); return true;
        case Token::String: _parser->emit(_tokenValue.string); popToken(); return true;
        case Token::Float: _parser->emit(_tokenValue.number); popToken(); return true;
        case Token::Integer: _parser->emit(_tokenValue.integer); popToken(); return true;
        default: return false;
    }
}

Function* ParseEngine::function()
{
    expect(Token::LParen);
    _parser->functionStart();
    formalParameterList();
    _parser->functionParamsEnd();
    expect(Token::RParen);
    expect(Token::LBrace);
    sourceElements();
    expect(Token::RBrace);
    _parser->emit(m8r::Op::END);
    return _parser->functionEnd();
}

void ParseEngine::formalParameterList()
{
    if (_token != Token::Identifier) {
        return;
    }
    while (1) {
        _parser->functionAddParam(_tokenValue.atom);
        popToken();
        if (_token != Token::Comma) {
            return;
        }
        popToken();
        if (_token != Token::Identifier) {
            syntaxError(Error::Expected, Token::Identifier);
            return;
        }
    }
}


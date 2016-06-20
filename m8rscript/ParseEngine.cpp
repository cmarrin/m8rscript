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

Map<uint8_t, ParseEngine::OpInfo> ParseEngine::_opInfo;

ParseEngine::ParseEngine(Parser* parser)
    : _parser(parser)
{
    if (!_opInfo.size()) {
        _opInfo.emplace('=',            { 1, OpInfo::Assoc::Right, Op::STO });
        _opInfo.emplace(O_ADDEQ,        { 2, OpInfo::Assoc::Right, Op::STOADD });
        _opInfo.emplace(O_SUBEQ,        { 2, OpInfo::Assoc::Right, Op::STOSUB });
        _opInfo.emplace(O_MULEQ,        { 3, OpInfo::Assoc::Right, Op::STOMUL });
        _opInfo.emplace(O_DIVEQ,        { 3, OpInfo::Assoc::Right, Op::STODIV });
        _opInfo.emplace(O_MODEQ,        { 3, OpInfo::Assoc::Right, Op::STOMOD });
        _opInfo.emplace(O_LSHIFTEQ,     { 4, OpInfo::Assoc::Right, Op::STOSHL });
        _opInfo.emplace(O_RSHIFTEQ,     { 4, OpInfo::Assoc::Right, Op::STOSHR });
        _opInfo.emplace(O_RSHIFTFILLEQ, { 4, OpInfo::Assoc::Right, Op::STOSAR });
        _opInfo.emplace(O_ANDEQ,        { 5, OpInfo::Assoc::Right, Op::STOAND });
        _opInfo.emplace(O_OREQ,         { 5, OpInfo::Assoc::Right, Op::STOOR });
        _opInfo.emplace(O_XOREQ,        { 5, OpInfo::Assoc::Right, Op::STOXOR });
        _opInfo.emplace(O_LOR,          { 6, OpInfo::Assoc::Left, Op::LOR });
        _opInfo.emplace(O_LAND,         { 7, OpInfo::Assoc::Left, Op::LAND });
        _opInfo.emplace('|',            { 8, OpInfo::Assoc::Left, Op::OR });
        _opInfo.emplace('^',            { 9, OpInfo::Assoc::Left, Op::XOR });
        _opInfo.emplace('&',            { 10, OpInfo::Assoc::Left, Op::AND });
        _opInfo.emplace(O_EQ,           { 11, OpInfo::Assoc::Left, Op::EQ });
        _opInfo.emplace(O_NE,           { 11, OpInfo::Assoc::Left, Op::NE });
        _opInfo.emplace('<',            { 12, OpInfo::Assoc::Left, Op::LT });
        _opInfo.emplace('>',            { 12, OpInfo::Assoc::Left, Op::GT });
        _opInfo.emplace(O_GE,           { 12, OpInfo::Assoc::Left, Op::GE });
        _opInfo.emplace(O_LE,           { 12, OpInfo::Assoc::Left, Op::LE });
        _opInfo.emplace(O_LSHIFT,       { 13, OpInfo::Assoc::Left, Op::SHL });
        _opInfo.emplace(O_RSHIFT,       { 13, OpInfo::Assoc::Left, Op::SHR });
        _opInfo.emplace(O_RSHIFTFILL,   { 13, OpInfo::Assoc::Left, Op::SAR });
        _opInfo.emplace('+',            { 14, OpInfo::Assoc::Left, Op::ADD });
        _opInfo.emplace('-',            { 14, OpInfo::Assoc::Left, Op::SUB });
        _opInfo.emplace('*',            { 15, OpInfo::Assoc::Left, Op::MUL });
        _opInfo.emplace('/',            { 15, OpInfo::Assoc::Left, Op::DIV });
        _opInfo.emplace('%',            { 15, OpInfo::Assoc::Left, Op::MOD });
    }
}

void ParseEngine::syntaxError(Error error, uint8_t token)
{
    String s;
    
    switch(error) {
        case Error::Expected:
            s = "syntax error: expected '";
            s += token;
            s += "'";
            break;
    }
    
    _parser->printError(s.c_str());
}

bool ParseEngine::expect(uint8_t token)
{
    if (_token != token) {
        syntaxError(Error::Expected, token);
        return false;
    } else {
        popToken();
        return true;
    }
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
    return statement() | functionDeclaration();
}

bool ParseEngine::statement()
{
    if (_token == C_EOF) {
        return false;
    }
    if (_token == ';') {
        popToken();
        return true;
    }
    if (compoundStatement() || selectionStatement() || 
        switchStatement() || iterationStatement() || jumpStatement()) {
        return true;
    }
    if (_token == K_VAR) {
        popToken();
        variableDeclarationList();
        expect(';');
        return true;
    } else if (_token == K_DELETE) {
        popToken();
        leftHandSideExpression();
        expect(';');
        return true;
    } else {
        if (expression()) {
            _parser->emit(m8r::Op::POP);
            expect(';');
            return true;
        }
        
        // We have a token we don't recognize, pop it and continue
        popToken();
        return true;
    }        
}

bool ParseEngine::functionDeclaration()
{
    if (_token != K_FUNCTION) {
        return false;
    }
    Atom name = _tokenValue.atom;
    expect(T_IDENTIFIER);
    Function* f = function();
    _parser->addNamedFunction(f, name);
    return true;
}

bool ParseEngine::compoundStatement()
{
    // FIXME: Implement
    return false;
}

bool ParseEngine::selectionStatement()
{
    // FIXME: Implement
    return false;
}

bool ParseEngine::switchStatement()
{
    // FIXME: Implement
    return false;
}

bool ParseEngine::iterationStatement()
{
    // FIXME: Implement
    return false;
}

bool ParseEngine::jumpStatement()
{
    // FIXME: Implement
    return false;
}

bool ParseEngine::variableDeclarationList()
{
    bool haveOne = false;
    while (variableDeclaration()) {
        if (_token != ',') {
            return true;
        }
        haveOne = true;
    }
    return haveOne;
}

bool ParseEngine::variableDeclaration()
{
    if (_token != T_IDENTIFIER) {
        return false;
    }
    Atom name = _tokenValue.atom;
    _parser->addVar(name);
    popToken();
    if (_token != '=') {
        return true;
    }
    popToken();
    _parser->emit(name);
    expression();
    _parser->emit(m8r::Op::STOPOP);
    return true;
}

bool ParseEngine::arithmeticPrimary()
{
    if (_token == '(') {
        popToken();
        expression();
        expect(')');
        return true;
    }
    
    Op op;
    switch(_token) {
        case O_INC: op = Op::PREINC; break;
        case O_DEC: op = Op::PREDEC; break;
        case '+': op = Op::UPLUS; break;
        case '-': op = Op::UMINUS; break;
        case '~': op = Op::UNEG; break;
        case '!': op = Op::UNOT; break;
        default: op = Op::UNKNOWN; break;
    }
    
    if (op != Op::UNKNOWN) {
        popToken();
    }
    if (!leftHandSideExpression()) {
        return false;
    }
    if (op != Op::UNKNOWN) {
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
    if (_token == K_NEW) {
        popToken();
        leftHandSideExpression();
        return true;
    }
    
    if (!primaryExpression()) {
        return false;
    }
    while(1) {
        if (_token == '(') {
            popToken();
            uint32_t argCount = argumentList();
            expect(')');
            _parser->emitWithCount(m8r::Op::CALL, argCount);
        } else if (_token == '[') {
            popToken();
            expression();
            expect(']');
            _parser->emit(m8r::Op::DEREF);
        } else if (_token == '.') {
            popToken();
            Atom name = _tokenValue.atom;
            if (expect(T_IDENTIFIER)) {
                _parser->emit(name);
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
    while (_token == ',') {
        popToken();
        expression();
        ++i;
    }
    return i;
}

bool ParseEngine::primaryExpression()
{
    // FIXME: Add the rest of the cases:
    //
    //    | object_literal
    //    | array_literal
    //    | '(' expression ')'
    
    switch(_token) {
        case T_IDENTIFIER: _parser->emit(_tokenValue.atom); break;
        case T_FLOAT: _parser->emit(_tokenValue.number); break;
        case T_INTEGER: _parser->emit(_tokenValue.integer); break;
        case T_STRING: _parser->emit(_tokenValue.string); break;
        default: return false;
    }
    popToken();
    return true;
}

Function* ParseEngine::function()
{
    // FIXME: Implement
    return nullptr;
}

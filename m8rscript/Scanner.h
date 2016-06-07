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

#pragma once

#include <stdint.h>
#include <assert.h>

#include "Stream.h"
#include "FixedPointFloat.h"
#include "Atom.h"
#include "ExecutionUnit.h"

#include "parse.tab.h"

#define MAX_ID_LENGTH 32

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Scanner
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Scanner  {
public:
  	Scanner(Stream* istream)
  	 : _lastChar(C_EOF)
  	 , _istream(istream)
     , _lineno(1)
     , _nerrors(0)
  	{
        _currentExecutionUnit = new ExecutionUnit(this);
    }
  	
  	~Scanner()
  	{
        delete _currentExecutionUnit;
        for (uint32_t i = 0; i < _executionUnits.size(); ++i) {
            delete _executionUnits[i];
        }
    }
  
  	uint8_t getToken(YYSTYPE* token);

	void printError(const char* s);
    
    uint32_t nerrors() const { return _nerrors; }
    
    void emit(const char*);
    void emit(uint32_t);
    void emit(float);
    void emit(const Atom&);
    void emit(Op);
    void emitCallOrNew(bool call, uint32_t nparams);
    void emit(ExecutionUnit*);

    Label label() const;
    void loopStart(bool cond, Label&);
    void loopEnd(Label&);
    
    void functionAddParam(const Atom& atom) { _currentExecutionUnit->addParam(atom); }
    void functionStart();
    ExecutionUnit* functionEnd();
    
    String toString() const { return _currentExecutionUnit->toString(0); }
    
    void stringFromAtom(String& s, const Atom& atom) const { _atomTable.stringFromAtom(s, atom); }
    void stringFromRawAtom(String& s, uint16_t rawAtom) const { _atomTable.stringFromRawAtom(s, rawAtom); }
  	
private:
    uint8_t get() const;
    
	void putback(uint8_t c) const
	{
  		assert(_lastChar == C_EOF && c != C_EOF);
  		_lastChar = c;
	}

  	uint8_t scanKeyword(const char*);
  	uint8_t scanString(char terminal);
  	uint8_t scanSpecial();
  	uint8_t scanIdentifier();
  	uint8_t scanNumber();
  	uint8_t scanComment();
  	void scanDigits(bool hex);
  	bool scanFloat();
    
  	mutable uint8_t _lastChar;
  	String _tokenString;
  	Stream* _istream;
    mutable uint32_t _lineno;
    uint32_t _nerrors;
    AtomTable _atomTable;
    ExecutionUnit *_currentExecutionUnit;
    Vector<ExecutionUnit*> _executionUnits;
};

}

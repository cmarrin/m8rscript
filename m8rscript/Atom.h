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

#include "Containers.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Atom
//
//
//
//////////////////////////////////////////////////////////////////////////////

class Atom {
    friend class AtomTable;
        
public:
    static constexpr uint16_t NoAtom = 0xffff;
    static constexpr uint8_t MaxAtomSize = 127;
    
    bool valid() const { return _index != NoAtom; }
    uint16_t rawAtom() const { return _index; }
    void set(uint16_t rawAtom) { _index = rawAtom; }
    static Atom atomFromRawAtom(uint16_t rawAtom) { Atom a; a._index = rawAtom; return a; }

    int compare(const Atom& other) const { return static_cast<int>(_index) - static_cast<int>(other._index); }

protected:
    uint16_t _index;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: AtomTable
//
//  Atoms go in a single string where each atom is preceeded by its size.
//  Size byte is negated so its high order bit is set to identify it as
//  a size. So max size of an individual atom is 127 bytes
//
//////////////////////////////////////////////////////////////////////////////

class AtomTable {
public:
    Atom atomizeString(const char*);
    void stringFromAtom(String& s, const Atom& atom) const { stringFromRawAtom(s, atom._index); }
    void stringFromRawAtom(String& s, uint16_t rawAtom) const { s.set(&(_table[rawAtom + 1]), -_table[rawAtom]); }

private:
    String _table;
};

}

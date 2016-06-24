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

class RawAtom
{
    friend class Atom;
    
public:
    uint16_t raw() const { return _index; }
    static RawAtom make(uint16_t raw) { RawAtom r; r._index = raw; return r; }
    
private:
    uint16_t _index;
};

class Atom {
    friend class AtomTable;
        
public:
    Atom() { _raw._index = NoAtom; }
    Atom(RawAtom raw) { _raw._index = raw._index; }
    Atom(const Atom& other) { _raw._index = other._raw._index; }
    Atom(Atom& other) { _raw._index = other._raw._index; }

    const Atom& operator=(const Atom& other) { _raw._index = other._raw._index; return *this; }
    Atom& operator=(Atom& other) { _raw._index = other._raw._index; return *this; }
    operator bool() const { return _raw._index != NoAtom; }
    operator RawAtom() const { return _raw; }

    int operator-(const Atom& other) const { return static_cast<int>(_raw._index) - static_cast<int>(other._raw._index); }
    bool operator==(const Atom& other) const { return _raw._index == other._raw._index; }

private:
    static constexpr uint16_t NoAtom = std::numeric_limits<uint16_t>::max();
    static constexpr uint8_t MaxAtomSize = 127;

    RawAtom _raw;
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
    m8r::String stringFromAtom(const Atom atom) const
    {
        uint16_t index = static_cast<RawAtom>(atom).raw();
        return m8r::String(&(_table[index + 1]), -_table[index]);
    }

private:
    m8r::String _table;
};

}

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

class Stream;

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
    friend class Program;
    
public:
    enum class SharedAtom {
        Base64,
        BothEdges,
        FallingEdge,
        GPIO,
        High,
        Input,
        InputPulldown,
        InputPullup,
        Iterator,
        Low,
        None,
        Output,
        OutputOpenDrain,
        PinMode,
        RisingEdge,
        Socket,
        Trigger,
        arguments,
        currentTime,
        decode,
        delay,
        digitalRead,
        digitalWrite,
        encode,
        end,
        length,
        next,
        onInterrupt,
        print,
        printf,
        println,
        setPinMode,
        value,
        __construct,
        __typeName,
        __count__
    };
    
    AtomTable();

    Atom atomizeString(const char* s) const { return atomizeString(s, false); }
    m8r::String stringFromAtom(const Atom atom) const
    {
        uint16_t index = atom.raw();
        bool shared = index < _sharedTable.size();
        if (!shared) {
            index -= _sharedTable.size();
        }
        std::vector<int8_t>& table = shared ? _sharedTable : _table;
        return m8r::String(reinterpret_cast<const char*>(&(table[index + 1])), -table[index]);
    }
    
    const std::vector<int8_t>& stringTable() const { return _table; }
    
    static Atom sharedAtom(SharedAtom id)
    {
        auto it = _sharedAtomMap.find(static_cast<uint32_t>(id));
        if (it == _sharedAtomMap.end()) {
            return Atom();
        }
        return it->value;
    }

private:
    Atom atomizeString(const char*, bool shared) const;
    int32_t findAtom(const char* s, bool shared) const;

    std::vector<int8_t>& stringTable() { return _table; }

    static constexpr uint8_t MaxAtomSize = 127;

    mutable std::vector<int8_t> _table;
    
    static std::vector<int8_t> _sharedTable;
    static Map<uint16_t, Atom> _sharedAtomMap;
};

#define ATOM(a) m8r::AtomTable::sharedAtom(m8r::AtomTable::SharedAtom::a)

}

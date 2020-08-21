/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include "MString.h"

namespace m8r {

class Stream;

//************************************************************************
//
//  Class: AtomTable
//
//  Atoms go in a single string where each atom is preceeded by its size.
//  Size byte is negated so its high order bit is set to identify it as
//  a size. So max size of an individual atom is 127 bytes.
//
//  Atoms are a 16 bit id, so 64K maximum. For non built-in Atoms, this
//  id is the index into the string table. Given an average size of 8
//  characters per atom, that can hold around 8000 Atoms.
//
//  Predefined Atoms, those in the SA enum, have a table of their own
//  which is a simple array of char pointers. These will have an Atom
//  id which matches their enumerant. We'll make space for 500 of these
//  ids, so the normal Atom Ids will be offset by that amount.
//
//************************************************************************

static constexpr uint16_t ExternalAtomOffset = 32768;


class Atom : public Id<uint16_t>
{
    using Id::Id;

public:
    Atom() { }
    explicit Atom(Atom::value_type value) : Id(Id::Raw(value)) { }
    
    friend int compare(const Atom& a, const Atom& b) { return int(a - b); }
};

class AtomTable {
public:
    
    AtomTable();

    Atom atomizeString(const char*) const;

    const char* stringFromAtom(const Atom) const;
    
    void setSharedAtomList(const char** list, uint16_t count) { _sharedAtoms = list; _sharedAtomCount = count; }
        
private:
    Atom findAtom(const char* s) const;

    static constexpr uint8_t MaxAtomSize = 127;

    mutable Vector<char> _table;
    
    // Shared atom table
    const char** _sharedAtoms = nullptr;
    uint16_t _sharedAtomCount = 0;
};

}

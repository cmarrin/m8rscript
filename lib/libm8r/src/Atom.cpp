/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Atom.h"
#include <algorithm>

using namespace m8r;

static int32_t binarySearch(const char** names, uint16_t nelts, const char* value)
{
    int32_t first = 0;
    int32_t last = nelts - 1;

    while (first <= last)
    {
        int32_t middle = (first + last) / 2;
        int result = ::strcmp(names[middle], value);
        if (result == 0) {
            return middle;
        } else if (result > 0) {
            last = middle - 1;
        } else {
            first = middle + 1;
        }
    }
    return -1;
}

// Check that p and s are the same up to and including the '\0' at the end of s
static bool atomcomp(const char* p, const char* s)
{
    while (1) {
        if (*p++ != *s++) {
            return false;
        }
        if (*s == '\0') {
            return *p == '\0';
        }
    }
}

static const char* atomfind(const char* buf, size_t size, const char* atom)
{
    const char* end = buf + size;
    const char* p = buf;
    while (p < end) {
        if (*p == *atom) {
            if (atomcomp(p, atom)) {
                // We know the substrings match fully. and we know that
                // the char at the end of p is a '\0'. But we need to also
                // make sure the char before p is '\0'
                if (p == buf || p[-1] == '\0') {
                    return p;
                }
            }
        }
        ++p;
    }
    return nullptr;
}

AtomTable::AtomTable()
{
}

Atom AtomTable::atomizeString(const char* str) const
{
    Atom atom = findAtom(str);
    if (atom) {
        return atom;
    }
    
    uint16_t len = strlen(str);
    
    Atom a(static_cast<Atom::value_type>(_table.size() + ExternalAtomOffset));
    _table.reserve(_table.size() + len + 1);
    for (uint16_t i = 0; i < len; ++i) {
        _table.push_back(str[i]);
    }
    _table.push_back('\0');
    return a;
}

Atom AtomTable::findAtom(const char* s) const
{
    // First look in the sharedAtom table, if any
    int32_t result = -1;
    if (_sharedAtoms) {
        result = binarySearch(_sharedAtoms, _sharedAtomCount, s);
    }
   if (result >= 0) {
        return Atom(static_cast<Atom::value_type>(result));
    }
    
    if (_table.size()) {
        const char* p = atomfind(&(_table[0]), _table.size(), s);
        if (p) {
            return Atom(static_cast<Atom::value_type>(p - &(_table[0]) + ExternalAtomOffset));
        }
    }
    
    return Atom();
}

const char* AtomTable::stringFromAtom(const Atom atom) const
{
    if (!atom) {
        return "";
    }
    uint16_t index = atom.raw();
    if (index < ExternalAtomOffset) {
        if (!_sharedAtoms || _sharedAtomCount <= index) {
            return "";
        }
        return _sharedAtoms[index];
    }
    
    index -= ExternalAtomOffset;
    return &(_table[index]);
}
    

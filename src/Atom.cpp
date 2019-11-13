/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Atom.h"
#include "Program.h"
#include <algorithm>

using namespace m8r;

static int32_t binarySearch(const char** names, uint16_t nelts, const char* value)
{
    int32_t first = 0;
    int32_t last = nelts - 1;

    while (first <= last)
    {
        int32_t middle = (first + last) / 2;
        int result = strcmp(names[middle], value);
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

AtomTable::AtomTable()
{
}

Atom AtomTable::atomizeString(const char* romstr) const
{
    uint16_t len = ROMstrlen(romstr);
    if (len > MaxAtomSize || len == 0) {
        return Atom();
    }

    Mad<char> s = Mad<char>::create(len + 1);
    ROMCopyString(s.get(), romstr);
    
    Atom atom = findAtom(s.get());
    if (atom) {
        s.destroy(len + 1);
        return atom;
    }
    
     if (_table.size() == 0) {
        _table.push_back('\0');
    }
    
    Atom a(static_cast<Atom::value_type>(_table.size() - 1 + ExternalAtomOffset));
    _table[_table.size() - 1] = -static_cast<int8_t>(len);
    for (uint16_t i = 0; i < len; ++i) {
        _table.push_back(s.get()[i]);
    }
    _table.push_back('\0');
    s.destroy(len + 1);
    return a;
}

Atom AtomTable::findAtom(const char* s) const
{
    // First look in the sharedAtom table
    uint16_t nelts;
    const char** sharedAtomTable = sharedAtoms(nelts);
    int32_t result = binarySearch(sharedAtomTable, nelts, s);
    if (result >= 0) {
        return Atom(static_cast<Atom::value_type>(result));
    }
    
    size_t len = strlen(s);

    if (_table.size() == 0) {
        return Atom();
    }

    if (_table.size() > 1) {
        const char* start = reinterpret_cast<const char*>(&(_table[0]));
        const char* p = start;
        while(p && *p != '\0') {
            p++;
            p = strstr(p, s);
            assert(p != start); // Since the first string is preceded by a length, this should never happen
            if (p && static_cast<int8_t>(p[-1]) < 0) {
                // The next char either needs to be negative (meaning the start of the next word) or the end of the string
                if (static_cast<int8_t>(p[len]) <= 0) {
                    return Atom(static_cast<Atom::value_type>(p - start - 1 + ExternalAtomOffset));
                }
            }
        }
    }
    
    return Atom();
}

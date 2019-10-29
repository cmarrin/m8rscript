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
    size_t len = ROMstrlen(romstr);
    if (len > MaxAtomSize || len == 0) {
        return Atom();
    }

    char* s = new char[len + 1];
    ROMCopyString(s, romstr);
    
    Atom atom = findAtom(s);
    if (atom) {
        delete [ ] s;
        return atom;
    }
    
     if (_table.size() == 0) {
        _table.push_back('\0');
    }
    
    Atom a(static_cast<Atom::value_type>(_table.size() - 1 + ExternalAtomOffset));
    _table[_table.size() - 1] = -static_cast<int8_t>(len);
    for (size_t i = 0; i < len; ++i) {
        _table.push_back(s[i]);
    }
    _table.push_back('\0');
    delete [ ] s;
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

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

using namespace m8r;

std::vector<int8_t> AtomTable::_sharedTable;
Map<uint16_t, Atom> AtomTable::_sharedAtomMap;

struct SharedAtomTableEntry { uint32_t id; const char* name; };

#define AtomEntry(a) { static_cast<uint32_t>(AtomTable::SharedAtom::a), _##a }

static const char _Array[] ROMSTR_ATTR = "Array";
static const char _Base64[] ROMSTR_ATTR = "Base64";
static const char _BothEdges[] ROMSTR_ATTR = "BothEdges";
static const char _Connected[] ROMSTR_ATTR = "Connected";
static const char _Disconnected[] ROMSTR_ATTR = "Disconnected";
static const char _Error[] ROMSTR_ATTR = "Error";
static const char _FallingEdge[] ROMSTR_ATTR = "FallingEdge";
static const char _High[] ROMSTR_ATTR = "High";
static const char _GPIO[] ROMSTR_ATTR = "GPIO";
static const char _Input[] ROMSTR_ATTR = "Input";
static const char _InputPulldown[] ROMSTR_ATTR = "InputPulldown";
static const char _InputPullup[] ROMSTR_ATTR = "InputPullup";
static const char _IPAddr[] ROMSTR_ATTR = "IPAddr";
static const char _Iterator[] ROMSTR_ATTR = "Iterator";
static const char _JSON[] ROMSTR_ATTR = "JSON";
static const char _Low[] ROMSTR_ATTR = "Low";
static const char _MaxConnections[] ROMSTR_ATTR = "MaxConnections";
static const char _None[] ROMSTR_ATTR = "None";
static const char _Output[] ROMSTR_ATTR = "Output";
static const char _OutputOpenDrain[] ROMSTR_ATTR = "OutputOpenDrain";
static const char _PinMode[] ROMSTR_ATTR = "PinMode";
static const char _ReceivedData[] ROMSTR_ATTR = "ReceivedData";
static const char _Reconnected[] ROMSTR_ATTR = "Reconnected";
static const char _RisingEdge[] ROMSTR_ATTR = "RisingEdge";
static const char _SentData[] ROMSTR_ATTR = "SentData";
static const char _TCP[] ROMSTR_ATTR = "TCP";
static const char _Trigger[] ROMSTR_ATTR = "Trigger";
static const char _UDP[] ROMSTR_ATTR = "UDP";
static const char _a[] ROMSTR_ATTR = "a";
static const char _arguments[] ROMSTR_ATTR = "arguments";
static const char _b[] ROMSTR_ATTR = "b";
static const char _c[] ROMSTR_ATTR = "c";
static const char _call[] ROMSTR_ATTR = "call";
static const char _constructor[] ROMSTR_ATTR = "constructor";
static const char _currentTime[] ROMSTR_ATTR = "currentTime";
static const char _d[] ROMSTR_ATTR = "d";
static const char _decode[] ROMSTR_ATTR = "decode";
static const char _delay[] ROMSTR_ATTR = "delay";
static const char _digitalRead[] ROMSTR_ATTR = "digitalRead";
static const char _digitalWrite[] ROMSTR_ATTR = "digitalWrite";
static const char _disconnect[] ROMSTR_ATTR = "disconnect";
static const char _encode[] ROMSTR_ATTR = "encode";
static const char _end[] ROMSTR_ATTR = "end";
static const char _length[] ROMSTR_ATTR = "length";
static const char _lookupHostname[] ROMSTR_ATTR = "lookupHostname";
static const char _next[] ROMSTR_ATTR = "next";
static const char _onInterrupt[] ROMSTR_ATTR = "onInterrupt";
static const char _parse[] ROMSTR_ATTR = "parse";
static const char _print[] ROMSTR_ATTR = "print";
static const char _printf[] ROMSTR_ATTR = "printf";
static const char _println[] ROMSTR_ATTR = "println";
static const char _send[] ROMSTR_ATTR = "send";
static const char _setPinMode[] ROMSTR_ATTR = "setPinMode";
static const char _split[] ROMSTR_ATTR = "split";
static const char _stringify[] ROMSTR_ATTR = "stringify";
static const char _toFloat[] ROMSTR_ATTR = "toFloat";
static const char _toInt[] ROMSTR_ATTR = "toInt";
static const char _toUInt[] ROMSTR_ATTR = "toUInt";
static const char _trim[] ROMSTR_ATTR = "trim";
static const char _value[] ROMSTR_ATTR = "value";
static const char ___nativeObject[] ROMSTR_ATTR = "__nativeObject";
static const char ___this[] ROMSTR_ATTR = "this";
static const char ___typeName[] ROMSTR_ATTR = "__typeName";



static SharedAtomTableEntry RODATA_ATTR sharedAtoms[] = {
    AtomEntry(Array),
    AtomEntry(Base64),
    AtomEntry(BothEdges),
    AtomEntry(Connected),
    AtomEntry(Disconnected),
    AtomEntry(Error),
    AtomEntry(FallingEdge),
    AtomEntry(GPIO),
    AtomEntry(High),
    AtomEntry(Input),
    AtomEntry(InputPulldown),
    AtomEntry(InputPullup),
    AtomEntry(IPAddr),
    AtomEntry(Iterator),
    AtomEntry(JSON),
    AtomEntry(Low),
    AtomEntry(MaxConnections),
    AtomEntry(None),
    AtomEntry(Output),
    AtomEntry(OutputOpenDrain),
    AtomEntry(PinMode),
    AtomEntry(ReceivedData),
    AtomEntry(Reconnected),
    AtomEntry(RisingEdge),
    AtomEntry(SentData),
    AtomEntry(TCP),
    AtomEntry(Trigger),
    AtomEntry(UDP),
    
    AtomEntry(a),
    AtomEntry(arguments),
    AtomEntry(b),
    AtomEntry(c),
    AtomEntry(call),
    AtomEntry(constructor),
    AtomEntry(currentTime),
    AtomEntry(d),
    AtomEntry(decode),
    AtomEntry(delay),
    AtomEntry(digitalRead),
    AtomEntry(digitalWrite),
    AtomEntry(disconnect),
    AtomEntry(encode),
    AtomEntry(end),
    AtomEntry(length),
    AtomEntry(lookupHostname),
    AtomEntry(next),
    AtomEntry(onInterrupt),
    AtomEntry(parse),
    AtomEntry(print),
    AtomEntry(printf),
    AtomEntry(println),
    AtomEntry(send),
    AtomEntry(setPinMode),
    AtomEntry(split),
    AtomEntry(stringify),
    AtomEntry(toFloat),
    AtomEntry(toInt),
    AtomEntry(toUInt),
    AtomEntry(trim),
    AtomEntry(value),
    AtomEntry(__nativeObject),
    AtomEntry(__this),
    AtomEntry(__typeName),
};

static_assert (sizeof(sharedAtoms) / sizeof(SharedAtomTableEntry) == static_cast<size_t>(AtomTable::SharedAtom::__count__), "sharedAtomMap is incomplete");
    
AtomTable::AtomTable()
{
    if (_sharedTable.empty()) {
        for (auto it : sharedAtoms) {
            Atom atom = atomizeString(it.name, true);
            _sharedAtomMap.emplace(static_cast<uint32_t>(it.id), atom);
        }
    }
}

Atom AtomTable::atomizeString(const char* romstr, bool shared) const
{
    size_t len = ROMstrlen(romstr);
    if (len > MaxAtomSize || len == 0) {
        return Atom();
    }

    char* s = new char[len + 1];
    ROMCopyString(s, romstr);
    
    int32_t index = findAtom(s, true);
    if (index >= 0) {
        delete [ ] s;
        return Atom(static_cast<uint16_t>(index));
    }
    
    index = findAtom(s, false);
    if (index >= 0) {
        delete [ ] s;
        return Atom(static_cast<uint16_t>(index) + (shared ? 0 : _sharedTable.size()));
    }

    // Atom wasn't in either table add it to the one requested
    std::vector<int8_t>& table = shared ? _sharedTable : _table;

    if (table.size() == 0) {
        table.push_back('\0');
    }
    
    Atom a(static_cast<Atom::value_type>(table.size() - 1 + (shared ? 0 : _sharedTable.size())));
    table[table.size() - 1] = -static_cast<int8_t>(len);
    for (size_t i = 0; i < len; ++i) {
        table.push_back(s[i]);
    }
    table.push_back('\0');
    delete [ ] s;
    return a;
}

int32_t AtomTable::findAtom(const char* s, bool shared) const
{
    size_t len = strlen(s);
    std::vector<int8_t>& table = shared ? _sharedTable : _table;

    if (table.size() == 0) {
        return -1;
    }

    if (table.size() > 1) {
        const char* start = reinterpret_cast<const char*>(&(table[0]));
        const char* p = start;
        while(p && *p != '\0') {
            p++;
            p = strstr(p, s);
            assert(p != start); // Since the first string is preceded by a length, this should never happen
            if (p && static_cast<int8_t>(p[-1]) < 0) {
                // The next char either needs to be negative (meaning the start of the next word) or the end of the string
                if (static_cast<int8_t>(p[len]) <= 0) {
                    return static_cast<int32_t>(p - start - 1);
                }
            }
        }
    }
    
    return -1;
}

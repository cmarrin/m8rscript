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

#include <cstdint>

#if __APPLE__
#include <string>
#define STRING std::string
#else
#include <Arduino.h>
#define STRING String
#endif

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: MString
//
//  Wrapper String class that works on both Mac and ESP
//
//////////////////////////////////////////////////////////////////////////////

class MString {
public:
	MString() { }
	MString(const char* s, uint32_t len)
    {
        for (int i = 0; i < len; ++i) {
            _string += s[i];
        }
    }
	
    char& operator[](const int index) { return _string[index]; }
    const char& operator[](const int index) const { return _string[index]; }
	uint32_t length() const { return static_cast<uint32_t>(_string.length()); }
	STRING& operator+=(uint8_t c) { return _string += c; }
	STRING& operator+=(const char* s) { return _string += s; }
    const char* c_str() const { return _string.c_str(); }
    void clear() { _string = ""; }
	
private:
    STRING _string;
};

}

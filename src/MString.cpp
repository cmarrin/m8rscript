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

#include "MString.h"

using namespace m8r;

String& String::erase(size_t pos, size_t len)
{
    if (pos >= _size - 1) {
        return *this;
    }
    if (pos + len >= _size) {
        len = _size - pos - 1;
    }
    memmove(_data + pos, _data + pos + len, _size - pos - len);
    _size -= len;
    return *this;
}

String String::slice(int32_t start, int32_t end) const
{
    int32_t sz = static_cast<int32_t>(size());
    if (start < 0) {
        start = sz + start;
    }
    if (end < 0) {
        end = sz + end;
    }
    if (end > sz) {
        end = sz;
    }
    if (start >= end) {
        return String();
    }
    return String(_data + start, end - start);
}

String String::trim() const
{
    if (_size < 2 || !_data) {
        return String();
    }
    size_t l = _size - 1;
    char* s = _data;
    while (isspace(s[l - 1])) {
        --l;
    }
    while (*s && isspace(*s)) {
        ++s;
        --l;
    }
    return String(s, static_cast<int32_t>(l));
}

std::vector<String> String::split(const String& separator, bool skipEmpty) const
{
    std::vector<String> array;
    char* p = _data;
    while (1) {
        char* n = strstr(p, separator.c_str());
        if (!n || n - p != 0 || !skipEmpty) {
            array.push_back(String(p, static_cast<int32_t>(n ? (n - p) : -1)));
        }
        if (!n) {
            break;
        }
        p = n ? (n + separator.size()) : nullptr;
    }
    return array;
}

String String::join(const std::vector<String>& array, const String& separator)
{
    String s;
    bool first = true;
    for (auto it : array) {
        if (first) {
            first = false;
        } else {
            s += separator;
        }
        s += it;
    }
    return s;
}

String String::join(const std::vector<char>& array)
{
    String s;
    s.ensureCapacity(array.size());
    for (auto it : array) {
        s += it;
    }
    return s;
}
void String::doEnsureCapacity(size_t size)
{
    _capacity = _capacity ? _capacity * 2 : 1;
    if (_capacity < size) {
        _capacity = size;
    }
    char *newData = new char[_capacity];
    assert(newData);
    if (_data) {
        if (newData) {
            memcpy(newData, _data, _size);
        } else {
            _capacity = 0;
            _size = 1;
        }
        delete _data;
    }
    _data = newData;
}

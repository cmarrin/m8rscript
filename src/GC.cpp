/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "GC.h"

#include "Containers.h"
#include "MStream.h"
#include "SystemInterface.h"

using namespace m8r;

static Vector<RawMad> _objectStore;
static Vector<RawMad> _stringStore;

template<>
void GC<MemoryType::Object>::addToStore(RawMad v)
{
    _objectStore.push_back(v);
}

template<>
void GC<MemoryType::String>::addToStore(RawMad v)
{
    _stringStore.push_back(v);
}

template<>
void GC<MemoryType::Object>::removeFromStore(RawMad v)
{
    auto it = std::find(_objectStore.begin(), _objectStore.end(), v);
    if (it != _objectStore.end()) {
        _objectStore.erase(it);
    }
}

template<>
void GC<MemoryType::String>::removeFromStore(RawMad v)
{
    auto it = std::find(_stringStore.begin(), _stringStore.end(), v);
    if (it != _stringStore.end()) {
        _stringStore.erase(it);
    }
}

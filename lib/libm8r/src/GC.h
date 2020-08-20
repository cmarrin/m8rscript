/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Mallocator.h"
#include "SharedPtr.h"

namespace m8r {

class Executable;

class GC {
public:
    static void gc(bool force = false);
    
    template<MemoryType Type>
    static void addToStore(RawMad);

    template<MemoryType Type>
    static void removeFromStore(RawMad);

    static void addStaticObject(RawMad);
    static void removeStaticObject(RawMad);
    static void addExecutable(const SharedPtr<Executable>&);
    static void removeExecutable(const SharedPtr<Executable>&);

private:
    static constexpr int32_t MaxGCObjectDiff = 10;
    static constexpr int32_t MaxGCStringDiff = 10;
    static constexpr int32_t MaxCountSinceLastGC = 20;

    enum class GCState { ClearMarkedObj, ClearMarkedStr, MarkActive, MarkStatic, SweepObj, SweepStr };
    static GCState gcState;
    static uint32_t prevGCObjects;
    static uint32_t prevGCStrings;
    static uint8_t countSinceLastGC;
    static bool inGC;
};
    
}

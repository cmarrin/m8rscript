/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Executable.h"
#include "Mallocator.h"
#include "SharedPtr.h"

namespace m8rscript {

class GC {
public:
    static void gc(bool force = false);
    
    template<m8r::MemoryType Type>
    static void addToStore(m8r::RawMad);

    template<m8r::MemoryType Type>
    static void removeFromStore(m8r::RawMad);

    static void addStaticObject(m8r::RawMad);
    static void removeStaticObject(m8r::RawMad);
    static void addExecutable(const m8r::SharedPtr<m8r::Executable>&);
    static void removeExecutable(const m8r::SharedPtr<m8r::Executable>&);

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

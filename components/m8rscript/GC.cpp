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
#include "Executable.h"
#include "Object.h"
#include "MStream.h"
#include "SystemInterface.h"

using namespace m8rscript;
using namespace m8r;

static Vector<RawMad> _objectStore;
static Vector<RawMad> _stringStore;
static Vector<SharedPtr<Executable>> _executableStore;
static Vector<RawMad> _staticObjects;

GC::GCState GC::gcState = GCState::ClearMarkedObj;
uint32_t GC::prevGCObjects = 0;
uint32_t GC::prevGCStrings = 0;
uint8_t GC::countSinceLastGC = 0;
bool GC::inGC = false;

void GC::gc(bool force)
{
    if (inGC) {
        return;
    }
    
    force = true;
    
    inGC = true;
    bool didFullCycle = gcState == GCState::ClearMarkedObj;
    while (1) {
        switch(gcState) {
            case GCState::ClearMarkedObj:
                if (!force && _objectStore.size() - prevGCObjects < MaxGCObjectDiff && _stringStore.size() - prevGCStrings < MaxGCStringDiff && ++countSinceLastGC < MaxCountSinceLastGC) {
                    inGC = false;
                    return;
                }
                for (RawMad& it : _objectStore) {
                    Mad<Object> obj = Mad<Object>(it);
                    obj->setMarked(false);
                }
                gcState = GCState::ClearMarkedStr;
                break;
            case GCState::ClearMarkedStr:
                for (RawMad& it : _stringStore) {
                    Mad<String> str = Mad<String>(it);
                    str->setMarked(false);
                }
                gcState = GCState::MarkActive;
                break;
            case GCState::MarkActive:
                for (auto it : _executableStore) {
                    it->gcMark();
                }
                gcState = GCState::MarkStatic;
                break;
            case GCState::MarkStatic:
                for (RawMad& it : _staticObjects) {
                    Mad<Object> obj = Mad<Object>(it);
                    obj->gcMark();
                }
                gcState = GCState::SweepObj;
                break;
            case GCState::SweepObj: {
                auto it = std::remove_if(_objectStore.begin(), _objectStore.end(), [](RawMad m) {
                    Mad<Object> obj = Mad<Object>(m);
                    if (!obj->isMarked()) {
                        delete obj.get();
                        return true;
                    }
                    return false;
                });
                _objectStore.erase(it, _objectStore.end());
                gcState = GCState::SweepStr;
                break;
            }
            case GCState::SweepStr: {
                auto it = std::remove_if(_stringStore.begin(), _stringStore.end(), [](RawMad m) {
                    Mad<String> str = Mad<String>(m);
                    if (!str->isMarked()) {
                        str.destroy();
                        return true;
                    }
                    return false;
                });
                _stringStore.erase(it, _stringStore.end());
                gcState = GCState::ClearMarkedObj;
                
                if (!force || didFullCycle) {
                    inGC = false;
                    return;
                }
                didFullCycle = true;
                break;
            }
        }
        
        if (!force) {
            break;
        }
    }
    inGC = false;
}

namespace m8rscript {

template<>
void GC::addToStore<MemoryType::Object>(RawMad v)
{
    _objectStore.push_back(v);
}

template<>
void GC::addToStore<MemoryType::String>(RawMad v)
{
    _stringStore.push_back(v);
}

template<>
void GC::removeFromStore<MemoryType::Object>(RawMad v)
{
    auto it = std::find(_objectStore.begin(), _objectStore.end(), v);
    if (it != _objectStore.end()) {
        _objectStore.erase(it);
    }
}

template<>
void GC::removeFromStore<MemoryType::String>(RawMad v)
{
    auto it = std::find(_stringStore.begin(), _stringStore.end(), v);
    if (it != _stringStore.end()) {
        _stringStore.erase(it);
    }
}

}

void GC::addStaticObject(RawMad obj)
{
    _staticObjects.push_back(obj);
}

void GC::removeStaticObject(RawMad obj)
{
    auto it = std::find(_staticObjects.begin(), _staticObjects.end(), obj);
    if (it != _staticObjects.end()) {
        _staticObjects.erase(it);
    }
}

void GC::addExecutable(const SharedPtr<Executable>& eu)
{
    _executableStore.push_back(eu);
}

void GC::removeExecutable(const SharedPtr<Executable>& eu)
{
    auto it = std::find(_executableStore.begin(), _executableStore.end(), eu);
    if (it != _executableStore.end()) {
        _executableStore.erase(it);
    }
}

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

#include "Object.h"

#include "ExecutionUnit.h"
#include "Function.h"
#include "MStream.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

std::vector<String*> Object::_stringStore;
std::vector<Object*> Object::_objectStore;
std::vector<Object*> Object::_staticObjects;

void Object::memoryInfo(MemoryInfo& info)
{
    SystemInterface::memoryInfo(info);
    info.numAllocationsByType.resize(static_cast<uint32_t>(MemoryType::NumTypes));
    info.numAllocationsByType[static_cast<uint32_t>(MemoryType::Object)] = Object::numObjectAllocations();
    info.numAllocationsByType[static_cast<uint32_t>(MemoryType::String)] = Object::numStringAllocations();
}

void* Object::operator new(size_t size)
{
    void* p = SystemInterface::alloc(MemoryType::Object, size);
    _objectStore.push_back(reinterpret_cast<Object*>(p));
    return p;
}

void Object::operator delete(void* p)
{
    SystemInterface::free(MemoryType::Object, p);
}

void Object::_gcMark(ExecutionUnit* eu)
{
    gcMark(this);
    gcMark(_proto);
}

enum class GCState { ClearMarkedObj, ClearMarkedStr, MarkActive, MarkStatic, SweepObj, SweepStr, ClearNullObj, ClearNullStr };
static GCState gcState = GCState::ClearMarkedObj;
static uint32_t prevGCObjects = 0;
static uint32_t prevGCStrings = 0;
static uint8_t countSinceLastGC = 0;
static constexpr int32_t MaxGCObjectDiff = 10;
static constexpr int32_t MaxGCStringDiff = 10;
static constexpr int32_t MaxCountSinceLastGC = 50;

void Object::gc(ExecutionUnit* eu, bool force)
{
    bool didFullCycle = gcState == GCState::ClearMarkedObj;
    while (1) {
        switch(gcState) {
            case GCState::ClearMarkedObj:
                if (!force && _objectStore.size() - prevGCObjects < MaxGCObjectDiff && _stringStore.size() - prevGCStrings < MaxGCStringDiff && ++countSinceLastGC < MaxCountSinceLastGC) {
                    return;
                }
//    #ifndef NDEBUG
//                debugf("+++ before:%lu object, %lu strings\n", _objectStore.size(), _stringStore.size());
//    #endif
                for (auto it : _objectStore) {
                    if (it) {
                        it->setMarked(false);
                    }
                }
                gcState = GCState::ClearMarkedStr;
                break;
            case GCState::ClearMarkedStr:
                for (auto it : _stringStore) {
                    if (it) {
                        it->setMarked(false);
                    }
                }
                gcState = GCState::MarkActive;
                break;
            case GCState::MarkActive:
                eu->gcMark();
                gcState = GCState::MarkStatic;
                break;
            case GCState::MarkStatic:
                for (auto it : _staticObjects) {
                    it->gcMark(eu);
                }
                gcState = GCState::SweepObj;
                break;
            case GCState::SweepObj:
                for (auto& it : _objectStore) {
                    if (it && !it->isMarked()) {
                        delete it;
                        it = nullptr;
                    }
                }
                gcState = GCState::SweepStr;
                break;
            case GCState::SweepStr:
                for (auto& it : _stringStore) {
                    if (it && !it->isMarked()) {
                        delete it;
                        it = nullptr;
                    }
                }
                gcState = GCState::ClearNullObj;
                break;
            case GCState::ClearNullObj:
                _objectStore.erase(std::remove(_objectStore.begin(), _objectStore.end(), nullptr), _objectStore.end());
                gcState = GCState::ClearNullStr;
                break;
            case GCState::ClearNullStr:
                _stringStore.erase(std::remove(_stringStore.begin(), _stringStore.end(), nullptr), _stringStore.end());
//    #ifndef NDEBUG
//                debugf("--- after:%lu object, %lu strings\n", _objectStore.size(), _stringStore.size());
//    #endif
                prevGCObjects = static_cast<uint32_t>(_objectStore.size());
                prevGCStrings = static_cast<uint32_t>(_stringStore.size());
                gcState = GCState::ClearMarkedObj;
                
                if (!force || didFullCycle) {
                    return;
                }
                didFullCycle = true;
                break;
        }
        
        if (!force) {
            break;
        }
    }
}

MaterObject::~MaterObject()
{
    for (auto it : _properties) {
        NativeObject* obj = it.value.asNativeObject();
        if (obj) {
            delete obj;
            it.value = Value();
        }
    }
}

String MaterObject::toString(ExecutionUnit* eu, bool typeOnly) const
{
    if (typeOnly) {
        String typeName = eu->program()->stringFromAtom(property(eu, ATOM(eu, SA::__typeName)).asIdValue());
        return typeName.empty() ? (_isArray ? String("Array") : String("Object")) : typeName;
    }
    
    Value callable = property(eu, ATOM(eu, SA::toString));
    
    if (callable) {
        CallReturnValue retval = callable.call(eu, Value(const_cast<MaterObject*>(this)), 0, true);
        if (!retval.isReturnCount()) {
            return "";
        }
        Value stringValue = eu->stack().top(1 - retval.returnCount());
        eu->stack().pop(retval.returnCount());
        return stringValue.toStringValue(eu);
    }

    if (_isArray) {
        String s = "[ ";
        bool first = true;
        for (auto& it : _array) {
            if (first) {
                first = false;
            } else {
                s += ", ";
            }
            s += it.toStringValue(eu);
        }
        s += " ]";
        return s;
    }
    
    // FIXME: Pretty print
    String s = "{ ";
    bool first = true;
    for (auto prop : _properties) {
        if (!first) {
            s += ", ";
        } else {
            first = false;
        }
        s += eu->program()->stringFromAtom(prop.key);
        s += " : ";
        
        // Avoid loops
        if (prop.value.isObject()) {
            s += Object::toString(eu);
        } else {
            s += prop.value.toStringValue(eu);
        }
    }
    s += " }";
    return s;
}

void MaterObject::gcMark(ExecutionUnit* eu)
{
    Object::gcMark(eu);
    for (auto entry : _properties) {
        entry.value.gcMark(eu);
    }
    auto it = _properties.find(ATOM(eu, SA::__nativeObject));
    if (it != _properties.end()) {
        NativeObject* obj = it->value.asNativeObject();
        if (obj) {
            obj->gcMark(eu);
        }
    }
    
    if (!_arrayNeedsGC) {
        return;
    }
    _arrayNeedsGC = false;
    for (auto entry : _array) {
        entry.gcMark(eu);
        if (entry.needsGC()) {
            _arrayNeedsGC = true;
        }
    }
    
    if (_iterator) {
        _iterator->gcMark(eu);
    }
}

const Value MaterObject::element(ExecutionUnit* eu, const Value& elt) const
{
    if (elt.isString() || !_isArray) {
        Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
        return property(eu, prop);
    }
    int32_t index = elt.toIntValue(eu);
    return (index >= 0 && index < _array.size()) ? _array[index] : Value();
}

bool MaterObject::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
{
    if (append) {
        _array.push_back(value);
        return true;
    }
    
    if (elt.isString() || !_isArray) {
        Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
        return setProperty(eu, prop, value, Value::SetPropertyType::NeverAdd);
    }
    
    int32_t index = elt.toIntValue(eu);
    if (index < 0 || index >= _array.size()) {
        return false;
    }
    _array[index] = value;
    _arrayNeedsGC |= value.needsGC();
    return true;
}

CallReturnValue MaterObject::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    Value callee = property(eu, prop);
    if (!callee) {
        return CallReturnValue(CallReturnValue::Error::PropertyDoesNotExist);
    }
    return callee.call(eu, Value(this), nparams, false);
}

const Value MaterObject::property(ExecutionUnit* eu, const Atom& prop) const
{
    if (prop == ATOM(eu, SA::length)) {
        return Value(static_cast<int32_t>(_array.size()));
    }
    
    if (prop == ATOM(eu, SA::iterator)) {
        return Value(_iterator ?: eu->program()->global()->property(eu, ATOM(eu, SA::Iterator)).asObject());
    }

    auto it = _properties.find(prop);
    if (it == _properties.end()) {
        return proto() ? proto()->property(eu, prop) : Value();
    }
    return it->value;
}

bool MaterObject::setProperty(ExecutionUnit* eu, const Atom& prop, const Value& v, Value::SetPropertyType type)
{
    if (prop == ATOM(eu, SA::iterator)) {
        _iterator = v.asObject();
        return true;
    }
    
    Value oldValue = property(eu, prop);
    
    if (oldValue && type == Value::SetPropertyType::AlwaysAdd) {
        return false;
    }
    if (!oldValue && type == Value::SetPropertyType::NeverAdd) {
        return false;
    }

    if (prop == ATOM(eu, SA::length)) {
        _array.resize(v.asIntValue());
        return true;
    }

    return setProperty(prop, v);
}

bool MaterObject::setProperty(const Atom& prop, const Value& v)
{
    auto it = _properties.find(prop);
    if (it == _properties.end()) {
        auto ret = _properties.emplace(prop, Value());
        assert(ret.second);
        ret.first->value = v;
    } else {
        it->value = v;
    }
    return true;
}

CallReturnValue MaterObject::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    if (!ctor) {
        // FIXME: Do we want to handle calling an object as a function, like JavaScript does?
        return CallReturnValue(CallReturnValue::Error::ConstructorOnly);
    }
    
    MaterObject* obj = new MaterObject();
    Value objectValue(obj);
    obj->setProto(this);
    obj->_isArray = _isArray;

    auto it = _properties.find(ATOM(eu, SA::constructor));
    if (it != _properties.end()) {
        CallReturnValue retval = it->value.call(eu, objectValue, nparams, true);
        if (!retval.isReturnCount() || retval.returnCount() > 0) {
            return retval;
        }
    }
    eu->stack().push(objectValue);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

NativeFunction::NativeFunction(CallableFunction func, Program* program, SA sa, ObjectFactory* parent)
    : _func(func)
{
    if (!_func) {
        return;
    }
    
    Atom name = ATOM(program, sa);
    if (name && parent) {
        parent->addProperty(name, this);
    }
}

ObjectFactory::ObjectFactory(Program* program, SA sa, ObjectFactory* parent, CallableFunction constructor)
    : _constructor(constructor, program, SA::constructor, this)
{
    Atom name = ATOM(program, sa);
    if (name) {
        _obj.setProperty(ATOM(program, SA::__typeName), Value(name));
        
        if (parent) {
            parent->addProperty(name, nativeObject());
        }
    }
    
}

ObjectFactory::~ObjectFactory()
{
    // FIXME: Do we need this anymore? The _obj should be marked as usual and if it's collectable flag is set (maybe renamed to deletable)
    // we won't delete it when it's swept.
    //Object::removeObject(_obj);
}

void ObjectFactory::addProperty(Atom prop, Object* obj)
{
    assert(obj);
    addProperty(prop, Value(obj));
}

void ObjectFactory::addProperty(Atom prop, const Value& value)
{
    _obj.setProperty(prop, value);
}

void ObjectFactory::addProperty(Program* program, SA sa, Object* obj)
{
    addProperty(ATOM(program, sa), obj);
}

void ObjectFactory::addProperty(Program* program, SA sa, const Value& value)
{
    addProperty(ATOM(program, sa), value);
}

void ObjectFactory::addProperty(Program* program, SA sa, CallableFunctionPtr f)
{
    addProperty(ATOM(program, sa), Value(f));    
}

Object* ObjectFactory::create(Atom objectName, ExecutionUnit* eu, uint32_t nparams)
{
    Value objectValue = eu->program()->global()->property(eu, objectName);
    if (!objectValue) {
        return nullptr;
    }
    
    Object* object = objectValue.asObject();
    if (!object) {
        return nullptr;
    }

    CallReturnValue r = object->call(eu, Value(), nparams, true);
    Value retValue;
    if (r.isReturnCount() && r.returnCount() > 0) {
        retValue = eu->stack().top(1 - r.returnCount());
        eu->stack().pop(r.returnCount());
    }    
    return retValue.asObject();
}

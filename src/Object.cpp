/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Object.h"

#include "ExecutionUnit.h"
#include "Function.h"
#include "MStream.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

std::vector<Mad<String>> Object::_stringStore;
std::vector<Mad<Object>> Object::_objectStore;
std::vector<Mad<Object>> Object::_staticObjects;
std::vector<Mad<ExecutionUnit>> Object::_euStore;

void Object::memoryInfo(MemoryInfo& info)
{
    SystemInterface::memoryInfo(info);
    info.numAllocationsByType.resize(static_cast<uint32_t>(MemoryType::NumTypes));
    
    for (uint32_t i = 0; i < static_cast<uint32_t>(MemoryType::NumTypes); ++i) {
        //info.numAllocationsByType[i] = MallocatorBase::entry(static_cast<MemoryType>(i)).count;
    }
}

enum class GCState { ClearMarkedObj, ClearMarkedStr, MarkActive, MarkStatic, SweepObj, SweepStr, ClearNullObj, ClearNullStr };
static GCState gcState = GCState::ClearMarkedObj;
static uint32_t prevGCObjects = 0;
static uint32_t prevGCStrings = 0;
static uint8_t countSinceLastGC = 0;
static constexpr int32_t MaxGCObjectDiff = 10;
static constexpr int32_t MaxGCStringDiff = 10;
static constexpr int32_t MaxCountSinceLastGC = 50;

void Object::gc(bool force)
{
    system()->lock();
    bool didFullCycle = gcState == GCState::ClearMarkedObj;
    while (1) {
        switch(gcState) {
            case GCState::ClearMarkedObj:
                if (!force && _objectStore.size() - prevGCObjects < MaxGCObjectDiff && _stringStore.size() - prevGCStrings < MaxGCStringDiff && ++countSinceLastGC < MaxCountSinceLastGC) {
                    system()->unlock();
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
                for (auto it : _euStore) {
                    it->gcMark();
                }
                gcState = GCState::MarkStatic;
                break;
            case GCState::MarkStatic:
                for (auto it : _staticObjects) {
                    it->gcMark();
                }
                gcState = GCState::SweepObj;
                break;
            case GCState::SweepObj:
                for (auto& it : _objectStore) {
                    if (it && !it->isMarked()) {
                        it.destroy();
                        it.reset();
                    }
                }
                gcState = GCState::SweepStr;
                break;
            case GCState::SweepStr:
                for (auto& it : _stringStore) {
                    if (it && !it->isMarked()) {
                        it.destroy();
                        it.reset();
                    }
                }
                gcState = GCState::ClearNullObj;
                break;
            case GCState::ClearNullObj:
                _objectStore.erase(
                    std::remove(_objectStore.begin(), _objectStore.end(), Mad<Object>()), _objectStore.end());
                gcState = GCState::ClearNullStr;
                break;
            case GCState::ClearNullStr:
                _stringStore.erase(std::remove(_stringStore.begin(), _stringStore.end(), Mad<String>()), _stringStore.end());
//    #ifndef NDEBUG
//                debugf("--- after:%lu object, %lu strings\n", _objectStore.size(), _stringStore.size());
//    #endif
                prevGCObjects = static_cast<uint32_t>(_objectStore.size());
                prevGCStrings = static_cast<uint32_t>(_stringStore.size());
                gcState = GCState::ClearMarkedObj;
                
                if (!force || didFullCycle) {
                    system()->unlock();
                    return;
                }
                didFullCycle = true;
                break;
        }
        
        if (!force) {
            break;
        }
    }
    system()->unlock();
}

Atom Object::typeName(ExecutionUnit* eu) const
{
    Value nameValue = property(eu, Atom(SA::__typeName));
    return nameValue.asIdValue();
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
        String typeName = eu->program()->stringFromAtom(property(eu, Atom(SA::__typeName)).asIdValue());
        return typeName.empty() ? (_isArray ? String("Array") : String("Object")) : typeName;
    }
    
    Value callable = property(eu, Atom(SA::toString));
    
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
            if (prop.value.isString()) {
                s += "\"";
            }
            s += prop.value.toStringValue(eu);
            if (prop.value.isString()) {
                s += "\"";
            }
        }
    }
    s += " }";
    return s;
}

void MaterObject::gcMark()
{
    if (isMarked()) {
        return;
    }
    Object::gcMark();
    for (auto entry : _properties) {
        entry.value.gcMark();
    }
    if (_iterator) {
        _iterator->gcMark();
    }
    if (_nativeObject) {
        _nativeObject->gcMark();
    }
    if (_arrayNeedsGC) {
        _arrayNeedsGC = false;
        for (auto entry : _array) {
            entry.gcMark();
            if (entry.needsGC()) {
                _arrayNeedsGC = true;
            }
        }
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
        if (prop == Atom(SA::pop_back)) {
            if (!_array.empty()) {
                _array.pop_back();
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }

        if (prop == Atom(SA::pop_front)) {
            if (!_array.empty()) {
                _array.erase(_array.begin());
            }
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }

        if (prop == Atom(SA::push_back)) {
            // Push all the params
            for (int32_t i = 1 - nparams; i <= 0; ++i) {
                _array.push_back(eu->stack().top(i));
            }

            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }
        
        if (prop == Atom(SA::push_front)) {
            // Push all the params, efficiently
            if (nparams == 1) {
                _array.insert(_array.begin(), eu->stack().top());
            }
            
            std::vector<Value> vec;
            for (int32_t i = 1 - nparams; i <= 0; ++i) {
                vec.push_back(eu->stack().top(i));
            }
            _array.insert(_array.begin(), vec.begin(), vec.end());

            return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
        }

        if (prop == Atom(SA::join)) {
            String separator = (nparams > 0) ? eu->stack().top(1 - nparams).toStringValue(eu) : String("");
            String s;
            bool first = true;
            for (auto it : _array) {
                if (!first) {
                    s += separator;
                } else {
                    first = false;
                }
                s += it.toStringValue(eu);
            }
            
            eu->stack().push(Value(eu->program()->createString(s)));
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        
        return CallReturnValue(CallReturnValue::Error::PropertyDoesNotExist);
    }
    return callee.call(eu, Value(this), nparams, false);
}

const Value MaterObject::property(ExecutionUnit* eu, const Atom& prop) const
{
    if (prop == Atom(SA::length)) {
        return Value(static_cast<int32_t>(_array.size()));
    }
    
    if (prop == Atom(SA::front)) {
        return _array.empty() ? Value() : _array[0];
    }
    
    if (prop == Atom(SA::back)) {
        return _array.empty() ? Value() : _array[_array.size() - 1];
    }
    
    if (prop == Atom(SA::iterator)) {
        return Value(_iterator ?: eu->program()->global()->property(eu, Atom(SA::Iterator)).asObject());
    }

    if (prop == Atom(SA::__nativeObject)) {
        return _nativeObject ? Value(_nativeObject) : Value();
    }

    auto it = _properties.find(prop);
    if (it == _properties.end()) {
        return proto() ? proto()->property(eu, prop) : Value();
    }
    return it->value;
}

bool MaterObject::setProperty(ExecutionUnit* eu, const Atom& prop, const Value& v, Value::SetPropertyType type)
{
    if (prop == Atom(SA::iterator)) {
        _iterator = v.asObject();
        return true;
    }
    
    if (prop == Atom(SA::__nativeObject)) {
        _nativeObject = v.asNativeObject();
        return true;
    }
    
    Value oldValue = property(eu, prop);
    
    if (oldValue && type == Value::SetPropertyType::AlwaysAdd) {
        return false;
    }
    if (!oldValue && type == Value::SetPropertyType::NeverAdd) {
        return false;
    }

    if (prop == Atom(SA::length)) {
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

    auto it = _properties.find(Atom(SA::constructor));
    if (it != _properties.end()) {
        CallReturnValue retval = it->value.call(eu, objectValue, nparams, true);
        // ctor should not return anything
        if (!retval.isReturnCount() || retval.returnCount() > 0) {
            return retval;
        }
    }
    eu->stack().push(objectValue);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

ObjectFactory::ObjectFactory(Mad<Program> program, SA sa, ObjectFactory* parent, NativeFunction constructor)
    : _constructor(constructor)
{
    _obj = Mad<MaterObject>::create();
    
    if (!program) {
        return;
    }
    
    Atom name = Atom(sa);
    if (name) {
        _obj->setProperty(Atom(SA::__typeName), Value(name));
        
        if (parent) {
            parent->addProperty(name, nativeObject());
        }
    }
    if (constructor && name) {
        addProperty(program, SA::constructor, _constructor);
    }
}

ObjectFactory::~ObjectFactory()
{
    // FIXME: Do we need this anymore? The _obj should be marked as usual and if it's collectable flag is set (maybe renamed to deletable)
    // we won't delete it when it's swept.
    //Object::removeObject(_obj);
}

void ObjectFactory::addProperty(Atom prop, Mad<Object> obj)
{
    assert(obj);
    addProperty(prop, Value(obj));
}

void ObjectFactory::addProperty(Atom prop, const Value& value)
{
    _obj->setProperty(prop, value);
}

void ObjectFactory::addProperty(Mad<Program> program, SA sa, Mad<Object> obj)
{
    addProperty(Atom(sa), obj);
}

void ObjectFactory::addProperty(Mad<Program> program, SA sa, const Value& value)
{
    addProperty(Atom(sa), value);
}

void ObjectFactory::addProperty(Mad<Program> program, SA sa, NativeFunction f)
{
    addProperty(Atom(sa), Value(f));    
}

Mad<Object> ObjectFactory::create(Atom objectName, ExecutionUnit* eu, uint32_t nparams)
{
    Value objectValue = eu->program()->global()->property(eu, objectName);
    if (!objectValue) {
        return nullptr;
    }
    
    return create(objectValue.asObject(), eu, nparams);
}

Mad<Object> ObjectFactory::create(Mad<Object> proto, ExecutionUnit* eu, uint32_t nparams)
{
    if (!proto) {
        return nullptr;
    }

    CallReturnValue r = proto->call(eu, Value(), nparams, true);
    Value retValue;
    if (r.isReturnCount() && r.returnCount() > 0) {
        retValue = eu->stack().top(1 - r.returnCount());
        eu->stack().pop(r.returnCount());
        return retValue.asObject();
    } else {
        return nullptr;
    }
}

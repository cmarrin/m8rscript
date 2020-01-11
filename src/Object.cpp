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
#include "GC.h"
#include "MStream.h"
#include "Program.h"
#include "SystemInterface.h"

using namespace m8r;

void NativeObject::operator delete(void* p)
{
    Mad<NativeObject> mad(reinterpret_cast<NativeObject*>(p));
    mad.destroy();
}

void Object::operator delete(void* p)
{
    Mad<Object> mad(reinterpret_cast<Object*>(p));
    mad.destroy();
}

void Object::addToObjectStore(RawMad obj)
{
    GC::addToStore<MemoryType::Object>(obj);
}

CallReturnValue Object::construct(const Value& proto, ExecutionUnit* eu, uint32_t nparams)
{
    Mad<Object> obj = create<MaterObject>();
    Value objectValue(obj);
    obj->setProto(proto);
    
    Value ctor = proto.property(eu, Atom(SA::constructor));
    if (ctor) {
        CallReturnValue retval = ctor.call(eu, objectValue, nparams);
        // ctor should not return anything
        if (!retval.isReturnCount() || retval.returnCount() > 0) {
            return retval;
        }
    }
    eu->stack().push(objectValue);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}


String Object::toString(ExecutionUnit* eu, bool typeOnly) const
{
    if (typeOnly) {
        return eu->program()->stringFromAtom(typeName());
    }
    
    Mad<Object> obj = property(Atom(SA::toString)).asObject();
    
    if (obj.valid()) {
        CallReturnValue retval = obj->call(eu, Value(Mad<Object>(this)), 0);
        if (!retval.isReturnCount()) {
            return "";
        }
        Value stringValue = eu->stack().top(1 - retval.returnCount());
        eu->stack().pop(retval.returnCount());
        return stringValue.toStringValue(eu);
    }

    return String();
}

MaterObject::~MaterObject()
{
    for (auto it : _properties) {
        Mad<NativeObject> obj = it.value.asNativeObject();
        if (obj.valid()) {
            delete obj.get();
            it.value = Value();
        }
    }
}

String MaterObject::toString(ExecutionUnit* eu, bool typeOnly) const
{
    String s = Object::toString(eu, typeOnly);
    if (!s.empty()) {
        return s;
    }

    // FIXME: Pretty print
    s = "{ ";
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

String MaterArray::toString(ExecutionUnit* eu, bool typeOnly) const
{
    String s = Object::toString(eu, typeOnly);
    if (!s.empty()) {
        return s;
    }

    s = "[ ";
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

void MaterObject::gcMark()
{
    if (isMarked()) {
        return;
    }
    Object::gcMark();
    for (auto entry : _properties) {
        entry.value.gcMark();
    }
}

void MaterArray::gcMark()
{
    if (isMarked()) {
        return;
    }
    Object::gcMark();

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
    Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
    return property(prop);
}

const Value MaterArray::element(ExecutionUnit* eu, const Value& elt) const
{
    int32_t index = elt.toIntValue(eu);
    return (index >= 0 && index < _array.size()) ? _array[index] : Value();
}

bool MaterObject::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
{
    Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
    return setProperty(prop, value, Value::SetPropertyType::NeverAdd);
}

bool MaterArray::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, bool append)
{
    if (append) {
        _array.push_back(value);
        _arrayNeedsGC |= value.needsGC();
        return true;
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
    Value callee = property(prop);
    if (!callee) {
        return CallReturnValue(CallReturnValue::Error::PropertyDoesNotExist);
    }
    
    return callee.call(eu, Value(Mad<Object>(this)), nparams);
}

CallReturnValue MaterArray::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
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
        
        Vector<Value> vec;
        vec.reserve(nparams);
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
        
        eu->stack().push(Value(ExecutionUnit::createString(s)));
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
    }
    
    return CallReturnValue(CallReturnValue::Error::PropertyDoesNotExist);
}

const Value MaterObject::property(const Atom& prop) const
{
    auto it = _properties.find(prop);
    return (it == _properties.end()) ? (proto() ? proto().property(prop) : Value()) : it->value;
}

const Value MaterArray::property(const Atom& prop) const
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
    
    return Value();
}

bool MaterObject::setProperty(const Atom& prop, const Value& v, Value::SetPropertyType type)
{
    Value oldValue = property(prop);
    
    if (oldValue && type == Value::SetPropertyType::AlwaysAdd) {
        return false;
    }
    if (!oldValue && type == Value::SetPropertyType::NeverAdd) {
        return false;
    }

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

bool MaterArray::setProperty(const Atom& prop, const Value& v, Value::SetPropertyType type)
{
    if (prop == Atom(SA::length)) {
        _array.resize(v.asIntValue());
        return true;
    }
    return false;
}

ObjectFactory::ObjectFactory(SA sa, ObjectFactory* parent, NativeFunction constructor)
    : _constructor(constructor)
{
    _obj = Object::create<MaterObject>();
    
    Atom name = Atom(sa);
    if (name) {
        _obj->setTypeName(name);
        
        if (parent) {
            parent->addProperty(name, nativeObject());
        }
    }
    if (constructor && name) {
        addProperty(SA::constructor, _constructor);
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
    assert(obj.valid());
    addProperty(prop, Value(obj));
}

void ObjectFactory::addProperty(Atom prop, const Value& value)
{
    _obj->setProperty(prop, value);
}

void ObjectFactory::addProperty(SA sa, Mad<Object> obj)
{
    addProperty(Atom(sa), obj);
}

void ObjectFactory::addProperty(SA sa, const Value& value)
{
    addProperty(Atom(sa), value);
}

void ObjectFactory::addProperty(SA sa, NativeFunction f)
{
    addProperty(Atom(sa), Value(f));    
}

Mad<Object> ObjectFactory::create(Atom objectName, ExecutionUnit* eu, uint32_t nparams)
{
    Value protoValue = Global::shared()->property(objectName);
    CallReturnValue ret = protoValue.call(eu, Value(), nparams);
    
    if (ret.isReturnCount() && ret.returnCount() > 0) {
        Value value = eu->stack().top(1 - ret.returnCount());
        eu->stack().pop(ret.returnCount());
        return value.asObject();
    } else {
        return Mad<Object>();
    }
}

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

using namespace m8rscript;
using namespace m8r;

//void operator delete(void* p) throw()
//{
//    ::free(p);
//}
//
//void operator delete [](void* p) throw()
//{
//    ::free(p);
//}
//
//void operator delete(void* p, std::size_t) throw()
//{
//    ::free(p);
//}
//
//void operator delete[](void* p, std::size_t) throw()
//{
//    ::free(p);
//}

void Object::addToObjectStore(RawMad obj)
{
    GC::addToStore<MemoryType::Object>(obj);
}

CallReturnValue Object::construct(const Value& proto, ExecutionUnit* eu, uint32_t nparams)
{
    Mad<Object> obj = create<MaterObject>();
    Value objectValue(obj);
    obj->setProto(proto);
    
    Value ctor = proto.property(eu, SAtom(SA::constructor));
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


Value Object::value(ExecutionUnit* eu) const
{
    Value propValue = property(SAtom(SA::getValue));
    CallReturnValue retval(Error::Code::PropertyDoesNotExist);
    
    if (propValue.isObject()) {
        retval = propValue.asObject()->call(eu, Value(Mad<Object>(this)), 0);
    } else if (propValue.isNativeFunction()) {
        retval = propValue.asNativeFunction()(eu, Value(Mad<Object>(this)), 0);
    }

    if (!retval.isReturnCount() || retval.returnCount() == 0) {
        return Value();
    }
    Value v = eu->stack().top(1 - retval.returnCount());
    eu->stack().pop(retval.returnCount());
    return v;
}

m8r::String Object::toString(ExecutionUnit* eu, bool typeOnly) const
{
    if (typeOnly) {
        return eu->program()->stringFromAtom(typeName());
    }

    Value v = value(eu);
    if (v) {
        assert(!v.isObject());
        return v.toStringValue(eu);
    }
    
    Value propValue = property(SAtom(SA::toString));
    CallReturnValue retval(Error::Code::PropertyDoesNotExist);
    
    if (propValue.isObject()) {
        retval = propValue.asObject()->call(eu, Value(Mad<Object>(this)), 0);
    } else if (propValue.isNativeFunction()) {
        retval = propValue.asNativeFunction()(eu, Value(Mad<Object>(this)), 0);
    }

    if (!retval.isReturnCount()) {
        return "";
    }
    Value stringValue = eu->stack().top(1 - retval.returnCount());
    eu->stack().pop(retval.returnCount());
    return stringValue.toStringValue(eu);


    return String();
}

MaterObject::~MaterObject()
{
    // Call destructor, if any
    Value dtor = property(SAtom(SA::__destructor));
    if (dtor.isNativeFunction()) {
        dtor.asNativeFunction()(nullptr, Value(Mad<Object>(this)), 0);
    }
    
    for (auto it : _properties) {
        Mad<NativeObject> obj = it.value.asNativeObject();
        if (obj.valid()) {
            obj.destroy();
            it.value = Value();
        }
    }
}

m8r::String MaterObject::toString(ExecutionUnit* eu, bool typeOnly) const
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

m8r::String MaterArray::toString(ExecutionUnit* eu, bool typeOnly) const
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
    return (index >= 0 && index < static_cast<int32_t>(_array.size())) ? _array[index] : Value();
}

bool MaterObject::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType type)
{
    Atom prop = eu->program()->atomizeString(elt.toStringValue(eu).c_str());
    return setProperty(prop, value, type);
}

bool MaterArray::setElement(ExecutionUnit* eu, const Value& elt, const Value& value, Value::SetType type)
{
    if (type == Value::SetType::AlwaysAdd) {
        value.gcMark();
       _array.push_back(value);
        _arrayNeedsGC |= value.needsGC();
        return true;
    }
    
    int32_t index = elt.toIntValue(eu);
    if ((index < 0 || index >= static_cast<int32_t>(_array.size())) && (type == Value::SetType::NeverAdd)) {
        return false;
    }
    
    if (static_cast<int32_t>(_array.size()) <= index) {
        _array.resize(index + 1);
    }
    
    value.gcMark();
    _array[index] = value;
    _arrayNeedsGC |= value.needsGC();
    return true;
}

CallReturnValue MaterObject::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    Value callee = property(prop);
    if (!callee) {
        return CallReturnValue(Error::Code::PropertyDoesNotExist);
    }
    
    return callee.call(eu, Value(Mad<Object>(this)), nparams);
}

CallReturnValue MaterArray::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    if (prop == SAtom(SA::pop_back)) {
        if (!_array.empty()) {
            _array.pop_back();
        }
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }

    if (prop == SAtom(SA::pop_front)) {
        if (!_array.empty()) {
            _array.erase(_array.begin());
        }
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }

    if (prop == SAtom(SA::push_back)) {
        // Push all the params
        for (int32_t i = 1 - nparams; i <= 0; ++i) {
            const Value& value = eu->stack().top(i);
            value.gcMark();
            _array.push_back(value);
            _arrayNeedsGC |= value.needsGC();
        }

        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }
    
    if (prop == SAtom(SA::push_front)) {
        // Push all the params, efficiently
        if (nparams == 1) {
            const Value& value = eu->stack().top();
            value.gcMark();
            _array.insert(_array.begin(), value);
            _arrayNeedsGC |= value.needsGC();
        } else {
            Vector<Value> vec;
            vec.reserve(nparams);
            for (int32_t i = 1 - nparams; i <= 0; ++i) {
                const Value& value = eu->stack().top(i);
                value.gcMark();
                vec.push_back(value);
                _arrayNeedsGC |= value.needsGC();
            }
            _array.insert(_array.begin(), vec.begin(), vec.end());
        }
        return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
    }

    if (prop == SAtom(SA::join)) {
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
    
    return CallReturnValue(Error::Code::PropertyDoesNotExist);
}

const Value MaterObject::property(const Atom& prop) const
{
    auto it = _properties.find(prop);
    return (it == _properties.end()) ? (proto() ? proto().property(prop) : Value()) : it->value;
}

const Value MaterArray::property(const Atom& prop) const
{
    if (prop == SAtom(SA::length)) {
        return Value(static_cast<int32_t>(_array.size()));
    }
    
    if (prop == SAtom(SA::front)) {
        return _array.empty() ? Value() : _array[0];
    }
    
    if (prop == SAtom(SA::back)) {
        return _array.empty() ? Value() : _array[_array.size() - 1];
    }
    
    return Value();
}

bool MaterObject::setProperty(const Atom& prop, const Value& v, Value::SetType type)
{
    Value oldValue = property(prop);
    
    if (oldValue && type == Value::SetType::AlwaysAdd) {
        return false;
    }
    if (!oldValue && type == Value::SetType::NeverAdd) {
        return false;
    }

    v.gcMark();
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

bool MaterArray::setProperty(const Atom& prop, const Value& v, Value::SetType type)
{
    if (prop == SAtom(SA::length)) {
        _array.resize(v.asIntValue());
        return true;
    }
    return false;
}

ObjectFactory::ObjectFactory(SA sa, ObjectFactory* parent, NativeFunction constructor)
    : _constructor(constructor)
{
    _obj = Object::create<MaterObject>();
    
    Atom name = SAtom(sa);
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
    addProperty(SAtom(sa), obj);
}

void ObjectFactory::addProperty(SA sa, const Value& value)
{
    addProperty(SAtom(sa), value);
}

void ObjectFactory::addProperty(SA sa, NativeFunction f)
{
    addProperty(SAtom(sa), Value(f));    
}

Mad<Object> ObjectFactory::create(Atom objectName, ExecutionUnit* eu, uint32_t nparams)
{
    Value protoValue = Global::shared()->property(objectName);
    CallReturnValue ret = protoValue.construct(eu, nparams);
    
    if (ret.isReturnCount() && ret.returnCount() > 0) {
        Value value = eu->stack().top(1 - ret.returnCount());
        eu->stack().pop(ret.returnCount());
        return value.asObject();
    } else {
        return Mad<Object>();
    }
}

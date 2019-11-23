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

void NativeObject::operator delete(void* p, std::size_t sz)
{
    Mad<NativeObject> mad(reinterpret_cast<NativeObject*>(p));
    mad.destroy(static_cast<uint16_t>(sz));
}

void Object::operator delete(void* p, std::size_t sz)
{
    Mad<Object> mad(reinterpret_cast<Object*>(p));
    mad.destroy(static_cast<uint16_t>(sz));
}

Atom Object::typeName() const
{
    Value nameValue = property(Atom(SA::__typeName));
    return nameValue.asIdValue();
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
    if (typeOnly) {
        String typeName = eu->program()->stringFromAtom(property(Atom(SA::__typeName)).asIdValue());
        return typeName.empty() ? (_isArray ? String("Array") : String("Object")) : typeName;
    }
    
    Value callable = property(Atom(SA::toString));
    
    if (callable) {
        CallReturnValue retval = callable.call(eu, Value(Mad<MaterObject>(this)), 0, true);
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
        return property(prop);
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
        return setProperty(prop, value, Value::SetPropertyType::NeverAdd);
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
            
            eu->stack().push(Value(Mad<String>::create(s)));
            return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
        }
        
        return CallReturnValue(CallReturnValue::Error::PropertyDoesNotExist);
    }
    return callee.call(eu, Value(Mad<Object>(this)), nparams, false);
}

const Value MaterObject::property(const Atom& prop) const
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
    
    auto it = _properties.find(prop);
    return (it == _properties.end()) ? (proto() ? proto()->property(prop) : Value()) : it->value;
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

    if (prop == Atom(SA::length)) {
        _array.resize(v.asIntValue());
        return true;
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

CallReturnValue MaterObject::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    if (!ctor) {
        // FIXME: Do we want to handle calling an object as a function, like JavaScript does?
        return CallReturnValue(CallReturnValue::Error::ConstructorOnly);
    }
    
    Mad<MaterObject> obj = Mad<MaterObject>::create();
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

ObjectFactory::ObjectFactory(SA sa, ObjectFactory* parent, NativeFunction constructor)
    : _constructor(constructor)
{
    _obj = Mad<MaterObject>::create();
    
    Atom name = Atom(sa);
    if (name) {
        _obj->setProperty(Atom(SA::__typeName), Value(name));
        
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
    Value objectValue = eu->program()->global()->property(objectName);
    if (!objectValue) {
        return Mad<Object>();
    }
    
    return create(objectValue.asObject(), eu, nparams);
}

Mad<Object> ObjectFactory::create(Mad<Object> proto, ExecutionUnit* eu, uint32_t nparams)
{
    if (!proto.valid()) {
        return Mad<Object>();
    }

    CallReturnValue r = proto->call(eu, Value(), nparams, true);
    Value retValue;
    if (r.isReturnCount() && r.returnCount() > 0) {
        retValue = eu->stack().top(1 - r.returnCount());
        eu->stack().pop(r.returnCount());
        return retValue.asObject();
    } else {
        return Mad<Object>();
    }
}

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

using namespace m8r;

std::vector<String*> Object::_stringStore;
std::vector<Object*> Object::_objectStore;
std::vector<Object*> Object::_staticObjects;

void Object::_gcMark(ExecutionUnit* eu)
{
    gcMark(this);
    gcMark(_proto);
}

void Object::gc(ExecutionUnit* eu)
{
    for (auto it : _objectStore) {
        if (it) {
            it->setMarked(false);
        }
    }
    for (auto it : _stringStore) {
        if (it) {
            it->setMarked(false);
        }
    }
    
    eu->gcMark();
    
    for (auto it : _staticObjects) {
        it->gcMark(eu);
    }
    
    for (auto& it : _objectStore) {
        if (it && !it->isMarked() && it->isCollectable()) {
            delete it;
            it = nullptr;
        }
    }
    
    for (auto& it : _stringStore) {
        if (it && !it->isMarked()) {
            delete it;
            it = nullptr;
        }
    }
}

void Object::gcMark(const Value& value)
{
    String* string = value.asString();
    if (string) {
        string->setMarked(true);
        return;
    }
    
    gcMark(value.asObject());
}

MaterObject::~MaterObject()
{
    auto it = _properties.find(ATOM(__nativeObject));
    if (it != _properties.end()) {
        NativeObject* obj = it->value.asNativeObject();
        if (obj) {
            delete obj;
        }
    }
    removeNoncollectableObjects();
}

String MaterObject::toString(ExecutionUnit* eu, bool typeOnly) const
{
    String typeName = eu->program()->stringFromAtom(property(eu, ATOM(__typeName)).asIdValue());
    
    if (typeOnly) {
        return typeName.empty() ? String("Object") : typeName;
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
    auto it = _properties.find(ATOM(__nativeObject));
    if (it != _properties.end()) {
        NativeObject* obj = it->value.asNativeObject();
        if (obj) {
            obj->gcMark(eu);
        }
    }
}

CallReturnValue MaterObject::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    Value callee = property(eu, prop);
    return callee.call(eu, Value(this), nparams, false);
}

const Value MaterObject::property(ExecutionUnit* eu, const Atom& prop) const
{
    auto it = _properties.find(prop);
    if (it == _properties.end()) {
        return proto() ? proto()->property(eu, prop) : Value();
    }
    return it->value;
}

bool MaterObject::setProperty(ExecutionUnit* eu, const Atom& prop, const Value& v, Value::SetPropertyType type)
{
    Value oldValue = property(nullptr, prop);
    
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
        return true;
    }
    
    it->value = v;
    return true;
}

CallReturnValue MaterObject::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    if (!ctor) {
        // FIXME: Do we want to handle calling an object as a function, like JavaScript does?
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    MaterObject* obj = new MaterObject();
    Value objectValue(obj);
    obj->setProto(this);

    auto it = _properties.find(ATOM(constructor));
    if (it != _properties.end()) {
        CallReturnValue retval = it->value.call(eu, objectValue, nparams, true);
        if (!retval.isReturnCount() || retval.returnCount() > 0) {
            return retval;
        }
    }
    eu->stack().push(objectValue);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

// FIXME: We need to mark non-collectable objects as "zombies" and then clear them out of
// the object store without deleting them. Or should we maybe change "collectable" to
// "deletable" and mark them as usual. But when it comes time to sweep, remove them from
// the object store, but don't delete them. The idea of something that should never be
// collected is really now handled by the concept of static objects.
void MaterObject::removeNoncollectableObjects()
{
//    for (auto it : _properties) {
//        Object* obj = it.value.asObject();
//        if (obj && !obj->collectable()) {
//            Object::removeObject(it.value.asObject());
//        }
//    }
}

NativeFunction::NativeFunction(Func func)
    : _func(func)
{
}

ObjectFactory::ObjectFactory(Program* program, const char* name)
{
    _obj.setCollectable(false);
    if (name) {
        _obj.setProperty(nullptr, ATOM(__typeName), Value(program->atomizeString(name)), Value::SetPropertyType::AlwaysAdd);
    }
}

ObjectFactory::~ObjectFactory()
{
    // FIXME: Do we need this anymore? The _obj should be marked as usual and if it's collectable flag is set (maybe renamed to deletable)
    // we won't delete it when it's swept.
    //Object::removeObject(_obj);
}

void ObjectFactory::addProperty(Program* program, Atom prop, Object* obj)
{
    assert(obj);
    obj->setCollectable(false);
    addProperty(program, prop, Value(obj));
}

void ObjectFactory::addProperty(Program* program, Atom prop, const Value& value)
{
    _obj.setProperty(nullptr, prop, value, Value::SetPropertyType::AlwaysAdd);
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

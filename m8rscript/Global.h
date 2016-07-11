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

#pragma once

#include "Object.h"

namespace m8r {

class SystemInterface;

class Global : public Object {
public:
    static constexpr uint32_t PLATFORM_GPIO_FLOAT = 0;
    static constexpr uint32_t PLATFORM_GPIO_PULLUP = 1;

    static constexpr uint32_t PLATFORM_GPIO_INT = 2;
    static constexpr uint32_t PLATFORM_GPIO_OUTPUT = 1;
    static constexpr uint32_t PLATFORM_GPIO_OPENDRAIN = 3;
    static constexpr uint32_t PLATFORM_GPIO_INPUT = 0;

    static constexpr uint32_t PLATFORM_GPIO_HIGH = 1;
    static constexpr uint32_t PLATFORM_GPIO_LOW = 0;

    Global(SystemInterface*);
    
    virtual ~Global();
    
    virtual const char* typeName() const override { return "Global"; }

    // Global has built-in properties. Handle those here
    virtual int32_t propertyIndex(const Atom& s, bool canExist) override;
    virtual Value propertyRef(int32_t index) override;
    virtual const Value property(int32_t index) const override;
    virtual bool setProperty(int32_t index, const Value&) override;
    virtual Atom propertyName(uint32_t index) const override;
    virtual size_t propertyCount() const override;
    virtual Value appendPropertyRef(uint32_t index, const Atom&) override;

    virtual int32_t callProperty(uint32_t index, Program*, ExecutionUnit*, uint32_t nparams) override;
    
protected:
    virtual bool serialize(Stream*) const override
    {
        return true;
    }

protected:
    virtual uint64_t currentTime() const = 0;

    uint64_t _startTime = 0;

    enum class Property : uint8_t
    {
        None = 0,
        print = 0x01,
        System = 0x10, System_delay, 
        Date = 0x20, Date_now,
        GPIO = 0x30, GPIO_pinMode, GPIO_digitalWrite, GPIO_OUTPUT, GPIO_INPUT, GPIO_HIGH, GPIO_LOW,
            GPIO_FLOAT, GPIO_PULLUP, GPIO_INT, GPIO_OPENDRAIN,
        Serial = 0x40, Serial_begin, Serial_print, Serial_printf,
        Base64 = 0x50, Base64_encode, Base64_decode,
    };
    static Map<Atom, Property> _properties;
    
    SystemInterface* _system;

private:        
    static Atom _nowAtom;
    static Atom _delayAtom;
    static Atom _pinModeAtom;
    static Atom _digitalWriteAtom;
    static Atom _OUTPUTAtom;
    static Atom _INPUTAtom;
    static Atom _LOWAtom;
    static Atom _HIGHAtom;
    static Atom _FLOATAtom;
    static Atom _PULLUPAtom;
    static Atom _INTAtom;
    static Atom _OPENDRAINAtom;
    static Atom _beginAtom;
    static Atom _printAtom;
    static Atom _printfAtom;
    static Atom _encodeAtom;
    static Atom _decodeAtom;
};
    
}

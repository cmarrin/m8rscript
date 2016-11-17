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

    Global(SystemInterface*, Program*);
    
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

    virtual CallReturnValue callProperty(uint32_t index, ExecutionUnit*, uint32_t nparams) override;
    
    SystemInterface* system() const { return _system; }

protected:
    virtual bool serialize(Stream*, Error&) const override
    {
        return true;
    }

    virtual bool deserialize(Stream*, Error&, Program*, const AtomTable&, const std::vector<char>&) override
    {
        return true;
    }

    uint64_t _startTime = 0;

    static constexpr size_t PropertyCount = 5; // Date, GPIO, Serial, Base64 and System
    
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
    
    SystemInterface* _system;

private:        
    Atom _DateAtom;
    Atom _GPIOAtom;
    Atom _SerialAtom;
    Atom _Base64Atom;
    Atom _SystemAtom;
    
    Atom _nowAtom;
    Atom _delayAtom;
    Atom _pinModeAtom;
    Atom _digitalWriteAtom;
    Atom _OUTPUTAtom;
    Atom _INPUTAtom;
    Atom _LOWAtom;
    Atom _HIGHAtom;
    Atom _FLOATAtom;
    Atom _PULLUPAtom;
    Atom _INTAtom;
    Atom _OPENDRAINAtom;
    Atom _beginAtom;
    Atom _printAtom;
    Atom _printfAtom;
    Atom _encodeAtom;
    Atom _decodeAtom;
};
    
}

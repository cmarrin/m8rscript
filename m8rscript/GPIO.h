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

#include "Defines.h"
#include "Object.h"
#include <functional>

namespace m8r {

class PinMode : public Object {
public:
    PinMode(Program*);

    virtual const char* typeName() const override { return "PinMode"; }

    virtual const Value property(ExecutionUnit*, const Atom&) const override;

private:
    Atom _OutputAtom;
    Atom _OutputOpenDrainAtom;
    Atom _InputAtom;
    Atom _InputPullupAtom;
    Atom _InputPulldownAtom;
};

class Trigger : public Object {
public:
    Trigger(Program*);

    virtual const char* typeName() const override { return "Trigger"; }

    virtual const Value property(ExecutionUnit*, const Atom&) const override;

private:
    Atom _NoneAtom;
    Atom _RisingEdgeAtom;
    Atom _FallingEdgeAtom;
    Atom _BothEdgesAtom;
    Atom _LowAtom;
    Atom _HighAtom;
};

class GPIO : public Object {
public:
    GPIO(Program*);
    virtual ~GPIO() { }
    
    virtual const char* typeName() const override { return "GPIO"; }

    virtual const Value property(ExecutionUnit*, const Atom&) const override;

    virtual CallReturnValue callProperty(ExecutionUnit*, Atom prop, uint32_t nparams) override;

private:
    Atom _setPinModeAtom;
    Atom _digitalWriteAtom;
    Atom _digitalReadAtom;
    Atom _onInterruptAtom;
    Atom _PinModeAtom;
    Atom _TriggerAtom;

    m8r::PinMode _pinMode;
    m8r::Trigger _trigger;
};

}

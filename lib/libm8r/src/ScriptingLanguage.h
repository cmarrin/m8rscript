/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "SharedPtr.h"

namespace m8r {

// To register a scripting language create subclass of the
// ScriptingLanguage class and implement the create method.
// Then instantiate this class in the global space.

class Executable;

class ScriptingLanguage : public Shared {
public:
    ScriptingLanguage() { }
    virtual ~ScriptingLanguage() { }
    
    virtual const char* suffix() const = 0;
    virtual SharedPtr<Executable> create() const = 0;
};

}

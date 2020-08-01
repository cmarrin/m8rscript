/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Executable.h"

struct lua_State;

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: LuaEngine
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Stream;

class LuaEngine  : public Executable
{
public:
    LuaEngine(const Stream&);
    
    ~LuaEngine();
    
    uint32_t nerrors() const { return _nerrors; }

    virtual CallReturnValue execute() override;

private:
    lua_State * _state = nullptr;
    uint32_t _nerrors = 0;
};

}

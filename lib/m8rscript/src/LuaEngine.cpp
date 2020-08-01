/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "LuaEngine.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
}

using namespace m8r;

static int pmain (lua_State *L)
{
    luaL_checkversion(L);
    printf("***** Hello from Lua!\n");
    return 1;
}

LuaEngine::LuaEngine(const Stream&)
{
    _state = luaL_newstate();
    if (!_state) {
        _nerrors = 1;
    }
}

LuaEngine::~LuaEngine()
{
    if (_state) {
        lua_close(_state);
        _state = nullptr;
    }
}

CallReturnValue LuaEngine::execute()
{
    if (!_state) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }
    
    lua_pushcfunction(_state, &pmain);
    int status = lua_pcall(_state, 0, 0, 0);
    if (status != 0) {
        printf(ROMString("***** Lua error on exit: returned status %d\n"), status);
    }
    lua_close(_state);
    _state = nullptr;
    return status == 0 ?
        CallReturnValue(CallReturnValue::Type::Finished) :
        CallReturnValue(CallReturnValue::Error::InternalError);
}

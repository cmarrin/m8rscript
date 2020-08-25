/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "LuaEngine.h"

#include "MStream.h"
#include "SystemInterface.h"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

using namespace m8r;

const char* LuaEngine::readStream(lua_State* L, void* data, size_t* size)
{
    LuaEngine* engine = reinterpret_cast<LuaEngine*>(data);
    int c = engine->_stream->read();
    if (c < 0) {
        size = 0;
        return nullptr;
    }
    
    engine->_buf = char(c);
    *size = 1;
    return &(engine->_buf);
}

LuaEngine::LuaEngine(const Stream& stream)
{
    system()->printf("LuaEngine ctos enter: Free heap: %d\n\n", system()->heapFreeSize());
    _stream = &stream;
    _state = luaL_newstate();
    system()->printf("LuaEngine after luaL_newstate: Free heap: %d\n\n", system()->heapFreeSize());
    if (!_state) {
        _error = Error::Code::OutOfMemory;
        _nerrors = 1;
    }
    
    luaL_openlibs(_state);
    system()->printf("LuaEngine after luaL_openlibs: Free heap: %d\n\n", system()->heapFreeSize());
    
    int result = lua_load(_state, readStream, this, "", nullptr);
    system()->printf("LuaEngine after lua_load: Free heap: %d\n\n", system()->heapFreeSize());
    if (result == LUA_OK) {
        _error = Error::Code::OK;
        _nerrors = 0;
        _functionIndex = luaL_ref(_state, LUA_REGISTRYINDEX);
    } else {
        // On error TOS will have an error string
        _errorString = lua_tostring(_state, -1);
        
        if (result == LUA_ERRSYNTAX) {
            _error = Error::Code::ParseError;
            _nerrors = 1;
        } else {
            _error = Error::Code::InternalError;
            _nerrors = 1;
        }
        lua_close(_state);
        _state = nullptr;
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
    system()->printf("LuaEngine::execute enter: Free heap: %d\n\n", system()->heapFreeSize());
    if (!_state) {
        return CallReturnValue(CallReturnValue::Error::InternalError);
    }

    lua_rawgeti(_state, LUA_REGISTRYINDEX, _functionIndex);
    system()->printf("LuaEngine::execute before pcall: Free heap: %d\n\n", system()->heapFreeSize());
    int status = lua_pcall(_state, 0, 0, 0);
    if (status != 0) {
        printf("***** Lua error on exit: returned status %d\n", status);
    }
    lua_close(_state);
    _state = nullptr;
    system()->printf("LuaEngine::execute exit: Free heap: %d\n\n", system()->heapFreeSize());
    return status == 0 ?
        CallReturnValue(CallReturnValue::Type::Finished) :
        CallReturnValue(CallReturnValue::Error::InternalError);
}

/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2020, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Defines.h"
#include "SystemTime.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace m8r {

class Timer {
public:
    enum class Behavior { Once, Repeating };

    using Callback = std::function<void(Timer*)>;
    
    Timer() { init(); }

    ~Timer();
    
    void init();
    
    void setCallback(const Callback& cb) { _cb = cb; }

    void start(Duration duration, Behavior behavior = Behavior::Once);
    void stop();
    
    bool running() const;
    Behavior behavior() const { return _behavior; }
    
    void fire() { if (_cb) { _cb(this); } }
    
private:
    Callback _cb;
    void* _data = nullptr;
    Behavior _behavior = Behavior::Once;
};

}

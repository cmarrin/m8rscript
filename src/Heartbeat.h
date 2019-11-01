/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Task.h"

namespace m8r {

class Heartbeat {
public:
    Heartbeat();
    
private:
    static constexpr uint32_t HeartrateMs = 3000;
    static constexpr uint32_t DownbeatMs = 50;

    bool _upbeat = false;
    NativeTask _task;
};

}

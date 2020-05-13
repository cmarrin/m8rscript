/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <functional>

namespace m8r {

using ConsoleCB = std::function<void(const char*)>;
void initMacSystemInterface(const char* fsFile, ConsoleCB);

}

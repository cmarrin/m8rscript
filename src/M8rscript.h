/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "ExecutionUnit.h"
#include "ScriptingLanguage.h"
#include "SharedPtr.h"

namespace m8rscript {

class M8rscriptScriptingLanguage : public m8r::ScriptingLanguage
{
public:
    virtual const char* suffix() const override { return "m8r"; }
    virtual m8r::SharedPtr<m8r::Executable> create() const override
    {
        return m8r::SharedPtr<m8r::Executable>(new ExecutionUnit());
    }
};

}

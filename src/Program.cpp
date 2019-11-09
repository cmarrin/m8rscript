/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Program.h"

#include "ExecutionUnit.h"

using namespace m8r;

Program::Program()
    : _global(Mad<Program>(this))
{
}

Program::~Program()
{
}

void Program::printAtomId(Mad<Program> program, int id)
{
    String s = program->stringFromAtom(Atom(id)).c_str();
    debugf("atom=\"%s\"\n", s.c_str());
}

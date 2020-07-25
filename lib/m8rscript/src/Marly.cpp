/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Marly.h"

#include "Atom.h"
#include "Scanner.h"

using namespace m8r;

Marly::Marly(const Stream& stream)
{
    Scanner scanner(&stream);
}

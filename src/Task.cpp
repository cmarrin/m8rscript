/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
      
    - Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
      
    - Neither the name of the <ORGANIZATION> nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "Task.h"

#include "Application.h"
#include "MStream.h"

#ifndef NO_PARSER_SUPPORT
#include "Parser.h"
#endif

#include <cassert>

using namespace m8r;

Task::Task(const char* filename)
{
    Object::addEU(&_eu);
    
    // FIXME: What do we do with these?
    ErrorList syntaxErrors;
    Parser::Debug debug = Parser::Debug::None;
    
    if (filename) {
        FileStream m8rStream(system()->fileSystem(), filename);
        if (!m8rStream.loaded()) {
            _error = Error::Code::FileNotFound;
            return;
        }
        
#ifdef NO_PARSER_SUPPORT
        return;
#else
        // See if we can parse it
        system()->printf(ROMSTR("Parsing...\n"));
        Parser parser;
        parser.parse(m8rStream, debug);
        system()->printf(ROMSTR("Finished parsing %s. %d error%s\n\n"), filename, parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");
        if (parser.nerrors()) {
            syntaxErrors.swap(parser.syntaxErrors());
            _error = Error::Code::ParseError;
            return;
        }
        
        _eu.startExecution(parser.program());
#endif
    }
}

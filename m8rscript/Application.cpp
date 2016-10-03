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

#include "Application.h"

#include "ExecutionUnit.h"
#include "MStream.h"
#include "Parser.h"
#include "SystemInterface.h"

using namespace m8r;

Application::Application(SystemInterface* system)
{
    // See if there is a 'main' file (which contains the name of the program to run)
    String name = "main";
    FileStream mainstream(name.c_str());
    if (mainstream.loaded()) {
        name.erase();
        while (!mainstream.eof()) {
            int c = mainstream.read();
            if (c < 0) { 
                break;
            }
            name += static_cast<char>(c);
        }
    } else {
        system->printf("'main' not found in filesystem, trying default...\n");
    }
    
    name += ".m8rb";
    system->printf("Opening '%s'...\n", name.c_str());

    FileStream stream(name.c_str());
    
    if (!stream.loaded()) {
        name = name.slice(0, -1);
        system->printf("File not found, trying '%s'...\n", name.c_str());
        FileStream stream(name.c_str());
        
        if (!stream.loaded()) {
            system->printf("Error: no files to open\n", name.c_str());
            return;
        }

        system->printf("Parsing...\n");
        Parser parser(system);
        parser.parse(&stream);
        
        system->printf("Finished. %d error%s\n\n", parser.nerrors(), (parser.nerrors() == 1) ? "" : "s");

        if (!parser.nerrors()) {
            _program = parser.program();
        }
    } else {
        _program = new m8r::Program(system);
        m8r::Error error;
        if (!_program->deserializeObject(&stream, error)) {
            error.showError(system);
        }
    }
}

Application::NameValidationType Application::validateFileName(const char* name)
{
    if (!name || name[0] == '\0') {
        return NameValidationType::BadLength;
    }
    
    for (size_t i = 0; name[i]; i++) {
        if (i >= 31) {
            return NameValidationType::BadLength;
        }
        
        char c = name[i];
        if (c == '-' || c == '.' || c == '_' || c == '+' ||
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')) {
            continue;
        }
        return NameValidationType::InvalidChar;
    }
    return NameValidationType::Ok;
}

Application::NameValidationType Application::validateBonjourName(const char* name)
{
    if (!name || name[0] == '\0') {
        return NameValidationType::BadLength;
    }
    
    for (size_t i = 0; name[i]; i++) {
        if (i >= 31) {
            return NameValidationType::BadLength;
        }
        
        char c = name[i];
        if (c == '-' ||
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z')) {
            continue;
        }
        return NameValidationType::InvalidChar;
    }
    return NameValidationType::Ok;
}

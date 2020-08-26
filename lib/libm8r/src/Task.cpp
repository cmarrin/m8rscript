/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Defines.h"

#include "Task.h"

#include "FileStream.h"
#include "ScriptingLanguage.h"
#include "SystemInterface.h"

#include <cassert>

using namespace m8r;

void Task::print(const char* s) const
{
    if (_executable) {
        _executable->print(s);
    } else {
        system()->print(s);
    }
}

Task::~Task()
{
}

bool Task::load(const char* filename)
{
    Vector<String> parts = String(filename).split(".");
    if (parts.size() < 2) {
        // Try appending known suffixes
        String baseFilename = filename;

        system()->printf("***** Missing suffix for '%s'\n\n", filename);
        return false;
    }
    
    Error error(Error::Code::NoFS);
    Mad<File> file;

    if (system()->fileSystem()) {
        file = system()->fileSystem()->open(filename, FS::FileOpenMode::Read);
        error = file->error();
    }

    if (error) {
        print(Error::formatError(error.code(), "Unable to open '%s' for execution", filename).c_str());
        _error = error;
        return false;
    }
    
    bool ret = load(FileStream(file), parts.back());

    if (file->error() != Error::Code::OK) {
        print(Error::formatError(file->error().code(), "Error reading '%s'", filename).c_str());
    }
        
    file.destroy(MemoryType::Native);
    return ret;
}

bool Task::load(const Stream& stream, const String& type)
{
#ifndef NDEBUG
    _name = String::format("Task(%p)", this);
#endif

    for (uint32_t i = 0; ; ++i) {
        const ScriptingLanguage* lang = system()->scriptingLanguage(i);
        if (!lang) {
            break;
        }
        
        if (String(lang->suffix()) == type) {
            _executable = lang->create();
            _executable->load(stream);
            
            // FIXME: Check for errors
            const ParseErrorList* errors = _executable->parseErrors();
            if (errors && errors->size() > 0) {
                _executable->print("\n");
                for (const auto& it : *errors) {
                    _executable->print(it.format().c_str());
                }
                _executable->print(String::format("%d parse error%s\n\n", errors->size(), (errors->size() == 1) ? "" : "s").c_str());
                return false;
            }
            return true;
        }
    }
    
    system()->printf("***** Unknown suffix '%s'.\n\n", type.c_str());

    return false;
}

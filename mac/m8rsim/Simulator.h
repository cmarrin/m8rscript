/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#import <Foundation/Foundation.h>

#include "Application.h"
#include "ExecutionUnit.h"
#include "Shell.h"

class Simulator
{
public:
    Simulator(m8r::GPIOInterface*);
    ~Simulator();
    
    bool setFiles(NSURL*, NSError**);
    NSFileWrapper* getFiles() const;
    NSArray* listFiles();
    NSData* getFileData(NSString*);
    void printCode();
    
    uint32_t localPort() const { return _localPort; }

private:
    std::unique_ptr<m8r::SystemInterface> _system;
    std::unique_ptr<m8r::FS> _fs;
    std::unique_ptr<m8r::Application> _application;
    
    uint32_t _localPort = 0;
    
    static uint32_t _nextLocalPort;
};


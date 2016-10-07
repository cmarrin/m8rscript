//
//  Simulator.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#pragma once

#include "Application.h"
#include "ExecutionUnit.h"
#include "Shell.h"
#include "SystemInterface.h"

class Simulator : public m8r::ShellOutput
{
public:
    Simulator(m8r::SystemInterface* system)
        : _system(system)
        , _isBuild(true)
        , _eu(_system)
        , _shell(this)
    { }
    
    ~Simulator() { }
    
    void importBinary(const char* filename);
    void exportBinary(const char* filename);
    void build(const char* source, const char* name);
    void run();
    void pause();
    void stop();
    void simulate();
    bool isBuild() const { return _isBuild; }

    bool canRun() { return _program && !_running; }
    bool canStop() { return _program && _running; }

    // ShellOutput
    virtual void shellSend(const char* data, uint16_t size = 0) override
    {
    }

private:
    m8r::SystemInterface* _system;
    bool _isBuild;
    m8r::ExecutionUnit _eu;
    m8r::Program* _program;
    m8r::Application* _application;
    bool _running = false;
    m8r::Shell _shell;
};


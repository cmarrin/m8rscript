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

class Simulator : public m8r::Shell
{
public:
    Simulator(m8r::SystemInterface* system)
        : _system(system)
        , _eu(_system)
        , _shell(this)
    { }
    
    ~Simulator();
    
    void importBinary(const char* filename);
    void exportBinary(const char* filename);
    void build(const char* source, const char* name);
    void run();
    void pause();
    void stop();
    void simulate();
    void clear()
    {
        if (_program) {
            delete _program;
            _program = nullptr;
        }
    }
    
    bool isBuild() const { return _isBuild; }
    bool isRunning() const { return _running; }

    bool canRun() { return _program && !_running; }
    bool canStop() { return _program && _running; }
    bool canSaveBinary() { return _program; }
    
    void initShell() { _shell.init(); _receivedString.clear(); }
    long sendToShell(const void* data, long size);
    long receiveFromShell(void* data, long size);

    void shellSend(const char* data, uint16_t size)
    {
        if (!size) {
            size = strlen(data);
        }
        _receivedString += m8r::String(data, size);
    }

private:
    class MyShell : public m8r::Shell {
    public:
        MyShell(Simulator* simulator) : _simulator(simulator) { }
        virtual void shellSend(const char* data, uint16_t size = 0) override { _simulator->shellSend(data, size); }

    private:
        Simulator* _simulator;
    };
    
    m8r::SystemInterface* _system;
    bool _isBuild = true;
    m8r::ExecutionUnit _eu;
    m8r::Program* _program = nullptr;
    m8r::Application* _application = nullptr;
    bool _running = false;
    MyShell _shell;
    m8r::String _receivedString;
};


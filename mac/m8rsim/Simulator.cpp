//
//  Simulator.cpp
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#include "Simulator.h"

class Simulator
{
public:
    Simulator(void* device) : _device(device) { }
    
private:
    void* _device;
};

void* Simulator_new(void* device) { return new Simulator(device); }

void Simulator_delete(void* simulator) { delete reinterpret_cast<Simulator*>(simulator); }

//
//  EventManager.cpp
//  m8rscript
//
//  Created by Chris Marrin on 1/18/17.
//  Copyright © 2017 MarrinTech. All rights reserved.
//

#include "EventManager.h"

using namespace m8r;

EventManager EventManager::_sharedEventManager;

void EventManager::removeListener(EventListener* listener)
{
    for (int i = 0; i < _listeners.size(); ++i) {
        if (_listeners[i] == listener) {
            _listeners.erase(_listeners.begin() + i);
            break;
        }
    }
}

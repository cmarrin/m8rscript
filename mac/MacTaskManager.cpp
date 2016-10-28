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

#include "MacTaskManager.h"

#include <cassert>
#include <future>

using namespace m8r;

MacTaskManager::MacTaskManager()
{
    _eventThread = new std::thread([this]{
        while (true) {
            {
                std::unique_lock<std::mutex> lock(_eventMutex);
                if (!_eventAvailable) {
                    _eventCondition.wait(lock);
                }
                assert(_eventAvailable);
                _eventAvailable = false;
                if (_terminating) {
                    return;
                }
            }
            fireEvent();
        }
    });
}

MacTaskManager::~MacTaskManager()
{
    _terminating = true;
    postEvent();
    _eventThread->join();
    delete _eventThread;
}

TaskManager* TaskManager::sharedTaskManager()
{
    if (!_sharedTaskManager) {
        _sharedTaskManager = new MacTaskManager();
    }
    return _sharedTaskManager;
}

void MacTaskManager::stopTimer()
{
    std::unique_lock<std::mutex> lock(_timerMutex);
    _timerCondition.notify_all();
}
    
void MacTaskManager::startTimer(int32_t ms)
{
    std::async([this, ms]{
        std::unique_lock<std::mutex> lock(_timerMutex);
        std::cv_status status = _timerCondition.wait_for(lock, std::chrono::milliseconds(ms));
        if (status == std::cv_status::timeout) {
            postEvent();
        }
    });
}
    
void MacTaskManager::postEvent()
{
    std::unique_lock<std::mutex> lock(_eventMutex);
    assert(!_eventAvailable);
    _eventAvailable = true;
    _eventCondition.notify_all();
}

/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2020, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <pthread.h>
#include <functional>

namespace m8r {

class Thread {
public:
    Thread() { }

    template< class Function, class... Args > 
    explicit Thread( Function&& f, Args&&... args )
    {
        _lambda = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
        pthread_create(&_thread, nullptr, threadFunc, this);
    }
    
    Thread(Thread&& other) { _thread = other._thread; other._thread = pthread_t(); }
    Thread(Thread&) = delete;
        
    ~Thread() { }
    
    void join() { pthread_join(_thread, nullptr); }
    void detach() { pthread_detach(_thread); }

private:
    static void* threadFunc(void* data)
    {
        Thread* t = reinterpret_cast<Thread*>(data);
        t->_lambda();
        return nullptr;
    }
    
    std::function<void()> _lambda;
    pthread_t _thread = pthread_t();
    
};

}

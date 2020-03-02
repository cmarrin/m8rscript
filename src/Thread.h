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
    Thread() = default;

    template< class Function, class... Args > 
    explicit Thread( Function&& f, Args&&... args )
    {
        auto storage = new ThreadStorage(std::move(std::bind(std::forward<Function>(f), std::forward<Args>(args)...)));
        pthread_create(&_thread, nullptr, threadFunc, storage);
    }
    
    Thread(Thread&& other) { swap(other); }
    Thread(const Thread&) = delete;
    Thread(Thread&) = delete;
        
    ~Thread()
    {
        if (joinable()) {
            join();
        }
    }
    
    Thread& operator=(const Thread&) = delete;

    Thread& operator=(Thread&& other)
    {
        if (joinable()) {
            terminate();
        }
        swap(other);
        return *this;
    }

    void swap(Thread& other) { std::swap(_thread, other._thread); }

    void join() { pthread_join(_thread, nullptr); }
    void detach() { pthread_detach(_thread); }
    void terminate() { pthread_cancel(_thread); }
    bool joinable() { return _running; }

private:
    struct ThreadStorage
    {
        ThreadStorage(std::function<void()>&& func)
            : _lambda(std::move(func))
        { }
            
        std::function<void()> _lambda;
    };
    
    static void* threadFunc(void* data)
    {
        auto t = reinterpret_cast<ThreadStorage*>(data);
        t->_lambda();
        delete t;
        return nullptr;
    }

    pthread_t _thread = pthread_t();
    bool _running = false;
};

}

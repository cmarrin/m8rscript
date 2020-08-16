/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2020, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "SystemTime.h"

#include <errno.h>
#include <pthread.h>
#include <functional>

namespace m8r {

class Mutex
{
    friend class Lock;
    
public:
    Mutex()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&_m, &attr);
    }
    
    Mutex(const Mutex&) = delete;
    ~Mutex() { pthread_mutex_destroy(&_m); }
    
    Mutex& operator=(const Mutex&) = delete;
    
    void lock() { pthread_mutex_lock(&_m); }
    bool try_lock() { return pthread_mutex_trylock(&_m) == 0; }
    void unlock() { pthread_mutex_unlock(&_m); }
    
private:
    pthread_mutex_t _m = pthread_mutex_t();
};

class Lock
{
    friend class Condition;
    
public:
    Lock(Mutex& m) : _mutex(m) { _mutex.lock(); }
    Lock(const Lock&) = delete;
    ~Lock() { _mutex.unlock(); }

private:
    pthread_mutex_t& mutex() const { return _mutex._m; }
    Mutex& _mutex;
};

class Condition
{
public:
    enum class WaitResult { TimedOut, Notified };
    
    Condition() { pthread_cond_init(&_c, nullptr); }
    Condition(const Condition&) = delete;
    ~Condition() { pthread_cond_destroy(&_c); }
    
    void notify(bool all = false)
    {
        if (all) {
            pthread_cond_broadcast(&_c);
        } else {
            pthread_cond_signal(&_c);
        }
    }
    
    void wait(Lock& lock)
    {
        pthread_cond_wait(&_c, &lock.mutex());
    }
    
    WaitResult waitFor(Lock& lock, Duration duration)
    {
        struct timespec ts;
        uint64_t t = Time::now().us() + duration.us();
        ts.tv_sec = t / 1000000;
        ts.tv_nsec = (t % 1000000) * 1000;
        while (1) {
            int result = pthread_cond_timedwait(&_c, &lock.mutex(), &ts);
            if (result == 0 || result == ETIMEDOUT) {
                return (result == 0) ? WaitResult::Notified : WaitResult::TimedOut;
            }
        }
    }
    
private:
    pthread_cond_t _c;
};

class Thread {
public:
    Thread() = default;
    
    using id = pthread_t;

    template<class Function, class... Args> 
    explicit Thread(uint32_t stackSize, Function&& f, Args&&... args )
    {
        auto storage = new ThreadStorage(this, std::move(std::bind(std::forward<Function>(f), std::forward<Args>(args)...)));
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, stackSize);
        pthread_create(&_thread, &attr, threadFunc, storage);
        pthread_attr_destroy(&attr);
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
    
    id get_id() const { return _thread; }

    Thread& operator=(const Thread&) = delete;

    Thread& operator=(Thread&& other)
    {
        if (joinable()) {
            terminate();
        }
        swap(other);
        return *this;
    }

    void swap(Thread& other)
    {
        pthread_t tmp = _thread;
        _thread = other._thread;
        other._thread = tmp;
    }
        
    void join() { pthread_join(_thread, nullptr); }
    void detach() { pthread_detach(_thread); _thread = pthread_t(); }
    void terminate() { pthread_cancel(_thread); }
    bool joinable() { return _thread != pthread_t(); }

private:
    struct ThreadStorage
    {
        ThreadStorage(Thread* thread, std::function<void()>&& func)
            : _thread(thread)
            , _lambda(std::move(func))
        { }
            
        Thread* _thread;
        std::function<void()> _lambda;
    };
    
    static void* threadFunc(void* data)
    {
        auto t = reinterpret_cast<ThreadStorage*>(data);
        t->_lambda();
        pthread_exit(nullptr);
        t->_thread->_thread = pthread_t();
        delete t;
        return nullptr;
    }
    
    pthread_t _thread = pthread_t();
};

namespace this_thread {
    
    inline Thread::id get_id() { return pthread_self(); }
    inline void yield() { Duration(1us).sleep(); }
    inline void sleep_for(Duration d) { d.sleep(); }
    inline void sleep_until(Time t) { sleep_for(t - Time::now()); }
}

}

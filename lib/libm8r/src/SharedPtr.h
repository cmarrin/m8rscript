/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cassert>
#include <cstdint>

namespace m8r {

    class Shared
    {
    public:
        friend class SharedPtrBase;
        
    private:
        uint16_t _count = 0;
    };
    
    class SharedPtrBase
    {
    public:
        uint16_t& count(Shared* ptr) { return ptr->_count; }
    };
    
    template<typename T>
    class SharedPtr : private SharedPtrBase
    {
    public:
        explicit SharedPtr(T* p = nullptr) { reset(p); }
        
        SharedPtr(SharedPtr<T>& other) { reset(other); }
        SharedPtr(SharedPtr<T>&& other) { _ptr = other._ptr; other._ptr = nullptr; }
        
        template <class U>
        SharedPtr(const SharedPtr<U>& other) { reset(other.get()); }
        
        ~SharedPtr() { reset(); }
        
        SharedPtr& operator=(const SharedPtr& other) { reset(other._ptr); return *this; }
        SharedPtr& operator=(SharedPtr&& other) { _ptr = other._ptr; other._ptr = nullptr; return *this; }

        void reset(T* p = nullptr)
        {
            if (_ptr) {
                assert(count(_ptr) > 0);
                if (--count(_ptr) == 0) {
                    delete _ptr;
                    _ptr = nullptr;
                }
            }
            if (p) {
                count(p)++;
                _ptr = p;
            }
        }

        void reset(SharedPtr<T>& p) { reset(p.get()); }
        
        T& operator*() { return *_ptr; }
        T* operator->() { return _ptr; }
        
        T& operator*() const { return *_ptr; }
        T* operator->() const { return _ptr; }
        
        T* get() const { return _ptr; }
        
        operator bool() const { return _ptr != nullptr; }
    
    private:
        T* _ptr = nullptr;
    };
    
}

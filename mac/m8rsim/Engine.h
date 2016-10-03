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

#ifndef Engine_h
#define Engine_h

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
extern void* Engine_createEngine(void* simulator);
extern void Engine_deleteEngine(void*);
extern void Engine_importBinary(void*, const char* filename);
extern void Engine_exportBinary(void*, const char* filename);

extern void Engine_build(void*, const char* source, const char* name);
extern void Engine_run(void*);
extern void Engine_pause(void*);
extern void Engine_stop(void*);
extern void Engine_simulate(void*);

extern bool Engine_canRun(void*);
extern bool Engine_canStop(void*);

extern void Simulator_vprintf(void* simulator, const char*, va_list, bool isBuild);
extern void Simulator_updateGPIOState(void* simulator, uint16_t mode, uint16_t state);

#define NameValidationOk 0
#define NameValidationBadLength 1
#define NameValidationInvalidChar 2

extern int validateFileName(const char*);
extern int validateBonjourName(const char*);

#ifdef __cplusplus
}
#endif

#endif /* Engine_h */

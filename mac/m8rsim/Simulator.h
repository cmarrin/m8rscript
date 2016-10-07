//
//  Simulator.h
//  m8rsim
//
//  Created by Chris Marrin on 6/24/16.
//  Copyright © 2016 Chris Marrin. All rights reserved.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
extern void* Simulator_new(void* device);
extern void Simulator_delete(void*);

//extern void Engine_importBinary(void*, const char* filename);
//extern void Engine_exportBinary(void*, const char* filename);
//
//extern void Engine_build(void*, const char* source, const char* name);
//extern void Engine_run(void*);
//extern void Engine_pause(void*);
//extern void Engine_stop(void*);
//extern void Engine_simulate(void*);
//
//extern bool Engine_canRun(void*);
//extern bool Engine_canStop(void*);
//
//extern void Simulator_vprintf(void* simulator, const char*, va_list, bool isBuild);
//extern void Simulator_updateGPIOState(void* simulator, uint16_t mode, uint16_t state);
//
//#define NameValidationOk 0
//#define NameValidationBadLength 1
//#define NameValidationInvalidChar 2
//
//extern int validateFileName(const char*);
//extern int validateBonjourName(const char*);

#ifdef __cplusplus
}
#endif

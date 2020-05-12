//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#include <stdlib.h>

void m8rInitialize(const char* fsFile);
void m8rRunOneIteration();
const char* m8rGetConsoleString();
void m8rGetGPIOState(uint32_t* value, uint32_t* change);

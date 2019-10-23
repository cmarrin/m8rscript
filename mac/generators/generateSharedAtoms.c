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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// read SharedAtoms.txt and use it to generate SharedAtoms.cpp/SharedAtoms.h

static void strip(char* out, const char* in)
{
    // remove leading spaces
    while(1) {
        if (isspace(*in)) {
            ++in;
        } else {
            break;
        }
    }
    
    while(1) {
        if (isspace(*in) || *in == '\n' || *in == '\0') {
            break;
        }
        *out++ = *in++;
    }
    *out = '\0';
}

int main()
{
    char* root = getenv("ROOT");
    chdir(root);

    FILE* ifile = fopen("generators/SharedAtoms.txt", "r");
    if (!ifile) {
        printf("could not open SharedAtoms.txt:%d\n", errno);
        return -1;
    }
    
    FILE* hfile = fopen("../src/SharedAtoms.h", "w");
    if (!hfile) {
        printf("could not open SharedAtoms.h:%d\n", errno);
        return -1;
    }
    
    FILE* cppfile = fopen("../src/SharedAtoms.cpp", "w");
    if (!cppfile) {
        printf("could not open SharedAtoms.cpp:%d\n", errno);
        return -1;
    }
    
    // Write the preambles
    fprintf(hfile, "// This file is generated. Do not edit\n\nenum class SA {\n");
    fprintf(cppfile, "// This file is generated. Do not edit\n\n#include \"SharedAtoms.h\"\n#include \"Defines.h\"\n#include <stdlib.h>\n\n");
    
    // Write the .h entries and the first .cpp entries
    char entry[128];
    
    while (1) {
        char* line;
        size_t length;
        ssize_t size = getline(&line, &length, ifile);
        if (size < 0) {
            if (feof(ifile)) {
                break;
            }
            printf("getline failed:%d\n", errno);
            return -1;
        }
        
        strip(entry, line);
        
        if (strlen(entry) == 0) {
            continue;
        }
        
        fprintf(hfile, "    %s,\n", entry);
        fprintf(cppfile, "static const char _%s[] ROMSTR_ATTR = \"%s\";\n", entry, entry);
    }
    
    // Write the second .cpp entries
    fprintf(cppfile, "\nconst char* RODATA_ATTR sharedAtoms[] = {\n");
    rewind(ifile);
    while (1) {
        char* line;
        size_t length;
        ssize_t size = getline(&line, &length, ifile);
        if (size < 0) {
            if (feof(ifile)) {
                break;
            }
            printf("getline failed:%d\n", errno);
            return -1;
        }
        
        strip(entry, line);

        if (strlen(entry) == 0) {
            continue;
        }
        
        fprintf(cppfile, "    _%s,\n", entry);
    }
    
    // Write the postambles
    fprintf(hfile, "};\n\nconst char* sharedAtom(SA id);\n");
    fprintf(cppfile, "};\n\nconst char* sharedAtom(SA id)\n{\n    return sharedAtoms[static_cast<uint32_t>(id)];\n}\n");
    
    fclose(ifile);
    fclose(hfile);
    fclose(cppfile);
    return 0;
}

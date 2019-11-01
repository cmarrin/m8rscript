/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
    fprintf(hfile, "// This file is generated. Do not edit\n\n#include <cstdint>\n\nenum class SA : uint16_t {\n");
    fprintf(cppfile, "// This file is generated. Do not edit\n\n#include \"SharedAtoms.h\"\n#include \"Defines.h\"\n#include <cstdlib>\n\n");
    
    // Write the .h entries and the first .cpp entries
    char entry[128];
    int32_t count = 0;

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
        
        fprintf(hfile, "    %s = %d,\n", entry, count);
        fprintf(cppfile, "static const char _%s[] ROMSTR_ATTR = \"%s\";\n", entry, entry);
        ++count;
    }
    
    // Write the second .cpp entries
    fprintf(cppfile, "\nconst char* RODATA_ATTR _sharedAtoms[] = {\n");
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
    
    // Round count to the nearest 100 to make it easier to compute byte offset into table.
    count = ((count + 99) / 100) * 100;
    
    // Write the postambles
    fprintf(hfile, "};\n\nconst char** sharedAtoms(uint16_t& nelts);\n");
    fprintf(hfile, "static constexpr uint16_t ExternalAtomOffset = %d;\n", count);

    fprintf(cppfile, "};\n\nconst char** sharedAtoms(uint16_t& nelts)\n{\n    nelts = sizeof(_sharedAtoms) / sizeof(const char*);\n    return _sharedAtoms;\n}\n");
    
    fclose(ifile);
    fclose(hfile);
    fclose(cppfile);
    return 0;
}

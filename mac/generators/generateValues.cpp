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
#include <string>
#include <vector>

#include "Scanner.h"

// read Shared.txt and use it to generate GeneratedValues.cpp/GeneratedValues.h

// Add string with tokens and special char sequences
struct SpecialEntry
{
    m8r::Token token;
    const char* str;
};

static SpecialEntry entries[ ] = {
    { m8r::Token::NE,           "!=" },
    { m8r::Token::MODSTO,       "%=" },
    { m8r::Token::LAND,         "&&" },
    { m8r::Token::ANDSTO,       "&=" },
    { m8r::Token::MULSTO,       "*=" },
    { m8r::Token::INCR,         "++" },
    { m8r::Token::ADDSTO,       "+=" },
    { m8r::Token::DECR,         "--" },
    { m8r::Token::SUBSTO,       "-=" },
    { m8r::Token::DIVSTO,       "/=" },
    { m8r::Token::EQ,           "==" },
    { m8r::Token::XORSTO,       "^=" },
    { m8r::Token::LOR,          "||" },
    { m8r::Token::ORSTO,        "|=" },
    { m8r::Token::LE,           "<=" },
    { m8r::Token::GE,           ">=" },
    { m8r::Token::SHL,          "<<" },
    { m8r::Token::SHR,          ">>" },
};



static std::string strip(const char* in)
{
    std::string out;
    
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
        out += *in++;
    }
    return out;
}

int main()
{
    printf("\n\n*** Generating Values ***\n\n");
    char* root = getenv("ROOT");
    chdir(root);

    FILE* afile = fopen("generators/SharedAtoms.txt", "r");
    if (!afile) {
        printf("could not open SharedAtoms.txt:%d\n", errno);
        return -1;
    }
    
    FILE* hfile = fopen("../lib/m8rscript/src/GeneratedValues.h", "w");
    if (!hfile) {
        printf("could not open GeneratedValues.h:%d\n", errno);
        return -1;
    }
    
    FILE* cppfile = fopen("../lib/m8rscript/src/GeneratedValues.cpp", "w");
    if (!cppfile) {
        printf("could not open GeneratedValues.cpp:%d\n", errno);
        return -1;
    }
    
    // Write the preambles
    fprintf(hfile, "// This file is generated. Do not edit\n\n#include <cstdint>\n\nenum class SA : uint16_t {\n");
    fprintf(cppfile, "// This file is generated. Do not edit\n\n#include \"GeneratedValues.h\"\n#include \"Defines.h\"\n#include <cstdlib>\n\n");
    
    // Get the strings into a vector
    std::vector<std::string> strings;

    while (1) {
        char* line = nullptr;
        size_t length;
        ssize_t size = getline(&line, &length, afile);
        if (size < 0) {
            if (feof(afile)) {
                free(line);
                break;
            }
            printf("getline failed:%d\n", errno);
            free(line);
            return -1;
        }
        
        // Ignore lines starting with "//" to allow for comments
        if (line[0] == '/' && line[1] == '/') {
            free(line);
            continue;
        }
        
        std::string entry = strip(line);
        
        if (!entry.size()) {
            free(line);
            continue;
        }
        
        strings.push_back(entry);
        free(line);
    }
    
    // Sort the vector
    std::sort(strings.begin(), strings.end());

    // Write the .h entries and the first .cpp entries
    for (int32_t i = 0; i < strings.size(); ++i) {
        // Some entries have a trailing '$'. This is done because they are
        // keywords, which can't be enumerants. So the enum will have the $
        // but we remove it from the string.
        std::string enumString = strings[i];
        std::string string = enumString;
        if (string.back() == '$') {
            string.erase(string.size() - 1);
        }
        fprintf(hfile, "    %s = %d,\n", enumString.c_str(), i);
        fprintf(cppfile, "static const char _%s[] = \"%s\";\n", enumString.c_str(), string.c_str());
    }
    
    // Write the second .cpp entries
    fprintf(cppfile, "\nconst char* _sharedAtoms[] = {\n");
    for (int32_t i = 0; i < strings.size(); ++i) {
        fprintf(cppfile, "    _%s,\n", strings[i].c_str());
    }
    fprintf(cppfile, "};\n\n");
    
    // Write the special char string
    fprintf(cppfile, "const char* _specialChars = \"\"\n");
    fprintf(cppfile, "#if M8RSCRIPT_SUPPORT == 1\n");
    
    for (auto it : entries) {
        fprintf(cppfile, "    \"\\x%02x%s\"\n", int(it.token), it.str);
    }
    
    fprintf(cppfile, "#endif\n");    
    fprintf(cppfile, ";\n\n");

    // Write the postambles
    fprintf(hfile, "};\n\nnamespace m8r {\n");
    fprintf(hfile, "    const char** sharedAtoms(uint16_t& nelts);\n");
    fprintf(hfile, "    const char* specialChars();\n");
    fprintf(hfile, "}\n");

    fprintf(cppfile, "const char** m8r::sharedAtoms(uint16_t& nelts)\n");
    fprintf(cppfile, "{\n");
    fprintf(cppfile, "    nelts = sizeof(_sharedAtoms) / sizeof(const char*);\n");
    fprintf(cppfile, "    return _sharedAtoms;\n");
    fprintf(cppfile, "}\n");
    fprintf(cppfile, "\n");

    fprintf(cppfile, "const char* m8r::specialChars()\n");
    fprintf(cppfile, "{\n");
    fprintf(cppfile, "    return _specialChars;\n");
    fprintf(cppfile, "}\n");
    fprintf(cppfile, "\n");
    
    fclose(afile);
    fclose(hfile);
    fclose(cppfile);
    return 0;
}

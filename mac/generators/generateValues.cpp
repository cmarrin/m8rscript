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
    { m8r::Token::LParen,       "(" },
    { m8r::Token::RParen,       ")" },
    { m8r::Token::Comma,        "," },
    { m8r::Token::Period,       "." },
    { m8r::Token::Colon,        ":" },
    { m8r::Token::Semicolon,    ";" },
    { m8r::Token::Question,     "?" },
    { m8r::Token::LBracket,     "[" },
    { m8r::Token::RBracket,     "]" },
    { m8r::Token::LBrace,       "{" },
    { m8r::Token::RBrace,       "}" },
    { m8r::Token::Twiddle,      "~" },
    { m8r::Token::Bang,         "!" },
    { m8r::Token::Percent,      "%" },
    { m8r::Token::Ampersand,    "&" },
    { m8r::Token::Star,         "*" },
    { m8r::Token::Plus,         "+" },
    { m8r::Token::Minus,        "-" },
    { m8r::Token::Slash,        "/" },
    { m8r::Token::STO,          "=" },
    { m8r::Token::XOR,          "^" },
    { m8r::Token::OR,           "|" },
    { m8r::Token::LT,           "<" },
    { m8r::Token::GT,           ">" },
    { m8r::Token::At,           "@" },
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
        fprintf(hfile, "    %s = %d,\n", strings[i].c_str(), i);
        fprintf(cppfile, "static const char _%s[] ROMSTR_ATTR = \"%s\";\n", strings[i].c_str(), strings[i].c_str());
    }
    
    // Write the second .cpp entries
    fprintf(cppfile, "\nconst char* RODATA_ATTR _sharedAtoms[] = {\n");
    for (int32_t i = 0; i < strings.size(); ++i) {
        fprintf(cppfile, "    _%s,\n", strings[i].c_str());
    }
    fprintf(cppfile, "};\n\n");
    
    // Write the special char string
    fprintf(cppfile, "const char* RODATA_ATTR _specialChars = \"\"\n");
    fprintf(cppfile, "#if M8RSCRIPT_SUPPORT == 1\n");
    
    for (auto it : entries) {
        fprintf(cppfile, "    \"\\x%02x%s\"\n", int(it.token), it.str);
    }
    
    fprintf(cppfile, "#endif\n");    
    fprintf(cppfile, ";\n\n");

    // Round count to the nearest 100 to make it easier to compute byte offset into table.
    size_t count = ((strings.size() + 99) / 100) * 100;

    // Write the postambles
    fprintf(hfile, "};\n\nnamespace m8r {\n");
    fprintf(hfile, "    const char** sharedAtoms(uint16_t& nelts);\n");
    fprintf(hfile, "    static constexpr uint16_t ExternalAtomOffset = %zu;\n", count);
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

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

// read Shared.txt and use it to generate GeneratedValues.cpp/GeneratedValues.h

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

int main(int argc, const char* argv[])
{
    // first arg is a filename containing the strings. Second arg is
    // dir to put output in. Both relative to mac. The third param
    // is a namespace name
    const char* inFilename = (argc > 1) ? argv[1] : "generators/SharedAtoms.txt";
    std::string outPath = (argc > 2) ? argv[2] : "../lib/m8rscript/src";
    if (outPath.back() != '/') {
        outPath += '/';
    }
    
    const char* ns = (argc > 3) ? argv[3] : nullptr;
    
    printf("\n\n*** Generating Values ****\n\n");
    char* root = getenv("PWD");
    chdir(root);
    printf("*** CWD=%s\n", root);

    FILE* afile = fopen(inFilename, "r");
    if (!afile) {
        printf("could not open '%s':%d\n", inFilename, errno);
        return -1;
    }
    
    FILE* hfile = fopen((outPath + "GeneratedValues.h").c_str(), "w");
    if (!hfile) {
        printf("could not open GeneratedValues.h:%d\n", errno);
        return -1;
    }
    
    FILE* cppfile = fopen((outPath + "GeneratedValues.cpp").c_str(), "w");
    if (!cppfile) {
        printf("could not open GeneratedValues.cpp:%d\n", errno);
        return -1;
    }
    
    // Write the preambles
    fprintf(hfile, "// This file is generated. Do not edit\n\n");
    fprintf(hfile, "#pragma once\n\n");
    fprintf(hfile, "#include <cstdint>\n");
    fprintf(hfile, "#include \"Atom.h\"\n\n");
    if (ns) {
        fprintf(hfile, "namespace %s {\n\n", ns);
    }
    fprintf(hfile, "enum class SA : uint16_t {\n");
    
    fprintf(cppfile, "// This file is generated. Do not edit\n\n");
    fprintf(cppfile, "#include \"GeneratedValues.h\"\n");
    fprintf(cppfile, "#include \"Defines.h\"\n");
    fprintf(cppfile, "#include <cstdlib>\n\n");
    if (ns) {
        fprintf(cppfile, "using namespace %s;\n\n", ns);
    }
    
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
    fprintf(cppfile, "\nconst char* _%s_sharedAtoms[] = {\n", ns);
    for (int32_t i = 0; i < strings.size(); ++i) {
        fprintf(cppfile, "    _%s,\n", strings[i].c_str());
    }
    fprintf(cppfile, "};\n\n");
    
    // Write the postambles
    fprintf(hfile, "};\n\n");
    fprintf(hfile, "const char** sharedAtoms(uint16_t& nelts);\n");
    fprintf(hfile, "const char* specialChars();\n");
    fprintf(hfile, "static inline m8r::Atom SAtom(SA sa) { return m8r::Atom(static_cast<m8r::Atom::value_type>(sa)); }\n");

    if (ns) {
        fprintf(hfile, "\n}\n");
    }

    fprintf(cppfile, "const char** %s::sharedAtoms(uint16_t& nelts)\n", ns);
    fprintf(cppfile, "{\n");
    fprintf(cppfile, "    nelts = sizeof(_%s_sharedAtoms) / sizeof(const char*);\n", ns);
    fprintf(cppfile, "    return _%s_sharedAtoms;\n", ns);
    fprintf(cppfile, "}\n");
    fprintf(cppfile, "\n");

    fclose(afile);
    fclose(hfile);
    fclose(cppfile);
    return 0;
}

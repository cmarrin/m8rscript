//
//  m8rscript.cpp
//  m8rscript
//
//  Created by Chris Marrin on 6/2/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

#include <iostream>
#include <unistd.h>
#include <ctime>

#include "Stream.h"
#include "Parser.h"

#define Root $(SRCROOT)

#ifdef YYDEBUG
extern int yydebug;
#endif

void print(const char* s) { std::cout << s; }

int main(int argc, const char* argv[])
{
#ifdef YYDEBUG
    yydebug = 0;
#endif
    
    if (argc < 2) {
        std::cout << "No file specified, exiting\n";
        return 0;
    }
    
    FileStream istream(argv[1]);
    std::cout << "Opening '" << argv[1] << "'\n";

    if (!istream.loaded()) {
        std::cout << "File not found, exiting\n";
        return 0;
    }
    
    std::cout << "Parsing...\n";
    
    std::clock_t startTime = std::clock();
    m8r::Parser parser(&istream, print);
    std::clock_t parseTime = std::clock() - startTime;
    std::cout << "Finished. " << parser.nerrors() << " error" << ((parser.nerrors() == 1) ? "" : "s") << "\n\n";
    std::cout << "Parse:" << (static_cast<double>(parseTime) / CLOCKS_PER_SEC * 1000000) << "us\n";

    if (!parser.nerrors()) {
        m8r::ExecutionUnit eu;
        
        startTime = std::clock();
        m8r::String s = eu.generateCodeString(parser.program());
        std::clock_t printTime = std::clock() - startTime;
        std::cout << s.c_str();
        
        startTime = std::clock();
        eu.run(parser.program(), print);
        std::clock_t runTime = std::clock() - startTime;
        
        std::cout << "Print:" << (static_cast<double>(printTime) / CLOCKS_PER_SEC * 1000000) << "us\n";
        std::cout << "Run  :" << (static_cast<double>(runTime) / CLOCKS_PER_SEC * 1000) << "ms\n";
    }
    
    std::cout << "\n";
    return 0;
}

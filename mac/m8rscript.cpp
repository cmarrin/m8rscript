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
    
    m8r::FileStream istream(argv[1]);
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

    std::clock_t printTime = 0;
    std::clock_t runTime = 0;
    
    if (!parser.nerrors()) {
        m8r::ExecutionUnit eu(print);
        
        startTime = std::clock();
        m8r::String s = eu.generateCodeString(parser.program().get());
        printTime = std::clock() - startTime;
        std::cout << "\n***** Start of Generated Code *****\n" << s.c_str() << "\n***** End of Generated Code *****\n";
        
        std::cout << "\n***** Start of Program Output *****\n\n";
        startTime = std::clock();
        eu.run(parser.program().get());
        runTime = std::clock() - startTime;
        std::cout << "\n***** End of Program Output *****\n";
    }
    
    std::cout << "\n\nTiming results:\n";
    std::cout << "    Parse:" << (static_cast<double>(parseTime) / CLOCKS_PER_SEC * 1000000) << "us\n";
        
    if (!parser.nerrors()) {
        std::cout << "    Print:" << (static_cast<double>(printTime) / CLOCKS_PER_SEC * 1000000) << "us\n";
        std::cout << "    Run  :" << (static_cast<double>(runTime) / CLOCKS_PER_SEC * 1000) << "ms\n";
    }
    std::cout << "\n";
    return 0;
}

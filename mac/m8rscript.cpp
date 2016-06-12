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
    m8r::Parser parser(&istream);
    std::clock_t parseTime = std::clock() - startTime;
    std::cout << "Finished. " << parser.nerrors() << " error" << ((parser.nerrors() == 1) ? "" : "s") << "\n\n";

    m8r::ExecutionUnit eu;
    
    startTime = std::clock();
    printf("%s\n", eu.generateCodeString(parser.program()).c_str());
    std::clock_t printTime = std::clock() - startTime;
    
    startTime = std::clock();
    eu.run(parser.program());
    std::clock_t runTime = std::clock() - startTime;
    
    std::cout << "Parse:" << (static_cast<double>(parseTime) / CLOCKS_PER_SEC * 1000000) << "us\n";
    std::cout << "Print:" << (static_cast<double>(printTime) / CLOCKS_PER_SEC * 1000000) << "us\n";
    std::cout << "Run  :" << (static_cast<double>(runTime) / CLOCKS_PER_SEC * 1000000) << "us\n";
    
    return 0;
}

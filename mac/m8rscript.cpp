//
//  m8rscript.cpp
//  m8rscript
//
//  Created by Chris Marrin on 6/2/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

#include <iostream>
#include <unistd.h>

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
    m8r::Parser parser(&istream);
    std::cout << "Finished. " << parser.nerrors() << " error" << ((parser.nerrors() == 1) ? "" : "s") << "\n";
    printf("%s", parser.toString().c_str());
    return 0;
}

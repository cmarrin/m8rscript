//
//  m8rscript.cpp
//  m8rscript
//
//  Created by Chris Marrin on 6/2/16.
//  Copyright © 2016 MarrinTech. All rights reserved.
//

#include <iostream>

#include "Stream.h"
#include "Parser.h"

int main(int argc, const char* argv[])
{
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
    std::cout << "Done!\n";
    return 0;
}

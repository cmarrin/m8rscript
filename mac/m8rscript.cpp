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

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        std::cout << "No file specified, exiting\n";
        return 0;
    }
    
    const char* root = getenv("SRCROOT");
    
    chdir(root);
    
    char cwd[256];
    getcwd(cwd, 255);
    std::cout << "cwd='" << cwd << "'\n";
    
    
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

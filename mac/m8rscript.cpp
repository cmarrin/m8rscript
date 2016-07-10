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
#include "CodePrinter.h"
#include "ExecutionUnit.h"
#include "SystemInterface.h"

class MySystemInterface : public m8r::SystemInterface
{
public:
    virtual void print(const char* s) const override { std::cout << s; }
    virtual int read() const override
    {
        return std::cin.get();
    }
    virtual void updateGPIOState(uint16_t mode, uint16_t state) override { std::cout << "mode=" << std::hex << mode << " state=" << std::hex << state << "\n"; }
};

class InputStream : public m8r::Stream
{
public:
    InputStream(m8r::SystemInterface* system) : _system(system) { }

    virtual ~InputStream() { }
	
	virtual int available() override
    {
        return 1;
    }
    virtual int read() override { return _system->read(); }
	virtual void flush() override { }
    
    void interactive();
	
private:
    m8r::SystemInterface* _system;
};

int main(int argc, const char* argv[])
{
    MySystemInterface system;
    std::clock_t startTime;
    std::clock_t parseTime;
    m8r::Parser parser(&system);

    if (argc < 2) {
        std::cout << "No file specified, using stdin\n";
        InputStream istream(&system);
        std::clock_t startTime = std::clock();
        parser.parse(&istream);
        parseTime = std::clock() - startTime;
    } else {
        m8r::FileStream istream(argv[1]);
        std::cout << "Opening '" << argv[1] << "'\n";

        if (!istream.loaded()) {
            std::cout << "File not found, exiting\n";
            return 0;
        }
            
        std::cout << "Parsing...\n";
        
        startTime = std::clock();
        parser.parse(&istream);
        parseTime = std::clock() - startTime;
    }
    
    std::cout << "Finished. " << parser.nerrors() << " error" << ((parser.nerrors() == 1) ? "" : "s") << "\n\n";
    std::clock_t printTime = 0;
    std::clock_t runTime = 0;
    
    if (!parser.nerrors()) {
        m8r::CodePrinter codePrinter(&system);
        startTime = std::clock();
        m8r::String s = codePrinter.generateCodeString(parser.program());
        printTime = std::clock() - startTime;
        std::cout << "\n***** Start of Generated Code *****\n" << s.c_str() << "\n***** End of Generated Code *****\n";
        
        std::cout << "\n***** Start of Program Output *****\n\n";
        m8r::ExecutionUnit eu(&system);
        startTime = std::clock();
        eu.run(parser.program());
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

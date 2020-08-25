/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>

#include "Application.h"
#include "MacSystemInterface.h"
#include "M8rscript.h"
#include "Marly.h"
#include "MFS.h"

static void usage(const char* name)
{
    fprintf(stderr,
                "usage: %s [-p <port>] [-f <filesystem file>] [-l <path>] [-h] <dir> <file> ...\n"
                "    -p     : set shell port (log port +1, sim port +2)\n"
                "    -f     : simulated filesystem path\n"
                "    -l     : path for uploaded files (default '/')\n"
                "    -h     : print this message\n"
                "    <file> : file(s) to be uploaded\n"
            , name);
}

m8rscript::M8rscriptScriptingLanguage m8rscriptScriptingLanguage;
marly::MarlyScriptingLanguage marlyScriptingLanguage;

int main(int argc, char * argv[])
{
    int opt;
    uint16_t port = 23;
    const char* fsFile = "m8rFSFile";
    const char* uploadPath = "/";
    
    while ((opt = getopt(argc, argv, "p:u:l:h")) != EOF) {
        switch(opt)
        {
            case 'p': port = atoi(optarg); break;
            case 'f': fsFile = optarg; break;
            case 'l': uploadPath = optarg; break;
            case 'h': usage(argv[0]); break;
            default : break;
        }
    }
        
    m8r::initMacSystemInterface(fsFile, [](const char* s) { ::printf("%s", s); });
    m8r::Application application(port);
    
    m8r::system()->registerScriptingLanguage(&m8rscriptScriptingLanguage);
    m8r::system()->registerScriptingLanguage(&marlyScriptingLanguage);
    
    // Upload files if present
    for (int i = optind; i < argc; ++i) {
        const char* uploadFilename = argv[i];

        m8r::String toPath;
        FILE* fromFile = fopen(uploadFilename, "r");
        if (!fromFile) {
            fprintf(stderr, "Unable to open '%s' for upload, skipping\n", uploadFilename);
        } else if (m8r::system()->fileSystem()) {
            m8r::Vector<m8r::String> parts = m8r::String(uploadFilename).split("/");
            m8r::String baseName = parts[parts.size() - 1];
            
            if (uploadPath[0] != '/') {
                toPath += '/';
            }
            toPath += uploadPath;
            if (toPath[toPath.size() - 1] != '/') {
                toPath += '/';
            }
            
            // Make sure the directory path exists
            m8r::system()->fileSystem()->makeDirectory(toPath.c_str());
            if (m8r::system()->fileSystem()->lastError() != m8r::Error::Code::OK) {
                m8r::system()->print(m8r::Error::formatError(m8r::system()->fileSystem()->lastError().code(), 
                                                        "Unable to create '%s'", toPath.c_str()).c_str());
            } else {
                toPath += baseName;
                
                m8r::Mad<m8r::File> toFile(m8r::system()->fileSystem()->open(toPath.c_str(), m8r::FS::FileOpenMode::Write));
                if (!toFile->valid()) {
                    m8r::system()->print(m8r::Error::formatError(toFile->error().code(), 
                                                            "Error: unable to open '%s'", toPath.c_str()).c_str());
                } else {
                    bool success = true;
                    while (1) {
                        char c;
                        size_t size = fread(&c, 1, 1, fromFile);
                        if (size != 1) {
                            if (!feof(fromFile)) {
                                fprintf(stderr, "Error reading '%s', upload failed\n", uploadFilename);
                                success = false;
                            }
                            break;
                        }
                        
                        toFile->write(c);
                        if (!toFile->valid()) {
                            fprintf(stderr, "Error writing '%s', upload failed\n", toPath.c_str());
                            success = false;
                            break;
                        }
                    }
                    toFile->close();
                    if (success) {
                        printf("Uploaded '%s' to '%s'\n", uploadFilename, toPath.c_str());
                    }
                }
            }
        }
        fclose(fromFile);
    }

    application.runAutostartTask();
    
    while (true) {
        application.runOneIteration();
    }

    return 0;
}

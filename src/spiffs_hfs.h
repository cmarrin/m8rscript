//
//  spiffs_hfs.h
//  m8rsim
//
//  Created by Chris Marrin on 12/13/19.
//  Copyright © 2019 MarrinTech. All rights reserved.
//

#ifndef spiffs_hfs_h
#define spiffs_hfs_h

#include "spiffs.h"

// Spiffs HFS File System
//
// Wraps a hierarchical file system around Spiffs.
//
// Use these calls in place of the normal Spiffs calls for all functions
typedef struct
{
    spiffs_file _file;
} SPIFFS_HFS_file;

// Supprted open modes:
//
// Mode           Read/write    File does not exist   File Exists            Seek
// -------------+-------------+---------------------+----------------------+--------------
// Read         | Read only   | NotFound error      | Contents preserved   | Yes
// ReadUpdate   | Read/write  | NotFound error      | Contents preserved   | Yes
// Write        | Write only  | Create file         | Contents deleted     | Yes
// WriteUpdate  | Read/write  | Create file         | Contents deleted     | Yes
// Append       | Write only  | Create file         | Position at end      | No
// AppendUpdate | Read/write  | Create file         | Position at end      | Only for read
//              |             |                     |                      | Resets to end on write
// Create       | Read/write  | Create file         | Contents preserved   | Yes
//
// Note: Create mode is not part of the Posix standard. It gives you the ability to
// open a file for read/write, creates it if it doesn't exist and allows full seek
// with no repositioning on write

typedef enum { SOMRead, SOMReadUpdate, SOMWrite, SOMWriteUpdate, SOMAppend, SOMAppendUpdate, SOMCreate } SPIFFS_open_mode;

int32_t SPIFFS_HFS_open(spiffs *fs, const char *path, SPIFFS_open_mode mode, SPIFFS_HFS_file* file);
int32_t SPIFFS_HFS_close(spiffs *fs, const SPIFFS_HFS_file* file);

#endif /* spiffs_hfs_h */

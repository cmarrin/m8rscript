/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include <cstdint>

namespace m8r {

//--------------------------------------------------------------------------
 
// Telnet interface
//
// Telnet is a symmetric interface, so this isn't really a client or a
// server. It sits between the incoming raw data from the Channel and the
// Client of that data. Data coming in from the Channel is unchanged unless
// it is IAC (0xff), then it is the start of a command. To receive a literal
// 0xff this command must be followed by another 0xff. So the sequence of
// events this class performs is:
//
//  1) If char is not IAC, pass through to Client, exit
//  2) If the next char is 0xff, pass 0xff through to Client, exit
//  3) Enter command mode
//
// Command mode is where all the action is. And example of a command is
// when the user types a ^c on a telnet client. This sends IAC IP.
//
// This class is a line editor. It maintains a line buffer which is
// edited based on input from the Channel. It supports delete, backspace,
// right and left arrow, ctl-a, ctl-e, etc. When a newline is received
// from the Channel, the line buffer is sent to the Client. As the line
// is edited edits are sent back to the Channel to keep the line up to
// date at that end.
//
//--------------------------------------------------------------------------

class Telnet {
public:
    // Interrupt is control-c
    enum class Action { None, UpArrow, DownArrow, RightArrow, LeftArrow, Delete, Backspace, Interrupt };
    
    // Commands from the Telnet Channel. See https://users.cs.cf.ac.uk/Dave.Marshall/Internet/node141.html
    enum class Command : uint8_t {
        SE      = 240,  // End of subnegotiation parameters.
        NOP     = 241,  // No operation
        DM      = 242,  // Data mark. Indicates the position of a Sync event within the data stream. This
                        // should always be accompanied by a TCP urgent notification.
        BRK     = 243,  // Break. Indicates that the "break" or "attention" key was hit.
        IP      = 244,  // Suspend, interrupt or abort the process to which the NVT is connected.
        AO      = 245,  // Abort output. Allows the current process to run to completion but do not send
                        // its output to the user.
        AYT     = 246,  // Are you there? Send back to the NVT some visible evidence that the AYT was received.
        EC      = 247,  // Erase character. The receiver should delete the last preceding undeleted
                        // character from the data stream.
        EL      = 248,  // Erase line. Delete characters from the data stream back to but not including the previous CRLF.
        GA      = 249,  // Go ahead. Used, under certain circumstances, to tell the other end that it can transmit.
        SB      = 250,  // Subnegotiation of the indicated option follows.
        WILL    = 251,  // Indicates the desire to begin performing, or confirmation that you are now performing,
                        // the indicated option.
        WONT    = 252,  // Indicates the refusal to perform, or continue performing, the indicated option.
        DO      = 253,  // Indicates the request that the other party perform, or confirmation that you are
                        // expecting the other party to perform, the indicated option.
        DONT    = 254,  // Indicates the demand that the other party stop performing, or confirmation that you
                        // are no longer expecting the other party to perform, the indicated option.
        IAC     = 255,  // Interpret as command
        
        ECHO    = 1,    // echo
        SGA     = 3,    // suppress go ahead
        STAT    = 5,    // status
        TM      = 6,    // timing mark
        TT      = 24,   // terminal type
        WS      = 31,   // window size
        TS      = 32,   // terminal speed
        RFC     = 33,   // remote flow control
        LINE    = 34,   // linemode
        EV      = 36,   // environment variables
    };
    
    template<typename T>
    String adder(String& s, T v) {
        return s + static_cast<char>(v);
    }

    template<typename T, typename... Args>
    String adder(String& s, T first, Args... args) {
      return s + static_cast<char>(first) + adder(s, args...);
    }
    
    // Return the init string to be sent to the Channel
    String init() { String s; return adder(s, Command::IAC, Command::WILL, Command::SGA, Command::IAC, Command::WONT, Command::LINE); }
    
    // This function:
    //
    //  - Receives characters from the Channel
    //  - Modifies internal state
    //  - Constructs string to send back to Channel (e.g., echo) if any
    //  - Constructs string to send to Client if any
    //  - Returns any command that resulted from the incoming data
    Action receive(const char* fromChannel, int16_t fromLength, String& toChannel, String& toClient);
    
    String sendCommand(Action);

private:
    std::vector<char> _line;
    int32_t _position = 0;
};

}

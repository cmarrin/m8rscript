/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MString.h"
#include "StateMachine.h"
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

static constexpr uint32_t makeAction(const char* s)
{
    return
        (static_cast<uint32_t>(s[0]) << 24) |
        (static_cast<uint32_t>(s[1]) << 16) |
        (static_cast<uint32_t>(s[2]) << 8) |
        static_cast<uint32_t>(s[3]);
}

class Telnet {
public:
    // Interrupt is control-c
    // Actions have a 1-4 character code which m8rscript can compare against. For instance
    // 
    //      function handleAction(action)
    //      {
    //          if (action == "down") ...
    //      }
    //
    // to make this work efficiently Action enumerants are uint32_t with the characters packed
    // in. These are converted to StringLiteral Values and sent to the script.     
    enum class Action : uint32_t {
        None = 0,
        UpArrow = makeAction("up  "),
        DownArrow = makeAction("down"),
        RightArrow = makeAction("rt  "),
        LeftArrow = makeAction("lt  "),
        Delete = makeAction("del "),
        Backspace = makeAction("bs  "),
        Interrupt = makeAction("intr"),
        NewLine = makeAction("newl"),
    };
    
    // Commands from the Telnet Channel. See https://users.cs.cf.ac.uk/Dave.Marshall/Internet/node141.html
    enum class Command : uint8_t {
        SE      = 240,  // (0xf0) End of subnegotiation parameters.
        NOP     = 241,  // (0xf1) No operation
        DM      = 242,  // (0xf2) Data mark. Indicates the position of a Sync event within the data stream. This
                        //        should always be accompanied by a TCP urgent notification.
        BRK     = 243,  // (0xf3) Break. Indicates that the "break" or "attention" key was hit.
        IP      = 244,  // (0xf4) Suspend, interrupt or abort the process to which the NVT is connected.
        AO      = 245,  // (0xf5) Abort output. Allows the current process to run to completion but do not send
                        //        its output to the user.
        AYT     = 246,  // (0xf6) Are you there? Send back to the NVT some visible evidence that the AYT was received.
        EC      = 247,  // (0xf7) Erase character. The receiver should delete the last preceding undeleted
                        //        character from the data stream.
        EL      = 248,  // (0xf8) Erase line. Delete characters from the data stream back to but not including the previous CRLF.
        GA      = 249,  // (0xf9) Go ahead. Used, under certain circumstances, to tell the other end that it can transmit.
        SB      = 250,  // (0xfa) Subnegotiation of the indicated option follows.
        WILL    = 251,  // (0xfb) Indicates the desire to begin performing, or confirmation that you are now performing,
                        //        the indicated option.
        WONT    = 252,  // (0xfc) Indicates the refusal to perform, or continue performing, the indicated option.
        DO      = 253,  // (0xfd) Indicates the request that the other party perform, or confirmation that you are
                        //        expecting the other party to perform, the indicated option.
        DONT    = 254,  // (0xfe) Indicates the demand that the other party stop performing, or confirmation that you
                        //        are no longer expecting the other party to perform, the indicated option.
        IAC     = 255,  // (0xff) Interpret as command
        
        ECHO    = 1,    // (0x01) echo
        SGA     = 3,    // (0x03) suppress go ahead
        STAT    = 5,    // (0x05) status
        TM      = 6,    // (0x06) timing mark
        TT      = 24,   // (0x18) terminal type
        WS      = 31,   // (0x1f) window size
        TS      = 32,   // (0x20) terminal speed
        RFC     = 33,   // (0x21) remote flow control
        LINE    = 34,   // (0x22) linemode
        EV      = 36,   // (0x24) environment variables
        SLE     = 45,   // (0x2d) suppress local echo
    };
    
    enum class State : uint8_t {
        Ready, Interrupt, Backspace, AddChar, AddFF, SendLine,
        IAC, IACVerb, IACCommand,
        CSI, CSIBracket, CSIParam, CSICommand,
    };
    
    struct Input {
        Input() { }
        Input(char a) : _a(a), _b(a) { }
        Input(char a, char b)
        {
            if (a < b) {
                _a = a;
                _b = b;
            } else {
                _a = b;
                _b = a;
            }
        }
    
        bool operator==(const Input& other) const
        {
            // a----------b              a-------b
            //      A----------B    A-------B
            //
            // a------------b       a------b
            //    A------B      A--------------B
            //
            // a-------b                           a-------b
            //           A-------B     A--------B
            return !(_b < other._a || _a > other._b);
        }

        char _a = 0;
        char _b = 0;
    };

    Telnet();
    
    template<typename T>
    String makeCommand(T v)
    {
        return String(static_cast<char>(v));
    }

    template<typename T, typename... Args>
    String makeCommand(T first, Args... args)
    {
        return String(static_cast<char>(first)) + makeCommand(args...);
    }
    
    // Return the init string to be sent to the Channel
    String init()
    {
        return makeCommand(
            Command::IAC, Command::WILL, Command::ECHO,
            Command::IAC, Command::WILL, Command::SGA,
            Command::IAC, Command::WONT, Command::LINE,
            static_cast<Command>('\r'), static_cast<Command>('\n'));
    }
    
    // This function:
    //
    //  - Receives characters from the Channel
    //  - Modifies internal state
    //  - Constructs string to send back to Channel (e.g., echo) if any
    //  - Constructs string to send to Client if any
    //  - Returns any command that resulted from the incoming data
    Action receive(char fromChannel, String& toChannel, String& toClient);
    
    String sendCommand(Action);

    void handleBackspace();
    void handleAddChar();
    void handleAddFF();
    void handleInterrupt();
    void handleCSICommand();
    void handleIACCommand();
    void handleSendLine();
    
private:
    String makeInputLine();
    
    enum class Verb { None, DO, DONT, WILL, WONT };
    
    Verb _verb = Verb::None;
    
    Vector<char> _line;
    int32_t _position = 0;
    char _escapeParam = 0;
    
    StateMachine<Telnet, State, Input> _stateMachine;
    static StateTable<Telnet, State, Input>::StateEntry _stateEntries[ ];
    static StateTable<Telnet, State, Input> _stateTable;

    String _toChannel;
    String _toClient;
    char _currentChar = 0;
    Action _currentAction = Action::None;
    char _csiParam = 0;
};

}

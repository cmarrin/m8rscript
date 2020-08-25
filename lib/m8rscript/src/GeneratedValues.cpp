// This file is generated. Do not edit

#include "GeneratedValues.h"
#include "Defines.h"
#include <cstdlib>

using namespace m8rscript;

static const char _Array[] = "Array";
static const char _Base64[] = "Base64";
static const char _BothEdges[] = "BothEdges";
static const char _Connected[] = "Connected";
static const char _Directory[] = "Directory";
static const char _Disconnected[] = "Disconnected";
static const char _Error[] = "Error";
static const char _FS[] = "FS";
static const char _FallingEdge[] = "FallingEdge";
static const char _File[] = "File";
static const char _GPIO[] = "GPIO";
static const char _Global[] = "Global";
static const char _High[] = "High";
static const char _IPAddr[] = "IPAddr";
static const char _Input[] = "Input";
static const char _InputPulldown[] = "InputPulldown";
static const char _InputPullup[] = "InputPullup";
static const char _Iterator[] = "Iterator";
static const char _JSON[] = "JSON";
static const char _Low[] = "Low";
static const char _MaxConnections[] = "MaxConnections";
static const char _None[] = "None";
static const char _Object[] = "Object";
static const char _Once[] = "Once";
static const char _Output[] = "Output";
static const char _OutputOpenDrain[] = "OutputOpenDrain";
static const char _PinMode[] = "PinMode";
static const char _ReceivedData[] = "ReceivedData";
static const char _Reconnected[] = "Reconnected";
static const char _Repeating[] = "Repeating";
static const char _RisingEdge[] = "RisingEdge";
static const char _SentData[] = "SentData";
static const char _TCP[] = "TCP";
static const char _TCPProto[] = "TCPProto";
static const char _Task[] = "Task";
static const char _Timer[] = "Timer";
static const char _Trigger[] = "Trigger";
static const char _UDP[] = "UDP";
static const char _UDPProto[] = "UDPProto";
static const char ___destructor[] = "__destructor";
static const char ___impl[] = "__impl";
static const char ___index[] = "__index";
static const char ___nativeObject[] = "__nativeObject";
static const char ___object[] = "__object";
static const char _arguments[] = "arguments";
static const char _back[] = "back";
static const char _call[] = "call";
static const char _close[] = "close";
static const char _consoleListener[] = "consoleListener";
static const char _constructor[] = "constructor";
static const char _currentTime[] = "currentTime";
static const char _decode[] = "decode";
static const char _delay[] = "delay";
static const char _digitalRead[] = "digitalRead";
static const char _digitalWrite[] = "digitalWrite";
static const char _disconnect[] = "disconnect";
static const char _done[] = "done";
static const char _encode[] = "encode";
static const char _env[] = "env";
static const char _eof[] = "eof";
static const char _error[] = "error";
static const char _errorString[] = "errorString";
static const char _format[] = "format";
static const char _front[] = "front";
static const char _getValue[] = "getValue";
static const char _import[] = "import";
static const char _importString[] = "importString";
static const char _iterator[] = "iterator";
static const char _join[] = "join";
static const char _lastError[] = "lastError";
static const char _length[] = "length";
static const char _lookupHostname[] = "lookupHostname";
static const char _makeDirectory[] = "makeDirectory";
static const char _meminfo[] = "meminfo";
static const char _mount[] = "mount";
static const char _mounted[] = "mounted";
static const char _name[] = "name";
static const char _next[] = "next";
static const char _null[] = "null";
static const char _onInterrupt[] = "onInterrupt";
static const char _open[] = "open";
static const char _openDirectory[] = "openDirectory";
static const char _parse[] = "parse";
static const char _pop_back[] = "pop_back";
static const char _pop_front[] = "pop_front";
static const char _print[] = "print";
static const char _printf[] = "printf";
static const char _println[] = "println";
static const char _push_back[] = "push_back";
static const char _push_front[] = "push_front";
static const char _read[] = "read";
static const char _remove[] = "remove";
static const char _rename[] = "rename";
static const char _run[] = "run";
static const char _seek[] = "seek";
static const char _send[] = "send";
static const char _setPinMode[] = "setPinMode";
static const char _setValue[] = "setValue";
static const char _size[] = "size";
static const char _split[] = "split";
static const char _start[] = "start";
static const char _stat[] = "stat";
static const char _stop[] = "stop";
static const char _stringify[] = "stringify";
static const char _toFloat[] = "toFloat";
static const char _toInt[] = "toInt";
static const char _toString[] = "toString";
static const char _toUInt[] = "toUInt";
static const char _trim[] = "trim";
static const char _type[] = "type";
static const char _undefined[] = "undefined";
static const char _unmount[] = "unmount";
static const char _valid[] = "valid";
static const char _value[] = "value";
static const char _waitForEvent[] = "waitForEvent";
static const char _write[] = "write";

const char* _sharedAtoms[] = {
    _Array,
    _Base64,
    _BothEdges,
    _Connected,
    _Directory,
    _Disconnected,
    _Error,
    _FS,
    _FallingEdge,
    _File,
    _GPIO,
    _Global,
    _High,
    _IPAddr,
    _Input,
    _InputPulldown,
    _InputPullup,
    _Iterator,
    _JSON,
    _Low,
    _MaxConnections,
    _None,
    _Object,
    _Once,
    _Output,
    _OutputOpenDrain,
    _PinMode,
    _ReceivedData,
    _Reconnected,
    _Repeating,
    _RisingEdge,
    _SentData,
    _TCP,
    _TCPProto,
    _Task,
    _Timer,
    _Trigger,
    _UDP,
    _UDPProto,
    ___destructor,
    ___impl,
    ___index,
    ___nativeObject,
    ___object,
    _arguments,
    _back,
    _call,
    _close,
    _consoleListener,
    _constructor,
    _currentTime,
    _decode,
    _delay,
    _digitalRead,
    _digitalWrite,
    _disconnect,
    _done,
    _encode,
    _env,
    _eof,
    _error,
    _errorString,
    _format,
    _front,
    _getValue,
    _import,
    _importString,
    _iterator,
    _join,
    _lastError,
    _length,
    _lookupHostname,
    _makeDirectory,
    _meminfo,
    _mount,
    _mounted,
    _name,
    _next,
    _null,
    _onInterrupt,
    _open,
    _openDirectory,
    _parse,
    _pop_back,
    _pop_front,
    _print,
    _printf,
    _println,
    _push_back,
    _push_front,
    _read,
    _remove,
    _rename,
    _run,
    _seek,
    _send,
    _setPinMode,
    _setValue,
    _size,
    _split,
    _start,
    _stat,
    _stop,
    _stringify,
    _toFloat,
    _toInt,
    _toString,
    _toUInt,
    _trim,
    _type,
    _undefined,
    _unmount,
    _valid,
    _value,
    _waitForEvent,
    _write,
};

const char** m8rscript::sharedAtoms(uint16_t& nelts)
{
    nelts = sizeof(_sharedAtoms) / sizeof(const char*);
    return _sharedAtoms;
}


/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Esp.h"
#include "Containers.h"
#include "UDP.h"

namespace m8r {

class MDNSResponder : public UDPDelegate {
public:
    enum class ServiceProtocol { TCP, UDP };
    
    void init(const char* name, uint32_t broadcastInterval = 30, uint32_t ttl = 120);
    ~MDNSResponder();

    void addService(uint16_t port, const char* instance, const char* serviceType, 
                    ServiceProtocol protocol = ServiceProtocol::TCP, const char* text = "");
    
private:
    enum class QuestionType {
       A = 0x0001,
       NS = 0x0002,
       CNAME = 0x0005,
       SOA = 0x0006,
       WKS = 0x000b,
       PTR = 0x000c,
       MX = 0x000f,
       TXT = 0x0010,
       SRV = 0x0021,
       AAAA = 0x001c,
    };
    
    static const uint16_t QClassIN = 1;

    struct MDNSHeader {
       uint16_t    xid;
       uint8_t     recursionDesired:1;
       uint8_t     truncated:1;
       uint8_t     authoritiveAnswer:1;
       uint8_t     opCode:4;
       uint8_t     queryResponse:1;
       uint8_t     responseCode:4;
       uint8_t     checkingDisabled:1;
       uint8_t     authenticatedData:1;
       uint8_t     zReserved:1;
       uint8_t     recursionAvailable:1;
       uint16_t    queryCount;
       uint16_t    answerCount;
       uint16_t    authorityCount;
       uint16_t    additionalCount;
    } __attribute__((__packed__));

    struct ServiceRecord {
        ServiceRecord() { }
        ServiceRecord(uint16_t port, const char* instance, const char* serviceType, ServiceProtocol protocol, const char* text)
            : _port(port)
            , _instance(String(instance))
            , _serviceType(String(serviceType))
            , _protocol(protocol)
            , _text(String(text))
        {
        }
        
        uint16_t _port = 0;
        String _instance;
        String _serviceType;
        ServiceProtocol _protocol = ServiceProtocol::TCP;
        String _text;
    };
    
    void receivedData(const char* data, uint16_t length);

    bool matchesHostname(const String& s) const
    {
        size_t length = _hostname.size();
        if (s.size() != length + 6) {
            return false;
        }
        return strncmp(_hostname.c_str(), s.c_str(), length) == 0 && strcmp(s.c_str() + length, ".local") == 0;
    }

    enum class ParseServiceReturn { OK, NotAService, BadProtocol };
    ParseServiceReturn parseService(const String& serviceString, const String& protocolString, 
                                 String& serviceName, ServiceProtocol& protocol);
    
    void write(uint32_t value)
    {
        write(static_cast<uint16_t>(value >> 16));
        write(static_cast<uint16_t>(value));
    }

    void write(uint16_t value)
    {
        write(static_cast<uint8_t>(value >> 8));
        write(static_cast<uint8_t>(value));
    }
    
    void write(uint8_t value) { _replyBuffer.push_back(value); }

    void write(const String& s);
    
    size_t writeAnswerHeader(QuestionType, uint16_t qclass);
    void writeHostname();
    void writeServiceName(int32_t serviceIndex);
    void writeInstanceName(int32_t serviceIndex);
    void setAnswerLength(size_t lengthIndex)
    {
        uint16_t size = _replyBuffer.size() - lengthIndex - 2;
        _replyBuffer[lengthIndex] = static_cast<uint8_t>(size >> 8);
        _replyBuffer[lengthIndex + 1] = static_cast<uint8_t>(size);
    }
    
    void writeHeader(uint8_t answerCount, uint8_t additionalCount);
    void writeA();
    void writePTR(int32_t serviceIndex);
    void writeSRV(int32_t serviceIndex);
    void writeTXT(int32_t serviceIndex);
    
    void sendReply();
    
    static void broadcastCB(void* arg) { reinterpret_cast<MDNSResponder*>(arg)->broadcast(); } 
    
    void broadcast();
    
    void sendAnswer(QuestionType, int32_t service);

    String _hostname;
    Vector<ServiceRecord> _services;
    uint32_t _ttl = 0;
    Vector<uint8_t> _replyBuffer;
    
    // UDPDelegate
    virtual void UDPevent(UDP*, Event event, const char* data = nullptr, uint16_t length = 0) override
    {
        if (event == Event::ReceivedData) {
            receivedData(data, length);
        }
    }
    
    Mad<UDP> _udp;
};

}

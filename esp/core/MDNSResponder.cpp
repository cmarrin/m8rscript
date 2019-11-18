/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MDNSResponder.h"

#include "SystemInterface.h"
#include "IPAddr.h"
#include <cstring>
#include <cstdlib>

extern "C" {
#include "user_interface.h"
}

using namespace m8r;

//#define MDNSRESP_DEBUG

static os_timer_t bc_timer;

void MDNSResponder::init(const char* name, uint32_t broadcastInterval, uint32_t ttl)
{
	_ttl = ttl;
    _hostname = name;
    
    UDP::joinMulticastGroup({ 224,0,0,251 });
    _udp = system()->createUDP(this, 5353);

	os_timer_disarm(&bc_timer);
	if (broadcastInterval > 0) {
        broadcastCB(this);
		os_timer_setfn(&bc_timer, (os_timer_func_t *)broadcastCB, this);
		os_timer_arm(&bc_timer, broadcastInterval * 1000, true);
	}
}

MDNSResponder::~MDNSResponder()
{
	os_timer_disarm(&bc_timer);
    UDP::leaveMulticastGroup({ 224,0,0,251 });
}

void MDNSResponder::addService(uint16_t port, const char* instance, const char* serviceType, ServiceProtocol protocol, const char* text)
{
    ServiceRecord service(port, instance, serviceType, protocol, text);
    int32_t serviceIndex = _services.size();
    _services.push_back(service);
    
    // Announce this service
    sendAnswer(QuestionType::PTR, serviceIndex);
}

const char* decodeNameStrings(const char* data, uint32_t dataLen, Vector<String>& names, const char* p)
{
    names.clear();
    
    const char* pointerReturn = nullptr;
    while(1) {
        uint32_t size = static_cast<uint32_t>(*p++);
        if ((size & 0xc0) == 0) {
            // this is a direct label
            names.push_back(String(reinterpret_cast<const char*>(p), size));
            p += size;
            if (*p == 0) {
                ++p;
                break;
            }
        } else {
            // This is a pointer label
            if (pointerReturn) {
                break;
            }
            uint32_t offset = ((size & ~0xc0) << 8) | *p++;
            pointerReturn = p;
            p = data + offset;
        }
    }
    return pointerReturn ?: p;
}

static inline uint16_t uint16FromBuf(const char* buf)
{
    return (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
}

MDNSResponder::ParseServiceReturn MDNSResponder::parseService(
                                    const String& serviceString, const String& protocolString, 
                                    String& serviceName, ServiceProtocol& protocol)
{
    if (serviceString[0] == '_') {
        // this is a service name
        serviceName = serviceString.slice(1);
        if (protocolString == "_tcp") {
            protocol = ServiceProtocol::TCP;
        } else if (protocolString == "_udp") {
            protocol = ServiceProtocol::UDP;
        } else {
            return ParseServiceReturn::BadProtocol;
        }
        return ParseServiceReturn::OK;
    }
    return ParseServiceReturn::NotAService;
}

void MDNSResponder::receivedData(const char* data, uint16_t length)
{
    if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0) {
        // only queries
        return;
    }

    int qcount = data[5]; // purposely ignore qcount > 255
    const char* p = data+12;
    Vector<String> names;
    
    while(qcount-- > 0) {
        p = decodeNameStrings(data, length, names, p);
        QuestionType qtype = static_cast<QuestionType>(uint16FromBuf(p));
        uint16_t qclass = uint16FromBuf(p + 2);
        p += 4;
        
        if ((qclass & 0x7fff) != 1 || names.size() < 2 || names[0].empty() || names[1].empty()) {
            continue;
        }
        
        // Supported formats:
        //
        //      1) <hostname>.local
        //      2) _<serviceName>.{_tcp | _udp).local
        //      3) <hostname>._<serviceName>.{_tcp | _udp).local
        
        String serviceName;
        ServiceProtocol protocol;
        bool haveService = false;
        bool haveHostname = false;
                
        ParseServiceReturn ret = parseService(names[0], names[1], serviceName, protocol);
        if (ret == ParseServiceReturn::BadProtocol) {
            continue;
        } else if (ret == ParseServiceReturn::NotAService) {
            // This is a hostname
            haveHostname = true;
            if (names[0] != _hostname) {
                // But it's not ours
                continue;
            }
        } else {
            haveService = true;
            if (names.size() < 3 || names[2] != "local") {
                continue;
            }
        }
        
        assert(haveHostname || haveService);
        
        if (haveHostname && names[1] != "local") {
            if (names.size() < 4) {
                continue;
            }
            ret = parseService(names[1], names[2], serviceName, protocol);
            if (ret != ParseServiceReturn::OK) {
                continue;
            }
            haveService = true;
            if (names[3] != "local") {
                continue;
            }
        }
        
        int32_t serviceIndex = -1;
        
        if (haveService) {
            for (int32_t i = 0; i < _services.size(); ++i) {
                if (_services[i]._serviceType == serviceName && _services[i]._protocol == protocol) {
                    serviceIndex = i;
                    break;
                }
            }
            if (serviceIndex < 0) {
                continue;
            }
        }

#ifdef MDNSRESP_DEBUG
        if (haveService) {
            if (haveHostname) {
                system()->printf ("Got one of our instances: %s.%s.%s.%s",
                            names[0].c_str(), names[1].c_str(), names[2].c_str(), names[3].c_str());
            } else {
                system()->printf ("Got one of our services: %s.%s.%s",
                            names[0].c_str(), names[1].c_str(), names[2].c_str());
            }
        } else {
            system()->printf ("Got our hostname: %s.%s", names[0].c_str(), names[1].c_str());
        }
        system()->printf (" - qtype=%d qclass=%d\n", qtype, qclass);
#endif

        sendAnswer(qtype, serviceIndex);
	}
}

void MDNSResponder::broadcast()
{
    // TBD
}

void MDNSResponder::writeHeader(uint8_t answerCount, uint8_t additionalCount)
{
    _replyBuffer.clear();
    
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0x84); // Response + Authoritative Answer
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(answerCount); // Answer count
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(0);
    _replyBuffer.push_back(additionalCount);
}

void MDNSResponder::write(const String& s)
{
    if (s.empty()) {
        return;
    }
    _replyBuffer.push_back(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        _replyBuffer.push_back(s[i]);
    }
}

void MDNSResponder::writeHostname()
{
    write(_hostname);
    write("local");
    _replyBuffer.push_back(0);
}

void MDNSResponder::writeServiceName(int32_t serviceIndex)
{
    write("_" + _services[serviceIndex]._serviceType);
    write((_services[serviceIndex]._protocol == ServiceProtocol::TCP) ? "_tcp" : "_udp");
    write("local");
    _replyBuffer.push_back(0);
}

void MDNSResponder::writeInstanceName(int32_t serviceIndex)
{
    write(_hostname);
    writeServiceName(serviceIndex);
}

size_t MDNSResponder::writeAnswerHeader(QuestionType qtype, uint16_t qclass)
{
    write(static_cast<uint16_t>(qtype));
    write(qclass);
    write(_ttl);
    write(static_cast<uint16_t>(0));
    return _replyBuffer.size() - 2;
}

void MDNSResponder::writeA()
{
    writeHostname();
    size_t lengthIndex = writeAnswerHeader(QuestionType::A, QClassIN);
    IPAddr addr = IPAddr::myIPAddr();
    write(addr[0]);
    write(addr[1]);
    write(addr[2]);
    write(addr[3]);
    setAnswerLength(lengthIndex);
}

void MDNSResponder::writePTR(int32_t serviceIndex)
{
    writeServiceName(serviceIndex);
    const ServiceRecord& service = _services[serviceIndex];
    size_t lengthIndex = writeAnswerHeader(QuestionType::PTR, QClassIN);
    writeInstanceName(serviceIndex);
    setAnswerLength(lengthIndex);
}

void MDNSResponder::writeSRV(int32_t serviceIndex)
{
    writeInstanceName(serviceIndex);
    size_t lengthIndex = writeAnswerHeader(QuestionType::SRV, QClassIN);
    write(static_cast<uint16_t>(0)); // Priority
    write(static_cast<uint16_t>(0)); // Weight
    write(_services[serviceIndex]._port);
    writeHostname();
    setAnswerLength(lengthIndex);
}

void MDNSResponder::writeTXT(int32_t serviceIndex)
{
    writeInstanceName(serviceIndex);
    size_t lengthIndex = writeAnswerHeader(QuestionType::TXT, QClassIN);
    write(_services[serviceIndex]._text);
    _replyBuffer.push_back(0);
    setAnswerLength(lengthIndex);
}

void MDNSResponder::sendReply()
{
    if (_replyBuffer.empty()) {
        return;
    }
    
#ifdef MDNSRESP_DEBUG
    hexdump("reply", &(_replyBuffer[0]), _replyBuffer.size());
#endif
    _udp->send({ 224,0,0,251 }, 5353, reinterpret_cast<char*>(&(_replyBuffer[0])), _replyBuffer.size());
    _replyBuffer.clear();
}

void MDNSResponder::sendAnswer(QuestionType qtype, int32_t service)
{
    if (service < 0 || service >= _services.size()) {
        return;
    }
    
    uint8_t additionalCount = 0;
    switch(qtype) {
        case QuestionType::A:
        case QuestionType::TXT: break;
        case QuestionType::SRV: additionalCount = 1; break;
        case QuestionType::PTR: additionalCount = 3; break;
        default: return;
    }
    
    writeHeader(1, additionalCount);
    
    switch(qtype) {
        case QuestionType::A:
            writeA();
            break;
        case QuestionType::TXT:
            writeTXT(service);
            break;
        case QuestionType::SRV:
            writeSRV(service);
            writeA();
            break;
        case QuestionType::PTR:
            writePTR(service);
            writeSRV(service);
            writeTXT(service);
            writeA();
            break;
    }

    sendReply();
}


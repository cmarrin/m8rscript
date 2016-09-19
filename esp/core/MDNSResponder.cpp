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

#include "MDNSResponder.h"

#include <cstring>
#include <cstdlib>

extern "C" {
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <mem.h>
#include <gpio.h>
#include "user_interface.h"
}

using namespace m8r;

#define MDNSRESP_DEBUG

static os_timer_t bc_timer;

MDNSResponder::MDNSResponder(const char* name, uint32_t broadcastInterval, uint32_t ttl)
{
	_ttl = ttl;
    _hostname = name;
    _hostname += ".local";
    
    esp::UDP::joinMulticastGroup({ 224,0,0,251 });
    _udp = new MyUDP(5353, this);

	os_timer_disarm(&bc_timer);
	if (broadcastInterval > 0) {
		os_timer_setfn(&bc_timer, (os_timer_func_t *)broadcastCB, this);
		os_timer_arm(&bc_timer, broadcastInterval * 1000, true);
	}
}

MDNSResponder::~MDNSResponder()
{
	os_timer_disarm(&bc_timer);
    delete _udp;
    esp::UDP::leaveMulticastGroup({ 224,0,0,251 });
}


//uint8_t* MDNSResponder::encodeResp(uint32_t ip, const char *name, int *len)
//{
//        *len = 12+strlen(name)+16;
//        unsigned char *data = (unsigned char *)malloc(*len);
//        memset(data, 0, *len);
//
//        data[2] = 0x84;
//        data[7] = 1;
//
//        unsigned char *p = data+12;
//        const char *np = name;
//        char *i;
//        while(i = ets_strstr(np,"."))
//        {
//                *p = i - np;
//                p++;
//                memcpy(p,np,i-np);
//                p += i - np;
//                np = i + 1;
//        }
//        *p = strlen(np);
//        p++;
//        memcpy(p,np,strlen(np));
//        p += strlen(np);
//        *p++ = 0; // terminate string sequence
//
//        *p++ = 0; *p++ = 1; // type 0001 (A)
//
//        *p++ = 0x80; *p++ = 1; // class code (IPV4)
//
//        *p++ = _ttl >> 24;
//        *p++ = _ttl >> 16;
//        *p++ = _ttl >> 8;
//        *p++ = _ttl;
//
//        *p++ = 0; *p++ = 4; // length (of ip)
//
//        memcpy(p, &ip, 4);
//
//        return data;
//}

const char* decodeNameStrings(const char* data, uint32_t dataLen, String& name, const char* p)
{
    name.erase();
    while(1) {
        if (name.length() != 0) {
            name += ".";
        }
        uint32_t size = static_cast<uint32_t>(*p++);
        if ((size & 0xc0) == 0) {
            // this is a direct label
            name += String(reinterpret_cast<const char*>(p), size);
            p += size;
            if (*p == 0) {
                ++p;
                break;
            }
        } else {
            // This is a pointer label
            uint32_t offset = ((size & ~0xc0) << 8) | *p++;
            name += String(reinterpret_cast<const char*>(data + offset + 1), data[offset]);
            break;
        }
    }
    return p;
}

//void MDNSResponder::sendOne(struct _host *h)
//{
//	_conn.proto.udp->remote_ip[0] = 224;
//	_conn.proto.udp->remote_ip[1] = 0;
//	_conn.proto.udp->remote_ip[2] = 0;
//	_conn.proto.udp->remote_ip[3] = 251;
//	_conn.proto.udp->remote_port = 5353;
//	_conn.proto.udp->local_port = 5353;
//#ifdef MDNSRESP_DEBUG
//	hexdump("sending",h->mdnsresp, h->len);
//#endif
//	espconn_send(&_conn,h->mdnsresp, h->len);
//
//}

static inline uint16_t uint16FromBuf(const char* buf)
{
    return (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
}

void MDNSResponder::receivedData(const char* data, uint16_t length)
{
    if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0) {
        // only queries
        return;
    }

    int qcount = data[5]; // purposely ignore qcount > 255
    const char* p = data+12;
    String name;
    uint32_t len;
    while(qcount-- > 0) {
        p = decodeNameStrings(data, length, name, p);
        QuestionType qtype = static_cast<QuestionType>(uint16FromBuf(p));
        uint16_t qclass = uint16FromBuf(p + 2);
        p += 4;
        
        if ((qclass & 0x7fff) != 1 || name != _hostname) {
            continue;
        }
        
#ifdef MDNSRESP_DEBUG
		os_printf ("decoded our hostname qtype=%d qclass=%d\n", qtype, qclass);
#endif
        _replyBuffer.clear();
        
		if (qtype == QuestionType::A || qtype == QuestionType::SRV || qtype == QuestionType::PTR) {
            writeA();
        }
        if (qtype == QuestionType::PTR) {
            writePTR();
        }
        if (qtype == QuestionType::SRV || qtype == QuestionType::PTR) {
            writeSRV();
        }
        if (qtype == QuestionType::TXT || qtype == QuestionType::PTR) {
            writeTXT();
        }
        
        sendReply();
	}
}

void MDNSResponder::broadcast()
{
    _replyBuffer.clear();
    writePTR();
    writeTXT();
    writeSRV();
    writeA();
    sendReply();
}

void MDNSResponder::addService(uint16_t port, const char* instance, const char* serviceType, ServiceProtocol protocol, const char* text)
{
    _services.emplace_back(port, instance, serviceType, protocol, text);
}

void MDNSResponder::writeA()
{
}

void MDNSResponder::writePTR()
{
}

void MDNSResponder::writeSRV()
{
}

void MDNSResponder::writeTXT()
{
}

void MDNSResponder::sendReply()
{
    if (_replyBuffer.empty()) {
        return;
    }
    
    _udp->send({ 224,0,0,251 }, 5353, reinterpret_cast<char*>(&(_replyBuffer[0])), _replyBuffer.size());
    _replyBuffer.clear();
}

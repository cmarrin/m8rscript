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

#include "MacUDP.h"

#include "Containers.h"
#include "Value.h"
#include <cstring>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <errno.h>

using namespace m8r;

IPAddr IPAddr::myIPAddr()
{
    return IPAddr();
}

UDP* UDP::create(UDPDelegate* delegate, uint16_t port)
{
    return new MacUDP(delegate, port);
}

void UDP::joinMulticastGroup(IPAddr addr)
{
//    struct ip_addr mDNSmulticast;
//    struct ip_addr any;
//    mDNSmulticast.addr = addr;
//    any.addr = IPADDR_ANY;
//	espconn_igmp_join(&any, &mDNSmulticast);
}

void UDP::leaveMulticastGroup(IPAddr addr)
{
//    struct ip_addr mDNSmulticast;
//    struct ip_addr any;
//    mDNSmulticast.addr = addr;
//    any.addr = IPADDR_ANY;
//	espconn_igmp_leave(&any, &mDNSmulticast);
}

MacUDP::MacUDP(UDPDelegate* delegate, uint16_t port)
    : UDP(delegate, port)
{
    _socketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socketFD == -1) {
        printf("Error opening TCP socket\n");
        return;
    }

    String queueName = "UDPSocketQueue-";
    queueName += Value::toString(_socketFD);
    _queue = dispatch_queue_create(queueName.c_str(), DISPATCH_QUEUE_SERIAL);
    
    if (port) {
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof sa);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(_socketFD,(struct sockaddr *)&sa, sizeof sa) == -1) {
            printf("Error: UDP bind failed\n");
            close(_socketFD);
            _socketFD = -1;
            return;
        }
    }

    dispatch_async(_queue, ^() {
        fd_set readfds;
        int maxsd;
        
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(_socketFD, &readfds);
            maxsd = _socketFD;
            
            int selectResult = select(maxsd + 1, &readfds, NULL, NULL, NULL);
            if (selectResult < 0 && errno != EINTR) {
                printf("ERROR: UDP select returned %d, error=%d\n", selectResult, errno);
                continue;
            }
            
            ssize_t readResult = read(_socketFD, _receiveBuffer, BufferSize - 1);
            if (readResult == 0) {
                // Disconnect
                dispatch_sync(dispatch_get_main_queue(), ^{
                    _delegate->UDPdisconnected(this);
                });
                close(_socketFD);
                _socketFD = -1;
                break;
            } else if (readResult < 0) {
                printf("ERROR: UDP read returned %zd, error=%d\n", readResult, errno);
            } else {
                dispatch_sync(dispatch_get_main_queue(), ^{
                    _delegate->UDPreceivedData(this, _receiveBuffer, readResult);
                });
            }
        }
    });
}

MacUDP::~MacUDP()
{
    close(_socketFD);
    _socketFD = -1;
}

void MacUDP::send(IPAddr addr, uint16_t port, const char* data, uint16_t length)
{
    if (!length) {
        length = strlen(data);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = addr;
    sa.sin_port = htons(port);
    
    ssize_t result = sendto(_socketFD, data, length, 0, (struct sockaddr*)&sa, sizeof(sa));
    if (result == -1) {
        printf("ERROR: send (UDP) returned %zd, error=%d\n", result, errno);
    }
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        _delegate->UDPsentData(this);
    });
}

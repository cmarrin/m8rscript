/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MacUDP.h"

#include "Containers.h"
#include "IPAddr.h"
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
    char name[40];
    gethostname(name, 39);
    struct hostent* entry = gethostbyname(name);
    if (entry) {
        IPAddr ip(entry->h_addr_list[0][0], entry->h_addr_list[0][1], entry->h_addr_list[0][2], entry->h_addr_list[0][3]);
        return ip;
    }
    return IPAddr();
}

void IPAddr::lookupHostName(const char* name, std::function<void (const char* name, IPAddr)> func)
{
    struct hostent* entry = gethostbyname(name);
    IPAddr ip(entry->h_addr_list[0][0], entry->h_addr_list[0][1], entry->h_addr_list[0][2], entry->h_addr_list[0][3]);
    func(name, ip);
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

void MacUDP::init(uint16_t port, EventFunction func)
{
    UDP::init(port, func);
    
    _socketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socketFD == -1) {
        printf("Error opening TCP socket\n");
        return;
    }
    
    if (!port) {
        return;
    }

    String queueName = "UDPSocketQueue-";
    queueName += String::toString(_socketFD);
    _queue = dispatch_queue_create(queueName.c_str(), DISPATCH_QUEUE_SERIAL);
    
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

    dispatch_async(_queue, ^() {
        while (1) {
            struct sockaddr_in saOther;
            socklen_t saOtherSize;
            ssize_t result = recvfrom(_socketFD, _receiveBuffer, BufferSize - 1, 0, (struct sockaddr *)&saOther, &saOtherSize);
            if (result <= 0) {
                if (result == 0 || errno == EINTR) {
                    // Disconnect
                    dispatch_sync(dispatch_get_main_queue(), ^{
                        _eventFunction(this, UDP::Event::Disconnected, nullptr, -1);
                    });
                    close(_socketFD);
                    _socketFD = -1;
                    return;
                }
                printf("ERROR: UDP recvfrom returned %zd, error=%d\n", result, errno);
                return;
            }
            dispatch_sync(dispatch_get_main_queue(), ^{
                _eventFunction(this, UDP::Event::ReceivedData, _receiveBuffer, result);
            });
        }
    });
}

MacUDP::~MacUDP()
{
    close(_socketFD);
    _socketFD = -1;
    dispatch_release(_queue);
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
        _eventFunction(this, UDP::Event::SentData, nullptr, -1);
    });
}

void MacUDP::disconnect()
{
    close(_socketFD);
    _socketFD = -1;
}


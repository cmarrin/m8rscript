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

#include "MacTCP.h"

#include "Containers.h"
#include "Value.h"
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

using namespace m8r;

TCP* TCP::create(TCPDelegate* delegate, uint16_t port)
{
    return new MacTCP(delegate, port, IPAddr());
}

TCP* TCP::create(TCPDelegate* delegate, uint16_t port, IPAddr ip)
{
    return new MacTCP(delegate, port, ip);
}

MacTCP::MacTCP(TCPDelegate* delegate, uint16_t port, IPAddr ip)
    : TCP(delegate, port, ip)
{
    _server = !ip;
    
    _socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_socketFD == -1) {
        _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "opening TCP socket", -1);
        return;
    }

    int enable = 1;
    if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "opening TCP socket, setsockopt failed", -1);
        return;
    }
    
    String queueName = "TCPQueue-";
    queueName += Value::toString(_socketFD);
    _queue = dispatch_queue_create(queueName.c_str(), DISPATCH_QUEUE_SERIAL);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (ip) {
        inet_aton(ip.toString().c_str(), &(sa.sin_addr));
    }

    if (_server) {
        if (bind(_socketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
            _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "TCP bind failed");
            close(_socketFD);
            _socketFD = -1;
            return;
        }
      
        if (listen(_socketFD, MaxConnections) == -1) {
            _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "TCP listen failed");
            close(_socketFD);
            _socketFD = -1;
            return;
        }

        memset(_clientSockets, 0, MaxConnections * sizeof(int));
    } else {
        if (connect(_socketFD, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
            _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "TCP connect failed");
        } else {
            _delegate->TCPevent(this, TCPDelegate::Event::Connected, 0);
        }
    }

    _dispatchSemaphore = dispatch_semaphore_create(0);
    dispatch_async(_queue, ^() {
        fd_set readfds;
        int maxsd;
        
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(_socketFD, &readfds);
            maxsd = _socketFD;
            
            if (_server) {
                for (int socket : _clientSockets) {
                    if (socket > 0) {
                        FD_SET(socket, &readfds);
                    }
                    if (socket > maxsd) {
                        maxsd = socket;
                    }
                }
            }
            
            int result = select(maxsd + 1, &readfds, NULL, NULL, NULL);
            if (result < 0 && errno != EINTR) {
                _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "select failed");
                break;
            }
            
            if (_server) {
                if (FD_ISSET(_socketFD, &readfds)) {
                    // We have an incoming connection
                    int addrlen;
                    
                    int clientSocket = accept(_socketFD, (struct sockaddr *)&sa, (socklen_t*)&addrlen);
                    if (clientSocket < 0) {
                        _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "accept failed");
                        continue;
                    }
                    printf("New connection , socket fd=%d, ip=%s, port=%d\n", clientSocket, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
                    int16_t connectionId = -1;
                    for (int& socket : _clientSockets) {
                        if (!socket) {
                            std::lock_guard<std::mutex> lock(_mutex);
                            socket = clientSocket;
                            connectionId = &socket - _clientSockets;
                            break;
                        }
                    }
                    
                    if (connectionId < 0) {
                        close(clientSocket);
                        _delegate->TCPevent(this, TCPDelegate::Event::Error, -1, "Too many connections on port\n");
                        break;
                    }
                    
                    _delegate->TCPevent(this, TCPDelegate::Event::Connected, connectionId);
                }

                for (int& socket : _clientSockets) {
                    if (FD_ISSET(socket, &readfds)) {
                        // Something came in on this client socket
                        int16_t connectionId = &socket - _clientSockets;
                        
                        ssize_t result = read(socket, _receiveBuffer, BufferSize - 1);
                        if (result == 0) {
                            // Disconnect
                            int addrlen;
                            getpeername(socket, (struct sockaddr*) &sa, (socklen_t*) &addrlen);
                            printf("Host disconnected, ip=%s, port=%d\n" , inet_ntoa(sa.sin_addr) , ntohs(sa.sin_port));
                            _delegate->TCPevent(this, TCPDelegate::Event::Disconnected, connectionId);
                            close(socket);
                            std::lock_guard<std::mutex> lock(_mutex);
                            socket = 0;
                        } else if (result < 0) {
                            _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "read error");
                        } else {
                            _delegate->TCPevent(this, TCPDelegate::Event::ReceivedData, connectionId, _receiveBuffer, result);
                        }
                    }
                }
            } else {
                // Client, received data
                ssize_t result = read(_socketFD, _receiveBuffer, BufferSize - 1);
                if (result == 0) {
                    // Disconnect
                    _delegate->TCPevent(this, TCPDelegate::Event::Disconnected, 0);
                    shutdown(_socketFD, SHUT_RDWR);
                    close(_socketFD);
                    _socketFD = -1;
                    break;
                } else if (result < 0) {
                    _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "read failed");
                } else {
                    _delegate->TCPevent(this, TCPDelegate::Event::ReceivedData, 0, _receiveBuffer, result);
                }
            }
        }
        dispatch_semaphore_signal(_dispatchSemaphore);
    });
}

MacTCP::~MacTCP()
{
    close(_socketFD);
    for (int& socket : _clientSockets) {
        if (socket) {
            close(socket);
        }
    }
    dispatch_semaphore_wait(_dispatchSemaphore, DISPATCH_TIME_FOREVER);
    dispatch_release(_queue);
}

void MacTCP::send(int16_t connectionId, const char* data, uint16_t length)
{
    if (_server) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (int socket : _clientSockets) {
            if (socket) {
                ssize_t result = ::send(socket, data, length, 0);
                if (result == -1) {
                    _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "send (server) failed");
                }
            }
        }
    } else {
        ssize_t result = ::send(_socketFD, data, length, 0);
        if (result == -1) {
            _delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "send (client) faile");
        }
    }
    
    _delegate->TCPevent(this, TCPDelegate::Event::SentData, connectionId);
}

void MacTCP::disconnect(int16_t connectionId)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return;
    }
    
    close(_clientSockets[connectionId]);
    _clientSockets[connectionId] = 0;
    _delegate->TCPevent(this, TCPDelegate::Event::Disconnected, connectionId);
}

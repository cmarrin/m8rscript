/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "RtosTCP.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_log.h"

using namespace m8r;

void RtosTCP::init(uint16_t port, IPAddr ip, EventFunction func)
{
    TCP::init(port, ip, func);
    xTaskCreate(_server ? staticServerTask : staticClientTask, _server ? "tcpServer" : "tcpClient", 4096, this, 5, NULL);
}

RtosTCP::~RtosTCP()
{
    close(_socketFD);
    for (int& socket : _clientSockets) {
        if (socket != InvalidFD) {
            close(socket);
        }
    }
}

void RtosTCP::clientTask()
{
}

void RtosTCP::serverTask()
{
    _socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (_socketFD < 0) {
        ESP_LOGE("TCP", "Unable to create socket: errno %d", errno);
        addEvent(TCP::Event::Error, errno, "opening TCP socket");
        return;
    }
    ESP_LOGI("TCP", "Socket created");

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(_port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    int err = bind(_socketFD, (struct sockaddr *)&sa, sizeof(sa));
    if (err != 0) {
        ESP_LOGE("TCP", "Socket unable to bind: errno %d", errno);
        addEvent(TCP::Event::Error, errno, "TCP bind failed");
        close(_socketFD);
        _socketFD = -1;
        return;
    }
    ESP_LOGI("TCP", "Socket bound");

    err = listen(_socketFD, 1);
    if (err != 0) {
        ESP_LOGE("TCP", "Error occured during listen: errno %d", errno);
        addEvent(TCP::Event::Error, errno, "TCP listen failed");
        close(_socketFD);
        _socketFD = -1;
        return;
    }
    ESP_LOGI("TCP", "Socket listening");

    for (auto& socket : _clientSockets) {
        socket = InvalidFD;
    }

    fd_set readfds;
    int maxsd;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(_socketFD, &readfds);
        maxsd = _socketFD;
        
        for (int socket : _clientSockets) {
            if (socket != InvalidFD) {
                FD_SET(socket, &readfds);
            }
            if (socket > maxsd) {
                maxsd = socket;
            }
        }
        
        int result = select(maxsd + 1, &readfds, NULL, NULL, NULL);
        if (result < 0 && errno != EINTR) {
            // Select failed, probably because we closed one of the sockets we
            // were waiting on. Ignore it
            //_delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "select failed");
            ESP_LOGE("TCP", "Select failed: error=%d", errno);
            continue;
        }
        
        if (FD_ISSET(_socketFD, &readfds)) {
            // We have an incoming connection
            uint addrLen = sizeof(sa);
            int clientSocket = accept(_socketFD, (struct sockaddr *)&sa, &addrLen);
            if (clientSocket < 0) {
                ESP_LOGE("TCP", "Unable to accept connection: errno %d", errno);
                addEvent(TCP::Event::Error, errno, "accept failed");
                continue;
            }

            int16_t connectionId = -1;
            for (int& socket : _clientSockets) {
                if (socket == InvalidFD) {
                    socket = clientSocket;
                    connectionId = &socket - _clientSockets;
                    break;
                }
            }
        
            if (connectionId < 0) {
                close(clientSocket);
                addEvent(TCP::Event::Error, -1, "Too many connections on port");
                break;
            }
        
            ESP_LOGI("TCP", "New connection: id=%d, socket fd=%d, ip=%s, port=%d", connectionId, _clientSockets[connectionId], inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
            addEvent(TCP::Event::Connected, connectionId, nullptr);
        }
        
        for (int& socket : _clientSockets) {
            if (FD_ISSET(socket, &readfds)) {
                // Something came in on this client socket
                int16_t connectionId = &socket - _clientSockets;
                        
                ssize_t result = read(socket, _receiveBuffer, BufferSize - 1);
                if (result == 0) {
                    // Connection closed
                    ESP_LOGI("TCP", "Connection closed");
                    addEvent(TCP::Event::Disconnected, connectionId, nullptr);
                    close(socket);
                    socket = InvalidFD;
                } else if (result < 0) {
                    // Error occured during receiving
                    ESP_LOGE("TCP", "read failed: id=%d, socket=%d, len=%d, errno=%d", connectionId, _clientSockets[connectionId], result, errno);
                    addEvent(TCP::Event::Error, errno, "read error");
                }
                else {
                    addEvent(TCP::Event::ReceivedData, connectionId, _receiveBuffer, static_cast<int32_t>(result));
                }
            }
        }
    }
    vTaskDelete(NULL);
}

int32_t RtosTCP::sendData(int16_t connectionId, const char* data, uint16_t length)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return -1;
    }
    if (!length) {
        length = ::strlen(data);
    }
    
    int socket = _server ? _clientSockets[connectionId] : _socketFD;

    return static_cast<int32_t>(::send(socket, data, length, 0));
}

void RtosTCP::disconnect(int16_t connectionId)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return;
    }
    
    int socket = _clientSockets[connectionId];
    _clientSockets[connectionId] = InvalidFD;
    close(socket);
    addEvent(TCP::Event::Disconnected, connectionId, nullptr);
}

void RtosTCP::handleEvents()
{
    TCP::handleEvents();
}

IPAddr RtosTCP::clientIPAddr(int16_t connectionId) const
{
    if (connectionId < 0 || connectionId > MaxConnections || !_clientSockets[connectionId]) {
        return IPAddr();
    }
    
    struct sockaddr_in addr;
    socklen_t len;
    if (getsockname(_clientSockets[connectionId], reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        return IPAddr();
    }
    
    return IPAddr(inet_ntoa(addr.sin_addr));
}

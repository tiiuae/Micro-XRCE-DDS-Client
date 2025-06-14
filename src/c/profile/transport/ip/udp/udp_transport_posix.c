#include <uxr/client/profile/transport/ip/udp/udp_transport_posix.h>
#include "udp_transport_internal.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>

bool uxr_init_udp_platform(
        uxrUDPPlatform* platform,
        uxrIpProtocol ip_protocol,
        const char* ip,
        const char* recv_port,
        const char* send_port)
{
    bool rv = false;

    switch (ip_protocol)
    {
        case UXR_IPv4:
            platform->poll_fd.fd = socket(AF_INET, SOCK_DGRAM, 0);
            uint16_t rport = (uint16_t)atoi(recv_port);
            if (rport > 0) {
                struct sockaddr_in _receiver_inaddr;
                memset((char *)&_receiver_inaddr, 0, sizeof(_receiver_inaddr));
                _receiver_inaddr.sin_family = AF_INET;
                _receiver_inaddr.sin_port = htons(rport);
                _receiver_inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
                if (bind(platform->poll_fd.fd, (struct sockaddr *)&_receiver_inaddr, sizeof(_receiver_inaddr)) < 0) {
                    return rv;
                }
            }
            break;
        case UXR_IPv6:
            platform->poll_fd.fd = socket(AF_INET6, SOCK_DGRAM, 0);
            break;
    }

    if (-1 != platform->poll_fd.fd)
    {
        struct addrinfo hints;
        struct addrinfo* result;
        struct addrinfo* ptr;

        memset(&hints, 0, sizeof(hints));
        switch (ip_protocol)
        {
            case UXR_IPv4:
                hints.ai_family = AF_INET;
                break;
            case UXR_IPv6:
                hints.ai_family = AF_INET6;
                break;
        }
        hints.ai_socktype = SOCK_DGRAM;

        if (0 == getaddrinfo(ip, send_port, &hints, &result))
        {
            for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
            {
                if (0 == connect(platform->poll_fd.fd, ptr->ai_addr, ptr->ai_addrlen))
                {
                    platform->poll_fd.events = POLLIN;
                    rv = true;
                    break;
                }
            }
            freeaddrinfo(result);
        }
    }
    return rv;
}

bool uxr_close_udp_platform(
        uxrUDPPlatform* platform)
{
    return (-1 == platform->poll_fd.fd) ? true : (0 == close(platform->poll_fd.fd));
}

size_t uxr_write_udp_data_platform(
        uxrUDPPlatform* platform,
        const uint8_t* buf,
        size_t len,
        uint8_t* errcode)
{
    size_t rv = 0;
    ssize_t bytes_sent = send(platform->poll_fd.fd, (void*)buf, len, 0);
    if (-1 != bytes_sent)
    {
        rv = (size_t)bytes_sent;
        *errcode = 0;
    }
    else
    {
        *errcode = 1;
    }
    return rv;
}

size_t uxr_read_udp_data_platform(
        uxrUDPPlatform* platform,
        uint8_t* buf,
        size_t len,
        int timeout,
        uint8_t* errcode)
{
    size_t rv = 0;
    int poll_rv = poll(&platform->poll_fd, 1, timeout);
    if (0 < poll_rv)
    {
        ssize_t bytes_received = recv(platform->poll_fd.fd, (void*)buf, len, 0);
        if (-1 != bytes_received)
        {
            rv = (size_t)bytes_received;
            *errcode = 0;
        }
        else
        {
            *errcode = 1;
        }
    }
    else
    {
        *errcode = (0 == poll_rv) ? 0 : 1;
    }
    return rv;
}

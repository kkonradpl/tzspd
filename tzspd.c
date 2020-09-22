/*
 *  TZSPd - TZSP repeater
 *  Copyright (c) 2020  Konrad Kosmatka
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
typedef SOCKET socket_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
typedef int socket_t;
#endif

#include "addr-list.h"
#include "utils.h"
#include "tzsp-decap.h"

#define VERSION "1.0"
#define TZSP_DEFAULT_PORT 0x9090
#define SOCKET_BUFF_LEN 65536
#define MAC80211_HEADER_LEN 24

struct tzspd
{
    /* Configuration */
    uint8_t daemon;
    uint8_t beacon_mode;
    uint8_t discard_mgmt;
    uint8_t discard_ctrl;
    uint8_t discard_data;
    uint8_t discard_ext;

    /* Internal data */
    uint16_t input;
    socket_t socket_in;
    socket_t socket_out;
    addr_list_t *outputs;
} typedef tzspd_t;

static void tzspd_show_usage(char*);
static int tzspd_bind(tzspd_t*);
static void tzspd_loop(tzspd_t*);
static void tzspd_close(tzspd_t*);
static void tzspd_free(tzspd_t*);

int
main(int   argc,
     char *argv[])
{
    tzspd_t *context;
    char *tmp;
    int c, i;

    context = calloc(sizeof(tzspd_t), 1);
    context->input = TZSP_DEFAULT_PORT;
        
#ifdef _WIN32
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        tzspd_log(context->daemon, TZSPD_LOG_ERR, "WSAStartup");
        return -1;
    }
#endif

    while((c = getopt(argc, argv, "i:dbMCDEh")) != -1)
    {
        switch(c)
        {
            case 'i':
                context->input = atoi(optarg);
                if(context->input <= 0 || context->input > 65535)
                {
                    tzspd_log(context->daemon, TZSPD_LOG_ERR, "Invalid input port (%d)", context->input);
                    return -1;
                }
                break;

#ifndef _WIN32
            case 'd':
                context->daemon = 1;
                break;
#endif

            case 'b':
                context->beacon_mode = 1;
                break;

            case 'M':
                context->discard_mgmt = 1;
                break;

            case 'C':
                context->discard_ctrl = 1;
                break;

            case 'D':
                context->discard_data = 1;
                break;

            case 'E':
                context->discard_ext = 1;
                break;

            case 'h':
                tzspd_show_usage(argv[0]);
                return 0;

            case ':':
            case '?':
                tzspd_show_usage(argv[0]);
                return -1;
        }
    }

    for(i = optind; i < argc; i++)
    {
        tmp = strdup(argv[i]);
        switch(addr_list_parse(&context->outputs, argv[i]))
        {
            case ADDR_LIST_INVALID_PORT_RANGE:
                tzspd_log(context->daemon, TZSPD_LOG_ERR, "Invalid port range (%s)", tmp);
                return -1;
            case ADDR_LIST_INVALID_PORT:
                tzspd_log(context->daemon, TZSPD_LOG_ERR, "Invalid port value (%s)", tmp);
                return -1;
            case ADDR_LIST_INVALID_MAC:
                tzspd_log(context->daemon, TZSPD_LOG_ERR, "Invalid MAC address (%s)", tmp);
                return -1;
            case ADDR_LIST_OK:
                break;
        }
        free(tmp);
    }

    if(!context->outputs)
    {
        tzspd_log(context->daemon, TZSPD_LOG_ERR, "No output specified");
        return -1;
    }

#ifndef _WIN32
    if(context->daemon)
    {
        switch(fork())
        {
            case -1:
                tzspd_log(context->daemon, TZSPD_LOG_ERR, "fork");
                return -1;

            case 0:
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                umask(0);
                break;

            default:
                return 0;
        }

        if(open("/dev/null", O_RDONLY) == -1 ||
           open("/dev/null", O_WRONLY) == -1 ||
           open("/dev/null", O_RDWR) == -1)
        {
            tzspd_log(context->daemon, TZSPD_LOG_ERR, "open /dev/null");
            return -1;
        }

        if(setsid() < 0)
        {
            tzspd_log(context->daemon, TZSPD_LOG_ERR, "setsid");
            return -1;
        }

        if(chdir("/") < 0)
        {
            tzspd_log(context->daemon, TZSPD_LOG_ERR, "chdir");
            return -1;
        }
    }
#endif

    tzspd_log(context->daemon, TZSPD_LOG_INFO, "tzspd " VERSION " is starting using UDP port %d", context->input);

    if(tzspd_bind(context) < 0)
        return -1;
        
    tzspd_loop(context);
    tzspd_close(context);
    tzspd_free(context);

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

static void
tzspd_show_usage(char *arg)
{
    printf("tzspd " VERSION " - TZSP repeater\n");
#ifndef _WIN32
    printf("%s usage: [-i port] [-d] [-b] [-M] [-C] [-D] [-E] port[,sensor] ...\n", arg);
#else
    printf("%s usage: [-i port] [-b] [-M] [-C] [-D] [-E] port[,sensor] ...\n", arg);
#endif
    printf("options:\n");
    printf("  -i  UDP input port (default 37008)\n");
#ifndef _WIN32
    printf("  -d  run server in background as daemon\n");
#endif
    printf("  -b  pass only beacons packets (for MTscan)\n");
    printf("  -M  discard 802.11 management frames\n");
    printf("  -C  discard 802.11 control frames\n");
    printf("  -D  discard 802.11 data frames\n");
    printf("  -E  discard 802.11 extension frames\n");
}

static int
tzspd_bind(tzspd_t *context)
{
    struct sockaddr_in addr;

    context->socket_in = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    if(context->socket_in == INVALID_SOCKET)
#else
    if(context->socket_in < 0)
#endif
    {
        tzspd_log(context->daemon, TZSPD_LOG_ERR, "Failed to create a socket");
        return -1;
    }

    context->socket_out = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    if(context->socket_out == INVALID_SOCKET)
#else
    if(context->socket_out < 0)
#endif
    {
        tzspd_log(context->daemon, TZSPD_LOG_ERR, "Failed to create a socket");
        return -1;
    }

    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(context->input);

    if(bind(context->socket_in, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        tzspd_log(context->daemon, TZSPD_LOG_ERR, "Failed to bind to port (%d)", context->input);
        return -1;
    }

    return 0;
}

static void
tzspd_loop(tzspd_t *context)
{
    uint8_t packet[SOCKET_BUFF_LEN];
    const uint8_t *data;
    struct sockaddr_in addr_in;
    struct sockaddr_in addr_out;
    socklen_t slen;
    ssize_t ret;
    uint32_t data_len;
    const uint8_t *sensor_mac;
    const addr_list_t *node;
    uint16_t i;
    uint8_t type;

    memset((char*)&addr_out, 0, sizeof(addr_out));
    addr_out.sin_family = AF_INET;
    addr_out.sin_addr.s_addr = inet_addr("127.0.0.1");

    while(1)
    {
        slen = sizeof(addr_in);
        ret = recvfrom(context->socket_in, (char*)packet, SOCKET_BUFF_LEN, 0, (struct sockaddr*)&addr_in, &slen);

        if(ret == -1 && errno == EINTR)
            continue;

        if(ret <= 0)
            break;

        /* Make sure this is a TZSP packet */
        data_len = ret;
        sensor_mac = NULL;
        if((data = decap_tzsp(packet, &data_len, &sensor_mac)) == NULL)
            continue;

        /* Discard invalid packets */
        if(data_len < MAC80211_HEADER_LEN)
            continue;

        type = data[0] & 0x0Fu;
        if(context->discard_mgmt && type == 0x00)
            continue;
        if(context->discard_ctrl && type == 0x04)
            continue;
        if(context->discard_data && type == 0x08)
            continue;
        if(context->discard_ext && type == 0x0C)
            continue;

        if(context->beacon_mode)
        {
            /* Discard other packets */
            if(!(data[0] == 0x80 || /* keep 802.11 beacon frames */
                 data[0] == 0x50 || /* keep 802.11 probe responses */
                 (data[0] == 0x08 && data[1] == 0x90))) /* keep NV2 beacons */
            {
                continue;
            }
        }

        /* Send the packet */
        for(node = context->outputs; node; node = addr_list_get_next(node))
        {
            if(!addr_list_get_mac(node) ||
               (addr_list_get_mac(node) && sensor_mac &&
                memcmp(addr_list_get_mac(node), sensor_mac, MAC_ADDR_LEN) == 0))
            {
                for(i = addr_list_get_port(node); i <= addr_list_get_range(node); i++)
                {
                    addr_out.sin_port = htons(i);
                    sendto(context->socket_out, packet, ret, 0, (struct sockaddr*)&addr_out, sizeof(addr_out));
                }
            }
        }
    }
}

static void
tzspd_close(tzspd_t *context)
{
#ifdef _WIN32
    if(context->socket_in == INVALID_SOCKET)
        return;
    shutdown(context->socket_in, SD_BOTH);
    shutdown(context->socket_out, SD_BOTH);
    closesocket(context->socket_in);
    closesocket(context->socket_out);
    context->socket_in = INVALID_SOCKET;
    context->socket_out = INVALID_SOCKET;
#else
    if(context->socket_in < 0)
        return;
    shutdown(context->socket_in, SHUT_RDWR);
    shutdown(context->socket_out, SHUT_RDWR);
    close(context->socket_in);
    close(context->socket_out);
    context->socket_in = -1;
    context->socket_out = -1;
#endif
}

static void
tzspd_free(tzspd_t *context)
{
    addr_list_free(context->outputs);
    free(context);
}

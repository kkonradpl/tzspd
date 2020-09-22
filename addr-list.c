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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include "win32.h"
#endif
#include "addr-list.h"

struct addr_list
{
    uint16_t port;
    uint16_t range;
    uint8_t *mac;
    struct addr_list *next;
};

static uint8_t* addr_list_parse_mac(char*);

addr_list_status_t
addr_list_parse(addr_list_t **list,
                char        *string)
{
    addr_list_t *node;
    char *parse;
    char *ptr;
    int port;
    int range = -1;
    uint8_t *mac = NULL;

    parse = string;
    ptr = strsep(&parse, ",");
    if(parse)
    {
        mac = addr_list_parse_mac(parse);
        if(mac == NULL)
            return ADDR_LIST_INVALID_MAC;
    }

    parse = ptr;
    ptr = strsep(&parse, "-");
    port = atoi(ptr);
    if(parse)
        range = atoi(parse);

    if(range < 0)
        range = port;

    if(port <= 0 || port > 65535 || range <= 0 || range > 65535)
    {
        free(mac);
        return ADDR_LIST_INVALID_PORT;
    }

    if(range < port)
    {
        free(mac);
        return ADDR_LIST_INVALID_PORT_RANGE;
    }

    node = malloc(sizeof(addr_list_t));
    node->port = port;
    node->range = range;
    node->mac = mac;
    node->next = *list;

    *list = node;
    return ADDR_LIST_OK;
}

void
addr_list_free(addr_list_t *node)
{
    addr_list_t *tmp;

    while(node)
    {
        tmp = node->next;
        free(node->mac);
        free(node);
        node = tmp;
    }
}

uint16_t
addr_list_get_port(const addr_list_t *list)
{
    return list->port;
}

uint16_t
addr_list_get_range(const addr_list_t *list)
{
    return list->range;
}

const uint8_t*
addr_list_get_mac(const addr_list_t *list)
{
    return list->mac;
}

const addr_list_t*
addr_list_get_next(const addr_list_t *list)
{
    return list->next;
}

static uint8_t*
addr_list_parse_mac(char *string)
{
    uint8_t *mac;
    unsigned int tmp[MAC_ADDR_LEN];
    int len, ret, i;

    len = strlen(string);

    if(len != MAC_ADDR_LEN*2+5)
        return NULL;

    ret = sscanf(string,
                 "%x:%x:%x:%x:%x:%x",
                 &tmp[0], &tmp[1], &tmp[2],
                 &tmp[3], &tmp[4], &tmp[5]);

    if(ret != MAC_ADDR_LEN)
        return NULL;

    mac = malloc(sizeof(uint8_t)*MAC_ADDR_LEN);
    for(i=0; i<MAC_ADDR_LEN; i++)
        mac[i] = (uint8_t)tmp[i];

    return mac;
}

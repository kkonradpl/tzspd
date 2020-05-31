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

#ifndef TZSPD_ADDR_LIST_H_
#define TZSPD_ADDR_LIST_H_

#define MAC_ADDR_LEN 6

typedef struct addr_list addr_list_t;
typedef enum addr_list_status
{
    ADDR_LIST_INVALID_PORT_RANGE = -3,
    ADDR_LIST_INVALID_PORT = -2,
    ADDR_LIST_INVALID_MAC = -1,
    ADDR_LIST_OK = 0
} addr_list_status_t;

addr_list_status_t addr_list_parse(addr_list_t**, char*);
void addr_list_free(addr_list_t*);
uint16_t addr_list_get_port(const addr_list_t*);
uint16_t addr_list_get_range(const addr_list_t*);
const uint8_t* addr_list_get_mac(const addr_list_t*);
const addr_list_t* addr_list_get_next(const addr_list_t*);

#endif

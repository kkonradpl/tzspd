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
#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
#include <Winsock2.h>
#include <Windows.h>
#include "win32.h"
#else
#include <sys/types.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#endif

#define TZSP_HEADER_LEN     4

#define TZSP_VERSION        0x01
#define TZSP_TYPE           0x00
#define TZSP_PROTO          0x12

#define TZSP_TAG_PADDING    0x00
#define TZSP_TAG_END        0x01
#define TZSP_TAG_SIGNAL     0x0A
#define TZSP_TAG_RATE       0x0C
#define TZSP_TAG_FCS        0x11
#define TZSP_TAG_CHANNEL    0x12
#define TZSP_TAG_LENGTH     0x29
#define TZSP_TAG_SENSOR_MAC 0x3C

struct tzsp_header
{
    uint8_t version;
    uint8_t type;
    uint16_t enc_proto;
};

static const uint8_t* tzsp_process_tags(const uint8_t*, uint32_t*, bool, const uint8_t**);

const uint8_t*
decap_tzsp(const uint8_t  *packet,
           uint32_t       *len,
           const uint8_t **sensor_mac)
{
    struct tzsp_header *tzsp;
    const uint8_t *data;
    uint32_t data_len;

    if(packet &&
       *len > TZSP_HEADER_LEN)
    {
        tzsp = (struct tzsp_header*)packet;

        if(tzsp->version != TZSP_VERSION)
            return NULL;

        if(tzsp->type != TZSP_TYPE)
            return NULL;

        if(tzsp->enc_proto != ntohs(TZSP_PROTO))
            return NULL;

        data_len = *len - TZSP_HEADER_LEN;
        data = tzsp_process_tags(packet + TZSP_HEADER_LEN, &data_len, false, sensor_mac);
        if(data)
        {
            *len = data_len;
            return data;
        }
    }
    return NULL;
}

static const uint8_t*
tzsp_process_tags(const uint8_t  *packet,
                  uint32_t       *len,
                  bool            allow_fcs_err,
                  const uint8_t **sensor_mac)
{
    uint8_t tag;
    uint8_t tag_len;
    const uint8_t *tag_data;
    int i = 0;

    while(i < *len)
    {
        tag = packet[i++];

        /* Padding tag has no length */
        if(tag == TZSP_TAG_PADDING)
            continue;

        /* Check whether the iterator is still valid */
        if(i >= *len)
            return NULL;

        /* End tag has no length */
        if(tag == TZSP_TAG_END)
        {
            *len -= i;
            return (packet + i);
        }

        tag_len = packet[i++];
        tag_data = packet+i;

        /* Check whether the tag length is valid */
        if(i+tag_len >= *len)
            return NULL;

        /* Process the tag */
        if(tag == TZSP_TAG_FCS &&
           tag_len == 1)
        {
            if(!allow_fcs_err && *tag_data)
                return NULL;
        }
        else if(tag == TZSP_TAG_SENSOR_MAC &&
                tag_len == 6)
        {
            *sensor_mac = tag_data;
        }

        /* Move forward */
        i += tag_len;
    }
    return NULL;
}

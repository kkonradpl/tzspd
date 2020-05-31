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

#include <string.h>

char*
strsep(char **string,
       char  *del)
{
    char *start = *string;
    char *p = (start ? strpbrk(start, del) : NULL);

    if(!p)
    {
        *string = NULL;
    }
    else
    {
        *p = '\0';
        *string = p + 1;
    }
    
    return start;
}

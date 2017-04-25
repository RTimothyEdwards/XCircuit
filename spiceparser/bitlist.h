/********************
    This file is part of the software library CADLIB written by Conrad Ziesler
    Copyright 2003, Conrad Ziesler, all rights reserved.

*************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

******************/
/* bitlist.h, declarations for bitlist functions
Conrad Ziesler
*/

#ifndef __BITLIST_H__
#define __BITLIST_H__


/*** bitlist:  1-d array of bits ******/
typedef struct bitlist_st
{
  int qty;
  int qbytes;
  unsigned char data[4];
}bitlist_t;

#define SCAN_FORWARD 1
#define SCAN_REVERSE 0
#define SCAN_SET     2
#define SCAN_CLEAR   0

bitlist_t * bitlist_new(int qty);
void bitlist_free(bitlist_t *b);
int bitlist_set(bitlist_t *bl, int index);
int bitlist_test(bitlist_t *bl, int index);
int bitlist_clear(bitlist_t *bl, int index);
bitlist_t *bitlist_resize(bitlist_t *bl,int qty);
int bitlist_scan(bitlist_t *bl, int flags);
int bitlist_copyhead(bitlist_t *dest, bitlist_t *src, int length);
void bitlist_clearall(bitlist_t *bl);
void bitlist_setall(bitlist_t *bl);

#endif

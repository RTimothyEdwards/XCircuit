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
/* memory.h, header for private memory allocation routines
   Conrad Ziesler
*/

#ifndef __MEMORY_H__
#define __MEMORY_H__

#define MEMORY_CHUNKSIZE (1024*16)
#define MEMORY_THRESHOLD  16  /* a block is full if it within this size of full */

typedef struct memory_chain_st
{
  struct memory_chain_st *next;
  int q;
  int qref; /* we maintain an reference count */
  char data[MEMORY_CHUNKSIZE];
  int magic[4];
}memory_chain_t;
#define MEMORY_MAGIC 0x35dfa315

typedef struct memory_other_st
{
  struct memory_other_st *next;
}memory_other_t;

typedef struct memory_head_st
{
  memory_chain_t *head,*full;
  memory_other_t *others;
}memory_t;

#define MEMORY_INIT { NULL, NULL, NULL }


void *memory_alloc(memory_t *ma, int size);
void memory_free(memory_t *ma, void *vp);
void memory_freeall(memory_t *ma);
void memory_init(memory_t *ma);

#endif




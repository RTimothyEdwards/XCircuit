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
/* list.h  list module, for array-type list operators 

   Conrad Ziesler
*/

#ifndef __LIST_H__
#define __LIST_H__

#ifndef __BITLIST_H__
#include "bitlist.h"
#endif

typedef struct list_st
{
  int q; /* qty or current index */
  int s; /* size of item */
  int m; /* max alloc */
  int mode; /* list access mode */
  void *d;
}list_t;


typedef struct listx_st
{
  list_t list;
  bitlist_t *bitmap;
}listx_t;


#define LIST_INITDATA { 0, 0, 0, 0, NULL }
#define LIST_MODECHUNK 0x000fff  /* mask for chunck bits */
#define LIST_DEFMODE   0x000010  /* reallocate every 16 */
#define LIST_EXPMODE   0x001fff  /* double size per allocation (for exponential growth lists) */ 
#define LIST_FIXEDMODE 0x002000  /* flag error if we exceed hinted size */
#define LIST_UNITMODE 0x01  /* reallocate every 1 */
#define LIST_USERFLAG1 0x010000
#define LIST_USERFLAG2 0x020000
#define LIST_USERMASK  0xff0000
#define list_getuser(l)    (((l)->mode&LIST_USERMASK)>>16)
#define list_putuser(l,u)  (l)->mode=((u<<16)&LIST_USERMASK)
#define list_setuser(l,u)  list_putuser((l),list_getuser((l))|(u))
#define list_clearuser(l,u) list_putuser((l),list_getuser((l))&(~u))
#define list_iterate(l,i,p) for((p)=list_data((l),(i)=0);(p)!=NULL;(p)=list_data((l),++(i)))
#define list_iterate_(l,i)  for((i)=0;(i)<((l)->q);(i)++)
#define list_iterate_backwards(l,i,p) for((p)=list_data((l),(i)=((l)->q-1));(p)!=NULL;(p)=list_data((l),--(i)))
#define list_lastitem(l)  (list_data((l),(l)->q-1))
#define list_firstitem(l) (list_data((l),0))
#define list_qty(l)       ((l)->q)
#define list_sizeof(l)    ((l)->s)

/* listx routines allow for allocating and freeing list elements */
void listx_init(listx_t *l, int size, int mode);
int listx_qty(listx_t *l);
void *listx_data(listx_t *l, int cell);
void listx_empty(listx_t *l);
void listx_free(listx_t *l, int cell);
int listx_alloc(listx_t *l);
void *listx_vdata(listx_t *l, int cell); 

#define listx_iterate(l,i,p) for((p)=list_data(&((l)->list),(i)=0);(p)!=NULL;(p)=list_data(&((l)->list),++(i))) if(bitlist_test((l)->bitmap,i))

/* #define list_data(l,i)   ((void *)( (((i)<((l)->q))&&((i)>=0))? (((char *)((l)->d))+((l)->s * (i))):NULL))
 
#define list_data(l,i) list_data_func(l,i)
 */

static inline void *list_data(const list_t *l, int i)
{
  if((i<l->q)&&(i>=0))
    return ((char *)(l->d))+(l->s * i);
  return NULL;
}

static inline void *list_vdata(const list_t *l, int i)
{
  if((i<l->q)&&(i>=0))
    return ((char *)(l->d))+(l->s * i);
  assert(0&&"invalid index into list");
}

int list_element_dup(list_t *l, int from, int to);
int list_remove(list_t *l, int i, void *save);
void list_empty(list_t *l);
void list_init(list_t *l, int s, int mode);
void list_hint(list_t *l, int max);
void *list_next(list_t *l);
void *list_prev(list_t *l);
void *list_block_next(list_t *l, int qty);
void inline *list_data_func(list_t *l, int i);
int list_contains(list_t *l, void *d);
int list_add(list_t *l, void *d);
int list_index(list_t *l, void *p);
void list_sort(list_t *l, int (*cmp)(const void *a, const void *b));
void list_start(list_t *l);
void list_copyfrom(list_t *newl, list_t *oldl);
void list_shrink(list_t *l);
int list_nextz(list_t *l);
void list_dup(list_t *newl, list_t *oldl);
void list_reset(list_t *l);
void list_append(list_t *growing, list_t *copyfrom);
int list_stdlib_bsearch(list_t *l, void *key, int (*cmp)(const void *a, const void *b));

typedef struct sort_func_st
{
  list_t *tosort;
  int (*cmpf)(struct sort_func_st *s, void *a, void *b);
  int aiskey; /* if element a is a key and not a normal element */
  void *user;
}sort_func_t;

int list_bsearch(sort_func_t *sf, void *key);

/* this is in sort.c */
void list_qsort (sort_func_t *sf);



#endif

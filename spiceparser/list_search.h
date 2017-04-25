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
/* list_search.h, list container that supports arbitary (binary) search 
   Conrad Ziesler
*/


#ifndef __LIST_SEARCH_H__
#define __LIST_SEARCH_H__

#ifndef __LIST_H__
#include "list.h"
#endif

typedef struct list_psearch_st
{
  void *user;
  int (*cmpf)(const void *user, const void *a, const void *b);
  int *isearchdata;
  int is_sorted;
  int len;
}list_psearch_t;

typedef struct list_search_st
{
  list_t list;
  list_t psearch;
}list_search_t;

typedef struct list_search_iterator_st
{
  list_search_t *sp;
  int which;
  int last_id;
}list_search_iterator_t;

#define list_search(lp)    (&((lp)->list))

#define list_search_data(a,b) list_data(list_search((a)),(b))
#define list_search_vdata(a,b) list_vdata(list_search((a)),(b))
#define list_search_iterate(l,i,p) for((p)=list_search_data((l),(i)=0);(p)!=NULL;(p)=list_search_data((l),++(i)))
#define list_search_add(a,b) list_add(list_search((a)),(b))
#define list_search_qty(a)   list_qty(list_search((a)))
#define list_search_index(a,b) list_index(list_search((a)),(b))
#define list_search_reset(a)   list_reset(list_search((a)))
#define list_search_iterate_backwards(l,i,p) for((p)=list_search_data((l),(i)=(list_search((l))->q-1));(p)!=NULL;(p)=list_search_data((l),--(i)))


void list_search_init(list_search_t *lp, int size, int mode);
int list_search_addrule(list_search_t *lp, int (*cmpf)(const void *user, const void *a, const void *b),void *user);
void list_search_resort(list_search_t *lp);
int list_search_find(list_search_t *lp, int which, void *key, list_search_iterator_t *sip);
int list_search_findnext(list_search_iterator_t *sip);
void list_search_empty(list_search_t *lp);


/* private */
void list_search_qsort (list_search_t *lsp, list_psearch_t *psp);






#endif

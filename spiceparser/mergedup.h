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
/* mergedup.h : utility routine header for detecting multiple same entities in nlogn time
   Conrad Ziesler 
*/

typedef struct sortlist_st 
{
  int index;
  unsigned char sorted[1];
} sortlist_t;


typedef struct mergedup_st 
{
  int qs,es;
  int qun;
  int run;
  int q;
  int i;
  int flb;
  sortlist_t *last;
  unsigned char *freelist;
  char *data;
}mergedup_t;

void mergedup_setbit(mergedup_t *m, int index);
int mergedup_testbit(mergedup_t *m, int index);
mergedup_t *mergedup_alloc(int q, int qs);
void  mergedup_fill(mergedup_t *m, unsigned char *d, int index);
void  mergedup_sort(mergedup_t *m);
int   mergedup_visit(mergedup_t *m, int w);
void  mergedup_free(mergedup_t *m);



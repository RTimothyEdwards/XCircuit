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
/* mergedup.c  -> code to find and merge common entities,
                  works in nlogn time, for general entities

Conrad Ziesler
*/

#include "debug.h"
#include "mergedup.h"




mergedup_t *mergedup_alloc(int q, int qs)
{
  mergedup_t *m;
  int es=sizeof(sortlist_t)+qs-1;
  
  m=malloc(sizeof(mergedup_t) +  (es * q) + ((q>>3)+2));
  assert(m!=NULL);
  m->flb=(q>>3)+2;
  m->qs=qs;
  m->es=es;
  m->qun=qs/4;
  m->run=(qs-(m->qun*4));
  m->q=q;
  m->i=0;
  m->data= (void *) (((char *)m)+sizeof(mergedup_t)) ;
  m->freelist= (void *) (((char *)(m->data))+ (es*q) );
  m->last=(void *)(m->data);
  memset(m->freelist,0,m->flb);
  return m;
}


void mergedup_fill(mergedup_t *m, unsigned char *d, int index)
{
  int i;
  sortlist_t *p;
  p= (sortlist_t *) ( (char *)m->data + (m->es*m->i) );
  for(i=0;i<m->qs;i++)
    p->sorted[i]=d[i];

  p->index=index;
  m->i++;
  assert(m->i<=m->q);
}

static int mergedup__qs=0;

static int comparesorted(const void *a, const void *b)
{
  unsigned char *pa=((sortlist_t *)a)->sorted;
  unsigned char *pb=((sortlist_t *)b)->sorted;
  int i;
  
  if(pa==pb)return 0;
  if(pa==NULL)return 1;
  if(pb==NULL)return -1;
  
  for(i=0;i<mergedup__qs;i++)
    {
      if      (pa[i]<pb[i]) return 1;
      else if (pa[i]>pb[i]) return -1;
    }
  return 0;
}


void mergedup_sort(mergedup_t *m)
{
  mergedup__qs=m->qs;
  comparesorted(NULL,NULL);
  qsort(m->data,m->q, m->es, comparesorted);
  m->i=0;
  m->last=NULL;
}

void mergedup_setbit(mergedup_t *m, int index)
{
  int byte,bit;
  byte=index>>3;
  bit=index-(byte<<3);
  assert(byte<m->flb);
  m->freelist[byte]|=(1<<bit);
}

int mergedup_testbit(mergedup_t *m, int index)
{
  int byte,bit;
  byte=index>>3;
  bit=index-(byte<<3);
  assert(byte<m->flb);
  if((m->freelist[byte]&(1<<bit))!=0)return 1;
  return 0;
}

int mergedup_visit(mergedup_t *m, int w)
{
  sortlist_t *p;
  int i;
  int v=-1;
  
  if(m->i>=m->q)return -1;

  /* get unsorted of current data[m->i] */
  p= (sortlist_t *) ( (char *)m->data + (m->es*m->i) );
  v=p->index;
  
  if(w) /* w=1, get next item */
    {
      m->last=p;
      m->i++;      
      return v;
    }
  else /* w=0, return next same item */ 
    {
     if(m->last==NULL)return -1;
     for(i=0;i<m->qs;i++)
       if(p->sorted[i]!=m->last->sorted[i])return -1;
     
     m->i++;
     return v;
    }
  return -1;
}



void mergedup_free(mergedup_t *m)
{
  if(m!=NULL)free(m);
}


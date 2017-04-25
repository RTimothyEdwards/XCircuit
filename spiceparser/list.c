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
/* list.c  list module, for array-type list operators 

   Conrad Ziesler
*/

#include "debug.h"
#include "list.h"

/* listx, supports allocation and freeing of list elements */

void listx_init(listx_t *l, int size, int mode)
{
  list_init(&(l->list),size,mode);
  l->bitmap=NULL;
}

int listx_alloc(listx_t *l)
{
  int cell;
  if(l->bitmap==NULL)
    {
      cell=list_nextz(&(l->list));
      l->bitmap=bitlist_new(l->list.m);
      assert(l->bitmap!=NULL);
      bitlist_set(l->bitmap,cell);
      return cell;
    }
  else
    {
      cell=bitlist_scan(l->bitmap,SCAN_FORWARD|SCAN_CLEAR);
      if((cell<0)||(cell>=l->list.q))
	cell=list_nextz(&(l->list));
      
      if(l->list.m != l->bitmap->qty )
	l->bitmap=bitlist_resize(l->bitmap,l->list.m);
      
      bitlist_set(l->bitmap,cell);
      return cell;
    }
  return -1;
}

void listx_free(listx_t *l, int cell)
{
  bitlist_clear(l->bitmap,cell);
  memset(list_data(&(l->list),cell),0,l->list.s);
}
void listx_empty(listx_t *l)
{
  list_empty(&(l->list));
  if(l->bitmap!=NULL)bitlist_free(l->bitmap);
  l->bitmap=NULL;
}

void *listx_vdata(listx_t *l, int cell)
{
  if(bitlist_test(l->bitmap,cell))
    return list_data(&(l->list),cell);
  return NULL;
}

void *listx_data(listx_t *l, int cell)
{
  return list_data(&(l->list),cell);
}

int listx_qty(listx_t *l)
{
  return list_qty(&(l->list));
}


void list_shrink(list_t *l)
{
  list_hint(l,l->q);
}


void list_empty(list_t *l)
{
  if(l->d!=NULL)
    free(l->d);
  l->q=0;
  l->m=0;
  l->d=NULL;
}

void list_init(list_t *l, int s, int mode)
{
  static const list_t id=LIST_INITDATA;
  if(l!=NULL)
    { *l=id; l->s=s; l->mode=mode; l->q=0; l->m=0; }
}

void list_hint(list_t *l, int max)
{
  
  if(max==0)
    {
      if(l->d!=NULL)
	free(l->d);
      l->d=NULL;
      l->m=0;
      l->q=0;
    }
  else
  {
    if(l->s!=0)
      l->d=realloc(l->d,(l->m=max)*l->s);
    
    assert(l->d!=NULL);
  }
}


void list_hint_grow(list_t *l, int add_q)
{
  int qm=l->q+add_q;
  if(l->m < qm )
    l->d=realloc(l->d,(l->m=qm)*l->s);
}

void inline *list_data_func(list_t *l, int i)
{
  if((i<l->q)&&(i>=0))
    return ((char *)(l->d))+(l->s * i);
  return NULL;
}

void *list_next(list_t *l)
{  
  int v;

  if(l->q>=l->m)
    {
      if( (l->mode & LIST_FIXEDMODE)!=0)
	{ assert(0); return NULL; }
      
      if( (l->mode & LIST_EXPMODE)!=0) 
	{
	  if(l->s<(1024*1024))
	    {
	      if((l->m*l->s)>(1024*1024*16))
		list_hint(l,l->m+((1024*1024*16)/l->s));
	      else
		list_hint(l,2*( (l->m==0)?16:l->m) );
	    }
	  else list_hint(l,l->m+1);
	}
      else
	{
	  v=l->m+(l->mode&LIST_MODECHUNK);
	  if(v<l->m)v=l->m+1;
	  list_hint(l, v);
	}
    }
  return ((char *)(l->d))+(l->s * l->q++);
}

void *list_prev(list_t *l)
{
  if(l->q>0)
    {
      return ((char *)(l->d))+(l->s * --(l->q));
    }
  return NULL;
}

void *list_block_next(list_t *l, int qty)
{
  void *r;
  if((l->q+qty)>=l->m)
    list_hint(l, l->q+qty);
  
  r=((char*)l->d)+(l->s*l->q);
  l->q+=qty;
  return r;
}




int list_contains(list_t *l, void *d)
{
  int i;
  for(i=0;i<l->q;i++)
    if(memcmp(d,list_data(l,i),l->s)==0)
      return i;
  return -1;
}

int list_add(list_t *l, void *d)
{
  void *p; int i;
  i=l->q;
  p=list_next(l);
  memcpy(p,d,l->s);
  return i;
}

int list_nextz(list_t *l)
{
  int i;
  void *p;
  i=l->q;
  p=list_next(l);
  memset(p,0,l->s);
  return i;
}

int list_index(list_t *l, void *p)
{
  if( p >= (void *) ((((char *)(l->d))+(l->s*l->q)))) return -1;
  if( p <  l->d) return -1;
  return  (((char *)p)-((char *)(l->d)))/l->s;
}

int list_stdlib_bsearch(list_t *l, void *key, int (*cmp)(const void *a, const void *b))
{
  void *p= bsearch(key,l->d, l->q,l->s,cmp);
  if(p==NULL)return -1;
  return list_index(l,p);
}

void list_sort(list_t *l, int (*cmp)(const void *a, const void *b))
{
  qsort(l->d,l->q,l->s,cmp);
}

void list_start(list_t *l)
{
  l->q=0;
}


int list_remove(list_t *l, int i, void *save)
{
  void *p;
  if(i>=l->q)return 1;
  if(i<0)return 1;
  p=list_data(l,i);
  if(save!=NULL)memcpy(save,p,l->s);
  l->q--;
  if(l->q>i)
    memmove(p,((char *)p)+l->s,(l->q-i)*l->s);

  return 0;
}

void list_copyfrom(list_t *newl, list_t *oldl)
{

  list_t id=LIST_INITDATA;
  if(newl!=NULL)
    { *newl=id; newl->s=oldl->s; newl->mode=oldl->mode; newl->q=0; newl->m=0; }
}

void list_dup(list_t *newl, list_t *oldl)
{
  void *d;
  list_copyfrom(newl,oldl);
  list_hint(newl,oldl->q);
  d=list_block_next(newl,oldl->q);
  memcpy(d,oldl->d,oldl->s*oldl->q);
}

void list_append(list_t *growing, list_t *copyfrom)
{
  void *d;
  if(copyfrom==NULL)return;
  assert(growing->s==copyfrom->s);
  if(copyfrom->q==0)return;
  list_hint(growing,growing->q+copyfrom->q);
  d=list_block_next(growing,copyfrom->q);
  memcpy(d,copyfrom->d,copyfrom->s*copyfrom->q);
}


int list_element_dup(list_t *l, int from, int to)
{
  void *a,*b;
  if( (to>l->q) || (to<0) || (from>=l->q) || (from<0) )return 1;
  if(to==l->q) list_next(l);
  a=list_data(l,to);
  b=list_data(l,from);
  assert((a!=NULL)&&(b!=NULL));
  memcpy(a,b,l->s);
  return 0;
}

void list_reset(list_t *l)
{
  l->q=0;
}


#include <stdio.h>

int list_bsearch(sort_func_t *sf, void *key)
{
  int l,u,idx, res;
  char *base=sf->tosort->d;
  void *p;
  int size=sf->tosort->s;

  if(size==0)return -1;
  if(sf->tosort->q==0)return -1;
  if(base==NULL)return -1;

  sf->aiskey=1;
  l =0;
  u = sf->tosort->q -1 ;
  
  res= sf->cmpf(sf,key,base);
  idx= sf->cmpf(sf,key,(void *)(((const char *)base)+((u-1)*size)));
  if(0)fprintf(stderr,"list_bsearch, cmpf(e 0)=%i cmpf(e %i)=%i  ",res,u-1,idx);
  
  while (l < u)
    {
      idx = (l + u) / 2;
      p = (void *) (((const char *) base) + (idx * size));
      res = sf->cmpf (sf, key, p);
      if (res <= 0) /* if we happen to hit the element, pretend we overshot, we want the first element of key */
        u = idx;
      else if (res > 0)
        l = idx + 1;
     }
  res=-1;
  if(sf->tosort->q>0)
    {
      p = (void *) (((const char *) base) + (l * size));
      res = sf->cmpf (sf,key, p);
      if(0)fprintf(stderr," l=%i u=%i res=%i\n",l,u,res);
      if(res==0)
	return l;
      return -1;
    }
  if(0)fprintf(stderr,"no elements\n");
  return -1;
}


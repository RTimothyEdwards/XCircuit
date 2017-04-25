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
/* list_search.c,  container supporting mulitple sorted searches 
   Conrad Ziesler
*/

#include "debug.h"
#include "list_search.h"

void list_search_init(list_search_t *lp, int size, int mode)
{
  list_init(&(lp->list),size,mode);
  list_init(&(lp->psearch),sizeof(list_psearch_t),LIST_DEFMODE);
}


void list_search_empty(list_search_t *lp)
{
  int i;
  list_psearch_t *p;
  list_empty(&(lp->list));
  list_iterate(&(lp->psearch),i,p)
    {
      if(p->isearchdata!=NULL)free(p->isearchdata);
    }
  list_empty(&(lp->list));
}



int list_search_addrule(list_search_t *lp, int (*cmpf)(const void *user, const void *a, const void *b),void *user)
{
  list_psearch_t t;
  t.isearchdata=NULL;
  t.is_sorted=0;
  t.user=user;
  t.cmpf=cmpf;
  t.len=0;
  return list_add(&(lp->psearch),&t);
}

int list_search_findnext(list_search_iterator_t *sip)
{
  list_psearch_t *psp;
  int id;
  void *pa,*pb;

  id=sip->last_id+1;
  if(sip->sp==NULL)return -1;
  if(sip->last_id<0)return -1;
  if(id>=sip->sp->list.q)return -1; /* last one, can't be any more */
  psp=list_data(&(sip->sp->psearch),sip->which);
  if(psp==NULL)return -1;
  if(!psp->is_sorted)return -1;
  if(psp->len!=sip->sp->list.q) return -1;
  
  pa=list_data(&(sip->sp->list),psp->isearchdata[sip->last_id]);
  pb=list_data(&(sip->sp->list),psp->isearchdata[id]);
  if((pa!=NULL) && (pb!=NULL)) 
    if( psp->cmpf(psp->user,pa,pb)==0) /* yes we match previous */
      {
	sip->last_id=id;
	return psp->isearchdata[id];
      }
  return -1;
}




static list_t *debug_cmpf_list=NULL;
static  int (*debug_psp_cmpf)(const void *user ,const void *a, const void *b);
static void *debug_user;

int debug_cmpf(const void *a, const void *b)
{
  const int *ap=a,*bp=b;
  return debug_psp_cmpf(debug_user,list_vdata(debug_cmpf_list,ap[0]),list_vdata(debug_cmpf_list,bp[0]));
}

void list_search_qsort_debug (list_search_t *lsp, list_psearch_t *psp)
{
  debug_cmpf_list=&(lsp->list);
  debug_psp_cmpf=psp->cmpf;
  debug_user=psp->user;
  qsort(psp->isearchdata,psp->len,sizeof(int),debug_cmpf);
}


int list_search_find(list_search_t *lp, int which, void *key, list_search_iterator_t *sip)
{
  list_psearch_t *psp;
  int i,l,u,idx, res;
  char *list_base=lp->list.d;
  int base_size=lp->list.s;
  int base_qty=lp->list.q;
  int *base;
 
  if(sip!=NULL)
    {
      sip->sp=lp;
      sip->which=which;
      sip->last_id=-1;
    }

  if(base_size==0)return -1;
  if(base_qty<=0)return -1;
  
  
  psp=list_data(&(lp->psearch),which);
  assert(psp!=NULL);
  
  if(psp->len!=base_qty)psp->is_sorted=0; /* guard against changes */
 
  if(!psp->is_sorted)
    {
      if(psp->isearchdata!=NULL)free(psp->isearchdata);
      psp->isearchdata=malloc(sizeof(int)*base_qty);
      assert(psp->isearchdata!=NULL);
      psp->len=base_qty;
      for(i=0;i<base_qty;i++) psp->isearchdata[i]=i;
      list_search_qsort_debug(lp,psp);
      psp->is_sorted=1;
      
    }

  base=psp->isearchdata;
  l =0;
  u = base_qty;

  if(base==NULL)return -1;
  
  while (l < u)
    {
      idx = (l + u) >> 1;
      res= psp->cmpf(psp->user, key, (void *)(list_base+ (base_size*base[idx])));
      if (res <= 0) 
        u = idx;
      else if (res > 0)
        l = idx + 1;
     }
  if(l>=base_qty) return -1;
  if(l<0)return -1;
  res= psp->cmpf(psp->user, key, (void *)(list_base+ (base_size*base[l])));
  if(res==0)
    { if(sip!=NULL)sip->last_id=l;  return base[l]; }
  return -1;
}



void list_search_resort(list_search_t *lp)
{
  int i;
  list_psearch_t *ps;
  list_iterate(&(lp->psearch),i,ps)
    {
      ps->is_sorted=0;
      ps->len=0;
      if(ps->isearchdata!=NULL)free(ps->isearchdata);
    }
}



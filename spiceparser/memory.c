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
/* memory.c,  sequential memory allocator
   Conrad Ziesler
*/  

/*
   these routines are used to allocate char aligned
   objects (namely strings) in such a way that we 
   1. allocate in big chunks
   2. keep track of all the chunk allocations
   3. free a bunch at once without having to 
      remember all the little pointers we once had.

   this allows any module to maintain private string 
   (or equations too) storage space that is leak-proof
*/
#include "debug.h"
#include "memory.h"


static memory_chain_t *memory_newblock(void)
{
  int i;
  memory_chain_t *p;
  p=malloc(sizeof(memory_chain_t));
  p->q=0;
  for(i=0;i<4;i++)p->magic[i]=MEMORY_MAGIC+i;
  p->next=NULL;
  p->qref=0;
  /*assert(p!=((void *)0x80d8008)); */
 return p;
}

void memory_init(memory_t *ma)
{
  memory_t def=MEMORY_INIT;
  *ma=def;
}

void memory_checkblock(memory_chain_t *p)
{
  int i;
  for(i=0;i<4;i++)
    if(p->magic[i]!=MEMORY_MAGIC+i)
      assert(0);
}

void *memory_alloc(memory_t *ma, int size)
{
  memory_chain_t *p,*pp;
  memory_other_t *op;
  
  if(size==0)return NULL;

  /* round up to word alignment, thus all allocations will be aligned */
  size=(size+(sizeof(int)-1))&(~(sizeof(int)-1)); 
  
  /* directly allocate big blocks */
  if(size> (MEMORY_CHUNKSIZE/2))
    { 
      op=malloc(size+sizeof(memory_other_t)+4);
      op->next=ma->others;
      ma->others=op;
      return (op+1);
    }
  
  /* indirectly allocate small blocks */
  while(1)
    {
      for(pp=NULL,p=ma->head;p!=NULL;p=p->next)
	{
	  memory_checkblock(p);
	  if((MEMORY_CHUNKSIZE-p->q)>=size)
	    {
	      void *d=p->data+p->q;
	      p->q+=size;
	      p->qref++;
	      if(p->q> (MEMORY_CHUNKSIZE-MEMORY_THRESHOLD))
		{
		  if(pp==NULL)
		    { 
		      ma->head=p->next;
		      p->next=ma->full;
		      ma->full=p;
		    }
		  else
		    {
		      pp->next=p->next;
		      p->next=ma->full;
		      ma->full=p;
		    }
		}
	      return d;
	    }
	  pp=p;
	}
      
      if(ma->head==NULL) ma->head=	  memory_newblock();
      else
	{
	  for(pp=NULL,p=ma->head;p!=NULL;p=p->next)
	    pp=p;
	  pp->next=memory_newblock();
	}
    }
}



void memory_free(memory_t *ma, void *vp)
{
  memory_chain_t *p,*pp, **hp;
  char *ca,*cb,*c=vp;
  memory_other_t *op,*pop;
  
  for(hp=&ma->head,pp=NULL,p=ma->head;p!=NULL;p=p->next)
    {
      ca=(void*)p;
      cb=(void*) (p+1);
      if((c>=ca)&&(c<cb)) { p->qref--; break; }
      pp=p;
    }
  if(p!=NULL)
    for(hp=&ma->full,pp=NULL,p=ma->full;p!=NULL;p=p->next)
      {
	ca=(void*)p;
	cb=(void*) (p+1);
	if((c>=ca)&&(c<cb)) { p->qref--; break; }
	pp=p;
      }
  if(p!=NULL)
    {
      if(pp==NULL)
	*hp=p->next;
      else
	pp->next=p->next;
      free(p);
      return ;
    }

  for(pop=NULL,op=ma->others;op!=NULL;op=op->next)
    {
      if(op==vp)
	{
	  if(pop==NULL)ma->others=op->next;
	  else pop->next=ma->others;
	  free(op);
	  return;
	}  
      pop=op;
    }
}

void memory_freeall(memory_t *ma)
{
  memory_chain_t *p,*pp;
  memory_other_t *op,*pop;
  
  for(pp=NULL,p=ma->head;p!=NULL;p=p->next)
    {
      if(pp!=NULL)
	{
	  memory_checkblock(pp);
	  free(pp);
	}
      pp=p;
    }
  
  if(pp!=NULL)
    {
      memory_checkblock(pp);
      free(pp);
    }
  ma->head=NULL;

  for(pp=NULL,p=ma->full;p!=NULL;p=p->next)
    {
      if(pp!=NULL)
	{
	  memory_checkblock(pp);
	  free(pp);
	}
      pp=p;
    }

  if(pp!=NULL)
    {
      memory_checkblock(pp);
      free(pp);
    }
  ma->full=NULL;
  for(pop=NULL,op=ma->others;op!=NULL;op=op->next)
    {
      if(pop!=NULL)
	  free(pop);
      pop=op;
    }
  if(pop!=NULL)
    {
      free(pop);
    }
  ma->others=NULL;
}



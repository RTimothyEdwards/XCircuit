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
/* hash.c :  Hash table functions for string identifier lookup 
   Conrad Ziesler
*/


#include "debug.h"
#include "hash.h"

 

unsigned int hash_strhash(int max, void *data, int size)
{
  int i;
  unsigned int r=0;
  char *str=data;
  
  for(i=0;i<size;i++)
    {
      if(str[0]==0)break;    
      r+=((unsigned int)(str[0]&0x7f));
      r%=max;
      str++;
    }

  assert(r<max);
  assert(r>=0);
  return r;
}


#define quickhashbin(a,b,c) hash_binhash(a,b,c)

unsigned int hash_binhash(int max, void *data, int size)
{
  unsigned int r=0;
  int i;
  uchar *str=data;
  
  for(i=0;i<size;i++)
    {
      r+=((unsigned int)(str[0]&0x7f));
      r%=max;
      str++;
    }

  assert(r<max);
  assert(r>=0);
  return r;
}

static hashbin_t *hash_newbin(void *copyfrom, int size)
{
  hashbin_t *p;
  
  p=malloc(size+sizeof(hashbin_t));
  assert(p!=NULL);
  
  p->size=size;
  p->next=NULL;
  memcpy(hash_bin2user(p),copyfrom,size);
  return p;
}


static hashbin_t *hash_insert(hash_t *h, unsigned int index, hashbin_t *cp)
{
  cp->next=h->bins[index];
  h->bins[index]=cp;
  return cp;
}

#ifdef UNUSED
static hashbin_t *hash_insertsorted(hash_t *h, hashbin_t *head, hashbin_t *cp)
{
  hashbin_t *a,*b=NULL;
  
  for(a=head;a!=NULL;a=a->next)
    {
      if(h->hashcmp(a,cp)>=0)
	{
	  if(b==NULL)
	    {
	      cp->next=head;
	      return cp;
	    }
	  else
	    {
	      cp->next=a;
	      b->next=cp;
	      return head;
	    }
	}
      b=a;
    }
  if(b==NULL)
    {
      cp->next=NULL;
      return cp;
    }
  else
    {
      cp->next=NULL;
      b->next=cp;
      return head;
    }
}
#endif

static hashbin_t *hash_findbin(hash_t *h, hashbin_t *start, int ksize, void *key)
{
  while(start!=NULL)
    {
      if(h->hashcmp(ksize,key,start->size,start+1)==0)return start;
      start=start->next;
    }
  return NULL;
}



/******* public functions ********/
void hash_free (hash_t *h)
{
  int i;
  hashbin_t *p,*np;

  if(h!=NULL)
    {
      for(i=0;i<h->qalloc;i++)
	if(h->alloc[i]!=NULL)free(h->alloc[i]);
      for(i=0;i<h->qbin;i++)
	{
	  for(p=h->bins[i];p!=NULL;p=np)
	    {
	      np=p->next;
	      free(p);
	    }
	}
      free(h);
    }
}


hash_t *hash_new    (
		     int qbins, 
		     uint (*hashfunc)(int max, void *data, int size),
		     int (*hashcmp)(int sizea, void *a, int sizeb, void *b)
		     ) 
{
  hash_t *p;
  int i;
  if(qbins<=0)qbins=4;
  p=malloc(sizeof(hash_t)+(sizeof(hashbin_t *)*qbins) );
  assert(p!=NULL);
  p->bins=(void *) (((char *)p)+sizeof(hash_t));
  p->uniq=0;
  p->qbin=qbins;
  p->hashfunc=hashfunc;
  p->hashcmp=hashcmp;
  for(i=0;i<qbins;i++)
    p->bins[i]=NULL;
  p->qalloc=0;
  p->alloc=NULL;
  return p;
}


void *hash_find(hash_t *h, void *key, int ksize)
{
  unsigned int ukey;
  hashbin_t *fbin;
  
  assert(h!=NULL);
  ukey=h->hashfunc(h->qbin,key,ksize);
  assert(ukey<h->qbin);
  
  fbin=hash_findbin(h,h->bins[ukey],ksize,key);

  if(fbin==NULL)return NULL;
  return hash_bin2user(fbin);
}

int hash_size(void *user)
{
  hashbin_t *bin;
  if(user==NULL)return 0;
  bin=hash_user2bin(user);
  return bin->size;
}

void *hash_add(hash_t *h, void *key, int ksize)
{
  unsigned int ukey;
  hashbin_t *fbin, *dbin;

  
  assert(h!=NULL);
  ukey=h->hashfunc(h->qbin,key,ksize);
  assert(ukey<h->qbin);
  
  fbin=hash_findbin(h,h->bins[ukey],ksize,key);
  if(fbin==NULL)
    {
      dbin=hash_newbin(key,ksize);
      fbin=hash_insert(h,ukey,dbin);
    }  
  if(fbin!=NULL)
    return hash_bin2user(fbin);
  else return NULL;
}	
	

uint hclient_ptrflagstr_hash(int max, void *data, int size)
{
  return hash_strhash(max, ((char *)data) + sizeof(hclient_ptrflagstr_t) ,size-sizeof(hclient_ptrflagstr_t)); 
}

int  hclient_ptrflagstr_cmp(hashbin_t *a, hashbin_t *b)
{
  if(a==NULL)return 1;
  if(b==NULL)return -1;
  if(a->size > b->size) return 1;
  if(a->size < b->size) return -1; 
  return strcmp( ((char *) hash_bin2user(a) ) + sizeof(hclient_ptrflagstr_t), ((char *) hash_bin2user(b) ) + sizeof(hclient_ptrflagstr_t) );

}


/* binary object 
   straight binary comparison and hash
*/

uint hclient_bobject_hash(int max, void *data, int size)
{
  return quickhashbin(max,data,size);
}

int  hclient_bobject_cmp(int sizea, void *a, int sizeb, void *b)
{
  if(a==NULL)return 1;
  if(b==NULL)return -1;
  if(sizea > sizeb) return 1;
  if(sizea < sizeb) return -1; 
  return memcmp(a,b,sizea);
}



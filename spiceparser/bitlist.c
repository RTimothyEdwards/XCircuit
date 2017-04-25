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
/* bitlist.c, routines for bit arrays 

Conrad Ziesler

*/

#include "debug.h"
#include "bitlist.h"




bitlist_t * bitlist_new(int qty)
{
  bitlist_t *bl;
  int qbytes;
  qbytes=(qty>>3)+1;
  
  bl=  malloc(sizeof(bitlist_t)+qbytes);
  assert(bl!=NULL);
  memset(bl,0,sizeof(bitlist_t)+qbytes);
  bl->qty=qty;
  bl->qbytes=qbytes;

  return bl;
}

void bitlist_free(bitlist_t *bl)
{
  if(bl!=NULL)
    free(bl);
}

void bitlist_setall(bitlist_t *bl)
{
  memset(bl->data,0xff,bl->qbytes);
}

void bitlist_clearall(bitlist_t *bl)
{
  memset(bl->data,0,bl->qbytes);
}


int bitlist_set(bitlist_t *bl, int index)
{
  int byte,bit,r=0;
  if(index<0) return 0;
  byte=index>>3;
  bit=index-(byte<<3);
  assert(byte<bl->qbytes);
  if((bl->data[byte]&(1<<bit))!=0)r=1;
  bl->data[byte]|=(1<<bit);
  return r;
}

int bitlist_test(bitlist_t *bl, int index)
{
  int byte,bit;
  if(index<0) return 0;
  byte=index>>3;
  bit=index-(byte<<3);
  assert(byte<bl->qbytes);
  if((bl->data[byte]&(1<<bit))!=0)return 1;
  return 0;
}


int bitlist_clear(bitlist_t *bl, int index)
{
  int byte,bit,r=0;
  if(index<0) return 0;
  byte=index>>3;
  bit=index-(byte<<3);
  assert(byte<bl->qbytes);
  if((bl->data[byte]&(1<<bit))!=0)r=1;
  bl->data[byte]&=~(1<<bit);
  return 0;
}

/* realloc bitlist */
bitlist_t *bitlist_resize(bitlist_t *bl, int qty)
{
  int oldqty;
  int qbytes;
  qbytes=(qty>>3)+1;
  oldqty=bl->qty;

  bl=realloc(bl,sizeof(bitlist_t)+qbytes);
  assert(bl!=NULL);
  if( (oldqty>>3) < (qbytes-(oldqty>>3)) )
    memset(bl->data+(oldqty>>3),0,qbytes-(oldqty>>3));
  
  bl->qty=qty;
  bl->qbytes=qbytes;
  return bl;
}


/* returns the index of the first bit set or cleared
   from dir
*/

int bitlist_scan(bitlist_t *bl, int flags)
{
  int i,j;
  assert(bl!=NULL);
  switch(flags)
    {
    case (SCAN_FORWARD|SCAN_SET):
      for(i=0;i<(bl->qbytes-1);i++)
	{
	  if(bl->data[i]!=0)
	    {
	      for(j=(i<<3);j<bl->qty;j++)
		if(bitlist_test(bl,j))return j;
	    }
	}
      break;
    case (SCAN_FORWARD|SCAN_CLEAR):
      for(i=0;i<(bl->qbytes-1);i++)
	{
	  if( (bl->data[i])< ((unsigned char)0x00ff))
	    {
	      
	      for(j=(i<<3);j<bl->qty;j++)
		if(!bitlist_test(bl,j))return j;
	    }
	}
      break;

    default:
      assert(0);
    }
  return -1;
}



int bitlist_copyhead(bitlist_t *dest, bitlist_t *src, int length)
{
  int i;
  int finalbyte;
  int finalbitoffset;
  unsigned char a,b;
  if(length>src->qty)length=src->qty;
  if(length>dest->qty)length=dest->qty;
  
  finalbyte=length>>3;
  finalbitoffset=length-(finalbyte<<3);
  
  for(i=0;i<finalbyte;i++)
    dest->data[i]=src->data[i];

  a=dest->data[finalbyte];
  b=src->data[finalbyte];
  
  for(i=0;i<finalbitoffset;i++)
    a= (a & (~(1<<i)) ) | ((1<<i)&b) ;

  dest->data[finalbyte]=a;
  return length;
}

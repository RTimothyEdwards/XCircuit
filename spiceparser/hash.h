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
/* hash.h, hash function headers
   Conrad Ziesler

*/

#ifndef __HASH_H__
#define __HASH_H__

#ifndef UCHAR
typedef unsigned char uchar;
#define UCHAR
#endif


#ifndef 	_SYS_TYPES_H	
typedef unsigned int uint; 
#endif


/**********************************/
/* HASH TABLES ********************/

typedef struct hashbin_st
{
  struct hashbin_st *next;
  int size;
  /* alloc in place data here */
}hashbin_t;

typedef struct hash_st
{
  int uniq;
  uint (*hashfunc)(int max, void *data, int size);
  int  (*hashcmp)(int sa, void *a, int sb, void *b);
  int qbin;  
  hashbin_t **bins; 
  int   qalloc;
  void **alloc;
}hash_t;

#define hash_bin2user(p)   ((void *)(((char *)p)+sizeof(hashbin_t)))
#define hash_user2bin(p)   ((hashbin_t *)((void *)(((char *)p)-sizeof(hashbin_t))))
#define hash_forall(h,i,p)  for((i)=0;(i)<(h)->qbin;(i)++) for((p)=(h)->bins[(i)];(p)!=NULL;(p)=(p)->next)
#define hash_malloc(h,s) hash_alloc((h),malloc((s)))


hash_t *hash_new    (
		     int qbins, 
		     uint (*hashfunc)(int max, void *data, int size),
		     int (*hashcmp)(int sizea, void *a, int sizeb, void *b)
		     ) ;

void    hash_free   (hash_t *h);  /* free hash table and associated memory */
void   *hash_find   (hash_t *h, void *key, int ksize); /* find key in table */
void   *hash_add    (hash_t *h, void *key, int ksize); /* add key of size to table */
void   *hash_alloc  (hash_t *h, void *data);  /* add data to list of allocs */
int     hash_size   (void *user); /* just return size of associated bin */

/* ----- some predefined hash clients  --------- */

unsigned int hash_strhash(int max, void *data, int size);
unsigned int hash_binhash(int max, void *data, int size);

/* ptr,flag, in place string 
   do strcmp on strings, hash on the string only
   store a payload of a ptr and a flag
*/

typedef struct hash_ptrflagstr_st
{
  void *ptr;
  char flag;
  /* inplace string to follow */
}hclient_ptrflagstr_t;

uint hclient_ptrflagstr_hash(int max, void *data, int size);



/* binary object 
   straight binary comparison and hash
*/

uint hclient_bobject_hash(int max, void *data, int size);
int  hclient_bobject_cmp(int sizea, void *a, int sizeb, void *b);



#endif


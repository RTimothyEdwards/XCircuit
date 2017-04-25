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
/* spice.c:  read hierachical spice deck composed of M and C and X
             
  Conrad Ziesler
*/


#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include "debug.h"

/* The following is copied from xcircuit.h */
#ifdef TCL_WRAPPER
  #define Fprintf tcl_printf
  #define Flush tcl_stdflush
#else
  #define Fprintf fprintf
  #define Flush fflush
#endif


int __debug_spice__=2;

#ifndef STRIP_DEBUGGING
#define D(level,a) do { if(__debug_spice__>(level)) a; } while(0)
#else
#define D(level,a) 
#endif

/* spice deck

.SUBCKT CKT1  3  4  5 
M0  6  3  1  1  NMOS W=1.025U L=0.350U AD=0.0 AS=0.0 PD=0.0 PS=0.0 
M1  7  4  6  1  NMOS W=1.025U L=0.350U AD=0.0 AS=0.0 PD=0.0 PS=0.0

C0  3  1  1.92627e-15 
C1  4  1  1.8152e-15 

.ENDS

*comment

X1464  3  583  589  CKT6 
*/

#define __NETLIST_SPICE_C__ 1


#ifndef __NETLIST_H__
#include "netlist_dev.h"
#endif



#ifndef __NETLIST_SPICE_H__
#include "netlist_spice.h"
#endif

tokenmap_t spice_tokens[]=SPICE_TOKENS;

/*******  Hashing Clients *******/

uint paramclient_hash(int max, void *data, int size)
{
  return hash_strhash(max, ((char *)data)+sizeof(paramload_t), size-sizeof(paramload_t));
}


int  paramclient_cmp(int sizea, void *a, int sizeb, void *b)
{
  if(a==NULL)return 1;
  if(b==NULL)return -1;
  if(sizea > sizeb) return 1;
  if(sizea < sizeb) return -1; 
  return memcmp( ((char *) a) + sizeof(paramload_t), ((char *)b ) + sizeof(paramload_t),sizeb-sizeof(paramload_t) );
}

static callparam_t *new_callparam(char *str, eqn_t eqn)
{
 callparam_t *pc;
 pc=malloc(sizeof(callparam_t)+strlen(str));
 assert(pc!=NULL);
 strcpy(pc->str,str);
 pc->eqn=eqn;
 return pc;
}


static paramclient_t *add_param(hash_t *h, char *str, eqn_t eqn)
{
  int l;
  char data[4096];
  paramclient_t *pc;
  pc=(void *)data;
  l=strlen(str)+1;
  if(l>(sizeof(data)-sizeof(paramclient_t)))
    l=sizeof(data)-sizeof(paramclient_t);
  pc->payload.eqn=eqn;
  pc->payload.patch_call=NULL;
  pc->payload.global_i=-1;
  memcpy(pc->str,str,l);
  return hash_add(h,pc,l+sizeof(paramload_t));
}

uint nodeclient_hash(int max, void *data, int size)
{
  return hash_strhash(max, ((char *)data)+sizeof(hashload_t), size-sizeof(hashload_t));
}

int  nodeclient_cmp(int sizea, void *a, int sizeb, void *b)
{
  if(a==NULL)return 1;
  if(b==NULL)return -1;
  if(sizea > sizeb) return 1;
  if(sizea < sizeb) return -1; 
  return memcmp( ((char *) a) + sizeof(hashload_t), ((char *)b ) + sizeof(hashload_t), sizea-sizeof(hashload_t) );
}

/* some helper functions for hash clients */

static nodeclient_t *find_hashtab(hash_t *tab, uchar *str)
{
  int l;
  l=strlen(str)+1;
  return hash_find(tab,str-sizeof(hashload_t),sizeof(hashload_t)+l);
}

subckt_t *spice_find_subckt(subckt_t *parent, char *name)
{
  nodeclient_t *tc;
  tc=find_hashtab(parent->cktdir,(uchar *)name);
  return tc->payload.data.subckt;
}


list_t spice_list_subckt(subckt_t *parent)
{
  list_t foo;
  subckt_t *ckt;
  int i;
  hash_t *tab;
  hashbin_t *bp;
  node_t p;

  list_init(&foo,sizeof(subckt_t *),LIST_EXPMODE);
  if(parent==NULL)return foo;

  tab=parent->cktdir;  
  FORALL_HASH(tab,i,bp)  
    {
      p=hash_bin2user(bp);
      ckt=p->payload.data.subckt;
      list_add (&foo,&ckt);
    }
  return foo;
}


static nodeclient_t *add_hashtab(hash_t *tab, uchar *str, hashload_t payload)
{
  int l;
  uchar *strp=str;
  nodeclient_t *cp;
  static unsigned data[16000];
  nodeclient_t *nd=(void *)data;
  assert(tab!=NULL);  
  nd->payload=payload;
  l=0;
  while(strp[0]!=0)
    {
      nd->str[l]=strp[0];
      strp++; l++;
    }
  nd->str[l]=0;
  l++;
  assert(l< (16000-sizeof(nodeclient_t)));
  if(tab->hashfunc!=nodeclient_hash)assert(0);
  cp=hash_add(tab,nd,(sizeof(hashload_t)+l));
  D(20,Fprintf(stderr,"add_hashtab(%p,%s,..) cp=%p \n",tab,str,cp));  
  return cp;
}



#define warning(d,c,s)  do { scan->errdeck=d; scan->errcard=c; parse_warn(scan,s);  } while(0)
#define error(d,c,s)    do { scan->errdeck=d; scan->errcard=c; parse_error(scan,s); } while(0)
#define checkvalid(d,q)  scanner_checkvalid(d,q)

static eqn_t parseval(deck_t *d, card_t *c, uchar *val)
{
  uchar *ps;
  eqn_t eqn;
  eqn=eqn_empty();
  
  if(val==NULL)return eqn;
  
  for(ps=val;ps[0]!=0;ps++)
    if(!isspace(ps[0]))break;
  
  if(ps[0]=='\'') ps++;  /* hack */
  
  eqn=eqn_parse(ps);

  return eqn;
}



/************************************************************************/
/*******  PASS 2 :  create subcircuit definitions  (for forward ref) ****/
/*******            and count devices (for memory alloc)             ****/
/*******            fill in all devices and nodes (except x subckt)  ****/
/************************************************************************/



static void skip_subckt(deck_t **thedeckpp)
{
  int i;
  deck_t *dp=*thedeckpp;
  i=0;
  
  while(dp!=NULL)
    {
      if(dp->card!=NULL)
	{
	  if(dp->card->token==Kends)i--; 
	  if(dp->card->token==Ksubckt)i++;
	}
      if(i==0)break;      
      dp=dp->next;
    }
  *thedeckpp=dp;
}

static subckt_t *new_subckt(deck_t **thedeckpp, subckt_t *parent, scanner_t *scan)
{
  int i, qtynodes, qtyparams, qtycallparams, qtylparams;
  int qtym, qtyc, qtyx, qtyr, qtyl, qtyv, qtyi;
  int qtysubckt, qtyglobal;
  subckt_t *scktp;
  card_t *cp;
  deck_t *dp,*d=*thedeckpp,*dplast=NULL;
  int size; 
  void *p;
  uchar *name;
  hashload_t payload={ PAYLOAD_null, {NULL} };

  qtycallparams=0;
  qtynodes=0;
  qtyparams=0;
  qtylparams=0;
  qtysubckt=0;
  qtyglobal=0;

  if(!checkvalid(d,2))error(d,d->card,"invalid subckt definition");
  name=d->card->next->str;

  i = 0;
  for(cp=d->card->next->next; cp!=NULL; cp=cp->next)
    { 
      if(cp->val==NULL) qtynodes++; 
      else qtylparams++;
      i++; 
    }

  qtym = 0;
  qtyc = 0;
  qtyr = 0;
  qtyl = 0;
  qtyx = 0;
  qtyv = 0;
  qtyi = 0;

  for(dp=d; dp!=NULL; dp=dp->next)
    {
      uchar a;
      dplast=dp;
      if(dp->card==NULL)continue;
      
      if( (dp->card->token==Ksubckt) && (dp!=d))
	{ skip_subckt(&dp); qtysubckt++; continue; } /* consume .ends token */
      
      if(dp==NULL)break;
      if(dp->card==NULL)continue;

      if(dp->card->token==Kend)break;
      if(dp->card->token==Kends)break;

      /*
	if(parent!=NULL)
	{ if(dp->card->token==Kends)break; }
	else { if(dp->card->token==Kend)break;  }
      */

      a=dp->card->str[0];
      if(a=='x') 
	{ 
	  qtyx++;
	  for(cp=dp->card->next;cp!=NULL;cp=cp->next)
	    if(cp->val!=NULL)qtycallparams++;
	}

      if(a=='m') qtym++;
      if(a=='c') qtyc++;
      if(a=='r') qtyr++;
      if(a=='l') qtyl++;
      if(a=='v') qtyv++;
      if(a=='i') qtyi++;

      if(dp->card->token==Kparam)
	{ 
	  for(cp=dp->card->next;cp!=NULL;cp=cp->next)
	    qtyparams++;
	}
      if(dp->card->token==Kglobal)
	{ 
	  for(cp=dp->card->next;cp!=NULL;cp=cp->next)
	    qtyglobal++;
	}
    }
  
  if(dp==NULL) {
    if(parent==NULL)
      {
	/*
	uchar *p,buf[64]=".END  ";
	D(50,Fprintf(stderr,"ignoring lack of .END token\n"));
	assert(dplast!=NULL);
	assert(dplast->next==NULL);
	dp=dplast->next=new_deck();
	dp->next=NULL;
	p=buf;
	dp->card=make_card(&p,PARSE_CASE_TOLOWER);
	dp->card->token=Kend;
	*/
      }
    else
      {
	if(parent==NULL)error(d,NULL,"No .END found"); 
	else error(d,NULL,"No .ENDS found"); 
      }
  }

  D(10, Fprintf(stderr,"Got %i subckts, %i x's %i m's %i c's %i r's "
		"%i l %i v %i i's\n",
		qtysubckt, qtyx, qtym, qtyc, qtyr, qtyl, qtyv, qtyi));

  size=
    sizeof(subckt_t) + 
    (sizeof(node_t)*qtynodes) +
    (sizeof(m_t)*qtym) +
    (sizeof(x_t)*qtyx) +
    (sizeof(c_t)*qtyc) + 
    (sizeof(r_t)*qtyr) +
    (sizeof(l_t)*qtyl) + 
    (sizeof(v_t)*qtyv) + 
    (sizeof(i_t)*qtyi) + 
    (strlen(name) + 2);

  scktp=p=malloc(size+1024); /* 1024 is debug pad */
  
  assert(scktp!=NULL);
  memset(scktp,0,size);
  memset(((char *)scktp)+size,'a',1024);
  p= ((char *)p+sizeof(subckt_t));
  
  scktp->defn=p;
  scktp->ndefn=qtynodes; 
  p= ((char *)p+(sizeof(node_t)*qtynodes));
  
  scktp->m=p;
  scktp->nm=qtym;  
  p= ((char *)p+(sizeof(m_t)*qtym));

  scktp->c=p;
  scktp->nc=qtyc;
  p= ((char *)p+(sizeof(c_t)*qtyc));

  scktp->r=p;
  scktp->nr=qtyr;
  p= ((char *)p+(sizeof(r_t)*qtyr));

  scktp->l=p;
  scktp->nl=qtyl;
  p= ((char *)p+(sizeof(l_t)*qtyl));

  scktp->v=p;
  scktp->nv=qtyv;  
  p= ((char *)p+(sizeof(v_t)*qtyv));

  scktp->i=p;
  scktp->ni=qtyi;  
  p= ((char *)p+(sizeof(i_t)*qtyi));

  scktp->x=p;
  scktp->nx=qtyx;
  p= ((char *)p+(sizeof(x_t)*qtyx));

  scktp->name=p;
  strcpy(scktp->name,name);
  p= ((char *)p+strlen(name)+2);


  scktp->scale=1.0;
  scktp->mult=1.0;
  scktp->subckt_magic=SUBCKT_MAGIC;
  assert((((char *)p)-((char *)scktp))==size); /* sanity check */
  
  /* estimate good size for hashtable */
  scktp->nodes=hash_new((qtym*3)+(qtyx*4),nodeclient_hash,nodeclient_cmp);

  scktp->flag=0;
  scktp->params=hash_new(qtyparams,paramclient_hash,paramclient_cmp);
  scktp->lparams=hash_new(qtylparams,paramclient_hash,paramclient_cmp);
  scktp->parent=parent;
  scktp->cktdir=hash_new(qtysubckt,nodeclient_hash,nodeclient_cmp);
  scktp->global=hash_new(qtyglobal,nodeclient_hash,nodeclient_cmp);

  /* store defn nodes, and ptrs */
  for(i=0,cp=d->card->next->next;cp!=NULL;cp=cp->next)
    { 
      if(cp->val==NULL)  /* skip params on line */
	{
	  if(find_hashtab(scktp->nodes,cp->str)!=NULL)
	    warning(d,cp,"duplicate node in subckt definition");
	  
	  payload.flag=PAYLOAD_null;
	  payload.data.p=NULL;
	  scktp->defn[i]=add_hashtab(scktp->nodes,cp->str,payload); 
	  i++;
	}
    }
    
  /* do subckt params (those on the .subckt line) */
  
  for(cp=d->card->next->next;cp!=NULL;cp=cp->next)
    { 
      if(cp->val!=NULL) /* skip nodes on line */
	add_param(scktp->lparams,cp->str,parseval(d,cp,cp->val));
    }
  
  /* do spice deck */
  
  /* reinitialize device counters */
  qtyr = 0;
  qtyl = 0;
  qtym = 0;
  qtyc = 0;
  qtyx = 0;
  qtyv = 0;
  qtyi = 0;

  for (dp = d; dp != NULL; dp = dp->next)
    {
      uchar a;
      if (dp->card == NULL) continue;
      
      if ((dp->card->token == Ksubckt) && (dp != d))
	{
	  subckt_t *child=NULL;
	  child=new_subckt(&dp,scktp,scan);
	  payload.data.subckt=child;
	  payload.flag=PAYLOAD_subckt;
	  add_hashtab(scktp->cktdir,child->name,payload);	  
	  continue;
	}
      if(dp==NULL)break;
      if(dp->card==NULL)continue;
      if(parent!=NULL)
	   { if(dp->card->token==Kends)break; }
      else { if(dp->card->token==Kend)break;  }
      
      a=dp->card->str[0];
      if(a=='x') 
	{
	  int k=0;
	  x_t *xp;
	  card_t *last;
	
	  assert(qtyx<scktp->nx);
	  xp=&(scktp->x[qtyx]);
	  xp->nn=0;
	  xp->nl=0;
	  xp->nodes=NULL;
	  xp->locals=NULL;
	  
	  if(!checkvalid(dp,2))error(d,NULL,"invalid subckt call");
	  for(i=0,last=cp=dp->card->next;cp!=NULL;cp=cp->next,i++)
	    { 
	      if(cp->val!=NULL)break; last=cp; 
	    }
	  
	  if(cp!=NULL)
	    {
	      for(k=0;cp!=NULL;cp=cp->next,k++);
	    }
	  else k=0;

	  xp->deck=dp;
	  xp->rest=last;
	  xp->nodes=malloc((sizeof(node_t)*i)+(sizeof(paramclient_t*)*k));
	  if(k==0)
	    xp->locals=NULL;
	  else
	    xp->locals=(void *)((char *)(xp->nodes))+(sizeof(node_t)*(i));
	  xp->nn=i-1;
	  xp->nl=k;
	  assert(xp->nodes!=NULL);	  
	  
	  payload.flag=PAYLOAD_null;
	  payload.data.s=NULL;
	  
	  for(i=0,cp=dp->card->next;(cp!=NULL)&&(cp!=last);cp=cp->next,i++)
	   {
	     xp->nodes[i]=add_hashtab(scktp->nodes,cp->str,payload);
	     if(0)Fprintf(stderr,"X:%s  adding node %p cp->str=%s\n",dp->card->str,xp->nodes[i],cp->str);
	   }
	  if(cp!=NULL)
	    {
	    for(k=0,cp=cp->next;(cp!=NULL);cp=cp->next,k++)
	      if(cp->val!=NULL)
		{ xp->locals[k]=new_callparam(cp->str,parseval(dp,cp,cp->val)); }
	      else xp->locals[k]=NULL;
	    }
	  qtyx++;
	}
      if(a=='m') /* Mident d g s b model L=val W=val other_params... */
	{
	  m_t *mp;
	  if(!checkvalid(dp,8))error(d,NULL,"invalid mosfet card");
	  mp=&(scktp->m[qtym]);
	  cp=dp->card->next;

	  payload.flag=PAYLOAD_null;
	  payload.data.s=NULL;
	  
	  for(i=0;i<4;i++)
	    {
	      mp->nodes[i]=add_hashtab(scktp->nodes,cp->str,payload);
	      cp=cp->next;
	    }
	  mp->type=0;
	  /* hope this is n or p */
	  if((cp->str[0]=='n')||(cp->str[0]=='p'))
	    mp->type=cp->str[0];
	  else
	    {
	      uchar *p;
	      for(p=cp->str;p[0]!=0;p++) 
		if((p[0]=='n')||(p[0]=='p'))
		  mp->type=p[0];
	    }
	  if(mp->type==0)
	    {
	      Fprintf(stderr,"Couldn't determine fet type from model name %s\n",cp->str);
	    }

	  mp->rest=cp;
	  mp->deck=dp;
	  cp=cp->next;

	  mp->l =eqn_const(1);   mp->w=eqn_const(1);
	  mp->as=eqn_const(-1); mp->ad=eqn_const(-1); 
	  mp->ps=eqn_const(-1); mp->pd=eqn_const(-1);

	  for(i=0;cp!=NULL;cp=cp->next,i++)
	    {
	      if( (cp->str[0]=='l') && (cp->str[1]==0)) mp->l=parseval(dp,cp,cp->val);
	      if( (cp->str[0]=='w') && (cp->str[1]==0)) mp->w=parseval(dp,cp,cp->val);
	      if( (cp->str[0]=='a') && (cp->str[1]=='s') && (cp->str[2]==0) ) 
		mp->as=parseval(dp,cp,cp->val);
	      if( (cp->str[0]=='a') && (cp->str[1]=='d') && (cp->str[2]==0) ) 
		mp->ad=parseval(dp,cp,cp->val);
	      if( (cp->str[0]=='p') && (cp->str[1]=='s') && (cp->str[2]==0) ) 
		mp->ps=parseval(dp,cp,cp->val);
	      if( (cp->str[0]=='p') && (cp->str[1]=='d') && (cp->str[2]==0) ) 
		mp->pd=parseval(dp,cp,cp->val);
	    }
	  qtym++;
	}
      if(a=='c')  /* Cident node0 node1 value */ 
	{
	  if(!checkvalid(dp,4))error(d,NULL,"invalid capacitor card");

	  payload.flag=PAYLOAD_null;
	  payload.data.p=NULL;
	  scktp->c[qtyc].nodes[0]=add_hashtab(scktp->nodes,dp->card->next->str,payload);
	  scktp->c[qtyc].nodes[1]=add_hashtab(scktp->nodes,dp->card->next->next->str,payload);
	  cp=dp->card->next->next->next;
	  scktp->c[qtyc].deck=dp;
	  scktp->c[qtyc].c=parseval(dp,cp,cp->str); 
	  scktp->c[qtyc].rest=cp;
	  qtyc++;
	}

      if (a == 'r')  /* Rident node0 node1 value */ 
	{
	  if (!checkvalid(dp,4))error(d,NULL,"invalid resistor card");

	  payload.flag=PAYLOAD_null;
	  payload.data.p=NULL;
	  scktp->r[qtyr].nodes[0] =
			add_hashtab(scktp->nodes,dp->card->next->str,payload);
	  scktp->r[qtyr].nodes[1] =
			add_hashtab(scktp->nodes,dp->card->next->next->str, payload);
	  cp=dp->card->next->next->next;
	  scktp->r[qtyr].deck=dp;
	  scktp->r[qtyr].r=parseval(dp,cp,cp->str); 
	  scktp->r[qtyr].rest=cp;
	  qtyr++;
	}

      /* To be done:  AC, pulse, and PWL sources */

      if (a == 'v')  /* Vident node0 node1 value */ 
	{
	  if (!checkvalid(dp,4))error(d,NULL,"invalid voltage source card");

	  payload.flag=PAYLOAD_null;
	  payload.data.p=NULL;
	  scktp->v[qtyv].nodes[0] =
			add_hashtab(scktp->nodes,dp->card->next->str,payload);
	  scktp->v[qtyv].nodes[1] =
			add_hashtab(scktp->nodes,dp->card->next->next->str, payload);
	  cp=dp->card->next->next->next;
	  scktp->v[qtyv].deck=dp;
	  scktp->v[qtyv].v=parseval(dp,cp,cp->str); 
	  scktp->v[qtyv].rest=cp;
	  qtyv++;
	}

      if (a == 'i')  /* Iident node0 node1 value */ 
	{
	  if (!checkvalid(dp,4))error(d,NULL,"invalid current source card");

	  payload.flag=PAYLOAD_null;
	  payload.data.p=NULL;
	  scktp->i[qtyi].nodes[0] =
			add_hashtab(scktp->nodes,dp->card->next->str,payload);
	  scktp->i[qtyi].nodes[1] =
			add_hashtab(scktp->nodes,dp->card->next->next->str, payload);
	  cp=dp->card->next->next->next;
	  scktp->i[qtyi].deck=dp;
	  scktp->i[qtyi].i=parseval(dp,cp,cp->str); 
	  scktp->i[qtyi].rest=cp;
	  qtyi++;
	}

      if (a == 'l')  /* Lident node0 node1 value */ 
	{
	  if (!checkvalid(dp,4))error(d,NULL,"invalid inductor card");

	  payload.flag=PAYLOAD_null;
	  payload.data.p=NULL;
	  scktp->l[qtyl].nodes[0] =
			add_hashtab(scktp->nodes,dp->card->next->str,payload);
	  scktp->l[qtyl].nodes[1] =
			add_hashtab(scktp->nodes,dp->card->next->next->str,payload);
	  cp=dp->card->next->next->next;
	  scktp->l[qtyl].deck=dp;
	  scktp->l[qtyl].l=parseval(dp,cp,cp->str); 
	  scktp->l[qtyl].rest=cp;
	  qtyl++;
	}

      if(dp->card->token==Kparam)
	for(cp=dp->card->next;cp!=NULL;cp=cp->next)
	  { 
	    if(cp->val==NULL)error(dp,cp,"couldn't find = in .param statement"); 	    
	    add_param(scktp->params,cp->str,parseval(dp,cp,cp->val));
	    D(10, Fprintf(stderr, "spice: adding param %s to scktp->params=%p \n",
			cp->str, scktp->params));
	  }

      if(dp->card->token==Kscale)
	{
	  if(dp->card->val!=NULL)
	    scktp->scale=parse_float(dp->card->val);
	  else error(dp,dp->card,"need a value for .scale ");
	}

      if(dp->card->token==Kmult)
	{
	  if(dp->card->val!=NULL)
	    scktp->mult=parse_float(dp->card->val);
	  else error(dp,dp->card,"need a value for .scale ");
	}
      
      payload.flag=PAYLOAD_null;
      payload.data.p=NULL;
      
      if(dp->card->token==Kglobal)
	for(cp=dp->card->next;cp!=NULL;cp=cp->next)
	  add_hashtab(scktp->global,cp->str,payload);

      if(dp->next==NULL) *thedeckpp=dp; /* ignore lack of end token */
    }
  if(dp!=NULL)
    *thedeckpp=dp; /*dp->next;*/ /* skip .end token if present */
  
  return scktp;
}


static void do_subckts(subckt_t *ckt, scanner_t *scan)
{
  x_t *xp;
  nodeclient_t *hcp;
  int i;

  if(ckt==NULL)return;
  if(ckt->flag)return;
  ckt->flag=1;

  for(i=0;i<ckt->nx;i++)
    {
      uchar *nm=NULL;
      xp=ckt->x+i;
      hcp=NULL;
      if(xp->rest!=NULL)
	{
	  subckt_t *search;
	  nm=xp->rest->str;
	  for(search=ckt;search!=NULL;search=search->parent)
	    if( (hcp=find_hashtab(search->cktdir,nm)) != NULL) break;
	}
      if(hcp==NULL)
	{ Fprintf(stderr,"subcircuit named [%s] not found in ckt [%s]\n",	
			nm,ckt->name); }
      else {
         assert(hcp->payload.flag==PAYLOAD_subckt);
         xp->xp=hcp->payload.data.subckt;
         assert(xp->xp!=NULL);
         do_subckts(xp->xp,scan); /* recursion */
      }
    }

}


static subckt_t *new_ckt(deck_t *d, scanner_t *scan)
{
  deck_t *dp;
  subckt_t *first=NULL;
  deck_t topdeck;
  card_t topcard[2];
  topdeck.line.line=0;
  topdeck.line.fileindex=0;
  topdeck.next=d;
  topdeck.card=&(topcard[0]);
  topcard[0].next=&(topcard[1]);
  topcard[0].str[0]=0;
  topcard[0].token=Ksubckt;
  strcpy(topcard[0].str,".su"); /* max 4 bytes */
  topcard[0].val=NULL;
  topcard[1].next=NULL;
  topcard[1].token=Kother;
  topcard[1].val=NULL;
  strcpy(topcard[1].str,"TOP"); /* MAX 4 bytes */

  dp=&topdeck;

  first=new_subckt(&dp,NULL,scan);
  first->subckt_magic=SUBCKT_MAGIC;
  do_subckts(first,scan);
  return first;
}


static void debug_subckts(void * dbg, subckt_t *ckt, int level)
{
  int i;
  uchar pad[64]="                   ";

  if(ckt==NULL)return;
  /*
  if(ckt->flag)return;
  ckt->flag=1;
  */

  pad[level*2]=0;
  
  fprintf(dbg,"%s subckt %s nm=%i nc=%i nr=%i nx=%i\n",pad,ckt->name,ckt->nm,ckt->nc,ckt->nr,ckt->nx);
  for(i=0;i<ckt->nm;i++)
    {
      node_t *n=ckt->m[i].nodes;
      fprintf(dbg,"%s M %s %s %s %s\n%s  +",pad,
		  n[0]->str,n[1]->str,n[2]->str,n[3]->str,pad);
      debug_eqn(dbg,"l",&(ckt->m[i].l));
      debug_eqn(dbg,"w",&(ckt->m[i].w));
      debug_eqn(dbg,"as",&(ckt->m[i].as));
      debug_eqn(dbg,"ad",&(ckt->m[i].ad));
      debug_eqn(dbg,"ps",&(ckt->m[i].ps));
      debug_eqn(dbg,"pd",&(ckt->m[i].ps));
      fprintf(dbg,"\n");
    }

  for(i=0;i<ckt->nc;i++)
    {
      node_t *n=ckt->c[i].nodes;
      fprintf(dbg,"%s C %s %s ",pad,
	      n[0]->str,n[1]->str);
      debug_eqn(dbg,"c",&(ckt->c[i].c));
      fprintf(dbg,"\n");
    }

  for(i=0;i<ckt->nr;i++)
    {
      node_t *n=ckt->r[i].nodes;
      fprintf(dbg,"%s R %s %s ",pad,
	      n[0]->str,n[1]->str);
      debug_eqn(dbg,"r",&(ckt->r[i].r));
      fprintf(dbg,"\n");
    }

  
  for(i=0;i<ckt->nx;i++)
    {
      fprintf(dbg,"%s X %s : %p, %i nodes\n",pad,ckt->x[i].xp->name,ckt->x[i].nodes,ckt->x[i].nn);
      debug_subckts(dbg,ckt->x[i].xp,level+1); /* recursion */
    }

}







/**********************************************************************/
/***************** PASS 3:  operations on heirarchy *******************/
/******************************-- flattening --- **********************/




/* a couple of different lookup functions for equations and parameters
   with slightly different semantics 
   
   lookup_params is for TERMINAL equations that need multipliers
   lookup_params_nomult is for equations in '.param' statements
   lookup_parents_call_eqns is for equations in  subckt calls
      ie:  Xdddd blah blah v='eqn blah' ...  
*/
#ifndef NO_EQUATIONS
static eqn_t lookup_parents_call_eqns(stack_t *s, eqn_t *ep);
static eqn_t lookup_params_nomult(stack_t *s, eqn_t *ep);
static eqn_t lookup_params(stack_t *s, eqn_t *ep, float m);
#endif


static void spice_recurse_subckts(spice_t *spice, subckt_t *ckt, stack_t *parent, void (*process)(stack_t *s, int flag, void *data),void *data)
{
  stack_t s;
  int i;
  s.nl=NULL;
  s.scan=spice->scan;
  if(ckt==NULL)return;
  
  if(parent==NULL) { s.level=0; s.parent=NULL; s.ckt=ckt; s.call=NULL; s.top=&s;                          }
  else {   s.level=parent->level+1;  s.parent=parent;  s.ckt=ckt;     s.call=NULL; s.top=s.parent->top;   }
  
  process(&s,0,data);

  for(i=0;i<ckt->nx;i++)
    { s.call=ckt->x+i; spice_recurse_subckts(spice,s.call->xp,&s,process,data); } /* recursion */ 

  s.call=NULL;
  process(&s,1,data);
}



static char * do_name(stack_t *s, char *str)
{
  static char buf[1024*63];
  char *bp,*c;
  stack_t *sp;
  int l,q=0,sizebuf=sizeof(buf);
  int m;

  D(10,Fprintf(stderr,"doname: [%s] ",str));
  
  m=sizebuf -4;
  buf[sizebuf-1]=0;
  bp=buf+sizebuf-1;
  
  l=strlen(str);
  bp-=l;
  q+=l; assert(q<m);
  memcpy(bp,str,l);
  
  for(sp=s;sp!=NULL;sp=sp->parent)
    {
      if(sp->call==NULL)continue;
      assert(sp->call->deck!=NULL);
      assert(sp->call->deck->card!=NULL);
      c=sp->call->deck->card->str;
      l=strlen(c);
      q+=l+1; assert(q<m);
      bp--;
      bp[0]='.';
      bp-=l;
      memcpy(bp,c,l);
    }
  D(10,Fprintf(stderr,"(%s)\n",bp));
  return bp;
}





#ifdef NO_EQUATIONS
static eqn_t lookup_params(stack_t *s, eqn_t *ep, float m)
{
  ep->val=ep->val*m;
  return *ep;
}
#else



static paramclient_t * find_param_intable(hash_t *h, char *str)
{
 int l; 
 paramclient_t *pc,*tofind;
 /* note to self: this trickery is dependent on hash.c doing the right thing which it does */
 tofind=(void *)(str-sizeof(paramload_t));
 
 l=strlen(str)+1;
 D(30,Fprintf(stderr,"looking for %s in table %p l=%i ",str,h,l));
 pc=hash_find(h,tofind,sizeof(paramload_t)+l);
 D(30,Fprintf(stderr," hash_find=%p\n",pc));
 return pc;
}


/* local (bottom) takes precedence over top (global) */
static paramclient_t * find_param_bot(stack_t *s, char *str)
{
  stack_t *sp;
  paramclient_t *r=NULL,*toret=NULL;

  subckt_t *ckt;

  for(sp=s;sp!=NULL;sp=sp->parent)
    {
      if(sp->ckt!=NULL)
	{
	  ckt=sp->ckt;
	  r=find_param_intable(ckt->lparams,str);
	  if(toret==NULL)toret=r;
	  r=find_param_intable(ckt->params,str);
	  if(toret==NULL)toret=r;
	}
    }
  return toret;
}

/* global (top) takes precidence over bottom (local) */
static paramclient_t * find_param_top(stack_t *s, char *str)
{
  paramclient_t *r=NULL,*toret=NULL;
  subckt_t *ckt;
  stack_t *sp;

  for(sp=s;sp!=NULL;sp=sp->parent)
    {
      if(sp->ckt!=NULL)
	{
	  ckt=sp->ckt;
	  r=find_param_intable(ckt->lparams,str);
	  if(r!=NULL)toret=r;
	  r=find_param_intable(ckt->params,str);
	  if(r!=NULL)toret=r;
	}
    }
  return toret;
}




static eqn_litref_t lookup_function(plookup_t *user, char *str)
{
  paramlookup_t *me=user->user;
  paramclient_t *r;
  eqn_litref_t index=eqn_litref_INIT;
  char *hname;

  if(1)r=find_param_bot(me->sp,str);
  else r=find_param_top(me->sp,str);
  if(r!=NULL)
    {
      /* this is not a renameable local parameter */
      if(r->payload.patch_call==NULL) 
	{
	  /* we want to use this parameter, but don't have a global entry yet */
	  if(r->payload.global_i<0)
	    { 
	      /* todo: lookup this param=equation now, in case it has dependencies */
	      eqn_t eq;
	      hname=do_name(me->sp,str);
	      D(5,Fprintf(stderr,"Lookup function: Adding eqn (%s) (%s)\n",str,hname));
	      D(5,debug_eqn(stderr,str,&(r->payload.eqn)));
	      D(5,Fprintf(stderr,"\n"));
	      eq=lookup_params_nomult(me->sp,&(r->payload.eqn));
	      r->payload.global_i=eqnl_add(&(me->nl->eqnl),eq,hname);
	    }
	  index=r->payload.global_i;
	}
      else /* we renamed this parameter, follow the indirection, make heir. name */
	{  /* also note we probably want to call lookup params now
	      using parent's context for subckt call
	   */
	  D(50,Fprintf(stderr,"Lookup rename: %s\n",str));
	  D(50,debug_eqn(stderr,"    eqn ",&(r->payload.patch_call->eqn)));
	  D(50,Fprintf(stderr,"\n"));
	  if(r->payload.global_i<0)
	    {
	      hname=do_name(me->sp,str);
	      D(5,Fprintf(stderr,"Lookup function rename: Adding eqn (%s) (%s)\n",str,hname));
	      D(5,debug_eqn(stderr,"    eqn ",&(r->payload.patch_call->eqn)));
	      r->payload.global_i=eqnl_add(&(me->nl->eqnl),
					   lookup_parents_call_eqns(me->sp, &(r->payload.patch_call->eqn)),
					   hname);
	    }
	  index=r->payload.global_i;	 
	}
    }
  else /* an undefined reference, may be a sweep parameter, to be loaded from tracefile */
    {
      index=eqnl_find(&(me->nl->eqnl),str);
      if(index<0)
	{
	  D(3,Fprintf(stderr,"Netlist_sp: assuming %s is global sweep parameter\n",str));
	  index= eqnl_add(&(me->nl->eqnl),eqn_undefined(),str);
	}
      else D(3,Fprintf(stderr,"Netlist_sp: assume %s is a global parameter, not local\n",str));
    }
  return index;
}


static eqn_t lookup_parents_call_eqns(stack_t *s, eqn_t *ep)
{
  int r=0;
  eqn_t v;
  plookup_t plu;
  paramlookup_t me;

  if(s->parent!=NULL)
    {
      me.sp=s->parent;
      me.nl=s->nl;
      plu.user=&me;
      plu.lookup=lookup_function;
      v=eqn_copy(*ep);
      D(10,debug_eqn(stderr,"lookup parents:", &v));
      D(10,Fprintf(stderr,"\n"));
      r=eqntok_depend(v.eqn,&plu);
      if(r)D(1,Fprintf(stderr,"couldn't find a parameter in equation\n"));
    }
  else D(1,Fprintf(stderr,"lookup parents eqn called, but we are at the top level\n"));
  assert(s->parent!=NULL);
  return v;
}

static eqn_t lookup_params_nomult(stack_t *s, eqn_t *ep)
{
  eqn_t v;
  int r=0;
  plookup_t plu;
  paramlookup_t me;
  me.sp=s;
  me.nl=s->nl;
  plu.user=&me;
  plu.lookup=lookup_function;

  v=eqn_copy(*ep);
  
  if(v.eqn!=NULL)
    r=eqntok_depend(v.eqn,&plu);
  
  if(r)D(1,Fprintf(stderr,"couldn't find a parameter in equation\n"));

 return v;


}

static eqn_t lookup_params(stack_t *s, eqn_t *ep, float m)
{
  eqn_t v;
  int r=0;
  plookup_t plu;
  paramlookup_t me;
  me.sp=s;
  me.nl=s->nl;
  plu.user=&me;
  plu.lookup=lookup_function;

  v=eqn_copy_m(*ep,m);
  
  if(v.eqn!=NULL)
    r=eqntok_depend(v.eqn,&plu);
  
  if(r)D(1,Fprintf(stderr,"couldn't find a parameter in equation\n"));

 return v;
}
#endif



static void count_stuff(stack_t *s, int flag, void *data)
{
  netlist_t *nl=data;
  if(flag)return;
  nl->e[DEVT_FET].qcount+=s->ckt->nm;
  nl->e[DEVT_CAP].qcount+=s->ckt->nc;
}





static void build_stuff(stack_t *s, int flag, void *data)
{
  netlist_t *nl=data;
  int i,q,j;
  node_t n;
  hashbin_t *bp;
  hash_t *h,*globals,*h1;
  x_t *call;
  paramclient_t *pc,*findpc;
  scanner_t *scan=s->scan;
  if(flag)return;  /* preorder */

  h=s->ckt->nodes;  
  globals=s->top->ckt->global;
  s->nl=nl;

  D(40,Fprintf(stderr,"processing stack %p: s->top=%p s->parent=%p s->ckt=%p\n",s,s->top,s->parent,s->ckt));

  /* first assign parent payloads to all the nodes in nodedir */
  if(s->parent!=NULL)
    {
      call=s->parent->call;
      assert(call!=NULL);

      if(call->nn!=s->ckt->ndefn)
	error(call->deck,call->rest,"Definition does not match use");
      
      q=minimum(call->nn,s->ckt->ndefn);
      
      for(i=0;i<q;i++)
	{
	  assert(call->nodes[i]!=NULL);
	  assert(call->nodes[i]->payload.flag==PAYLOAD_gnptr);
	  s->ckt->defn[i]->payload.data=call->nodes[i]->payload.data;
	  s->ckt->defn[i]->payload.flag=PAYLOAD_parent;
	}


      /* lookup subckt call's and map to local params  */

      h1=s->ckt->lparams;
      hash_forall(h1,i,bp) /* init patch ptrs to Null (so any previous invokation won't be used) */
	{
	  pc=hash_bin2user(bp); pc->payload.patch_call=NULL; pc->payload.global_i=-1;
	  D(30,Fprintf(stderr,"considering hash table entry %i %s len=%i\n",i,pc->str,bp->size-sizeof(paramload_t)));
	}
      
      /* for each param in the x call line, patch local param if found */
      for(i=0;i<call->nl;i++)
	{
	  callparam_t *callp;
	  callp=call->locals[i];
	  if(callp==NULL)continue;
	  
	  /* hackish hash_find: we pass a bogus object, who's string is at same location */
	  findpc=hash_find(h1,callp->str-sizeof(paramload_t),strlen(callp->str)+sizeof(paramload_t)+1);
	  
	  if(findpc!=NULL) /* we link to this subckt eqn call */
	    findpc->payload.patch_call=callp;
	  else
	    D(2,Fprintf(stderr,"subckt call, specified parameter [%s] not found\n",callp->str));
	 
	  D(5,Fprintf(stderr,"patching call %s",callp->str));
	  if(findpc!=NULL)D(5,debug_eqn(stderr,"   orig ",&(findpc->payload.eqn)));
	  D(5,debug_eqn(stderr,"   patch ",&(callp->eqn)));
	  D(5,Fprintf(stderr,"\n"));
	}
    }
  
  /* if parent is NULL, then we are top level, specially create/process global nodes first */
  else
    {
      assert(s->top->ckt==s->ckt);
      
      FORALL_HASH(globals,i,bp)
	{
	  char *flatname;
	  n=hash_bin2user(bp);
	  n->payload.flag=PAYLOAD_gnptr;
	  /* note to self:  must use terminal indices, because node table may be reallocated as it grows */
	  flatname=do_name(s,n->str);
	  n->payload.data.t=netlist_node(nl,flatname,(void*)n);
	}
    }
  
  
  /* then  assign new node payload to all the rest, doing global checks    */

  FORALL_HASH(h,i,bp)
    {
      node_t gbn;
      n=hash_bin2user(bp);
      if((n->payload.flag==PAYLOAD_gnptr)||(n->payload.flag==PAYLOAD_parent)||(n->payload.flag==PAYLOAD_null))
	;
      else assert(0);

      gbn=find_hashtab(globals,n->str); /* i hope this search is efficient  */
      if(gbn!=NULL)
	{
	  if(n->payload.flag!=PAYLOAD_parent)
	    n->payload.data=gbn->payload.data;
	}
      else
	{
	  if(n->payload.flag!=PAYLOAD_parent)
	    {
	      char *flatname;
	      flatname=do_name(s,n->str);
	      n->payload.data.t=netlist_node(nl,flatname,(void *)n); 
	    } 
	}
      n->payload.flag=PAYLOAD_gnptr;  /* set both types to gnptr */
      
    }

  /* now we have ptrs to every gn_t in this subckt reference, so we can process elements */
  
  for(i=0;i<s->ckt->nc;i++)  /* FOR EACH capacitor */
    {
      dev_input_t devinp;
      devcap_t *dcp;
      c_t *cp;
      termptr_t np[2];
      for(j=0;j<2;j++)
	np[j]= s->ckt->c[i].nodes[j]->payload.data.t;
      devinp.spice_cap=cp=&(s->ckt->c[i]);
      dcp=list_data(&(nl->e[DEVT_CAP].l),netlist_newdev_fromnode(nl,DEVT_CAP,devinp,np));
      dcp->v[DEVCAP_c]=lookup_params(s,&(cp->c),s->ckt->mult);
    }
  
  for(i=0;i<s->ckt->nm;i++) /* FOR EACH fet */
    {
      dev_input_t devinp;
      termptr_t np[4];
      m_t *cp;
      devfet_t *dfp;
      int spiceorder[4]={INDEX_SOURCE, INDEX_DRAIN, INDEX_GATE, INDEX_BULK };
      int netlistorder[4]= {DEVFET_S, DEVFET_D, DEVFET_G, DEVFET_B };
      
      for(j=0;j<4;j++)
	np[netlistorder[j]]=s->ckt->m[i].nodes[spiceorder[j]]->payload.data.t;
      devinp.spice_fet=cp=&(s->ckt->m[i]);
      dfp=list_data(&(nl->e[DEVT_FET].l),netlist_newdev_fromnode(nl,DEVT_FET,devinp,np));
      
      for(j=0;j<DEVFET_V;j++)
	dfp->v[j]=eqn_const(0);

      dfp->type=0;

      if(cp->type=='p')
	{ dfp->type=DEVFET_pmos; }
      else
	if(cp->type=='n')
	  { dfp->type=DEVFET_nmos; }
      else
	assert(0);

      D(30,do 
      {
	static int mosfet_i=0;
	Fprintf(stderr,"processing mosfet %i \n",++mosfet_i);
      } while(0)
	);
      
      dfp->v[DEVFET_w] =lookup_params(s,&(cp->w),s->ckt->scale);
      dfp->v[DEVFET_l] =lookup_params(s,&(cp->l),s->ckt->scale);
      dfp->v[DEVFET_as]=lookup_params(s,&(cp->as),(s->ckt->scale)*(s->ckt->scale));
      dfp->v[DEVFET_ad]=lookup_params(s,&(cp->ad),(s->ckt->scale)*(s->ckt->scale));
      dfp->v[DEVFET_ps]=lookup_params(s,&(cp->ps),s->ckt->scale);
      dfp->v[DEVFET_pd]=lookup_params(s,&(cp->pd),s->ckt->scale);  
    }

  for(i=0;i<s->ckt->nx;i++) /* FOR EACH SUBCKT CALL */
    {
      x_t *xp;
      xp=s->ckt->x+i;
      for(j=0;j<xp->nl;j++)
	if(xp->locals[j]!=NULL)
	  {
	    /* we copy the actual equation at use instantiation time
	       via lookup_parent...	      	     
	     */
	  }
    }





  
  return ;  /* recurse on subckt calls */ 
}




static void free_ckt(subckt_t *ckt)
{
  int i;
  hash_t *tab;
  hashbin_t *bp;
  node_t p;
  subckt_t *tofree;

  if(ckt==NULL)return;

  tab=ckt->cktdir;  
  FORALL_HASH(tab,i,bp)  
    {
      p=hash_bin2user(bp);
      tofree=p->payload.data.subckt;
      p->payload.data.subckt=NULL; 
      free_ckt(tofree); 
    }

  for(i=0;i<ckt->nx;i++)
    {
      int j;

      for(j=0;j<ckt->x[i].nl;j++)
	if(ckt->x[i].locals[j]!=NULL)
	  free(ckt->x[i].locals[j]);
      
      if(ckt->x[i].nodes!=NULL)
	free(ckt->x[i].nodes);  /* this frees locals also (combined allocation) */
      
      /* note:  equations have their own memory allocation system
	 so NEVER free an eqntoken_t *
      */            
    }
    hash_free(ckt->cktdir); 
    hash_free(ckt->nodes);
    hash_free(ckt->params);
    hash_free(ckt->lparams);
    hash_free(ckt->global);
    
  free(ckt);
}

  
void spice_count(netlist_t *nl)
{
  spice_t *sp=nl->input.spice;
  
  spice_recurse_subckts(sp,sp->ckt,NULL,&count_stuff,(void *)nl);
  
}


void spice_build(netlist_t *nl)
{
  spice_t *sp=nl->input.spice;
  netlist_eqn_begin(nl);
  spice_recurse_subckts(sp,sp->ckt,NULL,&build_stuff,(void *)nl);
  netlist_eqn_end(nl);
}


void spice_release(spice_t *sp)
{
  if(sp!=NULL)
    {
      free_ckt(sp->ckt);
      eqn_mem_free(sp->eqn_mem);
      free(sp);
    }
}

spice_t *spice_new(scanner_t *scan)
{
  spice_t *p;
  p=malloc(sizeof(spice_t));
  assert(p!=NULL);
  memset(p,0,sizeof(spice_t));
  p->spice_magic=SPICE_MAGIC;
  p->scan=scan;
  p->eqn_mem=eqn_mem_new();
  eqn_mem_push(p->eqn_mem);
  scanner_sect_new(p->scan,scanner_def_spice(),spice_tokens);
  scanner_parse_all(p->scan);
  D(6,scanner_debug_all(p->scan,stderr));
  p->deck=p->scan->sectp->dhead;
  scanner_sect_release(p->scan);

  if(p->deck!=NULL)
    { p->ckt=new_ckt(p->deck,p->scan); }
  else { p->ckt=NULL;  }
  
  if(eqn_mem_pop()!=p->eqn_mem)
    assert(0);
  return p;
}





void spice_debug(void * dbg_fp, spice_t *sp)
{
  FILE *dbg=dbg_fp;
  fprintf(dbg,"debugging output for spice circuit file \n");

  debug_subckts(dbg,sp->ckt, 0);
}


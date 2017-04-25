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
/* extract.c :  optional magic .ext file input (bypassing ext2spice) 
                links to magic's extract library
                .ext files have more/better info than what's in the spice file,
                which gives us better debugging info, and integration with the layout

Conrad Ziesler
*/


/* this is loosely based on magic's extcheck sample program, 
   and links to the magic libraries, so if you don't have them, undef USE_EXTRACT_FORMAT 

   the following copyright notice was from extcheck.c, which this code was based on

*/

/*
 * extcheck.c --
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
*/









#include <stdio.h>
#include <math.h>
#include "debug.h"

#include "netlist_extract.h"

#ifndef __NETLIST_H__
#include "netlist.h"
#endif



void *mymalloc(unsigned size)
{
  return malloc(size);
}


#define CHECK_EXTINIT(g)  if(g!=NULL) if(g->input.generic->magic==EXTRACT_MAGIC) if(g->input.ext->did_EF_init_read) 

#ifdef USE_EXTRACT_FORMAT




#include <stdio.h>
#include <ctype.h>
#include <varargs.h>

#include "include/magic.h"
#include "include/paths.h"
#include "include/geometry.h"
#include "include/hash.h"
#include "include/utils.h"
#include "include/pathvisit.h"
#include "include/extflat.h"
#include "include/runstats.h"


int GeoScale(Transform *t);
void EFInit(void);
void EFReadFile(char *name);
void EFFlatBuild(char *rootName, int flags);
void EFFlatDone(void);
void EFVisitFets(
		 int (*fetProc)(Fet *fet, HierName *hname, Transform *t, int l , int w, ClientData data),
		 ClientData cdata
		 );

void EFVisitNodes(
		  int (*nodeProc)(EFNode *node, int r, double c, ClientData cdata),
		  ClientData cdata
		  );

bool EFHNIsGlob(HierName *hname);
char * EFHNToStr(HierName *hierName);		

void EFDone(void);


static int ecNumFets;
static int ecNumCaps;
static int ecNumResists;
static int ecNumThreshCaps;
static int ecNumThreshResists;
static int ecNumNodes;
static int ecNumGlobalNodes;
static int ecNumNodeCaps;
static int ecNumNodeResists;



#endif


/* 
   make a new extract_t container
   first create fake argc, *argv[] using extract options from config file 
*/

#ifdef USE_EXTRACT_FORMAT
extract_t *extract_new(char *rootname)
{
  extract_t *p;
  int i;
  assert(rootname!=NULL);
  p=mymalloc(sizeof(extract_t)+(sizeof(char*)*(PARAMS.extract_qty+4)));
  assert(p!=NULL);
  memset(p,0,sizeof(extract_t));
  p->root=strdup(rootname);
  assert(p->root!=NULL);
  p->extract_magic=EXTRACT_MAGIC;
  p->ext_argv=(char **) (&p[1]);
  p->ext_argv[0]="ext2spice";
  for(i=1;i< (PARAMS.extract_qty+1);i++)
    p->ext_argv[i]=PARAMS.extract_params[i-1];
  p->ext_argv[i]=p->root;
  p->ext_argv[i+1]=NULL;
  p->ext_argc=i+1;
  p->did_EF_init_read=0;


  
  /* Process command line arguments */
  EFInit();
  p->fname = EFArgs(p->ext_argc, p->ext_argv, (int (*)()) NULL, (ClientData) NULL);
  if(p->fname==NULL){ fprintf(stderr,"ERROR: extract configuration  error\n"); return NULL; }
   
  /* Read the hierarchical description of the input circuit */
  EFReadFile(p->fname);
  
  if(PARAMS.extfix!=NULL) /* tag with extfix data (merge Gnd/Vdd GND/VDD) */
    EFReadFile(PARAMS.extfix);
  
  if (EFArgTech) EFTech = StrDup((char **) NULL, EFArgTech);
  if (EFScale==0) EFScale = 1;
  p->did_EF_init_read=1;

  if(0)
    {
      fprintf(stderr,"EXT: %i layers defined\n",EFLayerNumNames);
      for(i=0;i<EFLayerNumNames;i++)
	fprintf(stderr,"EXT: layer %i=%s\n",i,EFLayerNames[i]);
    }
  


  return p;
}

#else

extract_t *extract_new(char *rootname)
{
  return NULL;
}
#endif



#ifndef USE_EXTRACT_FORMAT


/****************** LINK TO MAGIC EXTFLAT LIBRARY ************/
#else   




void extract_finish(graph_t *gr)
{
  
  EFFlatDone();
  EFDone();
  
  fprintf(stderr,"EXT: Memory used: %s\n", RunStats(RS_MEM, NULL, NULL));
  fprintf(stderr,"EXT: %d fets\n", ecNumFets);
  fprintf(stderr,"EXT: %d nodes (%d global, %d local)\n",
	  ecNumNodes, ecNumGlobalNodes, ecNumNodes - ecNumGlobalNodes);
  fprintf(stderr,"EXT: %d nodes above capacitance threshold\n", ecNumNodeCaps);
  fprintf(stderr,"EXT: %d nodes above resistance threshold\n", ecNumNodeResists);
  fprintf(stderr,"EXT: %d internodal capacitors (%d above threshold)\n",
	  ecNumCaps, ecNumThreshCaps);
  fprintf(stderr,"EXT: %d explicit resistors (%d above threshold)\n",
	  ecNumResists, ecNumThreshResists); 
}

static int countfetVisit(Fet *fet, HierName *hname, Transform *t, int l , int w, ClientData data)
{
  fet->fet_type&=0x07f; /* clear our sneaky bit */
  ecNumFets++;
  return 0;
}


static int countnodeVisit(EFNode *node, int res, double cap, ClientData data)
{
  ecNumNodes++;
  return 0;
}

static EFNode *GetNode(HierName *prefix,HierName *suffix)
{
  HashEntry *he;
  
  he = EFHNConcatLook(prefix, suffix, "output");
  return(((EFNodeName *) HashGetValue(he))->efnn_node);
}


void extract_setup_graph(graph_t *gr)
{
  int er=1;
  CHECK_EXTINIT(gr)er=0;
  assert(!er);
  
  /* Convert the hierarchical description to a flat one */
  EFFlatBuild(gr->input.ext->fname, EF_FLATNODES | EF_FLATCAPS | EF_FLATRESISTS);

  ecNumFets=0;
  ecNumNodes=0;
  EFVisitFets(countfetVisit, (void *) gr);
  EFVisitNodes(countnodeVisit, (void *) gr);
  gr->mge=ecNumFets;
  gr->mgn=ecNumNodes;
  
  fprintf(stderr,"EXT: setup %i fets %i nodes\n",gr->mge, gr->mgn);
}




static int buildnodeVisit(EFNode *node, int res, double cap, ClientData data)
{
  graph_t *g=(void *)data;
  hashchain_t *hc;
  hashload_t pay;
  char *str;
  gn_t *gn;
  int i;
  PerimArea *diff[2];

  /* 
     cap = (cap + 500) / 1000;
     res = (res + 500) / 1000; 
     what's up with this? 
  */

  assert(node!=NULL);
  assert(node->efnode_name!=NULL);
  str=EFHNToStr(node->efnode_name->efnn_hier);
  
  ecNumNodes++;
  if (EFHNIsGlob(node->efnode_name->efnn_hier)) ecNumGlobalNodes++;
  
  pay.flag=PAYLOAD_node;
  pay.data=gn=new_gn(g);
  node->efnode_client=(void *)gn; /* store our node ptr in here so we don't have to compare strings */
  hc=add_hashtab(g->names,str,pay);  /* add some string data for debugging, printing, etc memory hog*/
  if(EFHNIsGlob(node->efnode_name->efnn_hier))
    { 
      int l;
      gn->special=safestrdup(hc->str); 
      l=strlen(gn->special);
      if(gn->special[l-1]=='!')gn->special[l-1]=0;  
    }
  else gn->special=NULL;
  gn->name=hc->str;
  gn->debug=gn->name;
  gn->wtotal=0;
  
  gn->cfixed=cap*1e-18; /* cap in attofarrads 1e-18 */
  diff[FTpfet]=node->efnode_pa+1;  /* resist class 1 from techfile is pdiff */
  diff[FTnfet]=node->efnode_pa+0;  /* resist class 0 from techfile is ndiff */

  for(i=0;i<2;i++)
    gn->cdiff[i]= 
      (PARAMS.scale * PARAMS.scale * diff[i]->pa_area * PARAMS.c_area_sd[i]) + 
      (PARAMS.scale * diff[i]->pa_perim * PARAMS.c_perim_sd[i])
      ;

  /*
    note on capacitance:  
    The above code is very dependent on the magic techfile extract section,
    namely, it assumes a specific resistance class ordering
    and it assumes the extractor doesn't sum up diffusion and gate capacitance
  */

  return 0;
}

static graph_t *extract_Graph;

static int buildfetVisit(Fet *fet, HierName *hname, Transform *t, int l , int w, ClientData data)
{
  ge_t *ep;
  FetTerm *gate, *source, *drain;
  EFNode /* *bnode,*/  *snode, *dnode, *gnode;
  Rect r;
  int      scale;
  graph_t *gr=extract_Graph;
  int typ;
  char *ft;
  double sc=PARAMS.scale; /* lamda in meters */

  assert(sc>0);
  assert(fet!=NULL);
 
  gate = &fet->fet_terms[0];
  source = drain = &fet->fet_terms[1];
  if (fet->fet_nterm >= 3)
    drain = &fet->fet_terms[2];
  
  gnode = GetNode (hname, gate->fterm_node->efnode_name->efnn_hier);
  snode = GetNode (hname, source->fterm_node->efnode_name->efnn_hier);
  dnode = GetNode (hname, drain->fterm_node->efnode_name->efnn_hier);
  /*bnode = fetSubstrate(hname, fet->fet_subsnode->efnode_name->efnn_hier, fet->fet_type, NULL);*/
  
  assert(w!=0);
  assert(l!=0);
  GeoTransRect(t, &fet->fet_rect, &r);
  scale = GeoScale(t);
  assert(scale!=0);
  typ=fet->fet_type;
  ft=EFFetTypes[typ];
  typ=np_type((uchar)ft[0]);
  ep=new_ge(gr);
  ep->type=typ;


  ep->flatref.x=r.r_ll.p_x;
  ep->flatref.xx=r.r_ur.p_x;
  ep->flatref.y=r.r_ll.p_y;
  ep->flatref.yy=r.r_ur.p_y;

  ep->WL= (PARAMS.gm_woverl[ep->type]*(double)w)/(double)l;
  ep->Cg= sc*sc*PARAMS.c_area_gate[ep->type]*((double)w*scale)*((double)l*scale);
  ep->Cs=0; /* compute this from ? */
  ep->Cd=0; /* same with this */

  /* for above Cs,Cd: strategy, split the diffusion resistance classes between all fet s/d's connected
     to the node, proportional to the total width of each device 
     this is better than the hspice version, where diffusion is lumped onto a single fet.
     so one poor fet gets all the diffusion for resizing 
     we have to do this after we visit all the fets once, so node wtotals are updated correctly
  */ 

  ep->parent.vp=(void *)fet;
  ep->gn[GE_G]=(void *)(gnode->efnode_client);
  ep->gn[GE_S]=(void *)(snode->efnode_client);
  ep->gn[GE_D]=(void *)(dnode->efnode_client);
  if(ep->gn[GE_S] > ep->gn[GE_D] )  /* put source/drain in cannonical order (source is lower node number) */ 
    { void *tmp=ep->gn[GE_S]; ep->gn[GE_S]=ep->gn[GE_D]; ep->gn[GE_D]=tmp; }
  ep->gn[GE_S]->wtotal+=(w*scale*sc);
  ep->gn[GE_D]->wtotal+=(w*scale*sc);
  return 0;
}

/*
static int capVisit(HierName *hn1, HierName *hn2, double cap)
{
    ecNumCaps++;
    cap = (cap + 500) / 1000;
    if (cap > (double) EFCapThreshold) ecNumThreshCaps++;
    return 0;
}

static int resistVisit(HierName *hn1, HierName *hn2, int res)	
{
    ecNumResists++;
    res = (res + 500) / 1000;
    if (res > EFResistThreshold) ecNumThreshResists++;
    return 0;
}
*/



static int comparesorted(const void *a, const void *b)
{
  const ge_t *pa=a,*pb=b;
   
  if(pa==pb)return 0;
  if(pa==NULL)return 1;
  if(pb==NULL)return -1;


  if(pa->gn[GE_G] > pb->gn[GE_G])return 1;
  if(pa->gn[GE_G] < pb->gn[GE_G])return -1;
   
  if( (pa->gn[GE_S]==pb->gn[GE_S]) )
    {
      if(pa->gn[GE_D] > pb->gn[GE_D] ) return 1;
      else if(pa->gn[GE_D] < pb->gn[GE_D] ) return -1;
    }
  if( (pa->gn[GE_D]==pb->gn[GE_D]) )
    {
      if(pa->gn[GE_D] > pb->gn[GE_D] ) return 1;
      else if(pa->gn[GE_D] < pb->gn[GE_D] ) return -1;
    }
  else if( (pa->gn[GE_S]==pb->gn[GE_D]) )
    {
      if(pa->gn[GE_D] > pb->gn[GE_S] ) return 1;
      else if(pa->gn[GE_D] < pb->gn[GE_S] ) return -1;
    }
  else if( (pa->gn[GE_D]==pb->gn[GE_S]) )
    {
      if(pa->gn[GE_S] > pb->gn[GE_D] ) return 1;
      else if(pa->gn[GE_S] < pb->gn[GE_D] ) return -1;
    }
  else 
    {
      if(pa->gn[GE_S] > pb->gn[GE_S] ) return 1;
      else if(pa->gn[GE_S] < pb->gn[GE_S] ) return -1;
      assert(0);
    }
  return 0;
}


static double compute_diffcap(gn_t *np, double w, int type)
{
  double ws,c;
  ws=np->wtotal;
  if(ws<=0.0)assert(0);
  c= np->cdiff[type]*w/ws; 
  return c;
}

void extract_build_graph(graph_t *gr)
{
  ge_t *ep,*pep,*wep;
  int i,j;
  int er=1;
  CHECK_EXTINIT(gr)er=0;
  assert(!er);

  extract_Graph=gr;
  EFVisitNodes(buildnodeVisit, (ClientData) gr);
  EFVisitFets(buildfetVisit, (ClientData) gr);

  /*
    if (EFCapThreshold < INFINITE_THRESHOLD)    EFVisitCaps(capVisit, (ClientData) NULL);
    if (EFResistThreshold < INFINITE_THRESHOLD) EFVisitResists(resistVisit, (ClientData) NULL);
  */

  /* now we want to sort the edges to merge duplicate fets faster */
  qsort(gr->ge,gr->qge,sizeof(ge_t),comparesorted);
 
  /* mark and merge parallel transistors */
  for(i=0,j=0,pep=NULL,ep=gr->ge;i<gr->qge;i++)
    {
      ep->flag=0;
      if(pep!=NULL)
	if( (ep->gn[GE_G]==pep->gn[GE_G]) && 
	    (ep->gn[GE_S]==pep->gn[GE_S]) && 
	    (ep->gn[GE_D]==pep->gn[GE_D]) 
	    ) /* parallel fet, merge */
	  {
	    pep->WL+=ep->WL;
	    pep->Cg+=ep->Cg;
	    pep->Cs+=ep->Cs;
	    pep->Cd+=ep->Cd;
	    ep->flag=1;
	    j++;
	  }
	else pep=ep;
      else pep=ep;
      
      ep++;
    }
  
  /* eliminate flagged transistors 
     wep moves forward for each new good transistor 
     ep moves forward for each transistor
   */
  
  for(j=0,wep=ep=gr->ge,i=0;i<gr->qge;i++)
    {
      if(!ep->flag) { if(wep!=ep) { *wep=*ep; } j++; wep++; }
      ep++;
    }
  
  gr->qge=j;
  

  /* now divvy up s/d diffusions between fets */
  
  for(ep=gr->ge,i=0;i<gr->qge;i++)
    {
      double w;
      w=edge_width(ep);
      ep->Cs=compute_diffcap(ep->gn[GE_S],w,ep->type);
      ep->Cd=compute_diffcap(ep->gn[GE_D],w,ep->type);
      ep++;
    }
  
}





#endif /* USE_EXTRACT_FORMAT */







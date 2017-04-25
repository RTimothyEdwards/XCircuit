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
/* eqn.c, equation evaluation and parsing

Conrad Ziesler
*/


#include <ctype.h>
#include <math.h>
#include <stdio.h>

#include "debug.h"
#include "eqn.h"
extern int __debug_eqn__;

/* int __debug_eqn__=1; */

#ifndef STRIP_DEBUGGING
#define D(level,a) do { if(__debug_eqn__>(level)) a; } while(0)
#else
#define D(level,a) 
#endif


#define uchar unsigned char




#include "memory.h"
#define MAX_STACK 1024

static memory_t  eqn_mem_default=MEMORY_INIT;
static memory_t  *eqn_mem_stack[MAX_STACK]={ &eqn_mem_default , NULL, NULL };
static int eqn_mem_stackp=0;
static int eqn_mem_qalloc=0;

void eqn_mem_free(void *p)
{
  memory_t *mem=p;
  if(p!=NULL)
    {
      memory_freeall(mem);
      free(p);
    }
}

/* make and return a new eqn memory object */
void *eqn_mem_new(void)
{
  void *p;
  eqn_mem_qalloc++;
  p=malloc(sizeof(memory_t));
  assert(p!=NULL);
  memory_init(p);
  return p;
}

void eqn_mem_push(void *p)
{
  eqn_mem_stackp++;
  assert(eqn_mem_stackp<MAX_STACK);
  eqn_mem_stack[eqn_mem_stackp]=p;
}


/* return and pop stack */
void *eqn_mem_pop(void) 
{
  void *p;
  assert(eqn_mem_stackp>0);
  p=eqn_mem_stack[eqn_mem_stackp];
  eqn_mem_stackp--;
  return p;
}


#ifdef NO_EQUATIONS
void eqn_free(enq_t e)
{ } /* static equations are not mallocated */
#else

static void *eqn_malloc(int size)
{
  void *p=memory_alloc(eqn_mem_stack[eqn_mem_stackp],size); 
  if(p==((void *)0x81dd014))
    {
      int i=0;
      i++;
    }
  return p;
}

void eqn_free(eqn_t e)
{ memory_free(eqn_mem_stack[eqn_mem_stackp],e.eqn); }

#endif


#ifndef NO_EQUATIONS
eqntoken_t *eqntoken_next(eqntoken_t *p)
{
  eqntoken_t *r;
  switch(p->op) 
    { 
    case OPeolist: case OPeol_const: case OPeol_valp: case OPeol_qty: 
      r=p; break; 
    case OPval:  case OPvalm: r= (void *) (((char *)p)+(sizeof(itemop_t)+sizeof(eqntokenval_t) )) ; break; 
    case OPlit:  case OPlitm: 
    case OPlitv:  case OPlitvm: r= (void *) (((char *)p)+(sizeof(itemop_t)+sizeof(eqntokenlit_t) )) ; break; 
    default:  r= (void *) (((char *)p)+(sizeof(itemop_t))) ; break; 
    }
  return r;
}

eqntoken_t *eqntoken_last(eqntoken_t *p)
{
  eqntoken_t *r=p;
 
  while(!OP_ISEND(p->op))p=eqntoken_next(p);
  
  switch(p->op) 
    { 
    case OPeol_const: 
      r= (void *) (((char *)p)+(sizeof(itemop_t)+sizeof(eqntokenval_t))) ;  break; 
    case OPeol_valp:
      r= (void *) (((char *)p)+(sizeof(itemop_t)+sizeof(eqntokenpval_t))) ;  break; 
    case OPeolist:
      r= (void *) (((char *)p)+(sizeof(itemop_t))) ;  break; 
    case OPeol_qty:
      assert(0);
    default:
      assert(0);
    }
  return r;
}
#endif

static float float_parse(uchar *val, int *count)
{
  float v=0.0,sign=1,sig=0,rest=0;
  int i,offset=0;
  uchar *ps=NULL,*pa=NULL,*pb=NULL,*tmpc;
  uchar a;

  for(ps=val;(isspace(ps[0]))&&(ps[0]!=0);ps++);
  if(ps[(unsigned)0]=='-')      { sign=-1; pa=ps+1; }
  else if(ps[(unsigned)0]=='+') { sign=1;  pa=ps+1; }
  else pa=ps;
  
  if(pa[(unsigned)0]=='.'){ sig=0;  pb=pa+1; }
  else 
    if(isdigit(pa[(unsigned)0]))
      {
	sig=atoi(pa); 
	for(pb=pa;pb[0]!=0;pb++)
	  if(!isdigit(pb[0])) break; 
      }

  if(pb==NULL){  return -1e99; }
  if(pb[0]=='.')
    pb++;

  i=0;

  if(isdigit(pb[0]))
    { 
      rest=atoi(pb);
      for(i=0;pb[i]!=0;i++)
	{ 
	  if(!isdigit(pb[i]))break;
	  rest=rest*0.1;
	}
    }
  
  v=(sig+rest)*sign;
  offset=pb+i+1-val;


  switch(pb[i])
    {
    case 'f':  v*=1e-15; break;
    case 'p':  v*=1e-12; break;
    case 'n':  v*=1e-9; break;
    case 'u':  v*=1e-6; break;
    case 'm':  v*=1e-3; break;
    case 'x': v*=1e6;   break;
    case 'e': v*=pow(10.0,(float)atoi(pb+i+1)); 
      for(tmpc=pb+i+1;isdigit(tmpc[0]);tmpc++);
      offset=tmpc-val;
      break;
    case 0:  offset--; break;
    case ' ': offset--; break;
    default:  offset--; break; /* dont eat this char */
    }

  /* this is for the case of '5fF' where we have some trailing alpha chars for units which
     must be discarded so the parser doesn't get confused 
  */
  while((a=val[offset])!=0)
    {
      if(isalpha(a))offset++;
      else break;
    }

  D(10,fprintf(stderr,"%s = %e\n",val,v));
  if(count!=NULL)*count=offset;
  return v;
}

#ifndef NO_EQUATIONS
static int eqntoken_parse(eqntop_t *top)
{
  eqntoken_t *node;
  char *p=top->str;
  int i=0;
  int uselast=1;
  char c;
  int r=1;

  if(isspace(top->last)||(top->last==0))uselast=0;
  
  if(!uselast)
    {
      for(p=top->str;(isspace(p[0]))&&(p[0]!=0);p++);
      c=p[0];
    }
  else c=top->last;
  top->last=0;

  if(top->nodep>=MAXEQNSTACK)return 0;
  
  node=top->nodes+top->nodep++;
  node->z.lit.lit=NULL;
  switch(c)
    {
    case EQN_DELIM: if(!uselast)top->str++; /* fall through */
    case 0:   node->op=OPeolist;  r=0; break;
    case '+': node->op=OPadd; if(!uselast)top->str++; top->opc++; break;
    case '-': node->op=OPsub; if(!uselast)top->str++; top->opc++; break;
    case '*': node->op=OPmul; if(!uselast)top->str++; top->opc++; break;
    case '/': node->op=OPdiv; if(!uselast)top->str++; top->opc++; break;   
    case '^': node->op=OPexp; if(!uselast)top->str++; top->opc++; break;
    case '(': node->op=OPopen;  if(!uselast)top->str++; top->opc++; break;   
    case ')': node->op=OPclose; if(!uselast)top->str++; top->opc++; break;      

    default:  
      if(isdigit(p[0]))
	{
	  int count=0;
	  node->z.val.x=float_parse(p,&count);
	  node->op=OPval;
	  top->str=p+count;
	}
      else
	{
	  for(i=0;p[i]!=0;i++)
	    if(strchr("\'+-/*^\n\r()",p[i])!=NULL)break;
	  top->last=p[i];
	  p[i]=0;
	  if( (top->last==0) || (top->last==EQN_DELIM)) {  r=0; top->str=p+i; }
	  else top->str=p+i+1;
	  
	  assert(uselast==0);
	  /* can never have alphanumeral followed by alphanumeral without some delimiter,
	     since it would just become one big identifer
	  */
	  node->z.lit.ref=eqn_litref_INIT;
	  node->z.lit.cache=0.0;
	  node->z.lit.lit=p;
	  node->op=OPlit;  
	}
      
      top->litc++;
      
      break;
    }
  return r;
}


/* this code assumes dependencies have already been cached, see eqnlist.c */
static int token_value(eqntoken_t *token, float *v)
{
  int r=0;
  float a=0.0;
  switch(token->op)
    {
    case OPeol_valp: 
      if(token->z.pval.xp!=NULL) 
	a=*(token->z.pval.xp); 
      else r++;
      D(3,fprintf(stderr,"loaded pval=(%p->%g)\n",token->z.pval.xp,a));
      break;
    case OPeol_const: a=token->z.val.x;      break;
    case OPlit:   a= 0.0; r++;               break;
    case OPlitm:  a=-0.0; r++;               break;
    case OPlitv:  a=  token->z.lit.cache;    break;
    case OPlitvm: a=-(token->z.lit.cache);   break; 
    case OPval:   a=  token->z.val.x;        break;
    case OPvalm:  a=-(token->z.val.x);       break;
    default: assert(0);
    }  
  *v=a;
  return r;
}
#define PUSH(a)  do { if(data->stackp<MAXSTACK) data->stack[data->stackp++]=(a); \
                      D(5,fprintf(stderr,"Push(%g) new stackp=%i\n",(a),data->stackp)); } while (0)
#define POP(a)   do { if(data->stackp>0) (a)=data->stack[--(data->stackp)]; \
                      D(5,fprintf(stderr,"Pop(%g) new stackp=%i\n",(a),data->stackp)); } while (0)

static int eqntoken_eval(eqneval_t *data)
{
  int loop,toend;
  int r=0;
  float a,b;
  eqntoken_t *token,*lp;
  int i;

  static const int precidence[]=OPprecidence;
  static const int type[]=OPtype;

  int types[4]={ -10 , -10 , -10 , -10 };
  int precs[4]={ -10,  -10 , -10,  -10 };
  int stack_entry=data->stackp;
  
  toend=0;
  do
    {
      itemop_t ops[4]={OPeolist, OPeolist, OPeolist, OPeolist};
      eqntoken_t *tokens[4]={ NULL, NULL, NULL, NULL };

      loop=0;
      token=data->list;
      data->list=eqntoken_next(data->list);
      if(data->list==token)
	toend++;     /* catch endings */
     
      for(lp=token,i=0;(i<4);i++,lp=eqntoken_next(lp)) /* lookahead */
	{
	  ops[i]=lp->op;
	  tokens[i]=lp;
	  assert(ops[i]>=0);
	  assert((ops[i]*sizeof(int))<sizeof(precidence));
	  assert((ops[i]*sizeof(int))<sizeof(type));
	  types[i]=type[ops[i]];
	  precs[i]=precidence[ops[i]];
	  if(OP_ISEND(lp->op)) /* we did last token, stop */
	    break;
	}
      
      D(8,fprintf(stderr,"Eval: lookahead = "));
      for(i=0;i<4;i++)
	{
	  char *syms[]=OPsyms;
	  D(8,fprintf(stderr," %s,t%i,p%i" ,syms[ops[i]],types[i],precs[i]));
	}
      D(8,fprintf(stderr,"\n"));
      
      if(OP_ISANYV(token->op))
	{ 
	  r=token_value(token,&a);
	  D(5,fprintf(stderr,"Value: %g\n",a));
	  PUSH(a); 
	}
      
      else

      if(types[0]==2) /* binary op */
	{
	  a=1.0; b=1.0;
	  POP(a);
	  
	  if( /* literal    and    higher precidence operator after literal not () */
	     (OP_ISV(ops[1])  && (precs[2] > precs[0]) && (types[2]!=4) ) ||
	     
	     /* operator (ie open/close) which has a higher precidence */
	     ((types[1]>=2) && (precs[1] > precs[0]))
	     )
	    
	    {  /* SHIFT */
	      D(6,fprintf(stderr,"Shift: recursion\n"));
	      
	      eqntoken_eval(data);
	      
	      
	      POP(b);
	    }
	  else /* REDUCE */
	    {
	      D(6,fprintf(stderr,"Reduce: \n"));
	      if( OP_ISANYV(ops[1]) )
		{
		  data->list=eqntoken_next(data->list);
		  r+=token_value(tokens[1],&b);
	      
		}
	      else { /* error, need literal */ assert(0); }
	    }
	  
	  D(6,fprintf(stderr,"Doing Binary Op, %g op %g \n",a,b));
	  switch(token->op)
	    {  
	    case OPadd:     a=a+b;	  break;
	    case OPsub:     a=a-b;    break;
	    case OPmul:     a=a*b;    break;
	    case OPdiv:   
	      if(b!=0.0)   a=a/b; 
	      else         a=0.0;  
	      break;
	      
	    case OPexp:     a=pow(a,b);    break;
	      
	    default: assert(0);
	    }
	  PUSH(a);
	  
	}
      else
	if(types[0]==4) /* open/close */
	  {
	    if(token->op==OPopen)
	      {
		D(6,fprintf(stderr,"open { : \n"));
		loop=1;
	      }
	    else if(token->op==OPclose)
	      {
		D(6,fprintf(stderr,"close } : returning \n"));
		return 0;
	      }
	    
	  }
	else if(token->op == OPeolist);
      
      if(toend) { D(6,fprintf(stderr,"Ending:  \n")); return 0; } 
      else
	{
	  D(6,fprintf(stderr,"Stack: entry=%i now=%i  %s \n",stack_entry,data->stackp, 
			  ((stack_entry<data->stackp)||loop)?"looping":"returning")); 
	}
    }
  while((stack_entry<data->stackp)||loop);
  return r;
}

#undef PUSH
#undef POP


eqntoken_t *eqntok_parse(char *str)
{
  eqn_t eq;
  int i,j,qlit,qval,qvalp;
  int last_was_op;
  int type[]=OPtype;
  eqntoken_t *list,*lp,*lpn;
  eqntop_t data;
  eqntoken_t *cp,*np;
  data.nodep=0;
  data.litc=0;
  data.opc=0;
  data.last=0;
  data.str=str;
  
  D(2,fprintf(stderr,"Parsing [%s]\n",str));
  while(eqntoken_parse(&data));

  for(i=0,qval=0,qlit=0,qvalp=0,j=0;i<data.nodep;i++)
    {
      switch(data.nodes[i].op)
	{
	case OPlit: 
	case OPlitm:
	  qlit++; break;
	case OPval:
	case OPvalm:
	case OPeol_const:
	  qval++; break;
	case OPeol_valp:
	  qvalp++; break;
	default: break;
	}
      j++;
    }

  if((qval==1) && (qlit==0) && (qvalp==0) ) /* optimization for single constant, don't store eolist */
    {
      eq=eqn_const(data.nodes[0].z.val.x);
      lp=list=eq.eqn;
    }
  else if((qvalp==1) && (qlit==0) && (qval==0) )
    {
      eq=eqn_valp(data.nodes[0].z.pval.xp);
      lp=list=eq.eqn;
    }
  else if((qvalp==0) && (qlit==0) && (qval==0) )
    {
      /* it appears we have a parse error, no equation is possible */
      lp=list=NULL;
      fprintf(stderr,"Invalid Equation \n");
      assert(0);
    }
  else
    {
      lp=list=eqn_malloc((sizeof(itemop_t)*(j+1)) + (sizeof(eqntokenlit_t)*(qlit)) + (sizeof(eqntokenval_t)*(qval)));
      
      for(last_was_op=1,i=j=0;i<data.nodep;i++)
	{
	  cp=data.nodes+i;
	  if((i+1)<data.nodep)
	    {
	      np=data.nodes+i+1;
	      if( (last_was_op) && (cp->op==OPsub) && (type[np->op]==1) )
		{
		  np->op++; /* go from OPxxx to OPxxxm */
		  last_was_op=0;
		  continue;
		}
	    }
	  if(type[cp->op]>1)
	    last_was_op=1;
	  else last_was_op=0;
	  
	  lp->op=cp->op;
	  lpn=eqntoken_next(lp);
	  memcpy(lp,cp,((char *)lpn)-((char *)lp));
	  lp=lpn;
	  
	  /* list[j++]=cp[0]; */
	}
      /*memcpy(list,data.nodes,data->nodep*sizeof(eqntok_t));*/
      lp->op=OPeolist;
      /*  list[j].op=OPeolist; */
    }

  D(7,
    do {
      char *syms[]=OPsyms;
      fprintf(stderr, "eqnparse: %i tokens %p  ", data.nodep,lp);
      for(lp=list;!OP_ISEND(lp->op);lp=eqntoken_next(lp))
	fprintf(stderr, "%s ",syms[lp->op]);
      fprintf(stderr, "%s ",syms[lp->op]);
      fprintf(stderr,"\n");
    } while (0)
    );

  return list;
}


/* returns 0 if equation can be evaluated */
int eqntok_depend(eqntoken_t *list, plookup_t *lookup)
{
  int q=0;
  eqn_litref_t vp;
  eqntoken_t *lp;

  if(list->op==OPeol_const)return 0; /*optimization */
  assert(lookup!=NULL);

  for(lp=list;!OP_ISEND(lp->op);lp=eqntoken_next(lp))
    {
      if((lp->op==OPlit)||(lp->op==OPlitm))
	{	  
	  D(3,fprintf(stderr,"eqntok_depend: Looking up %s\n",lp->z.lit.lit));
	  if((vp=lookup->lookup(lookup,lp->z.lit.lit))!=eqn_litref_INIT)
	    {
	      lp->z.lit.ref=vp;;
	      lp->op=(lp->op==OPlit)?OPlitv:OPlitvm;
	    }
	  else q++;
	}
    }
  return q;
}

/* only evaluate after eqntok_depend, and
   call eqntok_eval IN PROPER ORDER
   returns 0 if eval ok, 1 if broken dependency, 2 on parse error
*/

int eqntok_eval(float *res, eqntoken_t *list)
{
  eqntoken_t *lp;
  eqneval_t data;
  memset(&data,0,sizeof(data));
  data.stack[(data.stackp=0)]=0.0;
  data.stackp++;
  data.list=list;
  {
    eqn_t v;
    v.eqn=list;
    D(3,debug_eqn(stderr,"Evaluating ",&v));
    D(3,fprintf(stderr,"\n"));
  }
  for(lp=list;!OP_ISEND(lp->op);lp=eqntoken_next(lp))
    {
      if(lp->op==OPlit)return 1;
    }

  eqntoken_eval(&data);

  if(res!=NULL)
    *res=data.stack[1];
  
  if(data.stackp==2)return 0;
  else return 2;
}



eqntoken_t *eqntok_copy(eqntoken_t *p)
{
  int q;
  eqntoken_t *np;
  np=eqntoken_last(p);
  q=((char *)np)-((char *)p);
  np=eqn_malloc(q);
  assert(np!=NULL);
  memcpy(np,p,q);
  return np;
}


eqntoken_t *eqntok_copy_m(eqntoken_t *p, float m)
{
  int q;
  eqntoken_t *np,*nnp;
  
  if(p->op==OPeol_const) /* common case  */
    {
      np=eqn_malloc(sizeof(itemop_t)+sizeof(eqntokenval_t));
      np->op=OPeol_const;
      np->z.val.x=m*p->z.val.x;
    }
  else /* build  np =  m*(old_equation)EOL */
    {
      np=eqntoken_last(p);
      q=((char *)np)-((char *)p);
      
      np=eqn_malloc(q+(sizeof(itemop_t)*4)+sizeof(eqntokenval_t));
      nnp=np;
      nnp->op=OPval;
      nnp->z.val.x=m;
      nnp=eqntoken_next(nnp);
      nnp->op=OPmul;
      nnp=eqntoken_next(nnp);
      nnp->op=OPopen;
      nnp=eqntoken_next(nnp);
      memcpy(nnp,p,q);
      nnp=(void *) (((char *)nnp)+q-sizeof(itemop_t));
      nnp->op=OPclose;
      nnp=eqntoken_next(nnp);
      nnp->op=OPeolist;
    }
  return np;
}

#endif



void debug_eqn(void *dbg_fp, char *str, eqn_t *eqn)
{
  FILE *dbg=dbg_fp;
#ifdef NO_EQUATIONS  
  if(eqn==NULL)return;
  fprintf(dbg,"%g",eqn->val);
#else
  eqntoken_t *eqt;
  if(eqn==NULL)return;
  if(0)fprintf(dbg," %s=[%p %p]",str,eqn,eqn->eqn);
  else fprintf(dbg," %s=",str);
  if(eqn->eqn!=NULL)
    {
      char s=' ';
      for(eqt=eqn->eqn;!OP_ISEND(eqt->op);eqt=eqntoken_next(eqt))
	{
	  switch(eqt->op)
	    {
	    case OPlitm:
	      s='-';
	    case OPlit:
	      fprintf(dbg,"(%c%s)",s,eqt->z.lit.lit);	      break;
	    case OPvalm:
	      s='-';
	    case OPval:
	      fprintf(dbg,"(%c%g)",s,eqt->z.val.x);	      break;
	    case OPlitvm:
	      s='-';
	    case OPlitv:
	      fprintf(dbg,"(%c%s=%i)",s,eqt->z.lit.lit,eqt->z.lit.ref);      break;
	      
	    case OPadd:	      fprintf(dbg,"+"); break;
	    case OPsub:	      fprintf(dbg,"-"); break;
	    case OPmul:	      fprintf(dbg,"*"); break;
	    case OPdiv:	      fprintf(dbg,"/"); break;
	    case OPexp:	      fprintf(dbg,"^"); break;
	    case OPopen:	      fprintf(dbg,"{"); break;
 	    case OPclose:	      fprintf(dbg,"}"); break;
	    default: break;
	    }
	}
      if(eqt->op==OPeol_const)
	fprintf(dbg,"%g",eqt->z.val.x);
      if(eqt->op==OPeol_valp)
	fprintf(dbg,"%p->%g",eqt->z.pval.xp, (eqt->z.pval.xp==NULL)?0.0:*(eqt->z.pval.xp));
    }
#endif
}









eqn_t eqn_const(float val)
{
  eqn_t eqn;
#ifdef NO_EQUATIONS
  eqn.val=val;
#else
  eqn.eqn=eqn_malloc(sizeof(itemop_t)+sizeof(eqntokenval_t));
  eqn.eqn->op=OPeol_const;
  eqn.eqn->z.val.x=val;
#endif
  return eqn;
}

eqn_t eqn_valp(float *valp)
{
  eqn_t r;
#ifdef NO_EQUATIONS
  r.val=0.0;
#else
  r.eqn=eqn_malloc(sizeof(itemop_t)+sizeof(eqntokenpval_t));
  r.eqn->z.pval.xp=valp;
  r.eqn->op=OPeol_valp;
#endif
  return r;
}


eqn_t eqn_undefined(void)
{
  return eqn_valp(NULL);
}

int eqn_is_undefined(eqn_t e)
{
#ifdef NO_EQUATIONS
  return 0;
#else
  if(e.eqn==NULL)
    return 1;
  if(e.eqn->op==OPeol_valp)
    return 2;
  return 0;
#endif
}


eqn_t eqn_copy(eqn_t e)
{
#ifdef NO_EQUATIONS
  return e;
#else
  if(e.eqn!=NULL)
    e.eqn=eqntok_copy(e.eqn);
  return e;
#endif
}

eqn_t eqn_copy_m(eqn_t e, float m)
{
  #ifdef NO_EQUATIONS
  e.val*=m;
  return e;
#else
  if(e.eqn!=NULL)
    {
      if(m==1.0) /* multiplication by 1 is a nop */
	e.eqn=eqntok_copy(e.eqn);
      else
	e.eqn=eqntok_copy_m(e.eqn,m);
    }
  return e;
#endif
}

eqn_t eqn_parse(uchar *val)
{
  eqn_t eqn;
#ifdef NO_EQUATIONS
  eqn.val=float_parse(val,NULL);
#else
  eqn.eqn=eqntok_parse(val);
#endif
  return eqn;
}

float parse_float(uchar *val)
{
  return float_parse(val,NULL);
}


/* this is valid ONLY if we have no literals, ie (6+3*5) */
/* use eqnl_eval for equations like (lambda*5+delta) */

float eqn_getval_(eqn_t *eqn)
{
#ifdef NO_EQUATIONS
  assert(eqn!=NULL);
  return eqn->val;
#else
  float f;
  assert(eqn!=NULL);
  assert(eqn->eqn!=NULL);
  if(eqn->eqn->op==OPeol_const)
    return eqn->eqn->z.val.x;
  f=0.0;
  eqntok_eval(&f,eqn->eqn);
  return f;
#endif
}

eqn_t eqn_empty(void)
{
  eqn_t r;
#ifdef NO_EQUATIONS
  r.val=0.0;
#else
  r.eqn=NULL;
#endif
  return r;
}

float eqn_setval(eqn_t *eqn, float val)
{
  assert(eqn!=NULL);
#ifdef NO_EQUATIONS
  return (eqn->val=val);
#else
  if(eqn->eqn==NULL)
    *eqn=eqn_const(val);
  else
    if(eqn->eqn->op==OPeol_const)
      eqn->eqn->z.val.x=val;
    else
    {
      D(1,debug_eqn(stderr,  "cannot change eqn",eqn));
      D(1,fprintf(stderr,"\n"));
      assert(0);
    }
  return val;
#endif
}

float eqn_setvalp(eqn_t *eqn, float *valp)
{
  assert(eqn!=NULL);
 
 #ifdef NO_EQUATIONS
  return (eqn->val=*valp);
#else
  if(eqn->eqn==NULL)
    *eqn=eqn_valp(valp);
  else
    if(eqn->eqn->op==OPeol_valp)
      eqn->eqn->z.pval.xp=valp;
    else
    {
      D(1,debug_eqn(stderr,  "cannot set valp",eqn));
      D(1,fprintf(stderr,"\n"));
      assert(0);
    }
  return *valp;
#endif
}




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
/* equations.c, equation evaluation and parsing

Conrad Ziesler
*/


#include <ctype.h>
#include <math.h>
#include <stdio.h>

#include "debug.h"
#include "equations.h"

int __debug_eqn__=0;

#ifndef STRIP_DEBUGGING
#define D(level,a) do { if(__debug_eqn__>(level)) a; } while(0)
#else
#define D(level,a) 
#endif


#define uchar unsigned char



static eqntoken_t *eqntoken_next(eqntoken_t *p);


static void debug_eqn(FILE  *fp, char *str, eqn_t *eqn)
{
  eqntoken_t *eqt;
  if(eqn==NULL)return;
  fprintf(fp," %s=",str);
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
	      fprintf(fp,"(%c%s)",s,eqt->z.lit.lit);	      break;
	    case OPvalm:
	      s='-';
	    case OPval:
	      fprintf(fp,"(%c%g)",s,eqt->z.val.x);	      break;
	    case OPlitvm:
	      s='-';
	    case OPlitv:
	      fprintf(fp,"(%c%p->%g)",s,eqt->z.pval.xp,eqt->z.pval.xp[0]);      break;
	      
	    case OPadd:	      fprintf(fp,"+"); break;
	    case OPsub:	      fprintf(fp,"-"); break;
	    case OPmul:	      fprintf(fp,"*"); break;
	    case OPdiv:	      fprintf(fp,"/"); break;
	    case OPexp:	      fprintf(fp,"^"); break;
	    case OPopen:      fprintf(fp,"{"); break;
 	    case OPclose:     fprintf(fp,"}"); break;
	    case OPnorm:      fprintf(fp,":norm:"); break;  /*  A norm B == norm_B(A) */         
	    default: break;
	    }
	}
      if(eqt->op==OPeol_const)
	fprintf(fp,"%g",eqt->z.val.x);
      if(eqt->op==OPeol_valp)
	fprintf(fp,"%p->%g",eqt->z.pval.xp, (eqt->z.pval.xp==NULL)?0.0:*(eqt->z.pval.xp));
    }
}




static void *eqn_malloc(int size)
{
  void *p=calloc(1,size);
  return p;
}
static char *eqn_strdup(char *p)
{
  return strdup(p);
}

static void eqn_free(void *p)
{ if(p!=NULL) free(p); }



static eqntoken_t *eqntoken_next(eqntoken_t *p)
{
  eqntoken_t *r;
  switch(p->op) 
    { 
    case OPeolist: case OPeol_const: case OPeol_valp: case OPeol_qty: 
      r=p; break; 
    case OPval:   case OPvalm: r= (void *) (((char *)p)+(sizeof(itemop_t)+sizeof(eqntokenval_t) )) ; break; 
    case OPlit:   case OPlitm: 
    case OPlitv:  case OPlitvm: r= (void *) (((char *)p)+(sizeof(itemop_t)+sizeof(eqntokenlit_t) )) ; break; 
    default:  r= (void *) (((char *)p)+(sizeof(itemop_t))) ; break; 
    }
  return r;
}

#ifdef UNUSED

static eqntoken_t *eqntoken_last(eqntoken_t *p)
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




static inline char t_peek(eqntop_t *top)
{
  if(top->n_unget>0) { return top->unget[top->n_unget-1]; }
  return top->str[0];
}

static inline char t_eat(eqntop_t *top)
{
  char c;
  c=t_peek(top);
  if(top->n_unget>0) top->n_unget++;
  else top->str++;
  return c;
}

static inline void t_uneat(eqntop_t *top, char a)
{
  if(top->n_unget>(sizeof(top->unget)-1)) assert(0);
  else
    {
      top->unget[top->n_unget]=a;
      top->n_unget++;
    }
}




static float float_parse(eqntop_t *top)
{
  float v=0.0,sign=1,sig=0,rest=0,digit,place,exp;
  uchar a;
  
  while(isspace(t_peek(top))) t_eat(top);
  a=t_peek(top);
  if(a=='-') { sign=-1; t_eat(top); }
  else if(a=='+') { sign=1; t_eat(top); }
  
  if(t_peek(top)=='.') { sig=0; t_eat(top); }
  else 
    {
      sig=0;
      while(isdigit(t_peek(top)))
	{ sig*=10; sig+=(t_eat(top)-'0'); }
    }
  
  if(t_peek(top)=='.') t_eat(top);
  if(isdigit(t_peek(top))) 
    { 
      place=0.1;
      while(isdigit(t_peek(top)))
	{ digit=(t_eat(top)-'0'); rest+=(digit*place); place*=0.1; }
    }

  v=(sig+rest)*sign;
  
  a=t_peek(top);
  
  switch(a)
    {
    case 'f':  t_eat(top); v*=1e-15; break;
    case 'p':  t_eat(top); v*=1e-12; break;
    case 'n':  t_eat(top); v*=1e-9; break;
    case 'u':  t_eat(top); v*=1e-6; break;
    case 'm':  t_eat(top); v*=1e-3; break;
    case 'x':  t_eat(top); v*=1e6;   break;
    case 'e':  t_eat(top); 
      exp=0;
      while(isdigit(t_peek(top)))
	{ exp*=10; exp+=(t_eat(top)-'0'); }
      v*=pow(10.0,exp);
      break;
    }
  if(isalpha(t_peek(top))) /* eat trailing alphanumerics (ie. 15fF <- F is extra alpha) */
    t_eat(top);

  D(10,fprintf(stdout,"NUM[%e]\n",v));
  return v;
}




static int eqntoken_parse(eqntop_t *top)
{
  eqntoken_t *node;
  int i,inquote=0;
  char c;

  c=t_peek(top);
  if(top->nodep>=MAXEQNSTACK)return 0;
  
  node=top->nodes+top->nodep++;
  node->z.lit.lit=NULL;
  switch(c)
    {
    case EQN_DELIM: t_eat(top);  /* fall through */
    case 0:   node->op=OPeolist;   return 0;
    case '+': node->op=OPadd; t_eat(top); top->opc++; break;
    case '-': node->op=OPsub; t_eat(top); top->opc++; break;
    case '*': node->op=OPmul; t_eat(top); top->opc++; break;
    case '/': node->op=OPdiv; t_eat(top); top->opc++; break;   
    case '^': node->op=OPexp; t_eat(top); top->opc++; break;
    case '(': node->op=OPopen;  t_eat(top); top->opc++; break;   
    case ')': node->op=OPclose; t_eat(top); top->opc++; break;      
    case '$': node->op=OPnorm ; t_eat(top); top->opc++; break;
 
    default:  
      
      if(isdigit(c))
	{
	  node->z.val.x=float_parse(top);
	  node->op=OPval;
	}
      else
	{
	  char *p;
	  p=alloca(strlen(top->str)+sizeof(top->unget)+16);
	  
	  if(c=='{') { inquote++; t_eat(top); c=t_peek(top); }
	  
	  for(i=0;c!=0;c=t_peek(top),i++)
	    {
	      if(!inquote) { if(strchr("\'+-/*^\n\r()",c)!=NULL)break; }
	      if(inquote && (c=='{')) inquote++;
	      if(inquote && (c=='}')) { inquote--; if(inquote==0) { t_eat(top); break; }} 
	      
	      p[i]=t_eat(top);
	      p[i+1]=0;
	    }	  
	  D(10,fprintf(stdout,"LIT[%s]\n",p));
	  node->z.lit.lit=eqn_strdup(p);
	  node->op=OPlit;  
	}
      top->litc++;
      break;
    }
  return 1;
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
      D(3,fprintf(stdout,"loaded pval=(%p->%g)\n",token->z.pval.xp,a));
      break;
    case OPeol_const: a=token->z.val.x;      break;
    case OPlit:   a= 0.0; r++;               break;
    case OPlitm:  a=-0.0; r++;               break;
    case OPlitv:  a= *(token->z.pval.xp);      break;
    case OPlitvm: a=-(*(token->z.pval.xp));   break; 
    case OPval:   a=  token->z.val.x;        break;
    case OPvalm:  a=-(token->z.val.x);       break;
    default: assert(0);
    }  
  *v=a;
  return r;
}
#define PUSH(a)  do { if(data->stackp<MAXSTACK) data->stack[data->stackp++]=(a); \
                      D(5,fprintf(stdout,"Push(%g) new stackp=%i\n",(a),data->stackp)); } while (0)
#define POP(a)   do { if(data->stackp>0) (a)=data->stack[--(data->stackp)]; \
                      D(5,fprintf(stdout,"Pop(%g) new stackp=%i\n",(a),data->stackp)); } while (0)

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
      
      D(8,fprintf(stdout,"Eval: lookahead = "));
      for(i=0;i<4;i++)
	{
	  char *syms[]=OPsyms;
	  D(8,fprintf(stdout," %s,t%i,p%i" ,syms[ops[i]],types[i],precs[i]));
	}
      D(8,fprintf(stdout,"\n"));
      
      if(OP_ISANYV(token->op))
	{ 
	  r=token_value(token,&a);
	  D(5,fprintf(stdout,"Value: %g\n",a));
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
	      D(6,fprintf(stdout,"Shift: recursion\n"));
	      
	      eqntoken_eval(data);
	      
	      
	      POP(b);
	    }
	  else /* REDUCE */
	    {
	      D(6,fprintf(stdout,"Reduce: \n"));
	      if( OP_ISANYV(ops[1]) )
		{
		  data->list=eqntoken_next(data->list);
		  r+=token_value(tokens[1],&b);
		}
	      else { /* error, need literal */ assert(0); }
	    }
	  
	  D(6,fprintf(stdout,"Doing Binary Op, %g op %g \n",a,b));
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
	    case OPnorm:    if(b!=0) a=pow(pow(a,b),1/b); else a=0;  break;
	      
	    default: assert(0);
	    }
	  PUSH(a);
	  
	}
      else
	if(types[0]==4) /* open/close */
	  {
	    if(token->op==OPopen)
	      {
		D(6,fprintf(stdout,"open { : \n"));
		loop=1;
	      }
	    else if(token->op==OPclose)
	      {
		D(6,fprintf(stdout,"close } : returning \n"));
		return 0;
	      }
	    
	  }
	else if(token->op == OPeolist);
      
      if(toend) { D(6,fprintf(stdout,"Ending:  \n")); return 0; } 
      else
	{
	  D(6,fprintf(stdout,"Stack: entry=%i now=%i  %s \n",stack_entry,data->stackp, 
			  ((stack_entry<data->stackp)||loop)?"looping":"returning")); 
	}
    }
  while((stack_entry<data->stackp)||loop);
  return r;
}

#undef PUSH
#undef POP


static eqntoken_t *eqntok_parse(char *str)
{
  int i,j,qlit,qval,qvalp;
  int last_was_op;
  int type[]=OPtype;
  eqntoken_t *list,*lp,*lpn;
  eqntop_t data;
  eqntoken_t *cp,*np;
  memset(&data,0,sizeof(data));
  data.n_unget=0;
  data.nodep=0;
  data.litc=0;
  data.opc=0;
  data.str=str;
  
  D(2,fprintf(stdout,"Parsing [%s]\n",str));
  data.errors=0;
  if(data.str[0]==EQN_DELIM) data.str++; /* eat equation prefix delimiter */
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

  /* fixme. undo bitrot */
#ifdef BITROT
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
      fprintf(stdout,"Invalid Equation \n");
      assert(0);
    }
  else
#endif
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
	}
      lp->op=OPeolist;
    }

  D(7,
    do {
      char *syms[]=OPsyms;
      fprintf(stdout, "eqnparse: %i tokens %p  ", data.nodep,lp);
      for(lp=list;!OP_ISEND(lp->op);lp=eqntoken_next(lp))
	fprintf(stdout, "%s ",syms[lp->op]);
      fprintf(stdout, "%s ",syms[lp->op]);
      fprintf(stdout,"\n");
    } while (0)
    );

  return list;
}





/* only evaluate after eqntok_depend, and
   call eqntok_eval IN PROPER ORDER
   returns 0 if eval ok, 1 if broken dependency, 2 on parse error
*/

static int eqntok_eval(float *res, eqntoken_t *list)
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
    D(3,debug_eqn(stdout,"Evaluating \n",&v));
  }

  /* check that all dependencies have been taken care of */
  for(lp=list;!OP_ISEND(lp->op);lp=eqntoken_next(lp))
    {
      if((lp->op==OPlit)||(lp->op==OPlitm))return 1;
    }

  eqntoken_eval(&data);

  if(res!=NULL)
    *res=data.stack[1];
  
  if(data.stackp==2)return 0;
  else return 2;
}


#ifdef UNUSED
static eqntoken_t *eqntok_copy(eqntoken_t *p)
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


static eqntoken_t *eqntok_copy_m(eqntoken_t *p, float m)
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







eqn_t equation_parse(uchar *str)
{
  eqn_t eqn;
  eqn.val=0.0;
  eqn.eqn=eqntok_parse(str);
  return eqn;
}





/* returns 0 if equation can be evaluated */
int equation_depend(eqn_t eqn, float *(lookup)(void *user, char *str), void *user)
{
  int q=0;
  float *vp;
  eqntoken_t *lp,*list;
  list=eqn.eqn;
  if(list==NULL)return -1;
 
  if(list->op==OPeol_const)return 0; /*optimization */
  
  for(lp=list;!OP_ISEND(lp->op);lp=eqntoken_next(lp))
    {
      if((lp->op==OPlit)||(lp->op==OPlitm))
	{	  
	  D(50,fprintf(stdout,"eqntok_depend: Looking up %s\n",lp->z.lit.lit));
	  if((vp=lookup(user,lp->z.lit.lit))!=NULL)
	    {
	      eqn_free(lp->z.lit.lit);
	      lp->z.pval.xp=vp;
	      lp->op=(lp->op==OPlit)?OPlitv:OPlitvm;
	    }
	  else q++;
	}
    }
  return q;
}


int equation_eval(eqn_t *peqn)
{
  int r=-1;
  if(peqn->eqn!=NULL)
    {
      r=eqntok_eval(&(peqn->val), peqn->eqn);
      D(1,fprintf(stdout,"Result: %g\n",peqn->val));      
    }
  return r;
}

eqn_t equation_empty(eqn_t eqn)
{
  eqntoken_t *eqt;
 /* should traverse equations first to free any literal strings */
  
  for(eqt=eqn.eqn;(eqt!=NULL);eqt=eqntoken_next(eqt))
    {
      switch(eqt->op)
	{
	case OPlit:
	case OPlitm:
	  eqn_free(eqt->z.lit.lit);
	  break;
	default: break;
	}
      if(OP_ISEND(eqt->op)) break;
    }
  eqn_free(eqn.eqn);
  eqn.eqn=NULL;
  eqn.val=0.0;
  return eqn;
}


void equation_debug(eqn_t eqn, void *fp)
{
  char buf[64];
  sprintf(buf,"eqn=%g \n    ",eqn.val);
  debug_eqn(fp, buf, &eqn);
}

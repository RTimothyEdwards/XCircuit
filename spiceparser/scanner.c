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
/* scanner.c, support for scanning text, tokenifies input, uses file_readline
   Conrad Ziesler
*/


/* goal: support cad tool file reading

   cad tool requirements:  large file size, regular file handling
   needs to:  minimize memory allocations, prevent memory leaks
   should provide:  allow higher up layers to never need to call strdup
                    unless they really want to. that is we alloc in place

*/

#include <ctype.h>
#include <stdio.h>
#include "debug.h"

#include "scanner.h"






/****  standard error and warning reporting ************/

static void parse_warn_error(int which, scanner_t *scan, char *fmt, va_list va)
{

  static char buf[4096];
  card_t *c=scan->errcard;
  deck_t *d=scan->errdeck;
  char *bp=buf;
  char *warnerr="WARNING";
  int line=-1;
  int file=-1;

  if(d==NULL)
    {
      if (scan->inputp!=NULL) 
        {
	  line=scan->inputp->line; 
	  file=scan->inputp->index;
	}
    }
  else { line=d->line.line; file=d->line.fileindex; }

  if(which) warnerr="ERROR";
  bp[0]=0;

  if(c!=NULL)
       sprintf(bp,"line %i: %s:  near %s : ",line,warnerr,c->str);
  else sprintf(bp,"line %i: %s: :",line,warnerr);

  if(d!=NULL)
    for(c=d->card;c!=NULL;c=c->next)
      {
	bp+=strlen(bp);
	if(bp>(buf+sizeof(buf)-256)) assert(0);
	if(c->val!=NULL)sprintf(bp,"%s=%s ",c->str,c->val); 
	else sprintf(bp,"%s ",c->str);
      }
  bp+=strlen(bp);
  sprintf(bp,"\n --> %s",fmt);
  bp+=strlen(bp);
  sprintf(bp,"\n");
  bp+=strlen(bp);
  if(which==0)
    if(scan->warnfunc!=NULL)
      {  scan->warnfunc(scan,buf,va); return; }
  
  if(scan->errfunc!=NULL)
    scan->errfunc(scan,buf,va); 
}

void parse_error (scanner_t *scan, char *fmt, ...)
{
  va_list va;
  va_start(va,fmt);
  parse_warn_error(1,scan,fmt,va);
  va_end(va);
}

void parse_warn  (scanner_t *scan, char *fmt, ...)
{
  va_list va;
  va_start(va,fmt);
  parse_warn_error(0,scan,fmt,va);
  va_end(va);
}

void scanner_def_err(scanner_t *scan, char *fmt, va_list vp)
{
  vfprintf(stderr,fmt,vp);
}



/***** private helper functions *********/

#ifdef NO_MEMORY
static void free_cards(scanner_t *scan, card_t *c)
{
  card_t *tofree;
  while(c!=NULL)
    {
      tofree=c;
      c=c->next;
      free(tofree);
    }
}


static void deck_free(scanner_t *scan, deck_t *deck)
{
  deck_t *tofree;
  while(deck!=NULL)
    {
      tofree=deck;
      deck=deck->next;
      free_cards(tofree->card);
      free(tofree);
    }
}
#endif


static deck_t *new_deck(scanner_t *scan)
{
  deck_t *d;
  d=memory_alloc(&(scan->strmem),sizeof(deck_t));
  assert(d!=NULL);
  d->next=NULL;
  d->card=NULL;
  d->line=scanner_current_file_line(scan);
  return d;
}


void scanner_add_tokens(scanner_t *scan, tokenmap_t *map)
{
  int i;
  if(scan->sectp==NULL)assert(0);
  else
    {
      for(i=0;map[i].str!=NULL;i++)
	names_add(scan->sectp->tokenizer,map[i].token,map[i].str);
    }
}

void scanner_reset_tokens(scanner_t *scan)
{
  if(scan->sectp!=NULL)
    {
      names_free(scan->sectp->tokenizer);
      scan->sectp->tokenizer=names_new();
    }
  else assert(0);
}


#ifdef OLD_WAY
void scanner_input_new(scanner_t *scan, file_spec_p file)
{
  scanner_input_t *p; 
  p=malloc(sizeof(scanner_input_t));
  if(p!=NULL)memset(p,0,sizeof(scanner_input_t));
  else assert(0);
  p->file=file;
  file_openread(file);
  p->line=0;
  if(scan->allinputs==NULL)
    p->index=0;
  else
    p->index=scan->allinputs->index+1;
  p->next=scan->allinputs;
  scan->allinputs=p;
  p->back=scan->inputp;
  scan->inputp=p;
}
#endif



void scanner_input_newfp(scanner_t *scan, void *fp)
{
  scanner_input_t *p; 
  p=malloc(sizeof(scanner_input_t));
  if(p!=NULL)memset(p,0,sizeof(scanner_input_t));
  else assert(0);
  p->file_fp=fp;
  p->line=0;
  if(scan->allinputs==NULL)
    p->index=0;
  else
    p->index=scan->allinputs->index+1;
  p->next=scan->allinputs;
  scan->allinputs=p;
  p->back=scan->inputp;
  scan->inputp=p;
}


void scanner_sect_new(scanner_t *scan, scanner_def_t *defs, tokenmap_t *map)
{
  scanner_sect_t *sect;

  sect=malloc(sizeof(scanner_sect_t));
  if(sect!=NULL)memset(sect,0,sizeof(scanner_sect_t));
  else assert(0);
  
  sect->tokenizer=names_new();
  sect->def=defs;
  sect->cp=NULL;
  sect->dp=NULL;
  sect->dhead=NULL;
  sect->eolstring=0;
  sect->line_cont=0;
  sect->lbuf[0]=0;
  sect->eoline[0]=0;
  sect->back=scan->sectp;
  scan->sectp=sect;
  scanner_add_tokens(scan,map);
}


void scanner_input_release(scanner_t *scan) /* release top of stack */
{
  scanner_input_t *p;
  if((p=scan->inputp)!=NULL)
    {
      scan->inputp=scan->inputp->back;
      /* we don't free, we keep on list,
	 for error reporting
      */
    }
}

void scanner_sect_release(scanner_t *scan)  /* release top of stack */
{
  scanner_sect_t *p;

  if((p=scan->sectp)!=NULL)
    {
      scan->sectp=scan->sectp->back;
      names_free(p->tokenizer);
      free(p);
    }
}

void scanner_init(scanner_t *scan)
{
  memory_init(&(scan->strmem));
  scan->sectp=NULL;
  scan->allinputs=NULL;
  scan->inputp=NULL;
  scan->errdeck=NULL;
  scan->errcard=NULL;
  scan->errfunc=scanner_def_err;
  scan->warnfunc=scanner_def_err;
}


void scanner_free_all(scanner_t *scan)
{
  memory_freeall(&(scan->strmem));
}

void scanner_release(scanner_t *scan)
{
  scanner_input_t *p,*np;
  
  while(scan->sectp!=NULL)
    scanner_sect_release(scan);
  
  while(scan->inputp!=NULL)
    scanner_input_release(scan);
  
  for(p=scan->allinputs;p!=NULL;p=np)
    {
      np=p->next;
      if(p->file_fp!=NULL) fclose(p->file_fp);
      free(p);
    }

  scanner_free_all(scan);
}




int scanner_checkvalid(deck_t *d, int qty)
{
  int i;
  card_t *c;
  if(d==NULL)return 0;
  for(i=0,c=d->card;c!=NULL;c=c->next,i++)
    { if(i>=qty)break; }
  
  if(i>=qty)return 1;
  return 0;
}








/***** the actual scanner routines *****/


static char *skip_stuff(char *toskip, char *str, int sense)
{
  char *cp;
  char *ws;
  for(cp=str;cp[0]!=0;cp++)
    {
      for(ws=toskip;ws[0]!=0;ws++)
	if(ws[0]==cp[0]) break;
      if( (ws[0]==0) && (sense==0) ) break; /* no toskip char found */
      if( (ws[0]!=0) && (sense!=0) ) break; /* toskip char found */
    }
  return cp;
}
  
static void skip_space(scanner_t *scan, char **p)
{  *p=skip_stuff(scan->sectp->def->whitespace,*p,0); }

#if 0
static void skip_nonspace(scanner_t *scan, char **p)
{  *p=skip_stuff(scan->sectp->def->whitespace,*p,1); }
#endif

static void skip_nonspace_quote(scanner_t *scan, char **p)
{
  scanner_def_t *def=scan->sectp->def;
  int quote=0;
  char *cp;
  char *ws;
  cp=*p;
  /* this is set to 2 origninally since first iteration hits same character */
  if(cp[0]==def->quote_char)quote=2;
  for(;cp[0]!=0;cp++)
    {
      if(cp[0]==def->quote_char){ quote--; if(quote==0){ if(cp[0]!=0)cp++; break; } }
      if(quote<=0)
	{
	  for(ws=def->whitespace;ws[0]!=0;ws++)
	    if(ws[0]==cp[0]) break;
	  if( (ws[0]!=0) ) break;  /* whitespace char found */
	}
      
    }
  *p=cp;
}


static int astreq(char *s, char *lng)
{
  if(s[0]==0)return 2;
  while(s[0]!=0)
    {
      if(lng[0]==0)return -1;
      if(s[0]!=lng[0])return 1;
      s++;
      lng++;
    }
  return 0;
}


static card_t *make_card(scanner_t *scan, char *p1, char *p2, char *p3, char *p4)
{
  scanner_def_t *def=scan->sectp->def;
  card_t *c;
  int l1=0,l2=0,lc;
  int i;

  if( (p1!=NULL) && (p2!=NULL)) l1=p2-p1;
  if( (p3!=NULL) && (p4!=NULL)) l2=p4-p3;

  assert(l1>=0);
  assert(l2>=0);
  lc=strlen(def->eol_continue);

  if(p1[0]==0) return NULL; /* nothing here */
  
  if(lc <= l1)
    if(!astreq(def->eol_continue,p2-lc))
      {
	if(lc < l1 )
	  {
	    memcpy(scan->sectp->eoline,p1,l1-lc);
	    scan->sectp->eoline[l1]=def->assignment;
	    scan->sectp->eoline[l1+1]=0;
	    scan->sectp->eolstring=1;
	  }
	return NULL; 
      }
  
  if(p3!=NULL)
    {
      if((p3[0]==0)||(!astreq(def->eol_continue,p3)))
	{
	  memcpy(scan->sectp->eoline,p1,l1);
	  scan->sectp->eoline[l1]=def->assignment;
	  scan->sectp->eoline[l1+1]=0;
	  scan->sectp->eolstring=1;
	  return NULL;
	}
    }
  
  c=memory_alloc(&(scan->strmem),sizeof(card_t)+l1+l2+2);
  assert(c!=NULL);
  c->next=NULL;
  c->token=0;
  c->str[0]=0;
  c->val=NULL;
  
  memcpy(c->str,p1,l1);
  c->str[l1]=0;
  if(l2!=0)
    {
      memcpy(c->str+l1+1,p3,l2);
      c->str[l1+1+l2]=0;
      c->val=c->str+l1+1;
    }

  if(def->convert_case==PARSE_CASE_TOLOWER)
    for(i=0;i<(l1+l2+2);i++)
      c->str[i]=tolower(c->str[i]);
  else if(def->convert_case==PARSE_CASE_TOUPPER)
    for(i=0;i<(l1+l2+2);i++)
      c->str[i]=toupper(c->str[i]);
 
 
  if((def->tokenize[0]==0) || (!astreq(def->tokenize,c->str)))
    c->token=names_check(scan->sectp->tokenizer,c->str);
  
  if(c->val!=NULL)
    if(c->val[0]==scan->sectp->def->quote_char)c->val++;
  
  return c;
}





int scanner_parse_line(scanner_t *scan)
{
  scanner_def_t *def=scan->sectp->def;
  char *line=scan->sectp->lbuf;
  char *lp, *tmp;
  char *p1=NULL,*p2,*p3,*p4,*p5,*p6;
  card_t *ncp=NULL;
  int dbg=0;

  if(scan->inputp==NULL)return 0;
  if(scan->inputp->file_fp!=NULL) { if(feof(((FILE *)scan->inputp->file_fp)))return 0; }
  /* return 0;  else { if(file_eof(scan->inputp->file)) return 0; } */
  
  line[0]=0;
  line[1]=0;
  line[sizeof(scan->sectp->lbuf)-1]=0;
  if(scan->inputp->file_fp!=NULL)
    fgets(line,sizeof(scan->sectp->lbuf)-2,scan->inputp->file_fp);
  else
    ; /* file_readline(scan->inputp->file,sizeof(scan->sectp->lbuf)-2,line); */

  scan->inputp->line++;
  if(line[0]==0)return 3;
  
  /* read one line */ 
  if(dbg)fprintf(stderr,"line: %s",line);
  lp=line;
  
  skip_space(scan,&lp);
  
  if(lp[0]==0) return 4;
  
  if(!astreq(def->commentstart, lp))return 5;
  if(!astreq(def->line_stop, lp)) return 0; /* stop scanning */
  if(!astreq(def->bol_continue,lp)) /* continue line */ 
    { scan->sectp->line_cont=1; lp+=strlen(def->bol_continue); }
  
  if(dbg)fprintf(stderr,"lp: %s",lp);
  if(dbg)
    {
      int l=strlen(lp);
      if(lp[l-1]=='\n')lp[l-1]=0;
    }

  if(! (scan->sectp->line_cont||scan->sectp->eolstring)) /* make a new deck unless we are continuing */
    {
      deck_t *dp;
      dp=new_deck(scan);
      if(scan->sectp->dhead==NULL)
	{ scan->sectp->dhead=scan->sectp->dp=dp; }
      else
	{ scan->sectp->dp->next=dp; scan->sectp->dp=dp; }
    } 
  scan->sectp->line_cont=0;
  
  /* continue last string if necessary */
  if(scan->sectp->eolstring)
    {
      p1=scan->sectp->eoline;
      skip_space(scan,&p1);
      p2=p1;
      skip_nonspace_quote(scan,&p2);
      p3=p2;
      skip_space(scan,&p3);
      p4=p3;
      skip_nonspace_quote(scan,&p4);
      p5=p4;
      skip_space(scan,&p5);
      p6=p5;
      skip_nonspace_quote(scan,&p6);
      
      if(p5[0]==0)p5=NULL;
      if(p3[0]==0)p3=NULL;
      if(p1[0]==0)p1=NULL;
      scan->sectp->eolstring=0;
    }

  while((lp[0]!=0)||(p1!=NULL))
    {
      int arg=0;
            
      if(p1==NULL)
	{
	  p1=lp;
	  skip_space(scan,&p1);
	  p2=p1;
	  skip_nonspace_quote(scan,&p2);
	  p3=p2;
	  skip_space(scan,&p3);
	  p4=p3;
	  skip_nonspace_quote(scan,&p4);
	  p5=p4;
	  skip_space(scan,&p5);
	  p6=p5;
	  skip_nonspace_quote(scan,&p6);
	}
      else if(p3==NULL)  
	{
	  p3=lp;
	  skip_space(scan,&p3);
	  p4=p3;
	  skip_nonspace_quote(scan,&p4);
	  p5=p4;
	  skip_space(scan,&p5);
	  p6=p5;
	  skip_nonspace_quote(scan,&p6);
	}

      else if(p5==NULL)
	{
	  p5=lp;
	  skip_space(scan,&p5);
	  p6=p5;
	  skip_nonspace_quote(scan,&p6);
	}
	
      if(dbg)
	fprintf(stderr,"doing line: line is [%s]\nlp=[%s]\np1=[%s]\np2=[%s]\np3=[%s]\np4=[%s]\np5=[%s]\np6=[%s]\n\n",
			  line,lp,p1,p2,p3,p4,p5,p6);



      /* cases: 
	     {p1}{p2}{p3}[ ]{p4}{p5}{p6}          -> end of line
	     {p1}text{p2}[ ]{p3}{p4}{p5}{p6}      -> word end of line 
	     {p1}text=moretext{p2}...             ->  one  word embedded assignment 
	     {p1}text={p2}{p3}{p4}{p5}{p6}        -> word assignment spills over line 
	     {p1}={p2} ..                         -> continue assignment from prev line if possible
	     {p1}text={p2} {p3}text{p4}           -> word assign 1
	     {p1}text{p2} {p3}=text{p4}           -> word assign 2
	     {p1}text{p2} {p3}={p4} {p5}text{p6}  -> word assign 3 
	     {p1}text{p2} {p3}text={p4}           -> word 
	     text = EOL {p1}text{p2} ..          -> eol continue
	   */
	  
	  if(p1==p2){ p1=NULL; break; } /* end of line */
	  
	  if(p2>p1)
	    { if(p2[-1]==def->assignment)arg=1; }
	  
	  if((arg==0)&&(p2>p1))
	
	    {
	      if( (tmp=memchr(p1,def->assignment,p2-p1))!=NULL) 
		{
		  arg=5;
		  p2=tmp+1;
		  p3=tmp+1;
		  p4=p3;
		  skip_nonspace_quote(scan,&p4);
		}
	    }
	  
	  if(arg==0)
	    {
	      if(p3[0]==def->assignment)
		arg=2;
	    }



	  switch(arg)
	    {
	    case 0: /* no arg */
	      ncp=make_card(scan,p1,p2,NULL,NULL); 
	      p1=p3; p2=p4; p3=p5; p4=p6; p5=NULL; lp=p6; 
	      break;
	    case 1: /* arg in p3 */
	      ncp=make_card(scan,p1,p2-1,p3,p4);
	      p1=p5; p2=p6; p3=NULL; p5=NULL; lp=p6;
	      break;
	    case 2: /* arg in p3 or p4 */
	      if((p3+1)<p4)
		{ 
		  ncp=make_card(scan,p1,p2,p3+1,p4);
		  p1=p5; p2=p6; p3=NULL; p5=NULL; lp=p6;
		}
	      else
		{
		  ncp=make_card(scan,p1,p2,p5,p6);
		  p1=NULL; p3=NULL; p5=NULL; lp=p6;
		}
	      break;
	    case 5: /* embedded arg, we've fixed up p's */
	      ncp=make_card(scan,p1,p2-1,p3,p4);
	      p1=NULL; p3=NULL; p5=NULL; lp=p4;
	      break;
	    default: assert(0);
	    }
	  
	  if(ncp!=NULL)
	    {
	      if(dbg)fprintf(stderr,"Adding card %s %i %s\n",ncp->str,ncp->token,ncp->val);
	      if(scan->sectp->dp==NULL)assert(0);
	      
	      if(scan->sectp->dp->card==NULL)
		scan->sectp->dp->card=scan->sectp->cp=ncp;
	      else 
		{ 
		  if(scan->sectp->cp!=NULL)
		    {
		      scan->sectp->cp->next=ncp; 
		      scan->sectp->cp=scan->sectp->cp->next;
		    }
		  else assert(0);
		}
	    }
	  else  
	    {
	      if(dbg)fprintf(stderr,"card is null, line is [%s]\nlp=[%s]\np1=[%s]\np2=[%s]\np3=[%s]\np4=[%s]\np5=[%s]\np6=[%s]\n\n",
			  line,lp,p1,p2,p3,p4,p5,p6);


	    }
    }
  
  return 6;
}





int scanner_parse_all(scanner_t *scan)
{
  scan->sectp->eolstring=0;
  scan->sectp->line_cont=0;
  while(scanner_parse_line(scan));
  return 0;
}


scanner_def_t *scanner_def_spice(void)
{
  static scanner_def_t spicedef;
  strcpy(spicedef.line_stop,".end_spice");
  strcpy(spicedef.eol_continue,"\\");
  strcpy(spicedef.bol_continue,"+");
  spicedef.convert_case=PARSE_CASE_TOLOWER;
  spicedef.quote_char='\'';        
  spicedef.newline='\n';          
  spicedef.assignment='=';        
  strcpy(spicedef.tokenize,"."); 
  strcpy(spicedef.whitespace," \t\n\r");   
  strcpy(spicedef.commentstart,"*");   
  return &spicedef;
}

char *scanner_token(scanner_t *scan, int token)
{
  return names_lookup(scan->sectp->tokenizer,token);
}

file_line_t scanner_current_file_line(scanner_t *scan)
{
  file_line_t l;
  l.fileindex=scan->inputp->index;
  l.line=scan->inputp->line;
  return l;
}

void scanner_debug_all(scanner_t *scan, void *dbg)
{
  FILE *fp=dbg;
  int q;
  deck_t *d;
  card_t *c;
 
  fprintf(fp,"\n\n** begin \n\n");
  for(d=scan->sectp->dhead;d!=NULL;d=d->next)
    {
      q=0;
      fprintf(fp,"\nline %i: ",d->line.line);
      for(c=d->card;c!=NULL;c=c->next)
	{
	  if(c->token>0)
	    fprintf(fp," **%i=%s** ",c->token,scanner_token(scan,c->token));
	  
	  if(c->val==NULL)
	    fprintf(fp," [%s] ",c->str); 
	  else fprintf(fp," [%s]=[%s] ",c->str,c->val);
	  q++;
	  if(q>=6){ fprintf(fp,"\n      "); q=1; }
	}
    }

  fprintf(fp,"\n\n** .end\n\n");
}

/* parses 0101.0101.0101011.01 */
unsigned parse_binary(char **str)
{
  unsigned result=0;
  char *p;
  for(p=*str;p[0]!=0;p++)
    {
      if(p[0]=='0')
	result<<=1;
	
      else if(p[0]=='1')
	{ result<<=1; result|=1; }
      
      else if(p[0]=='.');
      else break;
    }
  *str=p;
  return result;
}

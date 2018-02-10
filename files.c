/*-----------------------------------------------------------------------*/
/* files.c --- file handling routines for xcircuit			 */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	 */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef XC_WIN32
#include <pwd.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef XC_WIN32
#include <unistd.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#else
#include <winsock2.h>
#endif

#ifdef TCL_WRAPPER 
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/TextEdit.h"   /* for XwTextCopyBuffer() */
#endif
#endif

/*------------------------------------------------------------------------*/
/* Local includes                                                         */
/*------------------------------------------------------------------------*/

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

#ifdef ASG
extern void Route(XCWindowData *, Boolean);
extern int ReadSpice(FILE *);
#endif

/*------------------------------------------------------------------------*/
/* Useful (local) defines						  */
/*------------------------------------------------------------------------*/

#define OUTPUTWIDTH 80	/* maximum text width of output */

#define S_OBLIQUE   13	/* position of Symbol-Oblique in font array */

/*------------------------------------------------------------------------*/
/* External Variable definitions                                          */
/*------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif

extern char _STR2[250], _STR[150];
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern fontinfo *fonts;
extern short fontcount;
extern Cursor appcursors[NUM_CURSORS];
extern XtAppContext app;
extern Display *dpy;
extern Window win;
extern short beeper;
extern int number_colors;
extern colorindex *colorlist;


/*------------------------------------------------------*/
/* Global variable definitions				*/
/*------------------------------------------------------*/

Boolean load_in_progress = False;
char version[20];

/* Structure for remembering what names refer to the same object */

aliasptr aliastop;
   
/*------------------------------------------------------*/
/* Utility routine---compare version numbers		*/
/* Given version strings v1 and v2, return -1 if	*/
/* version v1 < v2, +1 if version v1 > v2, and 0 if	*/
/* they are equal.					*/
/*------------------------------------------------------*/

int compare_version(char *v1, char *v2)
{
   int vers1, subvers1, vers2, subvers2;

   sscanf(v1, "%d.%d", &vers1, &subvers1);
   sscanf(v2, "%d.%d", &vers2, &subvers2);

   if (vers1 < vers2) return -1;
   else if (vers1 > vers2) return 1;
   else {
      if (subvers1 < subvers2) return -1;
      if (subvers1 > subvers2) return 1;
      else return 0;
   }
}

/*------------------------------------------------------*/
/* Simple utility---get rid of newline character	*/
/*------------------------------------------------------*/

char *ridnewline(char *sptr)
{
   char *tstrp;

   for (tstrp = sptr; *tstrp != '\0' && *tstrp != '\n'; tstrp++);
   if (*tstrp == '\n') *tstrp = '\0';
   return tstrp;
}

/*----------------------------------------------------------------------*/
/* Check if two filenames are equivalent.  This requires separately	*/
/* checking any absolute or relative pathnames in front of the filename	*/
/* as well as the filename itself.					*/
/*----------------------------------------------------------------------*/

#define PATHSEP '/'

int filecmp(char *filename1, char *filename2)
{
   char *root1, *root2, *path1, *path2, *end1, *end2;
   int rval;
   struct stat statbuf;
   ino_t inode1;
   const char *cwdname = ".";

   if (filename1 == NULL || filename2 == NULL) return 1;

   if (!strcmp(filename1, filename2)) return 0;	/* exact match */

   root1 = strrchr(filename1, PATHSEP);
   root2 = strrchr(filename2, PATHSEP);

   if (root1 == NULL) {
      path1 = (char *)cwdname;
      end1 = NULL;
      root1 = filename1;
   }
   else {
      path1 = filename1;
      end1 = root1;
      root1++;
   }

   if (root2 == NULL) {
      path2 = (char *)cwdname;
      end2 = NULL;
      root2 = filename2;
   }
   else {
      path2 = filename2;
      end2 = root2;
      root2++;
   }
   
   if (strcmp(root1, root2)) return 1;	/* root names don't match */

   /* If we got here, one or both filenames specify a directory	*/
   /* path, and the directory paths are different strings.	*/
   /* However, one may be an absolute path and the other a	*/
   /* relative path, so we check the inodes of the paths for	*/
   /* equivalence.  Note that the file itself is not assumed to	*/
   /* exist.							*/

   rval = 1;
   if (end1 != NULL) *end1 = '\0';
   if (stat(path1, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
      inode1 = statbuf.st_ino;
      if (end2 != NULL) *end2 = '\0';
      if (stat(path2, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
	 if (inode1 == statbuf.st_ino)
	    rval = 0;
      }
      if (end2 != NULL) *end2 = PATHSEP;
   }
   if (end1 != NULL) *end1 = PATHSEP;
   return rval;
}

/*--------------------------------------------------------------*/
/* Make sure that a string (object or parameter name) is a	*/
/* valid PostScript name.  We do this by converting illegal	*/
/* characters to the PostScript \ooo (octal value) form.	*/
/*								*/
/* This routine does not consider whether the name might be a	*/
/* PostScript numeric value.  This problem is taken care of by	*/
/* having the load/save routines prepend '@' to parameters and	*/
/* a technology namespace to object names.			*/
/*								*/
/* If "need_prefix" is TRUE, then prepend "@" to the result	*/
/* string, unless teststring is a numerical parameter name	*/
/* (p_...).							*/
/*--------------------------------------------------------------*/

char *create_valid_psname(char *teststring, Boolean need_prefix)
{
   int i, isize, ssize;
   static char *optr = NULL;
   char *sptr, *pptr;
   Boolean prepend = need_prefix;
   char illegalchars[] = {'/', '}', '{', ']', '[', ')', '(', '<', '>', ' ', '%'};

   /* Check for illegal characters which have syntactical meaning in */
   /* PostScript, and the presence of nonprintable characters or     */
   /* whitespace.		     				     */

   ssize = strlen(teststring);
   isize = ssize;

   if (need_prefix && !strncmp(teststring, "p_", 2))
      prepend = FALSE;
   else
      isize++;

   for (sptr = teststring; *sptr != '\0'; sptr++) {
      if ((!isprint(*sptr)) || isspace(*sptr))
	 isize += 3;
      else {
         for (i = 0; i < sizeof(illegalchars); i++) {
	    if (*sptr == illegalchars[i]) {
	       isize += 3;
	       break;
	    }
	 }
      }
   }
   if (isize == ssize) return teststring;
   isize++;

   if (optr == NULL)
      optr = (char *)malloc(isize);
   else
      optr = (char *)realloc(optr, isize);

   pptr = optr;

   if (prepend) *pptr++ = '@';

   for (sptr = teststring; *sptr != '\0'; sptr++) {
      if ((!isprint(*sptr)) || isspace(*sptr)) {
	 sprintf(pptr, "\\%03o", *sptr);
	 pptr += 4;
      }
      else {
         for (i = 0; i < sizeof(illegalchars); i++) {
	    if (*sptr == illegalchars[i]) {
	       sprintf(pptr, "\\%03o", *sptr);
	       pptr += 4;
	       break;
	    }
         }
	 if (i == sizeof(illegalchars))
	    *pptr++ = *sptr;
      }
   }
   *pptr++ = '\0';
   return optr;
}

/*------------------------------------------------------*/
/* Turn a PostScript string with possible backslash	*/
/* escapes into a normal character string.		*/
/*							*/
/* if "spacelegal" is TRUE, we are parsing a PostScript	*/
/* string in parentheses () where whitespace is legal.	*/
/* If FALSE, we are parsing a PostScript name where	*/
/* whitespace is illegal, and any whitespace should be	*/
/* considered the end of the name.			*/
/*							*/
/* "dest" is ASSUMED to be large enough to hold the	*/
/* result.  "dest" is always equal to or smaller than	*/
/* "src" in length.  "size" should be the maximum size	*/
/* of the string, or 1 less that the allocated memory,	*/
/* allowing for a final NULL byte to be added.		*/
/*							*/
/* The fact that "dest" is always smaller than or equal	*/
/* to "src" means that parse_ps_string(a, a, ...) is	*/
/* legal.						*/
/*							*/
/* Return 0 if the result is empty, 1 otherwise.	*/
/*------------------------------------------------------*/

int parse_ps_string(char *src, char *dest, int size, Boolean spacelegal, Boolean strip)
{
   char *sptr = src;
   char *tptr = dest;
   int tmpdig, rval = 0;

   /* Strip leading "@", inserted by XCircuit to	*/
   /* prevent conflicts with PostScript reserved	*/
   /* keywords or numbers.				*/

   if (strip && (*sptr == '@')) sptr++;

   for (;; sptr++) {
      if ((*sptr == '\0') || (isspace(*sptr) && !spacelegal)) {
	 *tptr = '\0';
	 break;
      }
      else {
         if (*sptr == '\\') {
            sptr++;
	    if (*sptr >= '0' && *sptr < '8') {
	       sscanf(sptr, "%3o", &tmpdig);
	       *tptr++ = (u_char)tmpdig;
	       sptr += 2;
	    }
            else
	       *tptr++ = *sptr;
         }
	 else
	    *tptr++ = *sptr;
	 rval = 1;
      }
      if ((int)(tptr - dest) > size) {
	 Wprintf("Warning:  Name \"%s\" in input exceeded buffer length!\n", src);
	 *tptr = '\0';
	 return rval;
      }
   }
   return rval;
}

/*------------------------------------------------------*/
/* Free memory allocated to a label string		*/
/*------------------------------------------------------*/

void freelabel(stringpart *string)
{
   stringpart *strptr = string, *tmpptr;

   while (strptr != NULL) {
      if (strptr->type == TEXT_STRING || strptr->type == PARAM_START)
         free(strptr->data.string);
      tmpptr = strptr->nextpart;
      free(strptr);
      strptr = tmpptr;
   }
}

/*------------------------------------------------------*/
/* Free memory for a single element			*/
/*------------------------------------------------------*/

void free_single(genericptr genobj)
{
   objinstptr geninst;
   oparamptr ops, fops;

   if (IS_POLYGON(genobj)) free(((polyptr)(genobj))->points);
   else if (IS_LABEL(genobj)) freelabel(((labelptr)(genobj))->string);
   else if (IS_GRAPHIC(genobj)) freegraphic((graphicptr)(genobj));
   else if (IS_PATH(genobj)) free(((pathptr)(genobj))->plist);
   else if (IS_OBJINST(genobj)) {
      geninst = (objinstptr)genobj;
      ops = geninst->params;
      while (ops != NULL) {
	 /* Don't try to free data from indirect parameters */
	 /* (That's not true---all data are copied by epsubstitute) */
	 /* if (find_indirect_param(geninst, ops->key) == NULL) { */
	    switch(ops->type) {
	       case XC_STRING:
	          freelabel(ops->parameter.string);
	          break;
	       case XC_EXPR:
	          free(ops->parameter.expr);
	          break;
	    }
	 /* } */
	 free(ops->key);
	 fops = ops;
	 ops = ops->next;
	 free(fops);
      }
   }
   free_all_eparams(genobj);
}

/*---------------------------------------------------------*/
/* Reset an object structure by freeing all alloc'd memory */
/*---------------------------------------------------------*/

void reset(objectptr localdata, short mode)
{
  /* short i; (jdk) */

   if (localdata->polygons != NULL || localdata->labels != NULL)
      destroynets(localdata);

   localdata->valid = False;

   if (localdata->parts > 0) {
      genericptr *genobj;

      if (mode != SAVE) {

	 for (genobj = localdata->plist; genobj < localdata->plist
	        + localdata->parts; genobj++)

	    /* (*genobj == NULL) only on library pages		*/
	    /* where the instances are kept in the library	*/
	    /* definition, and are only referenced on the page.	*/

	    if (*genobj != NULL) {
	       free_single(*genobj);
	       free(*genobj);
	    }
      }
      free(localdata->plist);
      
      removeparams(localdata);

      initmem(localdata);
      if (mode == DESTROY)
	 free(localdata->plist);
   }
}

/*---------------------------------------------------------*/

void pagereset(short rpage)
{
   /* free alloc'd filename */

   if (xobjs.pagelist[rpage]->filename != NULL)
      free(xobjs.pagelist[rpage]->filename);
   xobjs.pagelist[rpage]->filename = (char *)NULL;

   if (xobjs.pagelist[rpage]->background.name != NULL)
      free(xobjs.pagelist[rpage]->background.name);
   xobjs.pagelist[rpage]->background.name = (char *)NULL;

   clearselects();

   /* New pages pick up their properties from page 0, which can be changed */
   /* from the .xcircuitrc file on startup (or loaded from a script).	   */
   /* Thanks to Norman Werner (norman.werner@student.uni-magdeburg.de) for */
   /* pointing out this more obvious way of doing the reset, and providing */
   /* a patch.								   */

   xobjs.pagelist[rpage]->wirewidth = xobjs.pagelist[0]->wirewidth;
   xobjs.pagelist[rpage]->orient = xobjs.pagelist[0]->orient;
   xobjs.pagelist[rpage]->pmode = xobjs.pagelist[0]->pmode;
   xobjs.pagelist[rpage]->outscale = xobjs.pagelist[0]->outscale;
   xobjs.pagelist[rpage]->drawingscale.x = xobjs.pagelist[0]->drawingscale.x;
   xobjs.pagelist[rpage]->drawingscale.y = xobjs.pagelist[0]->drawingscale.y;
   xobjs.pagelist[rpage]->gridspace = xobjs.pagelist[0]->gridspace;
   xobjs.pagelist[rpage]->snapspace = xobjs.pagelist[0]->snapspace;
   xobjs.pagelist[rpage]->coordstyle = xobjs.pagelist[0]->coordstyle;
   xobjs.pagelist[rpage]->margins = xobjs.pagelist[0]->margins;

   if (xobjs.pagelist[rpage]->coordstyle == CM) {
      xobjs.pagelist[rpage]->pagesize.x = 595;
      xobjs.pagelist[rpage]->pagesize.y = 842;  /* A4 */
   }
   else {
      xobjs.pagelist[rpage]->pagesize.x = 612;	/* letter */
      xobjs.pagelist[rpage]->pagesize.y = 792;
   }
}

/*---------------------------------------------------------*/

void initmem(objectptr localdata)
{
   localdata->parts = 0;
   localdata->plist = (genericptr *)malloc(sizeof(genericptr));
   localdata->hidden = False;
   localdata->changes = 0;
   localdata->params = NULL;

   localdata->viewscale = 0.5;

   /* Object should not reference the window:  this needs to be rethunk! */
   if (areawin != NULL) {
      localdata->pcorner.x = -areawin->width;
      localdata->pcorner.y = -areawin->height; 
   }
   localdata->bbox.width = 0;
   localdata->bbox.height = 0;
   localdata->bbox.lowerleft.x = 0;
   localdata->bbox.lowerleft.y = 0;

   localdata->highlight.netlist = NULL;
   localdata->highlight.thisinst = NULL;
   localdata->schemtype = PRIMARY;
   localdata->symschem = NULL;
   localdata->netnames = NULL;
   localdata->polygons = NULL;
   localdata->labels = NULL;
   localdata->ports = NULL;
   localdata->calls = NULL;
   localdata->valid = False;
   localdata->infolabels = False;
   localdata->traversed = False;
}

/*--------------------------------------------------------------*/
/* Exhaustively compare the contents of two objects and return  */
/* true if equivalent, false if not.				*/
/*--------------------------------------------------------------*/

Boolean elemcompare(genericptr *compgen, genericptr *gchk)
{
   Boolean bres;
   switch(ELEMENTTYPE(*compgen)) {
      case(ARC):
	 bres = (TOARC(compgen)->position.x == TOARC(gchk)->position.x &&
            TOARC(compgen)->position.y == TOARC(gchk)->position.y &&
	    TOARC(compgen)->style == TOARC(gchk)->style &&
	    TOARC(compgen)->width == TOARC(gchk)->width &&
	    abs(TOARC(compgen)->radius) == abs(TOARC(gchk)->radius) &&
	    TOARC(compgen)->yaxis  == TOARC(gchk)->yaxis &&
	    TOARC(compgen)->angle1 == TOARC(gchk)->angle1 &&
	    TOARC(compgen)->angle2 == TOARC(gchk)->angle2);
	 break;
      case(SPLINE):
	 bres = (TOSPLINE(compgen)->style == TOSPLINE(gchk)->style &&
	    TOSPLINE(compgen)->width == TOSPLINE(gchk)->width &&
	    TOSPLINE(compgen)->ctrl[0].x == TOSPLINE(gchk)->ctrl[0].x &&
	    TOSPLINE(compgen)->ctrl[0].y == TOSPLINE(gchk)->ctrl[0].y &&
	    TOSPLINE(compgen)->ctrl[1].x == TOSPLINE(gchk)->ctrl[1].x &&
	    TOSPLINE(compgen)->ctrl[1].y == TOSPLINE(gchk)->ctrl[1].y &&
	    TOSPLINE(compgen)->ctrl[2].x == TOSPLINE(gchk)->ctrl[2].x &&
	    TOSPLINE(compgen)->ctrl[2].y == TOSPLINE(gchk)->ctrl[2].y &&
	    TOSPLINE(compgen)->ctrl[3].x == TOSPLINE(gchk)->ctrl[3].x &&
	    TOSPLINE(compgen)->ctrl[3].y == TOSPLINE(gchk)->ctrl[3].y);
	 break;
      case(POLYGON): {
	 int i;
	 if (TOPOLY(compgen)->style == TOPOLY(gchk)->style &&
	       TOPOLY(compgen)->width == TOPOLY(gchk)->width &&
	       TOPOLY(compgen)->number == TOPOLY(gchk)->number) {
	    for (i = 0; i < TOPOLY(compgen)->number; i++) {
	       if (TOPOLY(compgen)->points[i].x != TOPOLY(gchk)->points[i].x
		     || TOPOLY(compgen)->points[i].y != TOPOLY(gchk)->points[i].y)
		  break;
	    }
	    bres = (i == TOPOLY(compgen)->number);
	 }
	 else bres = False;
	 }break;
   }
   return bres;
}

/*--------------------------------------------------------------*/
/* Compare any element with any other element.			*/
/*--------------------------------------------------------------*/

Boolean compare_single(genericptr *compgen, genericptr *gchk)
{
   Boolean bres = False;

   if ((*gchk)->type == (*compgen)->type) {
      switch(ELEMENTTYPE(*compgen)) {
	 case(OBJINST):{
	    objinst *newobj = TOOBJINST(compgen);
	    objinst *oldobj = TOOBJINST(gchk);
	    bres = (newobj->position.x == oldobj->position.x &&
	    		newobj->position.y == oldobj->position.y &&
	     		newobj->rotation == oldobj->rotation &&
	     		newobj->scale == oldobj->scale &&
	     		newobj->style == oldobj->style &&
	     		newobj->thisobject == oldobj->thisobject);
	    } break;
	 case(LABEL):
	    bres = (TOLABEL(compgen)->position.x == TOLABEL(gchk)->position.x &&
	    		TOLABEL(compgen)->position.y == TOLABEL(gchk)->position.y &&
	     		TOLABEL(compgen)->rotation == TOLABEL(gchk)->rotation &&
	     		TOLABEL(compgen)->scale == TOLABEL(gchk)->scale &&
	     		TOLABEL(compgen)->anchor == TOLABEL(gchk)->anchor &&
			TOLABEL(compgen)->pin == TOLABEL(gchk)->pin &&
	     		!stringcomp(TOLABEL(compgen)->string, TOLABEL(gchk)->string));
	    break;
	 case(PATH): /* elements *must* be in same order for a path */
	    bres = (TOPATH(compgen)->parts == TOPATH(gchk)->parts &&
			TOPATH(compgen)->style == TOPATH(gchk)->style &&
			TOPATH(compgen)->width == TOPATH(gchk)->width);
	    if (bres) {
	       genericptr *pathchk, *gpath;
	       for (pathchk = TOPATH(compgen)->plist, gpath =
			   TOPATH(gchk)->plist; pathchk < TOPATH(compgen)->plist
			   + TOPATH(compgen)->parts; pathchk++, gpath++) {
		  if (!elemcompare(pathchk, gpath)) bres = False;
	       }
	    }
	    break;
	 case(ARC): case(SPLINE): case(POLYGON):
	    bres = elemcompare(compgen, gchk);
	    break;
      }
   }
   return bres;
}

/*--------------------------------------------------------------------*/

short objcompare(objectptr obja, objectptr objb)
{
   genericptr *compgen, *glist, *gchk, *remg;
   short      csize;
   Boolean    bres;

   /* quick check on equivalence of number of objects */

   if (obja->parts != objb->parts) return False;

   /* check equivalence of parameters.  Parameters need not be in any	*/
   /* order; they must only match by key and value.			*/

   if (obja->params == NULL && objb->params != NULL) return False;
   else if (obja->params != NULL && objb->params == NULL) return False;
   else if (obja->params != NULL || objb->params != NULL) {
     oparamptr opsa, opsb;
     for (opsa = obja->params; opsa != NULL; opsa = opsa->next) {
	 opsb = match_param(objb, opsa->key);
	 if (opsb == NULL) return False;
	 else if (opsa->type != opsb->type) return False;
	 switch (opsa->type) {
	    case XC_STRING:
	       if (stringcomp(opsa->parameter.string, opsb->parameter.string))
		  return False;
	       break;
	    case XC_EXPR:
	       if (strcmp(opsa->parameter.expr, opsb->parameter.expr))
		  return False;
	       break;
	    case XC_INT: case XC_FLOAT:
	       if (opsa->parameter.ivalue != opsb->parameter.ivalue)
		  return False;
	       break;
	 }
      }
   }

   /* For the exhaustive check we must match component for component. */
   /* Best not to assume that elements are in same order for both.    */

   csize = obja->parts;

   glist = (genericptr *)malloc(csize * sizeof(genericptr));
   for (compgen = objb->plist; compgen < objb->plist + csize; compgen++)
      (*(glist + (int)(compgen - objb->plist))) = *compgen;
   for (compgen = obja->plist; compgen < obja->plist + obja->parts;
	 compgen++) {
      bres = False;
      for (gchk = glist; gchk < glist + csize; gchk++) {
	 if ((*compgen)->color == (*gchk)->color)
	    bres = compare_single(compgen, gchk);
	 if (bres) {
	   csize--;
	   for (remg = gchk; remg < glist + csize; remg++)
               *remg = *(remg + 1);
           break;
	 }
      }
   }
   free(glist);
   if (csize != 0) return False;

   /* Both objects cannot attempt to set an associated schematic/symbol to  */
   /* separate objects, although it is okay for one to make the association */
   /* and the other not to.						    */

   if (obja->symschem != NULL && objb->symschem != NULL)
      if (obja->symschem != objb->symschem)
	 return False;

   return(True);
}

/*------------------------*/
/* scale renormalization  */
/*------------------------*/

float getpsscale(float value, short page)
{
   if (xobjs.pagelist[page]->coordstyle != CM)
      return (value * INCHSCALE);
   else
      return (value * CMSCALE);
}

/*---------------------------------------------------------------*/
/* Keep track of columns of output and split lines when too long */
/*---------------------------------------------------------------*/

void dostcount(FILE *ps, short *count, short addlength)
{
   *count += addlength;
   if (*count > OUTPUTWIDTH) {
      *count = addlength;
      fprintf(ps, "\n");
   }
}

/*----------------------------------------------------------------------*/
/* Write a numerical value as a string to _STR, making a parameter	*/
/* substitution if appropriate.						*/
/* Return 1 if a parameter substitution was made, 0 if not.		*/
/*----------------------------------------------------------------------*/

Boolean varpcheck(FILE *ps, short value, objectptr localdata, int pointno,
	short *stptr, genericptr thiselem, u_char which)
{
   oparamptr ops;
   eparamptr epp;
   Boolean done = False;

   for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
      if ((epp->pdata.pointno != -1) && (epp->pdata.pointno != pointno)) continue;
      ops = match_param(localdata, epp->key);
      if (ops != NULL && (ops->which == which)) {
	 sprintf(_STR, "%s ", epp->key);
	 done = True;
	 break;
      }
   }
      
   if (!done) {
      if (pointno == -1) return done;
      sprintf(_STR, "%d ", (int)value);
   }
   else if ((epp->pdata.pointno == -1) && (pointno >= 0)) {
      sprintf(_STR, "%d ", (int)value - ops->parameter.ivalue);
   }

   dostcount (ps, stptr, strlen(_STR));
   fputs(_STR, ps);
   return done;
}

/*----------------------------------------------------------------------*/
/* like varpcheck(), but without pointnumber				*/
/*----------------------------------------------------------------------*/

void varcheck(FILE *ps, short value, objectptr localdata,
	short *stptr, genericptr thiselem, u_char which)
{
   varpcheck(ps, value, localdata, 0, stptr, thiselem, which);
}

/*----------------------------------------------------------------------*/
/* like varcheck(), but for floating-point values			*/
/*----------------------------------------------------------------------*/

void varfcheck(FILE *ps, float value, objectptr localdata, short *stptr,
	genericptr thiselem, u_char which)
{
   oparamptr ops;
   eparamptr epp;
   Boolean done = False;

   for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
      ops = match_param(localdata, epp->key);
      if (ops != NULL && (ops->which == which)) {
	 sprintf(_STR, "%s ", epp->key);
	 done = True;
	 break;
      }
   }
   
   if (!done)
      sprintf(_STR, "%3.3f ", value);

   dostcount (ps, stptr, strlen(_STR));
   fputs(_STR, ps);
}

/*----------------------------------------------------------------------*/
/* Like varpcheck(), for path types only.				*/
/*----------------------------------------------------------------------*/

Boolean varpathcheck(FILE *ps, short value, objectptr localdata, int pointno,
	short *stptr, genericptr *thiselem, pathptr thispath, u_char which)
{
   oparamptr ops;
   eparamptr epp;
   Boolean done = False;

   for (epp = thispath->passed; epp != NULL; epp = epp->next) {
      if ((epp->pdata.pathpt[0] != -1) && (epp->pdata.pathpt[1] != pointno)) continue;
      if ((epp->pdata.pathpt[0] != -1) && (epp->pdata.pathpt[0] !=
		(short)(thiselem - thispath->plist))) continue;
      ops = match_param(localdata, epp->key);
      if (ops != NULL && (ops->which == which)) {
	 sprintf(_STR, "%s ", epp->key);
	 done = True;
	 break;
      }
   }
      
   if (!done) {
      if (pointno == -1) return done;
      sprintf(_STR, "%d ", (int)value);
   }
   else if ((epp->pdata.pathpt[0] == -1) && (pointno >= 0)) {
      sprintf(_STR, "%d ", (int)value - ops->parameter.ivalue);
   }
   dostcount (ps, stptr, strlen(_STR));
   fputs(_STR, ps);
   return done;
}

/* Structure used to hold data specific to each load mode.  See	*/
/* xcircuit.h for the list of load modes (enum loadmodes)	*/

typedef struct _loaddata {
    void (*func)();		/* Routine to run to load the file */
    char *prompt;		/* Substring name of action, for prompting */
    char *filext;		/* Default extention of file to load */
} loaddata;

/*-------------------------------------------------------*/
/* Load a PostScript or Python (interpreter script) file */
/*-------------------------------------------------------*/

void getfile(xcWidget button, pointertype mode, caddr_t nulldata)
{
   static loaddata loadmodes[LOAD_MODES] = {
	{normalloadfile,  "load",    "ps"},	/* mode NORMAL */
	{importfile,     "import",  "ps"},	/* mode IMPORT */
	{loadbackground, "render",  "ps"},	/* mode PSBKGROUND */
#ifdef HAVE_PYTHON
	{execscript,	"execute",  "py"},
#else
	{execscript,	"execute",    ""},	/* mode SCRIPT */
#endif
	{crashrecover,	"recover",  "ps"},	/* mode RECOVER */
#ifdef ASG
	{importspice,	"import", "spice"},	/* mode IMPORTSPICE */
#endif
#ifdef HAVE_CAIRO
	{importgraphic,	"import", "ppm"},	/* mode IMPORTGRAPHIC */
#endif
   };

   buttonsave *savebutton = NULL;
   char *promptstr = NULL;
   /* char strext[10]; (jdk) */
   int idx = (int)mode;

   if (is_page(topobject) == -1) {
      Wprintf("Can only read file into top-level page!");
      return;
   }
   else if (idx >= LOAD_MODES) {
      Wprintf("Unknown mode passed to routine getfile()\n");
      return;
   }
#ifndef TCL_WRAPPER
   savebutton = getgeneric(button, getfile, (void *)mode);
#endif
   if (idx == RECOVER) {
      char *cfile = getcrashfilename();
      promptstr = (char *)malloc(18 + ((cfile == NULL) ? 9 : strlen(cfile)));
      sprintf(promptstr, "Recover file \'%s\'?", (cfile == NULL) ? "(unknown)" : cfile);
      popupprompt(button, promptstr, NULL, loadmodes[idx].func, savebutton, NULL);
      if (cfile) free(cfile);
   }
   else {
      promptstr = (char *)malloc(18 + strlen(loadmodes[idx].prompt));
      sprintf(promptstr, "Select file to %s:", loadmodes[idx].prompt);
      popupprompt(button, promptstr, "\0", loadmodes[idx].func,
		savebutton, loadmodes[idx].filext);
   }
   free(promptstr);
}

/*--------------------------------------------------------------*/
/* Tilde ('~') expansion in file name.  Assumes that filename	*/
/* is a static character array of size "nchars".		*/
/*--------------------------------------------------------------*/

Boolean xc_tilde_expand(char *filename, int nchars)
{
#ifndef _MSC_VER
   struct passwd *passwd;
   char *username = NULL, *expanded, *sptr;

   if (*filename == '~') {
      sptr = filename + 1;
      if (*sptr == '/' || *sptr == ' ' || *sptr == '\0')
	 username = getenv("HOME");
      else {
	 for (; *sptr != '/' && *sptr != '\0'; sptr++);
	 if (*sptr == '\0') *(sptr + 1) = '\0';
	 *sptr = '\0';

	 passwd = getpwnam(filename + 1);
	 if (passwd != NULL)
	    username = passwd->pw_dir;

	 *sptr = '/';
      }
      if (username != NULL) {
	 expanded = (char *)malloc(strlen(username) +
		strlen(filename));
	 strcpy(expanded, username);
         strcat(expanded, sptr);
	 strncpy(filename, expanded, nchars);
         free(expanded);
      }
      return True;
   }
   return False;
#else
   return False;
#endif
} 

/*--------------------------------------------------------------*/
/* Variable ('$') expansion in file name 			*/
/*--------------------------------------------------------------*/

Boolean xc_variable_expand(char *filename, int nchars)
{
   char *expanded, *sptr, tmpchar, *varpos, *varsub;

   if ((varpos = strchr(filename, '$')) != NULL) {
      for (sptr = varpos; *sptr != '/' && *sptr != '\0'; sptr++);
      if (*sptr == '\0') *(sptr + 1) = '\0';
      tmpchar = *sptr;
      *sptr = '\0';

#ifdef TCL_WRAPPER
      /* Interpret as a Tcl variable */
      varsub = (char *)Tcl_GetVar(xcinterp, varpos + 1, TCL_NAMESPACE_ONLY);
#else
      /* Interpret as an environment variable */
      varsub = (char *)getenv((const char *)(varpos + 1));
#endif

      if (varsub != NULL) {

	 *varpos = '\0';
	 expanded = (char *)malloc(strlen(varsub) + strlen(filename) + 
		strlen(sptr + 1) + 2);
	 strcpy(expanded, filename);
         strcat(expanded, varsub);
	 *sptr = tmpchar;
	 strcat(expanded, sptr);
	 strncpy(filename, expanded, nchars);
         free(expanded);
      }
      else
         *sptr = tmpchar;
      return True;
   }
   return False;
} 

/*--------------------------------------------------------------*/
/* Attempt to find a file and open it.				*/
/*--------------------------------------------------------------*/

FILE *fileopen(char *filename, char *suffix, char *name_return, int nchars)
{
   FILE *file = NULL;
   char inname[250], expname[250], *sptr, *cptr, *iptr, *froot;
   int slen;

   sscanf(filename, "%249s", expname);
   xc_tilde_expand(expname, 249);
   while (xc_variable_expand(expname, 249));

   sptr = xobjs.filesearchpath;
   while (1) {
      if ((xobjs.filesearchpath == NULL) || (expname[0] == '/')) {
	 strcpy(inname, expname);
	 iptr = inname;
      }
      else {
	 strcpy(inname, sptr);
	 cptr = strchr(sptr, ':');
	 slen = (cptr == NULL) ? strlen(sptr) : (int)(cptr - sptr);
	 sptr += (slen + ((cptr == NULL) ? 0 : 1));
	 iptr = inname + slen;
	 if (*(iptr - 1) != '/') strcpy(iptr++, "/");
	 strcpy(iptr, expname);
      }

      /* Attempt to open the filename with a suffix */

      if ((froot = strrchr(iptr, '/')) == NULL) froot = iptr;
      if (strrchr(froot, '.') == NULL) {
         if (suffix) {
	    if (suffix[0] != '.')
	       strncat(inname, ".", 249);
            strncat(inname, suffix, 249);
	 }
         file = fopen(inname, "r");
      }

      /* Attempt to open the filename as given, without a suffix */

      if (file == NULL) {
         strcpy(iptr, expname);
         file = fopen(inname, "r");
      }
  
      if (file != NULL) break;
      else if (sptr == NULL) break;
      else if (*sptr == '\0') break;
   } 

   if (name_return) strncpy(name_return, inname, nchars);
   return file;
}

/*---------------------------------------------------------*/

Boolean nextfilename()	/* extract next filename from _STR2 into _STR */
{
   char *cptr, *slptr;

   sprintf(_STR, "%.149s", _STR2);
   if ((cptr = strrchr(_STR2, ',')) != NULL) {
      slptr = strrchr(_STR, '/');
      if (slptr == NULL || ((slptr - _STR) > (cptr - _STR2))) slptr = _STR - 1;
      sprintf(slptr + 1, "%s", cptr + 1); 
      *cptr = '\0';
      return True;
   }
   else return False;
}

/*---------------------------------------------------------*/

void loadfontlib()
{
   loadlibrary(FONTLIB);
}

/*------------------------------------------------------*/
/* Handle library loading and refresh current page if 	*/
/* it is a library page that just changed.		*/
/*------------------------------------------------------*/

void loadglib(Boolean lflag, short ilib, short tlib)
{
   while (nextfilename()) {
      if (lflag)
	 lflag = False;
      else
         ilib = createlibrary(False);
      loadlibrary(ilib);
      /* if (ilib == tlib) zoomview(NULL, NULL, NULL); */
   }
   if (lflag)
      lflag = False;
   else
      ilib = createlibrary(False);
   loadlibrary(ilib);
   /* if (ilib == tlib) zoomview(NULL, NULL, NULL); */
}

/*------------------------------------------------------*/
/* Load new library:  Create new library page and load	*/
/* to it.						*/
/*------------------------------------------------------*/

void loadulib()
{
   loadglib(False, (short)0, (short)is_library(topobject) + LIBRARY);
}

/*-----------------------------------------------------------*/
/* Add to library:  If on library page, add to that library. */
/* Otherwise, create a new library page and load to it.	     */
/*-----------------------------------------------------------*/

void loadblib()
{
   short ilib, tlib;
   Boolean lflag = True;

   /* Flag whether current page is a library page or not */

   if ((tlib = is_library(topobject)) < 0) {
      ilib = LIBRARY;
      lflag = False;
   }
   else
      ilib = tlib + LIBRARY;

   loadglib(lflag, ilib, tlib + LIBRARY);
}

/*---------------------------------------------------------*/

void getlib(xcWidget button, caddr_t clientdata, caddr_t nulldata)
{
   buttonsave *savebutton;
#ifndef TCL_WRAPPER
   savebutton = getgeneric(button, getlib, NULL);
#endif
   popupprompt(button, "Enter library to load:", "\0", loadblib, savebutton,
	"lps");
}

/*---------------------------------------------------------*/

void getuserlib(xcWidget button, caddr_t clientdata, caddr_t nulldata)
{
   buttonsave *savebutton;

#ifndef TCL_WRAPPER
   savebutton = getgeneric(button, getuserlib, NULL);
#endif
   popupprompt(button, "Enter library to load:", "\0", loadulib, savebutton,
	"lps");
}

/*------------------------------------------------------*/
/* Add a new name to the list of aliases for an object	*/
/*------------------------------------------------------*/

Boolean addalias(objectptr thisobj, char *newname)
{
   aliasptr aref;
   slistptr sref;
   /* Boolean retval = False; (jdk) */
   char *origname = thisobj->name;

   for (aref = aliastop; aref != NULL; aref = aref->next)
      if (aref->baseobj == thisobj)
	 break;

   /* An equivalence, not an alias */
   if (!strcmp(origname, newname)) return True;

   if (aref == NULL) {	/* entry does not exist;  add new baseobj */
      aref = (aliasptr)malloc(sizeof(alias));
      aref->baseobj = thisobj;
      aref->aliases = NULL;
      aref->next = aliastop;
      aliastop = aref;
   }

   for (sref = aref->aliases; sref != NULL; sref = sref->next)
      if (!strcmp(sref->alias, newname))
	 break;

   if (sref == NULL) {		/* needs new entry */
      sref = (slistptr)malloc(sizeof(stringlist));
      sref->alias = strdup(newname);
      sref->next = aref->aliases;
      aref->aliases = sref;
      return False;
   }
   else return True;		/* alias already exists! */
}

/*------------------------------------------------------*/
/* Remove all object name aliases			*/
/*------------------------------------------------------*/

void cleanupaliases(short mode)
{
   aliasptr aref;
   slistptr sref;
   objectptr baseobj;
   char *sptr; /*  *basename, (jdk) */
   int i, j;

   if (aliastop == NULL) return;

   for (aref = aliastop; aref != NULL; aref = aref->next) {
      baseobj = aref->baseobj;
      for (sref = aref->aliases; sref != NULL; sref = sref->next)
	 free(sref->alias);
   }

   for (; (aref = aliastop->next); aliastop = aref)
      free(aliastop);
   free(aliastop);
   aliastop = NULL;

   /* Get rid of propagating underscores in names */

   for (i = 0; i < ((mode == FONTLIB) ? 1 : xobjs.numlibs); i++) {
      for (j = 0; j < ((mode == FONTLIB) ? xobjs.fontlib.number :
		xobjs.userlibs[i].number); j++) {
	 baseobj = (mode == FONTLIB) ? *(xobjs.fontlib.library + j) :
		*(xobjs.userlibs[i].library + j);

	 sptr = baseobj->name;
	 while (*sptr == '_') sptr++;
	 /* need memmove to avoid overwriting? */
	 memmove((void *)baseobj->name, (const void *)sptr, strlen(sptr) + 1);
	 checkname(baseobj);
      }
   }
}

/*------------------------------------------------------*/
/* Open a library file by name and return a pointer to	*/
/* the file structure, or NULL if an error occurred.	*/
/*------------------------------------------------------*/

FILE *libopen(char *libname, short mode, char *name_return, int nchars)
{
   FILE *file = NULL;
   char inname[150], expname[150], *sptr, *cptr, *iptr;
   int slen;
   char *suffix = (mode == FONTENCODING) ? ".xfe" : ".lps";

   sscanf(libname, "%149s", expname);
   xc_tilde_expand(expname, 149);
   while(xc_variable_expand(expname, 149));

   sptr = xobjs.libsearchpath;
   while (1) {

      if ((xobjs.libsearchpath == NULL) || (expname[0] == '/')) {
	 strcpy(inname, expname);
	 iptr = inname;
      }
      else {
	 strcpy(inname, sptr);
	 cptr = strchr(sptr, ':');
	 slen = (cptr == NULL) ? strlen(sptr) : (int)(cptr - sptr);
	 sptr += (slen + ((cptr == NULL) ? 0 : 1));
	 iptr = inname + slen;
	 if (*(iptr - 1) != '/') strcpy(iptr++, "/");
	 strcpy(iptr, expname);
      }

      /* Try to open the filename with a suffix if it doesn't have one */

      if (strrchr(iptr, '.') == NULL) {
	 strncat(inname, suffix, 149);
         file = fopen(inname, "r");
      }

      /* Try to open the filename as given, without a suffix */

      if (file == NULL) {
	 strcpy(iptr, expname);
	 file = fopen(inname, "r");
      }

      if (file != NULL) break;
      else if (sptr == NULL) break;
      else if (*sptr == '\0') break;
   }

   if ((file == NULL) && (xobjs.libsearchpath == NULL)) {

      /* if not found in cwd and there is no library search	  */
      /* path, look for environment variable "XCIRCUIT_LIB_DIR"	  */
      /* defined (Thanks to Ali Moini, U. Adelaide, S. Australia) */

      char *tmp_s = getenv((const char *)"XCIRCUIT_LIB_DIR");

      if (tmp_s != NULL) {
	 sprintf(inname, "%s/%s", tmp_s, expname);
      	 file = fopen(inname, "r");
      	 if (file == NULL) {
	    sprintf(inname, "%s/%s%s", tmp_s, expname, suffix);
	    file = fopen(inname, "r");
	 }
      }

      /* last resort:  hard-coded directory BUILTINS_DIR */

      if (file == NULL) {
	 sprintf(inname, "%s/%s", BUILTINS_DIR, expname);
      	 file = fopen(inname, "r");
      	 if (file == NULL) {
	    sprintf(inname, "%s/%s%s", BUILTINS_DIR, expname, suffix);
	    file = fopen(inname, "r");
	 }
      }
   }
   
   if (name_return) strncpy(name_return, inname, nchars);
   return file;
}

/*--------------------------------------------------------------*/
/* Add a record to the instlist list of the library indexed by  */
/* mode, create a new instance, and add it to the record.	*/
/*								*/
/* objname is the name of the library object to be instanced,	*/
/* and buffer is the line containing the instance parameters	*/
/* of the new instance, optionally preceded by scale and	*/
/* rotation values.						*/
/*								*/
/*--------------------------------------------------------------*/

objinstptr new_library_instance(short mode, char *objname, char *buffer,
	TechPtr defaulttech)
{
   char *lineptr;
   objectptr libobj, localdata;
   objinstptr newobjinst;
   int j;
   char *nsptr, *fullname = objname;

   localdata = xobjs.libtop[mode + LIBRARY]->thisobject;

   /* For (older) libraries that do not use technologies, give the      */
   /* object a technology name in the form <library>::<object>          */
   
   if ((nsptr = strstr(objname, "::")) == NULL) {
      int deftechlen = (defaulttech == NULL) ? 0 : strlen(defaulttech->technology);
      fullname = (char *)malloc(deftechlen + strlen(objname) + 3);
      if (defaulttech == NULL)
         sprintf(fullname, "::%s", objname);
      else
         sprintf(fullname, "%s::%s", defaulttech->technology, objname);
   }

   for (j = 0; j < xobjs.userlibs[mode].number; j++) {
      libobj = *(xobjs.userlibs[mode].library + j);
      if (!strcmp(fullname, libobj->name)) {
	 newobjinst = addtoinstlist(mode, libobj, TRUE);

	 lineptr = buffer;
	 while (isspace(*lineptr)) lineptr++;
	 if (*lineptr != '<') {
	    /* May declare instanced scale and rotation first */
	    lineptr = varfscan(localdata, lineptr, &newobjinst->scale,
			(genericptr)newobjinst, P_SCALE);
	    lineptr = varfscan(localdata, lineptr, &newobjinst->rotation,
			(genericptr)newobjinst, P_ROTATION);
	 }
	 readparams(NULL, newobjinst, libobj, lineptr);
	 if (fullname != objname) free(fullname);
	 return newobjinst;
      }
   }
   if (fullname != objname) free(fullname);
   return NULL;		/* error finding the library object */
}

/*------------------------------------------------------*/
/* Import a single object from a library file.  "mode"	*/
/* is the number of the library into which the object	*/
/* will be placed.  This function allows new libraries	*/
/* to be generated by "cutting and pasting" from	*/
/* existing libraries.  It also allows better library	*/
/* management when the number of objects becomes very	*/
/* large, such as when using "diplib" (7400 series	*/
/* chips, created from the PCB library).		*/ 	
/*------------------------------------------------------*/

void importfromlibrary(short mode, char *libname, char *objname)
{
   FILE *ps;
   char temp[150], keyword[100];
   char inname[150], *tptr;
   objectptr *newobject;
   objlistptr redef;
   char saveversion[20];
   Boolean dependencies = False;
   TechPtr nsptr = NULL;
   
   ps = libopen(libname, mode, inname, 149);
   if (ps == NULL) {
      Fprintf(stderr, "Cannot open library %s for import.\n", libname);
      return;
   }

   strcpy(version, "2.0"); /* Assume version is 2.0 unless found in header */

   for(;;) {
      if (fgets(temp, 149, ps) == NULL) {
         Wprintf("Error in library.");
	 goto endload;
      }
      else if (temp[0] == '/') {
	 int s = 1;
	 if (temp[1] == '@') s = 2;
	 sscanf(&temp[s], "%s", keyword);
	 if (!strcmp(keyword, objname))
	    break;
      }
      else if (*temp == '%') {
	 char *tptr = temp + 1;
	 while (isspace(*tptr)) tptr++;
	 if (!strncmp(tptr, "Version:", 8)) {
	    tptr += 9;
	    sscanf(tptr, "%20s", version);
	    ridnewline(version);
	 }
	 else if (!strncmp(tptr, "Library", 7)) {
	    char *techname = strstr(tptr, ":");
	    if (techname != NULL) {
	       techname++;	/* skip over colon */
	       while(isspace(*techname)) techname++;
	       ridnewline(techname);	/* Remove newline character */
	       if ((tptr = strrchr(techname, '/')) != NULL)
		  techname = tptr + 1;
	       if ((tptr = strrchr(techname, '.')) != NULL)
		  if (!strncmp(tptr, ".lps", 4))
		     *tptr = '\0';
	       nsptr = AddNewTechnology(techname, inname);
	       if (nsptr) {
		  // Set the IMPORTED flag, to help prevent overwriting
		  // the techfile with a truncated version of it, unless
		  // the filename of the techfile has been changed, in
		  // which case the technology should be considered to
		  // stand on its own, and is not considered a partially
		  // complete imported version of the original techfile.

		  if (!strcmp(inname, nsptr->filename))
		     nsptr->flags |= TECH_IMPORTED;
	       }
	    }
	 }
         else if (!strncmp(tptr, "Depend", 6)) {
	    dependencies = TRUE;
	    tptr += 7;
	    sscanf(tptr, "%s", keyword);
	    if (!strcmp(keyword, objname)) {
	       /* Load dependencies */
	       while (1) {
	          tptr += strlen(keyword) + 1;
	          if (sscanf(tptr, "%s", keyword) != 1) break;
	          if (keyword[0] == '\n' || keyword[0] == '\0') break;
	          /* Recursive import */
		  strcpy(saveversion, version);
	          importfromlibrary(mode, libname, keyword);
		  strcpy(version, saveversion);
	       }
	    }
	 }
      }
   }

   if ((compare_version(version, "3.2") < 0) && (!dependencies)) {
      Fprintf(stderr, "Library does not have dependency list and cannot "
		"be trusted.\nLoad and rewrite library to update.\n");
      goto endload;
   }

   newobject = new_library_object(mode, keyword, &redef, nsptr);

   load_in_progress = True;
   if (objectread(ps, *newobject, 0, 0, mode, temp, DEFAULTCOLOR, nsptr) == False) {

      if (library_object_unique(mode, *newobject, redef)) {
	 add_object_to_library(mode, *newobject);
	 cleanupaliases(mode);

	 /* pull in any instances of this object that	*/
	 /* are defined in the library			*/

	 for(;;) {
	    if (fgets(temp, 149, ps) == NULL) {
	       Wprintf("Error in library.");
	       goto endload;
	    }
	    else if (!strncmp(temp, "% EndLib", 8))
	       break;
	    else if (strstr(temp, "libinst") != NULL) {
	       if ((tptr = strstr(temp, objname)) != NULL) {
	          if (*(tptr - 1) == '/') {
		     char *eptr = tptr;
		     while (!isspace(*++eptr));
		     *eptr = '\0';
		     new_library_instance(mode - LIBRARY, tptr, temp, nsptr);
		  }
	       }
	    }
	 }

	 if (mode != FONTLIB) {
	    composelib(mode);
	    centerview(xobjs.libtop[mode]);
	 }
      }
   }

endload:
   fclose(ps);
   strcpy(version, PROG_VERSION);
   load_in_progress = False;
}

/*------------------------------------------------------*/
/* Copy all technologies' replace flags into temp flag.	*/
/* Then clear the replace flags (replace none)		*/
/*------------------------------------------------------*/

void TechReplaceSave()
{
   TechPtr nsp;

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next)
   {
      if (nsp->flags & TECH_REPLACE)
	 nsp->flags |= TECH_REPLACE_TEMP;
      else
         nsp->flags &= ~TECH_REPLACE_TEMP;
      nsp->flags &= ~TECH_REPLACE;
   }
}

/*------------------------------------------------------*/
/* Restore all technologies' replace flags 		*/
/*------------------------------------------------------*/

void TechReplaceRestore()
{
   TechPtr nsp;

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next)
   {
      if (nsp->flags & TECH_REPLACE_TEMP)
	 nsp->flags |= TECH_REPLACE;
      else
         nsp->flags &= ~TECH_REPLACE;
   }
}

/*------------------------------------------------------*/
/* Set all technologies' replace flags (replace all)	*/
/*------------------------------------------------------*/

void TechReplaceAll()
{
   TechPtr nsp;

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next)
      nsp->flags |= TECH_REPLACE;
}

/*------------------------------------------------------*/
/* Clear all technologies' replace flags (replace none)	*/
/*------------------------------------------------------*/

void TechReplaceNone()
{
   TechPtr nsp;

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next)
      nsp->flags &= ~TECH_REPLACE;
}


/*------------------------------------------------------*/
/* Compare an object's name with a specific technology	*/
/*							*/
/* A missing "::" prefix separator or an empty prefix	*/
/* both match a NULL technology or a technology name	*/
/* that is an empty string ("").  All of these		*/
/* conditions indicate the default "user" technology.	*/
/*------------------------------------------------------*/

Boolean CompareTechnology(objectptr cobj, char *technology)
{
   char *cptr;
   Boolean result = FALSE;

   if ((cptr = strstr(cobj->name, "::")) != NULL) {
      if (technology == NULL)
	 result = (cobj->name == cptr) ? TRUE : FALSE;
      else {
         *cptr = '\0';
         if (!strcmp(cobj->name, technology)) result = TRUE;
         *cptr = ':';
      }
   }
   else if (technology == NULL)
      result = TRUE;

   return result;
}

/*------------------------------------------------------*/
/* Find a technology record				*/
/*------------------------------------------------------*/

TechPtr LookupTechnology(char *technology)
{
   TechPtr nsp;
   Boolean usertech = FALSE;

   // A NULL technology is allowed as equivalent to a
   // technology name of "" (null string)

   if (technology == NULL)
      usertech = TRUE;
   else if (*technology == '\0')
      usertech = TRUE;
   else if (!strcmp(technology, "(user)"))
      usertech = TRUE;

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next) {
      if (usertech == TRUE) {
	 if (*nsp->technology == '\0')
	    return nsp;
      }
      if ((technology != NULL) && !strcmp(technology, nsp->technology))
	 return nsp;
   }
   return NULL;
}

/*------------------------------------------------------*/
/* Find a technology according to the filename		*/
/*------------------------------------------------------*/

TechPtr GetFilenameTechnology(char *filename)
{
   TechPtr nsp;

   if (filename == NULL) return NULL;

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next)
      if (!filecmp(filename, nsp->filename))
	 return nsp;

   return NULL;
}

/*------------------------------------------------------*/
/* Find a technology record corresponding to the	*/
/* indicated object's technology.			*/
/*------------------------------------------------------*/

TechPtr GetObjectTechnology(objectptr thisobj)
{
   TechPtr nsp;
   char *cptr;
   /* int nlen; (jdk) */

   cptr = strstr(thisobj->name, "::");
   if (cptr == NULL) return NULL;
   else *cptr = '\0';

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next)
      if (!strcmp(thisobj->name, nsp->technology))
	 break;

   *cptr = ':';
   return nsp;
}

/*------------------------------------------------------*/
/* Add a new technology name to the list		*/
/*------------------------------------------------------*/

TechPtr AddNewTechnology(char *technology, char *filename)
{
   TechPtr nsp;
   char usertech[] = "";
   char *localtech = technology;

   // In the case where somebody has saved the contents of the user
   // technology to a file, create a technology->filename mapping
   // using a null string ("") for the technology name.  If we are
   // only checking if a technology name exists (filename is NULL),
   // then ignore an reference to the user technology.

   if (technology == NULL) {
      if (filename == NULL) return NULL;
      else localtech = usertech;
   }

   for (nsp = xobjs.technologies; nsp != NULL; nsp = nsp->next) {
      if (!strcmp(localtech, nsp->technology)) {

	 /* A namespace may be created for an object that is a dependency */
	 /* in a different technology.  If so, it will have a NULL	  */
	 /* filename, and the filename should be replaced if we ever load */
	 /* the file that properly defines the technology.		  */

	 if ((nsp->filename == NULL) && (filename != NULL)) 
	    nsp->filename = strdup(filename);

	 return nsp;	/* Namespace already exists */
      }
   }

   nsp = (TechPtr)malloc(sizeof(Technology));
   nsp->next = xobjs.technologies;
   if (filename == NULL)
      nsp->filename = NULL;
   else
      nsp->filename = strdup(filename);
   nsp->technology = strdup(localtech);
   nsp->flags = (u_char)0;
   xobjs.technologies = nsp;

   return nsp;
}

/*------------------------------------------------------*/
/* Check an object's name for a technology, and add it	*/
/* to the list of technologies if it has one.		*/
/*------------------------------------------------------*/

void AddObjectTechnology(objectptr thisobj)
{
   char *cptr;

   cptr = strstr(thisobj->name, "::");
   if (cptr != NULL) {
      *cptr = '\0';
      AddNewTechnology(thisobj->name, NULL);
      *cptr = ':';
   }
}

/*------------------------------------------------------*/
/* Load a library page (given in parameter "mode") and	*/
/* rename the library page to match the library name as */
/* found in the file header.				*/
/*------------------------------------------------------*/

Boolean loadlibrary(short mode)
{
   FILE *ps;
   objinstptr saveinst;
   char temp[150], keyword[30], percentc, inname[150];
   TechPtr nsptr = NULL;

   ps = libopen(_STR, mode, inname, 149);

   if ((ps == NULL) && (mode == FONTLIB)) {
      /* We automatically try looking in all the usual places plus a	*/
      /* subdirectory named "fonts".					*/

      sprintf(temp, "fonts/%s", _STR);
      ps = libopen(temp, mode, inname, 149);
   }
   if (ps == NULL) {
      Wprintf("Library not found.");
      return False;
   }

   /* current version is PROG_VERSION;  however, all libraries newer than */
   /* version 2.0 require "Version" in the header.  So unnumbered	  */
   /* libraries may be assumed to be version 1.9 or earlier.		  */

   strcpy(version, "1.9");
   for(;;) {
      if (fgets(temp, 149, ps) == NULL) {
         Wprintf("Error in library.");
	 fclose(ps);
         return False;
      }
      sscanf(temp, "%c %29s", &percentc, keyword);

      /* Commands in header are PostScript comments (%) */
      if (percentc == '%') {

	 /* The library name in the header corresponds to the object	*/
	 /* technology defined by the library.  This no longer has 	*/
	 /* anything to do with the name of the library page where we	*/
	 /* are loading this file.					*/

	 /* Save the technology, filename, and flags in the technology list. */

         if ((mode != FONTLIB) && !strcmp(keyword, "Library")) {
	    char *cptr, *nptr;
	    cptr = strchr(temp, ':');
	    if (cptr != NULL) {
	       cptr += 2;

	       /* Don't write terminating newline to the object's name string */
	       ridnewline(cptr);

	       /* The default user technology is written to the output	*/
	       /* as "(user)".  If this is found, replace it with a	*/
	       /* null string.						*/
	       if (!strcmp(cptr, "(user)")) cptr += 6;

	       /* Removing any leading pathname from the library name */
	       if ((nptr = strrchr(cptr, '/')) != NULL) cptr = nptr + 1;

	       /* Remove any ".lps" extension from the library name */

	       nptr = strrchr(cptr, '.');
	       if ((nptr != NULL) && !strcmp(nptr, ".lps")) *nptr = '\0';

	       nsptr = AddNewTechnology(cptr, inname);

	       if (nsptr) {
		  // If anything was previously imported from this file
		  // using importfromlibrary(), then the IMPORTED flag
		  // will be set and needs to be cleared.
	          nsptr->flags &= ~TECH_IMPORTED;
	       }
	    }
         }

         /* This comment gives the Xcircuit version number */
	 else if (!strcmp(keyword, "Version:")) {
	    char tmpv[20];
	    if (sscanf(temp, "%*c %*s %s", tmpv) > 0) strcpy(version, tmpv);
	 }

         /* This PostScript comment marks the end of the file header */
         else if (!strcmp(keyword, "XCircuitLib")) break;
      }
   }

   /* Set the current top object to the library page so that any	*/
   /* expression parameters are computed with respect to the library,	*/
   /* not a page.  Revert back to the page after loading the library.	*/

   saveinst = areawin->topinstance;
   areawin->topinstance = xobjs.libtop[mode];

   load_in_progress = True;
   objectread(ps, topobject, 0, 0, mode, temp, DEFAULTCOLOR, nsptr);
   load_in_progress = False;
   cleanupaliases(mode);

   areawin->topinstance = saveinst;

   if (mode != FONTLIB) {
      composelib(mode);
      centerview(xobjs.libtop[mode]);
      if (nsptr == NULL) nsptr = GetFilenameTechnology(inname);
      if (nsptr != NULL)
         Wprintf("Loaded library file %s", inname);
      else
         Wprintf("Loaded library file %s (technology %s)", inname,
		nsptr->technology);
   }
   else
        Wprintf("Loaded font file %s", inname);

   strcpy(version, PROG_VERSION);
   fclose(ps);

   /* Check if the library is read-only by opening for append */

   if ((mode != FONTLIB) && (nsptr != NULL)) {
      ps = fopen(inname, "a");
      if (ps == NULL)
         nsptr->flags |= TECH_READONLY;
      else
         fclose(ps);
   }

   return True;
}

/*---------------------------------------------------------*/

void startloadfile(int libnum)
{
   int savemode;
   short firstpage = areawin->page;

   while (nextfilename()) {
      loadfile(0, libnum);

      /* find next undefined page */

      while(areawin->page < xobjs.pages &&
	   xobjs.pagelist[areawin->page]->pageinst != NULL) areawin->page++;
      changepage(areawin->page);
   }
   loadfile(0, libnum);


   /* Prevent page change from being registered as an undoable action */
   savemode = eventmode;
   eventmode = UNDO_MODE;

   /* Display the first page loaded */
   newpage(firstpage);
   eventmode = savemode;

   setsymschem();
}

/*------------------------------------------------------*/
/* normalloadfile() is a call to startloadfile(-1)	*/
/* meaning load symbols to the User Library 		*/
/*------------------------------------------------------*/

void normalloadfile()
{
   startloadfile(-1);
}

/*------------------------------------------------------*/
/* Import an xcircuit file onto the current page	*/
/*------------------------------------------------------*/

void importfile()
{
   while (nextfilename()) loadfile(1, -1);
   loadfile(1, -1);
}

/*------------------------------------------------------*/
/* Import an PPM graphic file onto the current page	*/
/*------------------------------------------------------*/

#ifdef HAVE_CAIRO
void importgraphic(void)
{
   char inname[250];
   FILE *spcfile;

   if (eventmode == CATALOG_MODE) {
      Wprintf("Cannot import a graphic while in the library window.");
      return;
   }

   if (!nextfilename()) {
      xc_tilde_expand(_STR, 149);
      sscanf(_STR, "%149s", inname);
      if (!new_graphic(NULL, inname, 0, 0)) {
         Wprintf("Error:  Graphic file not found.");
	 return;
      }
   }
   else {
      Wprintf("Error:  No graphic file to read.");
      return;
   }
}
#endif /* HAVE_CAIRO */

/*--------------------------------------------------------------*/
/* Skip forward in the input file to the next comment line	*/
/*--------------------------------------------------------------*/

void skiptocomment(char *temp, int length, FILE *ps)
{
   int pch;

   do {
      pch = getc(ps);
   } while (pch == '\n');

   ungetc(pch, ps);
   if (pch == '%') fgets(temp, length, ps);
}

/*--------------------------------------------------------------*/
/* ASG file import functions:					*/
/* This function loads a SPICE deck (hspice format)		*/
/*--------------------------------------------------------------*/

#ifdef ASG

void importspice()
{
   char inname[250];
   FILE *spcfile;

   if (eventmode == CATALOG_MODE) {
      Wprintf("Cannot import a netlist while in the library window.");
      return;
   }

   if (!nextfilename()) {
      xc_tilde_expand(_STR, 149);
      sscanf(_STR, "%149s", inname);
      spcfile = fopen(inname, "r");
      if (spcfile != NULL) {
         ReadSpice(spcfile);
         Route(areawin, False);
         fclose(spcfile);
      }
      else {
         Wprintf("Error:  Spice file not found.");
	 return;
      }
   }
   else {
      Wprintf("Error:  No spice file to read.");
      return;
   }
}

#endif

/*--------------------------------------------------------------*/
/* Load an xcircuit file into xcircuit				*/
/*								*/
/*    mode = 0 is a "load" function.  Behavior:  load on 	*/
/*	current page if empty.  Otherwise, load on first empty	*/
/*	page found.  If no empty pages exist, create a new page	*/
/*	and load there.						*/
/*    mode = 1 is an "import" function.  Behavior:  add		*/
/*	contents of file to the current page.			*/
/*    mode = 2 is a "library load" function.  Behavior:  add	*/
/*	objects in file to the user library but ignore the	*/
/*	page contents.						*/
/*								*/
/* Return value:  True if file was successfully loaded, False	*/
/*	if not.							*/
/*--------------------------------------------------------------*/

typedef struct _connects *connectptr;

typedef struct _connects {
   short page;
   char *master;
   connectptr next;
} connects;

Boolean loadfile(short mode, int libnum)
{
   FILE *ps;
   char inname[150], temp[150], keyword[30], percentc, *pdchar;
   char teststr[50], teststr2[20], pagestr[100];
   short offx, offy, multipage, page, temppmode = 0;
   float tmpfl;
   XPoint pagesize;
   connects *connlist = NULL;
   struct stat statbuf;
   int loclibnum = (libnum == -1) ? USERLIB : libnum;

   /* First, if we're in catalog mode, return with error */

   if (eventmode == CATALOG_MODE) {
      Wprintf("Cannot load file from library window");
      return False;
   }

   /* Do tilde/variable expansions on filename and open */
   ps = fileopen(_STR, "ps", inname, 149);

   /* Could possibly be a library file?				*/
   /* (Note---loadfile() has no problems loading libraries	*/
   /* except for setting technology names and setting the	*/
   /* library page view at the end.  The loadlibrary() routine	*/
   /* should probably be merged into this one.)			*/

   if (ps == NULL) {
      ps = fileopen(_STR, "lps", NULL, 0);
      if (ps != NULL) {
	 fclose(ps);
	 loadlibrary(loclibnum);
	 return True;
      }
   }
   else if (!strcmp(inname + strlen(inname) - 4, ".lps")) {
      fclose(ps);
      loadlibrary(loclibnum);
      return True;
   }
   

#ifdef LGF
   /* Could possibly be an LGF file? */
   if (ps == NULL) {
      ps = fileopen(_STR, "lgf", NULL, 0);
      if (ps != NULL) {
	 fclose(ps);
	 loadlgf(mode);
	 return True;
      }
   }

   /* Could possibly be an LGF backup (.lfo) file? */
   if (ps == NULL) {
      ps = fileopen(_STR, "lfo", NULL, 0);
      if (ps != NULL) {
	 fclose(ps);
	 loadlgf(mode);
	 return True;
      }
   }
#endif /* LGF */

   /* Check for empty file---don't attempt to read empty files */
   if (ps != NULL) {
      if (fstat(fileno(ps), &statbuf) == 0 && (statbuf.st_size == (off_t)0)) {
         fclose(ps);
	 ps = NULL;
      }
   }

   /* What to do if no file was found. . . */

   if (ps == NULL) {
      if (topobject->parts == 0 && (mode == 0)) {

         /* Check for file extension, and remove if "ps". */

         if ((pdchar = strchr(_STR, '.')) != NULL)
	    if (!strcmp(pdchar + 1, "ps")) *pdchar = '\0';

         free(xobjs.pagelist[areawin->page]->filename);
         xobjs.pagelist[areawin->page]->filename = strdup(_STR);

         /* If the name has a path component, use only the root		*/
         /* for the object name, but the full path for the filename.	*/

         if ((pdchar = strrchr(_STR, '/')) != NULL)
            sprintf(topobject->name, "%s", pdchar + 1);
         else
	    sprintf(topobject->name, "%s", _STR);

         renamepage(areawin->page);
         printname(topobject);
         Wprintf("Starting new drawing");
      }
      else {
         Wprintf("Can't find file %s, won't overwrite current page", _STR);
      }
      return False;
   }

   strcpy(version, "1.0");
   multipage = 1;
   pagesize.x = 612;
   pagesize.y = 792;

   for(;;) {
      if (fgets(temp, 149, ps) == NULL) {
	 Wprintf("Error: EOF in or before prolog.");
	 return False;
      }
      sscanf(temp, "%c%29s", &percentc, keyword);
      for (pdchar = keyword; isspace(*pdchar); pdchar++);
      if (percentc == '%') {
	 if (!strcmp(pdchar, "XCircuit")) break;
	 if (!strcmp(pdchar, "XCircuitLib")) {
	    /* version control in libraries is post version 1.9 */
	    if (compare_version(version, "1.0") == 0) strcpy(version, "1.9");
	    break;
	 }
	 if (!strcmp(pdchar, "%Page:")) break;
	 if (strstr(pdchar, "PS-Adobe") != NULL)
	    temppmode = (strstr(temp, "EPSF") != NULL) ? 0 : 1;
         else if (!strcmp(pdchar, "Version:"))
	    sscanf(temp, "%*c%*s %20s", version);
	 else if (!strcmp(pdchar, "%Pages:")) {
	    pdchar = advancetoken(temp);
	    multipage = atoi(pdchar);
	 }
	 /* Crash files get renamed back to their original filename */
	 else if (!strcmp(pdchar, "%Title:")) {
	    if (xobjs.tempfile != NULL)
	       if (!strcmp(inname, xobjs.tempfile))
	          sscanf(temp, "%*c%*s %s", inname);
	 }
	 else if ((temppmode == 1) && !strcmp(pdchar, "%BoundingBox:")) {
	    short botx, boty;
	    sscanf(temp, "%*s %hd %hd %hd %hd", &botx, &boty,
		&(pagesize.x), &(pagesize.y));
	    pagesize.x += botx;
	    pagesize.y += boty;
	 }
	
      }
#ifdef LGF
      else if (percentc == '-' && !strcmp(keyword, "5")) {
	 fclose(ps);
	 loadlgf(mode);
	 return True;
      }
#endif
   }

   /* Look for old-style files (no %%Page; maximum one page in file) */

   if (!strcmp(pdchar, "XCircuit"))
      skiptocomment(temp, 149, ps);

   for (page = 0; page < multipage; page++) {
      sprintf(pagestr, "%d", page + 1);

      /* read out-of-page library definitions */

      if (strstr(temp, "%%Page:") == NULL && strstr(temp, "offsets") == NULL) {
         load_in_progress = True;
	 objectread(ps, topobject, 0, 0, loclibnum, temp, DEFAULTCOLOR, NULL);
         load_in_progress = False;
      }

      if (strstr(temp, "%%Page:") != NULL) {
	 sscanf(temp + 8, "%99s", pagestr);

	 /* Read the next line so any keywords in the Page name don't	*/
	 /* confuse the parser.						*/
	 if (fgets(temp, 149, ps) == NULL) {
            Wprintf("Error: bad page header.");
            return False;
	 }

	 /* Library load mode:  Ignore all pages, just load objects */
	 if (mode == 2) {
	    while (strstr(temp, "showpage") == NULL) {
	       if (fgets(temp, 149, ps) == NULL) {
		  Wprintf("Error: bad page definition.");
		  return False;
	       }
	    }
	    skiptocomment(temp, 149, ps);
	    continue;
	 }
      }

      /* go to new page if necessary */

      if (page > 0) {

	 /* find next undefined page */

	 while(areawin->page < xobjs.pages &&
		xobjs.pagelist[areawin->page]->pageinst != NULL) areawin->page++;
	 changepage(areawin->page);
      }

      /* If this file was a library file then there is no page to load */

      if (strstr(temp, "EndLib") != NULL) {
   	 composelib(loclibnum);
	 centerview(xobjs.libtop[mode]);
	 Wprintf("Loaded library.");
	 return True;
      }

      /* good so far;  let's clear out the old data structure */

      if (mode == 0) {
         reset(topobject, NORMAL);
         pagereset(areawin->page);
	 xobjs.pagelist[areawin->page]->pmode = temppmode;
	 if (temppmode == 1) {
	    xobjs.pagelist[areawin->page]->pagesize.x = pagesize.x;
	    xobjs.pagelist[areawin->page]->pagesize.y = pagesize.y;
	 }
      }
      else {
	 invalidate_netlist(topobject);
	 /* ensure that the netlist for topobject is destroyed */
	 freenetlist(topobject);
      }

      /* clear the undo record */
      flush_undo_stack();

      /* read to the "scale" line, picking up inch/cm type, drawing */
      /* scale, and grid/snapspace along the way		    */

      offx = offy = 0;
      for(;;) {
         if (strstr(temp, "offsets") != NULL) {
	    /* Prior to version 3.1.28 only. . . */
            sscanf(temp, "%c %hd %hd %*s", &percentc, &offx, &offy);
            if(percentc != '%') {
               Wprintf("Something wrong in offsets line.");
               offx = offy = 0;
            }
	 }

	 if ((temppmode == 1) && strstr(temp, "%%PageBoundingBox:") != NULL) {
	    /* Recast the individual page size if specified per page */
	    sscanf(temp, "%*s %*d %*d %hd %hd",
		&xobjs.pagelist[areawin->page]->pagesize.x,
		&xobjs.pagelist[areawin->page]->pagesize.y);
	 }
	 else if (strstr(temp, "drawingscale") != NULL)
	    sscanf(temp, "%*c %hd:%hd %*s",
		&xobjs.pagelist[areawin->page]->drawingscale.x,
		&xobjs.pagelist[areawin->page]->drawingscale.y);

	 else if (strstr(temp, "hidden") != NULL)
	    topobject->hidden = True;

	 else if (strstr(temp, "is_symbol") != NULL) {
	    sscanf(temp, "%*c %49s", teststr);
	    checkschem(topobject, teststr);
	 }
         else if (strstr(temp, "is_primary") != NULL) {
	   /* objectptr master; (jdk) */
	    connects *newconn;

	    /* Save information about master schematic and link at end of load */
	    sscanf(temp, "%*c %49s", teststr);
	    newconn = (connects *)malloc(sizeof(connects));
	    newconn->next = connlist;
	    connlist = newconn;
	    newconn->page = areawin->page;
	    newconn->master = strdup(teststr);
         }  

	 else if (strstr(temp, "gridspace"))
	    sscanf(temp, "%*c %f %f %*s", &xobjs.pagelist[areawin->page]->gridspace,
		 &xobjs.pagelist[areawin->page]->snapspace);
         else if (strstr(temp, "scale") != NULL || strstr(temp, "rotate") != NULL) {
            /* rotation (landscape mode) is optional; parse accordingly */

            sscanf(temp, "%f %49s", &tmpfl, teststr);
            if (strstr(teststr, "scale") != NULL) {
#ifndef TCL_WRAPPER
	       setgridtype(teststr);
#else
	       if (strstr(teststr, "inch"))
		  Tcl_Eval(xcinterp, "xcircuit::coordstyle inches");
	       else
		  Tcl_Eval(xcinterp, "xcircuit::coordstyle centimeters");
#endif
               xobjs.pagelist[areawin->page]->outscale = tmpfl;
            }
            else if (!strcmp(teststr, "rotate")) {
               xobjs.pagelist[areawin->page]->orient = (short)tmpfl;
               fgets(temp, 149, ps);
               sscanf(temp, "%f %19s", &tmpfl, teststr2);
	       if (strstr(teststr2, "scale") != NULL) {
#ifndef TCL_WRAPPER
	          setgridtype(teststr2);
#else
	          if (strstr(teststr2, "inch"))
		     Tcl_Eval(xcinterp, "xcircuit::coordstyle inches");
	          else
		     Tcl_Eval(xcinterp, "xcircuit::coordstyle centimeters");
#endif
	          xobjs.pagelist[areawin->page]->outscale = tmpfl;
	       }
	       else {
	          sscanf(temp, "%*f %*f %19s", teststr2);
	          if (!strcmp(teststr2, "scale"))
	             xobjs.pagelist[areawin->page]->outscale = tmpfl /
		          getpsscale(1.0, areawin->page);
	          else {
	             Wprintf("Error in scale/rotate constructs.");
	             return False;
	          }
	       }
            }
            else {     /* old style scale? */
               sscanf(temp, "%*f %*s %19s", teststr2);
	       if ((teststr2 != NULL) && (!strcmp(teststr2, "scale")))
                  xobjs.pagelist[areawin->page]->outscale = tmpfl /
		        getpsscale(1.0, areawin->page);
	       else {
                  Wprintf("Error in scale/rotate constructs.");
                  return False;
	       }
            }
	 }
         else if (strstr(temp, "setlinewidth") != NULL) {
            sscanf(temp, "%f %*s", &xobjs.pagelist[areawin->page]->wirewidth);
	    xobjs.pagelist[areawin->page]->wirewidth /= 1.3;
	    break;
	 }
	 else if (strstr(temp, "insertion") != NULL) {
	    /* read in an included background image */
	    readbackground(ps);
	 }
	 else if (strstr(temp, "<<") != NULL) {
	    char *buffer = temp, *endptr;
	    /* Make sure we have the whole dictionary before calling */
	    while (strstr(buffer, ">>") == NULL) {
	       if (buffer == temp) {
		  buffer = (char *)malloc(strlen(buffer) + 150);
		  strcpy(buffer, temp);
	       }
	       else
	          buffer = (char *)realloc(buffer, strlen(buffer) + 150);
	       endptr = ridnewline(buffer);
	       *endptr++ = ' ';
               fgets(endptr, 149, ps);
	    }
	    /* read top-level parameter dictionary */
	    readparams(NULL, NULL, topobject, buffer);
	    if (buffer != temp) free(buffer);
	 }

	 if (fgets(temp, 149, ps) == NULL) {
            Wprintf("Error: Problems encountered in page header.");
            return False;
         }
      }

      load_in_progress = True;
      objectread(ps, topobject, offx, offy, LIBRARY, temp, DEFAULTCOLOR, NULL);
      load_in_progress = False;

      /* skip to next page boundary or file trailer */

      if (strstr(temp, "showpage") != NULL && multipage != 1) {
	 char *fstop;

	 skiptocomment(temp, 149, ps);

	 /* check for new filename if this is a crash recovery file */
         if ((fstop = strstr(temp, "is_filename")) != NULL) {
	    strncpy(inname, temp + 2, (int)(fstop - temp - 3));
	    *(inname + (int)(fstop - temp) - 3) = '\0';
	    fgets(temp, 149, ps);
	    skiptocomment(temp, 149, ps);
         }
      }

      /* Finally: set the filename and pagename for this page */

      if (mode == 0) {
	 char tpstr[6], *rootptr;

	 /* Set filename and page title.	      */

	 if (xobjs.pagelist[areawin->page]->filename != NULL)
	    free(xobjs.pagelist[areawin->page]->filename);
	 xobjs.pagelist[areawin->page]->filename = strdup(inname);

	 /* Get the root name (after all path components) */

	 rootptr = strrchr(xobjs.pagelist[areawin->page]->filename, '/');
	 if (rootptr == NULL) rootptr = xobjs.pagelist[areawin->page]->filename;
	 else rootptr++;

	 /* If we didn't read in a page name from the %%Page: header line, then */
	 /* set the page name to the root name of the file.			*/

	 sprintf(tpstr, "%d", page + 1);
	 if (!strcmp(pagestr, tpstr)) {
	    if (rootptr == NULL)
	      sprintf(topobject->name, "Page %d", page + 1);
	    else
	      sprintf(topobject->name, "%.79s", rootptr);

              /* Delete filename extensions ".ps" or ".eps" from the page name */
              if ((pdchar = strrchr(topobject->name, '.')) != NULL) {
	         if (!strcmp(pdchar + 1, "ps") || !strcmp(pdchar + 1, "eps"))
		    *pdchar = '\0';
	      }
	 }
	 else
	    sprintf(topobject->name, "%.79s", pagestr);

         renamepage(areawin->page);
      }

      /* set object position to fit to window separately for each page */
      calcbbox(areawin->topinstance);
      centerview(areawin->topinstance);
   }

   /* Crash file recovery: read any out-of-page library definitions tacked */
   /* onto the end of the file into the user library.			   */
   if (strncmp(temp, "%%Trailer", 9)) {
      load_in_progress = True;
      objectread(ps, topobject, 0, 0, USERLIB, temp, DEFAULTCOLOR, NULL);
      load_in_progress = False;
   }

   cleanupaliases(USERLIB);

   /* Connect master->slave schematics */
   while (connlist != NULL) {
      connects *thisconn = connlist;
      objectptr master = NameToPageObject(thisconn->master, NULL, NULL);
      if (master) {
	 xobjs.pagelist[thisconn->page]->pageinst->thisobject->symschem = master;
	 xobjs.pagelist[thisconn->page]->pageinst->thisobject->schemtype = SECONDARY;
      }     
      else {
	 Fprintf(stderr, "Error:  Cannot find primary schematic for %s\n",
		xobjs.pagelist[thisconn->page]->pageinst->thisobject->name);
      }
      connlist = thisconn->next;
      free(thisconn->master);
      free(thisconn);
   }        

   Wprintf("Loaded file: %s (%d page%s)", inname, multipage,
	(multipage > 1 ? "s" : ""));

   composelib(loclibnum);
   centerview(xobjs.libtop[loclibnum]);
   composelib(PAGELIB);

   if (compare_version(version, PROG_VERSION) == 1) {
      Wprintf("WARNING: file %s is version %s vs. executable version %s",
		inname, version, PROG_VERSION);
   }

   strcpy(version, PROG_VERSION);
   fclose(ps);
   return True;
}

/*------------------------------------------------------*/
/* Object name comparison:  True if names are equal,    */
/* not counting leading underscores.			*/
/*------------------------------------------------------*/

int objnamecmp(char *name1, char *name2)
{
   char *n1ptr = name1;
   char *n2ptr = name2;

   while (*n1ptr == '_') n1ptr++;                           
   while (*n2ptr == '_') n2ptr++;

   return (strcmp(n1ptr, n2ptr));
}

/*------------------------------------------------------*/
/* Standard delimiter matching character.		*/
/*------------------------------------------------------*/

char standard_delimiter_end(char source)
{
   char target;
   switch(source) {
      case '(':  target = ')'; break;
      case '[':  target = ']'; break;
      case '{':  target = '}'; break;
      case '<':  target = '>'; break;
      default:   target = source;
   }
   return target;
}

/*------------------------------------------------------*/
/* Find matching parenthesis, bracket, brace, or tag	*/
/* Don't count when backslash character '\' is in front */
/*------------------------------------------------------*/

u_char *find_delimiter(u_char *fstring)
{
   int count = 1;

   u_char *search = fstring; 
   u_char source = *fstring;
   u_char target;

   target = (u_char)standard_delimiter_end((char)source);
   while (*++search != '\0') {
      if (*search == source && *(search - 1) != '\\') count++;
      else if (*search == target && *(search - 1) != '\\') count--;
      if (count == 0) break;
   }
   return search;
}

/*----------------------------------------------------------------------*/
/* Remove unnecessary font change information from a label		*/
/*----------------------------------------------------------------------*/

void cleanuplabel(stringpart **strhead)
{
   stringpart *curpart = *strhead;
   int oldfont, curfont;
   Boolean fline = False;

   oldfont = curfont = -1;

   while (curpart != NULL) {
      switch (curpart->type) {
	 case FONT_NAME:
	    if (curfont == curpart->data.font) {
	       /* Font change is redundant:  remove it */
	       /* Careful!  font changes remove overline/underline; if	*/
	       /* either one is in effect, replace it with "noline"	*/
	       if (fline)
		  curpart->type = NOLINE;
	       else
	          curpart = deletestring(curpart, strhead, NULL);
	    }
	    else {
	       curfont = curpart->data.font;
	    }
	    break;

	 case FONT_SCALE:
	    /* Old style font scale is always written absolute, not relative.	*/
	    /* Changes in scale were not allowed, so just get rid of them.	*/
	    if (compare_version(version, "2.3") < 0)
	       curpart = deletestring(curpart, strhead, areawin->topinstance);
	    break;

	 /* A font change may occur inside a parameter, so any font	*/
	 /* declaration after a parameter must be considered to be	*/
	 /* intentional.						*/

	 case PARAM_END:
	    curfont = oldfont = -1;
	    break;

	 case OVERLINE: case UNDERLINE:
	    fline = True;
	    break;

	 case NOLINE:
	    fline = False;
	    break;

	 case NORMALSCRIPT: case RETURN:
	    if (oldfont != -1) {
	       curfont = oldfont;
	       oldfont = -1;
	    }
	    break;

	 case SUBSCRIPT: case SUPERSCRIPT:
	    if (oldfont == -1)
	       oldfont = curfont;
	    break;
      }
      if (curpart != NULL)
	 curpart = curpart->nextpart;
   }
}

/*----------------------------------------------------------------------*/
/* Read label segments							*/
/*----------------------------------------------------------------------*/

void readlabel(objectptr localdata, char *lineptr, stringpart **strhead)
{
   Boolean fline = False;
   /* char *sptr; (jdk) */
   short j;
   char *endptr, *segptr = lineptr;
   char key[100];
   stringpart *newpart;
   oparamptr ops;

   while (*segptr != '\0') {	/* Look through all segments */

      while (isspace(*segptr) && (*segptr != '\0')) segptr++;

      if (*segptr == '(' || *segptr == '{') {
         endptr = find_delimiter(segptr);
         *endptr++ = '\0';
	 /* null string (e.g., null parameter substitution) */
	 if ((*segptr == '(') && (*(segptr + 1) == '\0')) {
	    segptr = endptr;
	    continue;
	 }
      }
      else if (*segptr == '\0' || *segptr == '}') break;

      makesegment(strhead, *strhead);
      newpart = *strhead;

      /* Embedded command is in braces: {} */

      if (*segptr == '{') {	

         /* Find the command for this PostScript procedure */
	 char *cmdptr = endptr - 2;
         while (isspace(*cmdptr)) cmdptr--;
         while (!isspace(*cmdptr) && (cmdptr > segptr)) cmdptr--;
	 cmdptr++;
	 segptr++;

         if (!strncmp(cmdptr, "Ss", 2))
	    newpart->type = SUPERSCRIPT;
         else if (!strncmp(cmdptr, "ss", 2))
            newpart->type = SUBSCRIPT;
         else if (!strncmp(cmdptr, "ns", 2))
            newpart->type = NORMALSCRIPT;
         else if (!strncmp(cmdptr, "hS", 2))
            newpart->type = HALFSPACE;
         else if (!strncmp(cmdptr, "qS", 2))
            newpart->type = QTRSPACE;
         else if (!strncmp(cmdptr, "CR", 2)) {
	    newpart->type = RETURN;
	    newpart->data.flags = 0;
	 }
         else if (!strcmp(cmdptr, "Ts")) /* "Tab set" command */
	    newpart->type = TABSTOP;
         else if (!strcmp(cmdptr, "Tf")) /* "Tab forward" command */
	    newpart->type = TABFORWARD;
         else if (!strcmp(cmdptr, "Tb")) /* "Tab backward" command */
	    newpart->type = TABBACKWARD;
         else if (!strncmp(cmdptr, "ol", 2)) {
            newpart->type = OVERLINE;
            fline = True;
         }
         else if (!strncmp(cmdptr, "ul", 2)) {
            newpart->type = UNDERLINE;
            fline = True;
         }
         else if (!strncmp(cmdptr, "sce", 3)) {  /* revert to default color */
            newpart->type = FONT_COLOR;
	    newpart->data.color = DEFAULTCOLOR;
         }
         else if (cmdptr == segptr) {   /* cancel over- or underline */
            newpart->type = NOLINE;
            fline = False;
         }
	 /* To-do:  replace old-style backspace with tab stop */
         else if (!strcmp(cmdptr, "bs")) {
	    Wprintf("Warning:  Obsolete backspace command ommitted in text");
         }
         else if (!strcmp(cmdptr, "Kn")) {  /* "Kern" command */
	    int kx, ky;
	    sscanf(segptr, "%d %d", &kx, &ky);  
	    newpart->type = KERN;
	    newpart->data.kern[0] = kx;
	    newpart->data.kern[1] = ky;
         }
	 else if (!strcmp(cmdptr, "MR")) { /* "Margin stop" command */
	    int width;
	    sscanf(segptr, "%d", &width);  
	    newpart->type = MARGINSTOP;
	    newpart->data.width = width;
	 }
         else if (!strcmp(cmdptr, "scb")) {  /* change color command */
	    float cr, cg, cb;
	    int cval, cindex;
	    sscanf(segptr, "%f %f %f", &cr, &cg, &cb);  
            newpart->type = FONT_COLOR;
	    cindex = rgb_alloccolor((int)(cr * 65535), (int)(cg * 65535),
		   (int)(cb * 65535));
	    newpart->data.color = cindex;
         }
         else if (!strcmp(cmdptr, "cf")) {  /* change font or scale command */
	    char *nextptr, *newptr = segptr;

	    /* Set newptr to the fontname and nextptr to the next token. */
	    while (*newptr != '/' && *newptr != '\0') newptr++;
	    if (*newptr++ == '\0') {
	       Wprintf("Error:  Bad change-font command");
	       newpart->type = NOLINE;	/* placeholder */
	    }
	    for (nextptr = newptr; !isspace(*nextptr); nextptr++);
	    *(nextptr++) = '\0';
	    while (isspace(*nextptr)) nextptr++;

            for (j = 0; j < fontcount; j++)
               if (!strcmp(newptr, fonts[j].psname))
	          break;

            if (j == fontcount) 		/* this is a non-loaded font */
               if (loadfontfile(newptr) < 0) {
		  if (fontcount > 0) {
		     Wprintf("Error:  Font \"%s\" not found---using default.", newptr);
		     j = 0;
		  }
		  else {
		     Wprintf("Error:  No fonts!");
	    	     newpart->type = NOLINE;	/* placeholder */
		  }
	       }

            if (isdigit(*nextptr)) { /* second form of "cf" command---includes scale */
	       float locscale;
	       sscanf(nextptr, "%f", &locscale);
	       newpart->type = FONT_SCALE;
	       newpart->data.scale = locscale;
	       makesegment(strhead, *strhead);
	       newpart = *strhead;
            }
	    newpart->type = FONT_NAME;
	    newpart->data.font = j;
         }
         else {   /* This exec isn't a known label function */
	    Wprintf("Error:  unknown substring function");
	    newpart->type = NOLINE;	/* placeholder */
         }
      }

      /* Text substring is in parentheses: () */

      else if (*segptr == '(') {
         if (fline == True) {
	    newpart->type = NOLINE;
            makesegment(strhead, *strhead);
	    newpart = *strhead;
	    fline = False;
         }
         newpart->type = TEXT_STRING;
         newpart->data.string = (u_char *)malloc(1 + strlen(++segptr));

         /* Copy string, translating octal codes into 8-bit characters */
	 parse_ps_string(segptr, newpart->data.string, strlen(segptr), TRUE, TRUE);
      }

      /* Parameterized substrings are denoted by parameter key.	*/
      /* The parameter (default and/or substitution value) is 	*/
      /* assumed to exist.					*/

      else {

	 parse_ps_string(segptr, key, 99, FALSE, TRUE);
	 if (strlen(key) > 0) {
	    newpart->type = PARAM_START;
	    newpart->data.string = (char *)malloc(1 + strlen(key));
	    strcpy(newpart->data.string, key);

	    /* check for compatibility between the parameter value and	*/
	    /* the number of parameters and parameter type.		*/

	    ops = match_param(localdata, key);
	    if (ops == NULL) {
	       Fprintf(stderr, "readlabel() error:  No such parameter %s!\n", key);
	       deletestring(newpart, strhead, areawin->topinstance);
	    }

            /* Fprintf(stdout, "Parameter %s called from object %s\n",	*/
	    /* key,	localdata->name);				*/
         }
	 endptr = segptr + 1;
	 while (!isspace(*endptr) && (*endptr != '\0')) endptr++;
      }
      segptr = endptr;
   }
}

/*--------------------------------------*/
/* skip over whitespace in string input */
/*--------------------------------------*/

char *skipwhitespace(char *lineptr)
{
   char *locptr = lineptr;

   while (isspace(*locptr) && (*locptr != '\n') && (*locptr != '\0')) locptr++;
   return locptr;
}

/*-------------------------------------------*/
/* advance to the next token in string input */
/*-------------------------------------------*/

char *advancetoken(char *lineptr)
{
   char *locptr = lineptr;

   while (!isspace(*locptr) && (*locptr != '\n') && (*locptr != '\0')) locptr++;
   while (isspace(*locptr) && (*locptr != '\n') && (*locptr != '\0')) locptr++;
   return locptr;
}

/*------------------------------------------------------*/
/* Read a parameter list for an object instance call.	*/
/* This uses the key-value dictionary method but also	*/
/* allows the "old style" in which each parameter	*/
/* was automatically assigned the key v1, v2, etc.	*/
/*------------------------------------------------------*/

void readparams(objectptr localdata, objinstptr newinst, objectptr libobj,
		char *buffer)
{
   oparamptr newops, objops, fops;
   char *arrayptr, *endptr, *arraynext;
   int paramno = 0;
   char paramkey[100];

   if ((arrayptr = strstr(buffer, "<<")) == NULL)
      if ((arrayptr = strchr(buffer, '[')) == NULL)
	 return;

   endptr = find_delimiter(arrayptr);
   if (*arrayptr == '<') {
      arrayptr++;	/* move to second '<' in "<<"	*/
      endptr--;		/* back up to first '>' in ">>"	*/
   }

   /* move to next non-space token after opening bracket */
   arrayptr++;
   while (isspace(*arrayptr) && *arrayptr != '\0') arrayptr++;

   while ((*arrayptr != '\0') && (arrayptr < endptr)) {

      newops = (oparamptr)malloc(sizeof(oparam));

      /* Arrays contain values only.  Dictionaries contain key:value pairs */
      if (*endptr == '>') {	/* dictionary type */
	 if (*arrayptr != '/') {
	    Fprintf(stdout, "Error: Dictionary key is a literal, not a name\n");
	 }
	 else arrayptr++;	/* Skip PostScript name delimiter */
	 parse_ps_string(arrayptr, paramkey, 99, FALSE, TRUE);
	 newops->key = (char *)malloc(1 + strlen(paramkey));
	 strcpy(newops->key, paramkey);
	 arrayptr = advancetoken(arrayptr);
      }
      else {		/* array type; keys are "v1", "v2", etc. */
	 paramno++;
	 newops->key = (char *)malloc(6);
	 sprintf(newops->key, "v%d", paramno);
      }

      /* Find matching parameter in object definition */
      if (newinst) {
	 objops = match_param(libobj, newops->key);
	 if (objops == NULL) {
	    Fprintf(stdout, "Error: parameter %s does not exist in object %s!\n",
			newops->key, libobj->name);
	    free(newops->key);
	    free(newops);
	    return;
	 }
      }

      /* Add to instance's parameter list */
      /* If newinst is null, then the parameters are added to libobj */
      newops->next = NULL;
      if (newinst) {

	 /* Delete any parameters with duplicate names.  This	*/
	 /* This may indicate an expression parameter that was	*/
	 /* precomputed while determining the bounding box.	*/

	 for (fops = newinst->params; fops != NULL; fops = fops->next)
	    if (!strcmp(fops->key, newops->key))
	       if ((fops = free_instance_param(newinst, fops)) == NULL)
		  break;

	 if (newinst->params == NULL)
	    newinst->params = newops;
	 else {
	    for (fops = newinst->params; fops->next != NULL; fops = fops->next);
	    fops->next = newops;
	 }
      }
      else {
	 if (libobj->params == NULL)
	    libobj->params = newops;
	 else {
	    for (fops = libobj->params; fops->next != NULL; fops = fops->next);
	    fops->next = newops;
	 }
      }

      /* Fill in "which" entry from the object default */
      newops->which = (newinst) ? objops->which : 0;

      /* Check next token.  If not either end-of-dictionary or	*/
      /* the next parameter key, then value is an expression.	*/
      /* Expressions are written as two strings, the first the	*/
      /* result of evaluting the expression, and the second the	*/
      /* expression itself, followed by "pop" to prevent the	*/
      /* PostScript interpreter from trying to evaluate the	*/
      /* expression (which is not in PostScript).		*/

      if (*arrayptr == '(' || *arrayptr == '{')
	 arraynext = find_delimiter(arrayptr);
      else
	 arraynext = arrayptr;
      arraynext = advancetoken(arraynext);

      if ((*endptr == '>') && (arraynext < endptr) && (*arraynext != '/')) {
	 char *substrend, *arraysave;

	 if (*arraynext == '(' || *arraynext == '{') {

	    substrend = find_delimiter(arraynext);
	    arraysave = arraynext + 1;
	    arraynext = advancetoken(substrend);

	    newops->type = (u_char)XC_EXPR;
	    newops->which = P_EXPRESSION;	/* placeholder */
	 }

	 if (strncmp(arraynext, "pop ", 4)) {
	    Wprintf("Error:  bad expression parameter!\n");
#ifdef TCL_WRAPPER
	    newops->parameter.expr = strdup("expr 0");
#else
	    newops->parameter.expr = strdup("0");
#endif
	    arrayptr = advancetoken(arrayptr);
	 } else {
	    *substrend = '\0';
	    newops->parameter.expr = strdup(arraysave);
	    arrayptr = advancetoken(arraynext);
	 }
      }

      else if (*arrayptr == '(' || *arrayptr == '{') { 
	 float r, g, b;
	 char *substrend, csave;
	 stringpart *endpart;

	 /* type XC_STRING */

	 substrend = find_delimiter(arrayptr);
	 csave = *(++substrend);
	 *substrend = '\0';
	 if (*arrayptr == '{') arrayptr++;

	 /* A numerical value immediately following the opening */
	 /* brace indicates a color parameter.			*/
	 if (sscanf(arrayptr, "%f %f %f", &r, &g, &b) == 3) {
	    newops->type = (u_char)XC_INT;
	    newops->which = P_COLOR;
	    newops->parameter.ivalue = rgb_alloccolor((int)(r * 65535),
		(int)(g * 65535), (int)(b * 65535)); 
	    *substrend = csave;
	 }
	 else {
	    char *arraytmp = arrayptr;
	    char linkdefault[5] = "(%n)";

	    newops->type = (u_char)XC_STRING;
	    newops->which = P_SUBSTRING;
	    newops->parameter.string = NULL;

	    /* Quick check for "link" parameter:  make object name into "%n" */
	    if (!strcmp(newops->key, "link"))
	       if (!strncmp(arrayptr + 1, libobj->name, strlen(libobj->name)) &&
			!strcmp(arrayptr + strlen(libobj->name) + 1, ")"))
		  arraytmp = linkdefault;
	    
	    readlabel(libobj, arraytmp, &(newops->parameter.string));
	    *substrend = csave;

	    /* Append a PARAM_END to the parameter string */

	    endpart = makesegment(&(newops->parameter.string), NULL);
	    endpart->type = PARAM_END;
	    endpart->data.string = (u_char *)NULL;
	 }
	 arrayptr = substrend;
	 while (isspace(*arrayptr) && *arrayptr != '\0')
	    arrayptr++;
      }
      else {
	/* char *token; (jdk) */
	 int scanned = 0;

	 /* type XC_FLOAT or XC_INT, or an indirect reference */

	 newops->type = (newinst) ? objops->type : (u_char)XC_FLOAT;

	 if (newops->type == XC_FLOAT) {
	    scanned = sscanf(arrayptr, "%f", &(newops->parameter.fvalue));
	    /* Fprintf(stdout, "Object %s called with parameter "
			"%s value %g\n", libobj->name,
			newops->key, newops->parameter.fvalue); */
	 }
	 else if (newops->type == XC_INT) {
	    scanned = sscanf(arrayptr, "%d", &(newops->parameter.ivalue));
	    /* Fprintf(stdout, "Object %s called with parameter "
			"%s value %d\n", libobj->name,
			newops->key, newops->parameter.ivalue); */
	 }
	 else if (newops->type == XC_EXPR) {
	    /* Instance values of parameters hold the last evaluated	*/
	    /* result and will be regenerated, so we can ignore them	*/
	    /* here.  By ignoring it, we don't have to deal with issues	*/
	    /* like type promotion.					*/
	    free_instance_param(newinst, newops);
	    scanned = 1;	/* avoid treating as an indirect ref */
	 }
	 else if (newops->type == XC_STRING) {
	    /* Fill string record, so we have a valid record.  This will */
	    /* be blown away and replaced by opsubstitute(), but it must */
	    /* have an initial valid entry.				 */
	    stringpart *tmpptr;
	    newops->parameter.string = NULL;
	    tmpptr = makesegment(&newops->parameter.string, NULL);
	    tmpptr->type = TEXT_STRING;
	    tmpptr = makesegment(&newops->parameter.string, NULL);
	    tmpptr->type = PARAM_END;
	 }
	 else {
	    Fprintf(stderr, "Error: unknown parameter type!\n");
	 }

	 if (scanned == 0) {
	    /* Indirect reference --- create an eparam in the instance */
	    parse_ps_string(arrayptr, paramkey, 99, FALSE, TRUE);

	    if (!newinst || !localdata) {
		/* Only object instances can use indirect references */
		Fprintf(stderr, "Error: parameter default %s cannot "
			"be parsed!\n", paramkey);
	    }
	    else if (match_param(localdata, paramkey) == NULL) {
		/* Reference key must exist in the calling object */
		Fprintf(stderr, "Error: parameter value %s cannot be parsed!\n",
			paramkey);
	    }
	    else {
	       /* Create an eparam record in the instance */
	       eparamptr newepp = make_new_eparam(paramkey);
	       newepp->flags |= P_INDIRECT;
	       newepp->pdata.refkey = strdup(newops->key);
	       newepp->next = newinst->passed;
	       newinst->passed = newepp;
	    }

	 }
	 arrayptr = advancetoken(arrayptr);
      }
   }

   /* Calculate the unique bounding box for the instance */

   if (newinst && (newinst->params != NULL)) {
      opsubstitute(libobj, newinst);
      calcbboxinst(newinst);
   }
}

/*--------------------------------------------------------------*/
/* Read a value which might be a short integer or a parameter.	*/
/* If the value is a parameter, check the parameter list to see */
/* if it needs to be re-typecast.  Return the position to the	*/
/* next token in "lineptr".					*/
/*--------------------------------------------------------------*/

char *varpscan(objectptr localdata, char *lineptr, short *hvalue,
	genericptr thiselem, int pointno, int offset, u_char which)
{
   oparamptr ops = NULL;
   char key[100];
   /* char *nexttok; (jdk) */
   eparamptr newepp;

   if (sscanf(lineptr, "%hd", hvalue) != 1) {
      parse_ps_string(lineptr, key, 99, FALSE, TRUE);

      ops = match_param(localdata, key);
      newepp = make_new_eparam(key);

      /* Add parameter to the linked list */
      newepp->next = thiselem->passed;
      thiselem->passed = newepp;
      newepp->pdata.pointno = pointno;

      if (ops != NULL) {

	 /* It cannot be known whether a parameter value is a float or int */
	 /* until we see how the parameter is used.  So we always read the */
	 /* parameter default as a float, and re-typecast it if necessary. */

	 if (ops->type == XC_FLOAT) {
	    ops->type = XC_INT;
	    /* (add 0.1 to avoid roundoff error in conversion to integer) */
	    ops->parameter.ivalue = (int)(ops->parameter.fvalue +
			((ops->parameter.fvalue < 0) ? -0.1 : 0.1));
	 }
	 ops->which = which;
	 *hvalue = (short)ops->parameter.ivalue;
      }
      else {
	 *hvalue = 0; /* okay; will get filled in later */
	 Fprintf(stderr, "Error:  parameter %s was used but not defined!\n", key);
      }
   }

   *hvalue -= (short)offset;
  
   return advancetoken(skipwhitespace(lineptr));
}

/*--------------------------------------------------------------*/
/* Read a value which might be a short integer or a parameter,	*/
/* but which is not a point in a pointlist.			*/
/*--------------------------------------------------------------*/

char *varscan(objectptr localdata, char *lineptr, short *hvalue,
	 genericptr thiselem, u_char which)
{
   return varpscan(localdata, lineptr, hvalue, thiselem, 0, 0, which);
}

/*--------------------------------------------------------------*/
/* Read a value which might be a float or a parameter.		*/
/* Return the position to the next token in "lineptr".		*/
/*--------------------------------------------------------------*/

char *varfscan(objectptr localdata, char *lineptr, float *fvalue,
	genericptr thiselem, u_char which)
{
   oparamptr ops = NULL;
   eparamptr newepp;
   char key[100];

   if (sscanf(lineptr, "%f", fvalue) != 1) {
      parse_ps_string(lineptr, key, 99, FALSE, TRUE);

      /* This bit of a hack takes care of scale-variant	*/
      /* linewidth specifiers for object instances.	*/

      if (!strncmp(key, "/sv", 3)) {
	 ((objinstptr)thiselem)->style &= ~LINE_INVARIANT;
	 return varfscan(localdata, advancetoken(skipwhitespace(lineptr)),
			fvalue, thiselem, which);
      }
	
      ops = match_param(localdata, key);
      newepp = make_new_eparam(key);

      /* Add parameter to the linked list */
      newepp->next = thiselem->passed;
      thiselem->passed = newepp;

      if (ops != NULL) {
	 ops->which = which;
	 *fvalue = ops->parameter.fvalue;
      }
      else
	 Fprintf(stderr, "Error: no parameter \"%s\" defined!\n", key);
   }

   /* advance to next token */
   return advancetoken(skipwhitespace(lineptr));
}

/*--------------------------------------------------------------*/
/* Same as varpscan(), but for path types only.			*/
/*--------------------------------------------------------------*/

char *varpathscan(objectptr localdata, char *lineptr, short *hvalue,
	genericptr *thiselem, pathptr thispath, int pointno, int offset,
	u_char which, eparamptr *nepptr)
{
   oparamptr ops = NULL;
   char key[100];
   eparamptr newepp;

   if (nepptr != NULL) *nepptr = NULL;

   if (sscanf(lineptr, "%hd", hvalue) != 1) {
      parse_ps_string(lineptr, key, 99, FALSE, TRUE);
      ops = match_param(localdata, key);
      newepp = make_new_eparam(key);
      newepp->pdata.pathpt[1] = pointno;

      if (thiselem == NULL)
         newepp->pdata.pathpt[0] = (short)0;
      else {
	 short elemidx = (short)(thiselem - thispath->plist);
	 if (elemidx >= 0 && elemidx < thispath->parts)
            newepp->pdata.pathpt[0] = (short)(thiselem - thispath->plist);
	 else {
	    Fprintf(stderr, "Error:  Bad parameterized path point!\n");
	    free(newepp);
	    goto pathdone;
	 }
      }
      if (nepptr != NULL) *nepptr = newepp;

      /* Add parameter to the linked list. */

      newepp->next = thispath->passed;
      thispath->passed = newepp;

      if (ops != NULL) {

	 /* It cannot be known whether a parameter value is a float or int */
	 /* until we see how the parameter is used.  So we always read the */
	 /* parameter default as a float, and re-typecast it if necessary. */

	 if (ops->type == XC_FLOAT) {
	    ops->type = XC_INT;
	    /* (add 0.1 to avoid roundoff error in conversion to integer) */
	    ops->parameter.ivalue = (int)(ops->parameter.fvalue +
			((ops->parameter.fvalue < 0) ? -0.1 : 0.1));
	 }
	 ops->which = which;
	 *hvalue = (short)ops->parameter.ivalue;
      }
      else {
	 *hvalue = 0; /* okay; will get filled in later */
	 Fprintf(stderr, "Error:  parameter %s was used but not defined!\n", key);
      }
   }

pathdone:
   *hvalue -= (short)offset;
   return advancetoken(skipwhitespace(lineptr));
}

/*--------------------------------------------------------------*/
/* Create a new instance of an object in the library's list of	*/
/* instances.  This instance will be used on the library page	*/
/* when doing "composelib()".					*/
/*--------------------------------------------------------------*/

objinstptr addtoinstlist(int libnum, objectptr libobj, Boolean virtual)
{
   objinstptr newinst = (objinstptr) malloc(sizeof(objinst));
   liblistptr spec = (liblistptr) malloc(sizeof(liblist));
   liblistptr srch;

   newinst->type = OBJINST;
   instancedefaults(newinst, libobj, 0, 0);

   spec->virtual = (u_char)virtual;
   spec->thisinst = newinst;
   spec->next = NULL;

   /* Add to end, so that duplicate, parameterized instances	*/
   /* always come after the original instance with the default	*/
   /* parameters.						*/

   if ((srch = xobjs.userlibs[libnum].instlist) == NULL)
      xobjs.userlibs[libnum].instlist = spec;
   else {
      while (srch->next != NULL) srch = srch->next;
      srch->next = spec;
   }

   /* Calculate the instance-specific bounding box */
   calcbboxinst(newinst);

   return newinst;
}

/*--------------------------------------------------------------*/
/* Deal with object reads:  Create a new object and prepare for	*/
/* reading.  The library number is passed as "mode".		*/
/*--------------------------------------------------------------*/

objectptr *new_library_object(short mode, char *name, objlistptr *retlist,
		TechPtr defaulttech)
{
   objlistptr newdef, redef = NULL;
   objectptr *newobject, *libobj;
   objectptr *curlib = (mode == FONTLIB) ?
		xobjs.fontlib.library : xobjs.userlibs[mode - LIBRARY].library;
   short *libobjects = (mode == FONTLIB) ?
		&xobjs.fontlib.number : &xobjs.userlibs[mode - LIBRARY].number;
   int i, j;
   char *nsptr, *fullname = name;

   curlib = (objectptr *) realloc(curlib, (*libobjects + 1)
		      * sizeof(objectptr));
   if (mode == FONTLIB) xobjs.fontlib.library = curlib;
   else xobjs.userlibs[mode - LIBRARY].library = curlib;

   /* For (older) libraries that do not use technologies, give the	*/
   /* object a technology name in the form <library>::<object>		*/

   if ((nsptr = strstr(name, "::")) == NULL) {
      int deftechlen = (defaulttech == NULL) ? 0 : strlen(defaulttech->technology);
      fullname = (char *)malloc(deftechlen + strlen(name) + 3);
      if (defaulttech == NULL)
         sprintf(fullname, "::%s", name);
      else
         sprintf(fullname, "%s::%s", defaulttech->technology, name);
   }

   /* initial 1-pointer allocations */

   newobject = curlib + (*libobjects);
   *newobject = (objectptr) malloc(sizeof(object));
   initmem(*newobject);

   /* check that this object is not already in list of objects */

   if (mode == FONTLIB) {
      for (libobj = xobjs.fontlib.library; libobj != xobjs.fontlib.library +
            xobjs.fontlib.number; libobj++) {
	 /* This font character may be a redefinition of another */
	 if (!objnamecmp(fullname, (*libobj)->name)) {
	    newdef = (objlistptr) malloc(sizeof(objlist));
	    newdef->libno = FONTLIB;
	    newdef->thisobject = *libobj;
	    newdef->next = redef;
	    redef = newdef;
	 }
      }
   }
   else {
      for (i = 0; i < xobjs.numlibs; i++) {
	 for (j = 0; j < xobjs.userlibs[i].number; j++) {
	    libobj = xobjs.userlibs[i].library + j;
	    /* This object may be a redefinition of another object */
	    if (!objnamecmp(fullname, (*libobj)->name)) {
	       newdef = (objlistptr) malloc(sizeof(objlist));
	       newdef->libno = i + LIBRARY;
	       newdef->thisobject = *libobj;
	       newdef->next = redef;
	       redef = newdef;
	    }
	 }
      }
   }

   (*libobjects)++;
   sprintf((*newobject)->name, "%s", fullname);
   if (fullname != name) free(fullname);

   /* initmem() initialized schemtype to PRIMARY;  change it. */
   (*newobject)->schemtype = (mode == FONTLIB) ? GLYPH : SYMBOL;

   /* If the object declares a technology name that is different from the */
   /* default, then add the technology name to the list of technologies,  */
   /* with a NULL filename.						  */

   if (mode != FONTLIB) AddObjectTechnology(*newobject);

   *retlist = redef;
   return newobject;
}

/*--------------------------------------------------------------*/
/* do an exhaustive comparison between a new object and any	*/
/* object having the same name. If they are the same, destroy	*/
/* the duplicate.  If different, rename the original one.	*/
/*--------------------------------------------------------------*/

Boolean library_object_unique(short mode, objectptr newobject, objlistptr redef)
{
   Boolean is_unique = True;
   objlistptr newdef;
   short *libobjects = (mode == FONTLIB) ?
		&xobjs.fontlib.number : &xobjs.userlibs[mode - LIBRARY].number;

   if (redef == NULL)
      return is_unique;	/* No name conflicts; object is okay as-is */

   for (newdef = redef; newdef != NULL; newdef = newdef->next) {

      /* Must make sure that default parameter values are */
      /* plugged into both objects!		  	  */
      opsubstitute(newdef->thisobject, NULL);
      opsubstitute(newobject, NULL);

      if (objcompare(newobject, newdef->thisobject) == True) {
	  addalias(newdef->thisobject, newobject->name);

	  /* If the new object has declared an association to a */
	  /* schematic, transfer it to the original, and make   */
	  /* sure that the page points to the object which will */
	  /* be left, not the one which will be destroyed.	*/

	  if (newobject->symschem != NULL) {
	     newdef->thisobject->symschem = newobject->symschem;
	     newdef->thisobject->symschem->symschem = newdef->thisobject;
	  }

	  reset(newobject, DESTROY);
	  (*libobjects)--;
	  is_unique = False;
	  break;
       }

       /* Not the same object, but has the same name.  This can't	*/
       /* happen within the same input file, so the name of the		*/
       /* original object can safely be altered.			*/

       else if (!strcmp(newobject->name, newdef->thisobject->name)) {

	  /* Replacement---for project management, allow the technology	*/
	  /* master version to take precedence over a local version.	*/

	  TechPtr nsptr = GetObjectTechnology(newobject);

	  if (nsptr && (nsptr->flags & TECH_REPLACE)) {
	     reset(newobject, DESTROY);
	     (*libobjects)--;
	     is_unique = False;
	  }
	  else
	     checkname(newdef->thisobject);
	  break;
       }
    }
    for (; (newdef = redef->next); redef = newdef)
       free(redef);
    free(redef);

    return is_unique;
}

/*--------------------------------------------------------------*/
/* Add an instance of the object to the library's instance list */
/*--------------------------------------------------------------*/

void add_object_to_library(short mode, objectptr newobject)
{
   objinstptr libinst;

   if (mode == FONTLIB) return;

   libinst = addtoinstlist(mode - LIBRARY, newobject, False);
   calcbboxvalues(libinst, (genericptr *)NULL);

   /* Center the view of the object in its instance */
   centerview(libinst);
}

/*--------------------------------------------------------------*/
/* Continuation Line --- add memory to "buffer" as necessary.	*/
/* Add a space character to the current text in "buffer" and	*/
/* return a pointer to the new end-of-text.			*/
/*--------------------------------------------------------------*/

char *continueline(char **buffer)
{
   char *lineptr;
   int bufsize;

   for (lineptr = *buffer; (*lineptr != '\n') && (*lineptr != '\0'); lineptr++);
   /* Repair Windoze-mangled files */
   if ((lineptr > *buffer) && (*lineptr == '\n') && (*(lineptr - 1) == '\r'))
      *(lineptr - 1) = ' ';
   if (*lineptr == '\n') *lineptr++ = ' ';

   bufsize = (int)(lineptr - (*buffer)) + 256;
   *buffer = (char *)realloc((*buffer), bufsize * sizeof(char));

   return ((*buffer) + (bufsize - 256));
}

/*--------------------------------------------------------------*/
/* Read image data out of the Setup block of the input		*/
/* We assume that width and height have been parsed from the	*/
/* "imagedata" line and the file pointer is at the next line.	*/
/*--------------------------------------------------------------*/

void readimagedata(FILE *ps, int width, int height)
{
   char temp[150], ascbuf[6];
   int x, y, p, q, r, g, b, ilen;
   char *pptr;
   Imagedata *iptr;
   Boolean do_flate = False, do_ascii = False;
   u_char *filtbuf, *flatebuf;
   union {
      u_char b[4];
      u_long i;
   } pixel;

   iptr = addnewimage(NULL, width, height);

   /* Read the image data */
   
   fgets(temp, 149, ps);
   if (strstr(temp, "ASCII85Decode") != NULL) do_ascii = TRUE;
#ifdef HAVE_LIBZ
   if (strstr(temp, "FlateDecode") != NULL) do_flate = TRUE;
#else
   if (strstr(temp, "FlateDecode") != NULL)
      Fprintf(stderr, "Error:  Don't know how to Flate decode!"
		"  Get zlib and recompile xcircuit!\n");
#endif
   while (strstr(temp, "ReusableStreamDecode") == NULL)
      fgets(temp, 149, ps);  /* Additional piped filter lines */

   fgets(temp, 149, ps);  /* Initial data line */
   q = 0;
   pptr = temp;
   ilen = 3 * width * height;
   filtbuf = (u_char *)malloc(ilen + 4);

   if (!do_ascii) {	/* ASCIIHexDecode algorithm */
      q = 0;
      for (y = 0; y < height; y++) {
         for (x = 0; x < width; x++) {
            sscanf(pptr, "%02x%02x%02x", &r, &g, &b);
            filtbuf[q++] = (u_char)r;
            filtbuf[q++] = (u_char)g;
            filtbuf[q++] = (u_char)b;
            pptr += 6;
            if (*pptr == '\n') {
               fgets(temp, 149, ps);
               pptr = temp;
            }
         }
      }
   }
   else {		/* ASCII85Decode algorithm */
      p = 0;
      while (1) {
         ascbuf[0] = *pptr;
         pptr++;
         if (ascbuf[0] == '~')
	    break;
         else if (ascbuf[0] == 'z') {
	    for (y = 0; y < 5; y++) ascbuf[y] = '\0';
         }
         else {
	    for (y = 1; y < 5; y++) {
	       if (*pptr == '\n') {
	          fgets(temp, 149, ps);
	          pptr = temp;
	       }
	       ascbuf[y] = *pptr;
	       if (ascbuf[y] == '~') {
	          for (; y < 5; y++) {
		     ascbuf[y] = '!';
		     p++;
		  }
	          break;
	       }
	       else pptr++;
	    }
	    for (y = 0; y < 5; y++) ascbuf[y] -= '!';
         }

         if (*pptr == '\n') {
	    fgets(temp, 149, ps);
	    pptr = temp;
         }

         /* Decode from ASCII85 to binary */

         pixel.i = ascbuf[4] + ascbuf[3] * 85 + ascbuf[2] * 7225 +
		ascbuf[1] * 614125 + ascbuf[0] * 52200625;

	 /* Add in roundoff for final bytes */
	 if (p > 0) {
	    switch (p) {
	       case 3:
		  pixel.i += 0xff0000;
	       case 2:
		  pixel.i += 0xff00;
	       case 1:
		  pixel.i += 0xff;
	    }
	 }

         for (y = 0; y < (4 - p); y++) {
	    filtbuf[q + y] = pixel.b[3 - y];
         }
         q += (4 - p);
         if (q >= ilen) break;
      }
   }

   /* Extra decoding goes here */

#ifdef HAVE_LIBZ
   if (do_flate) {
      flatebuf = (char *)malloc(ilen);
      large_inflate(filtbuf, q, &flatebuf, ilen);
      free(filtbuf);
   }
   else
#endif

   flatebuf = filtbuf;

   q = 0;
   for (y = 0; y < height; y++)
      for (x = 0; x < width; x++) {
	 u_char r, g, b;
	 r = flatebuf[q++];
	 g = flatebuf[q++];
	 b = flatebuf[q++];
	 xcImagePutPixel(iptr->image, x, y, r, g, b);
      }

   free(flatebuf);

   fgets(temp, 149, ps);	/* definition line */
   fgets(temp, 149, ps);	/* pick up name of image from here */
   for (pptr = temp; !isspace(*pptr); pptr++);
   *pptr = '\0';
   iptr->filename = strdup(temp + 1);
   for (x = 0; x < 5; x++) fgets(temp, 149, ps);  /* skip image dictionary */
}

/*--------------------------------------------------------------*/
/* Read an object (page) from a file into xcircuit		*/
/*--------------------------------------------------------------*/

Boolean objectread(FILE *ps, objectptr localdata, short offx, short offy,
	short mode, char *retstr, int ccolor, TechPtr defaulttech)
{
   char *temp, *buffer, keyword[80];
   short tmpfont = -1;
   float tmpscale = 0.0;
   objectptr	*libobj;
   int curcolor = ccolor;
   char *colorkey = NULL;
   char *widthkey = NULL;
   int i, j, k;
   short px, py;
   objinstptr *newinst;
   eparamptr epptrx, epptry;	/* used for paths only */

   /* path-handling variables */
   pathptr *newpath;
   XPoint startpoint;

   keyword[0] = '\0';

   buffer = (char *)malloc(256 * sizeof(char));
   temp = buffer;

   for(;;) {
      char *lineptr, *keyptr, *saveptr;

      if (fgets(temp, 255, ps) == NULL) {
	 if (strcmp(keyword, "restore")) {
            Wprintf("Error: end of file.");
	    *retstr = '\0';
	 }
	 break;
      }
      temp = buffer;

      /* because PostScript is a stack language, we will scan from the end */
      for (lineptr = buffer; (*lineptr != '\n') && (*lineptr != '\0'); lineptr++);
      /* Avoid CR-LF at EOL added by stupid Windoze programs */
      if ((lineptr > buffer) && *(lineptr - 1) == '\r') lineptr--;
      if (lineptr != buffer) {  /* ignore any blank lines */
         for (keyptr = lineptr - 1; isspace(*keyptr) && keyptr != buffer; keyptr--);
         for (; !isspace(*keyptr) && keyptr != buffer; keyptr--);
         sscanf(keyptr, "%79s", keyword);

         if (!strcmp(keyword, "showpage")) {
            strncpy(retstr, buffer, 150);
            retstr[149] = '\0';
	    free(buffer);

	    /* If we have just read a schematic that is attached	*/
	    /* to a symbol, check all of the pin labels in the symbol	*/
	    /* to see if they correspond to pin names in the schematic.	*/
	    /* The other way around (pin in schematic with no		*/
	    /* corresponding name in the symbol) is not an error.	*/

	    if (localdata->symschem != NULL) {
	       genericptr *pgen, *lgen;
	       labelptr plab, lcmp;
	       for (pgen = localdata->symschem->plist; pgen < localdata->
			symschem->plist + localdata->symschem->parts; pgen++) {
		  if (IS_LABEL(*pgen)) {
		     plab = TOLABEL(pgen);
		     if (plab->pin == LOCAL) {
			for (lgen = localdata->plist; lgen < localdata->plist +
					localdata->parts; lgen++) {
			   if (IS_LABEL(*lgen)) {
			      lcmp = TOLABEL(lgen);
			      if (lcmp->pin == LOCAL)
				 if (!stringcomprelaxed(lcmp->string, plab->string,
						areawin->topinstance))
				    break;
			   }
			}
			if (lgen == localdata->plist + localdata->parts) {
			   char *cptr, *d1ptr, *d2ptr;
			   char *pch = textprint(plab->string, areawin->topinstance);

			   /* Check for likely delimiters before applying warning */

			   if ((cptr = strchr(pch, ':')) != NULL) {
			      d1ptr = strchr(pch, '[');
			      d2ptr = strchr(pch, ']');
			      if (d1ptr != NULL && d2ptr != NULL &&
					d1ptr < cptr && d2ptr > cptr) {
				 if (areawin->buschar != '[') {
				    areawin->buschar = '[';
				    Fprintf(stderr, "Warning:  Bus character \'[\'"
					" apparently used but not declared.\n");
				 }
			      }
			      d1ptr = strchr(pch, '{');
			      d2ptr = strchr(pch, '}');
			      if (d1ptr != NULL && d2ptr != NULL &&
					d1ptr < cptr && d2ptr > cptr) {
				 if (areawin->buschar != '{') {
				    areawin->buschar = '{';
				    Fprintf(stderr, "Warning:  Bus character \'{\'"
					" apparently used in pin \"%s\""
					" but not declared.\n", pch);
				 }
			      }
			      d1ptr = strchr(pch, '<');
			      d2ptr = strchr(pch, '>');
			      if (d1ptr != NULL && d2ptr != NULL &&
					d1ptr < cptr && d2ptr > cptr) {
				 if (areawin->buschar != '<') {
				    areawin->buschar = '<';
				    Fprintf(stderr, "Warning:  Bus character \'<\'"
					" apparently used in pin \"%s\""
					" but not declared.\n", pch);
				 }
			      }
			      d1ptr = strchr(pch, '(');
			      d2ptr = strchr(pch, ')');
			      if (d1ptr != NULL && d2ptr != NULL &&
					d1ptr < cptr && d2ptr > cptr) {
				 if (areawin->buschar != '(') {
				    areawin->buschar = '(';
				    Fprintf(stderr, "Warning:  Bus character \'(\'"
					" apparently used in pin \"%s\""
					" but not declared.\n", pch);
				 }
			      }
			   }
			   else
			       Fprintf(stderr, "Warning:  Unattached pin \"%s\" in "
					"symbol %s\n", pch,
					localdata->symschem->name);
			   free(pch);
			}
		     }
		  }
	       }
	    }
	    return False;  /* end of page */
	 }

	 /* make a color change, adding the color if necessary */

	 else if (!strcmp(keyword, "scb")) {
	    float red, green, blue;
	    if (sscanf(buffer, "%f %f %f", &red, &green, &blue) == 3) {
	       curcolor = rgb_alloccolor((int)(red * 65535), (int)(green * 65535),
		   	(int)(blue * 65535)); 
	       colorkey = NULL;
	    }
	    else {
	       char tmpkey[30];
	       oparamptr ops;

	       parse_ps_string(buffer, tmpkey, 29, FALSE, TRUE);
	       ops = match_param(localdata, tmpkey);
	       if (ops != NULL) {
		  /* Recast expression parameter, if necessary */
		  if (ops->which == P_EXPRESSION) ops->which = P_COLOR;
		  if (ops->which == P_COLOR) {
		     colorkey = ops->key;
		     switch (ops->type) {
			case XC_INT:
		           curcolor = ops->parameter.ivalue;
			   break;
			default:
		           curcolor = DEFAULTCOLOR;	/* placeholder */
			   break;
		     }
		  }
	       }
	    }
	 }

	 /* end the color change, returning to default */

	 else if (!strcmp(keyword, "sce")) {
	    curcolor = ccolor;
	    colorkey = NULL;
	 }

	 /* begin a path constructor */

	 else if (!strcmp(keyword, "beginpath")) {
	    px = py = 0;
	    
	    NEW_PATH(newpath, localdata);
	    (*newpath)->plist = (genericptr *)malloc(sizeof(genericptr));
	    (*newpath)->parts = 0;
	    (*newpath)->color = curcolor;
	    (*newpath)->passed = NULL;

	    for (--keyptr; *keyptr == ' '; keyptr--);
	    for (; *keyptr != ' '; keyptr--);

	    /* check for "addtox" and "addtoy" parameter specification */
	    while (!strncmp(keyptr + 1, "addto", 5)) {
	       saveptr = keyptr + 1;

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);

	       /* Get parameter and its value */
	       if (*(saveptr + 5) == 'x')
	          varpscan(localdata, keyptr + 1, &px, (genericptr)*newpath,
				-1, offx, P_POSITION_X);
	       else
	          varpscan(localdata, keyptr + 1, &py, (genericptr)*newpath,
				-1, offy, P_POSITION_Y);

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);
	    }

	    lineptr = varpathscan(localdata, buffer, &startpoint.x,
			(genericptr *)NULL, *newpath, 0, offx + px, P_POSITION_X,
			&epptrx);
	    lineptr = varpathscan(localdata, lineptr, &startpoint.y,
			(genericptr *)NULL, *newpath, 0, offy + py, P_POSITION_Y,
			&epptry);

	    std_eparam((genericptr)(*newpath), colorkey);
	 }

	 /* end the path constructor */

	 else if (!strcmp(keyword, "endpath")) {

	    lineptr = varscan(localdata, buffer, &(*newpath)->style,
			(genericptr)*newpath, P_STYLE);
	    lineptr = varfscan(localdata, lineptr, &(*newpath)->width,
			(genericptr)*newpath, P_LINEWIDTH);

	    if ((*newpath)->parts <= 0) {	/* in case of an empty path */
	       free((*newpath)->plist);
	       free(*newpath);
	       localdata->parts--;
	    }
	    newpath = NULL;
	 }

	 /* read path parts */

	 else if (!strcmp(keyword, "polyc")) {
	    polyptr *newpoly;
	    pointlist newpoints;
	    short tmpnum;

	    px = py = 0;

	    NEW_POLY(newpoly, (*newpath));

	    for (--keyptr; *keyptr == ' '; keyptr--);
	    for (; *keyptr != ' '; keyptr--);

	    /* check for "addtox" and "addtoy" parameter specification */
	    while (!strncmp(keyptr + 1, "addto", 5)) {
	       saveptr = keyptr + 1;

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);

	       /* Get parameter and its value */
	       if (*(saveptr + 5) == 'x')
	          varpscan(localdata, keyptr + 1, &px, (genericptr)*newpoly,
				-1, offx, P_POSITION_X);
	       else
	          varpscan(localdata, keyptr + 1, &py, (genericptr)*newpoly,
				-1, offy, P_POSITION_Y);

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);
	    }

	    sscanf(keyptr, "%hd", &tmpnum);
	    (*newpoly)->number = tmpnum + 1;
	    (*newpoly)->width = 1.0;
	    (*newpoly)->style = UNCLOSED;
	    (*newpoly)->color = curcolor;
	    (*newpoly)->passed = NULL;
	    (*newpoly)->cycle = NULL;

            (*newpoly)->points = (pointlist) malloc((*newpoly)->number * 
		   sizeof(XPoint));

	    /* If the last point on the last path part was parameterized, then	*/
	    /* the first point of the spline must be, too.			*/

	    if (epptrx != NULL) {
	       eparamptr newepp = copyeparam(epptrx, (genericptr)(*newpath));
	       newepp->next = (*newpath)->passed;
	       (*newpath)->passed = newepp;
	       newepp->pdata.pathpt[1] = 0;
	       newepp->pdata.pathpt[0] = (*newpath)->parts - 1;
	    }
	    if (epptry != NULL) {
	       eparamptr newepp = copyeparam(epptry, (genericptr)(*newpath));
	       newepp->next = (*newpath)->passed;
	       (*newpath)->passed = newepp;
	       newepp->pdata.pathpt[1] = 0;
	       newepp->pdata.pathpt[0] = (*newpath)->parts - 1;
	    }

	    lineptr = buffer;

            newpoints = (*newpoly)->points + (*newpoly)->number - 1;
	    lineptr = varpathscan(localdata, lineptr, &newpoints->x,
			(genericptr *)newpoly, *newpath, newpoints - (*newpoly)->points,
			 offx + px, P_POSITION_X, &epptrx);
	    lineptr = varpathscan(localdata, lineptr, &newpoints->y,
			(genericptr *)newpoly, *newpath, newpoints - (*newpoly)->points,
			offy + py, P_POSITION_Y, &epptry);

            for (--newpoints; newpoints > (*newpoly)->points; newpoints--) {

	       lineptr = varpathscan(localdata, lineptr, &newpoints->x,
			(genericptr *)newpoly, *newpath, newpoints - (*newpoly)->points,
			 offx + px, P_POSITION_X, NULL);
	       lineptr = varpathscan(localdata, lineptr, &newpoints->y,
			(genericptr *)newpoly, *newpath, newpoints - (*newpoly)->points,
			offy + py, P_POSITION_Y, NULL);
	    }
	    newpoints->x = startpoint.x;
	    newpoints->y = startpoint.y;
	    startpoint.x = (newpoints + (*newpoly)->number - 1)->x;
	    startpoint.y = (newpoints + (*newpoly)->number - 1)->y;
	 }

	 else if (!strcmp(keyword, "arc") || !strcmp(keyword, "arcn")) {
	    arcptr *newarc;
	    NEW_ARC(newarc, (*newpath));
	    (*newarc)->width = 1.0;
	    (*newarc)->style = UNCLOSED;
	    (*newarc)->color = curcolor;
	    (*newarc)->passed = NULL;
	    (*newarc)->cycle = NULL;

	    lineptr = varpscan(localdata, buffer, &(*newarc)->position.x,
			(genericptr)*newarc, 0, offx, P_POSITION_X);
	    lineptr = varpscan(localdata, lineptr, &(*newarc)->position.y,
			(genericptr)*newarc, 0, offy, P_POSITION_Y);
	    lineptr = varscan(localdata, lineptr, &(*newarc)->radius,
			(genericptr)*newarc, P_RADIUS);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->angle1,
			(genericptr)*newarc, P_ANGLE1);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->angle2,
			(genericptr)*newarc, P_ANGLE2);

	    (*newarc)->yaxis = (*newarc)->radius;
	    if (!strcmp(keyword, "arcn")) {
	       float tmpang = (*newarc)->angle1;
	       (*newarc)->radius = -((*newarc)->radius);
	       (*newarc)->angle1 = (*newarc)->angle2;
	       (*newarc)->angle2 = tmpang;
	    }
		
	    calcarc(*newarc);
	    startpoint.x = (short)(*newarc)->points[(*newarc)->number - 1].x;
	    startpoint.y = (short)(*newarc)->points[(*newarc)->number - 1].y;
	    decomposearc(*newpath);
	 }

	 else if (!strcmp(keyword, "pellip") || !strcmp(keyword, "nellip")) {
	    arcptr *newarc;
	    NEW_ARC(newarc, (*newpath));
	    (*newarc)->width = 1.0;
	    (*newarc)->style = UNCLOSED;
	    (*newarc)->color = curcolor;
	    (*newarc)->passed = NULL;
	    (*newarc)->cycle = NULL;

	    lineptr = varpscan(localdata, buffer, &(*newarc)->position.x,
			(genericptr)*newarc, 0, offx, P_POSITION_X);
	    lineptr = varpscan(localdata, lineptr, &(*newarc)->position.y,
			(genericptr)*newarc, 0, offy, P_POSITION_Y);
	    lineptr = varscan(localdata, lineptr, &(*newarc)->radius,
			(genericptr)*newarc, P_RADIUS);
	    lineptr = varscan(localdata, lineptr, &(*newarc)->yaxis,
			(genericptr)*newarc, P_MINOR_AXIS);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->angle1,
			(genericptr)*newarc, P_ANGLE1);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->angle2,
			(genericptr)*newarc, P_ANGLE2);

	    if (!strcmp(keyword, "nellip")) {
	       float tmpang = (*newarc)->angle1;
	       (*newarc)->radius = -((*newarc)->radius);
	       (*newarc)->angle1 = (*newarc)->angle2;
	       (*newarc)->angle2 = tmpang;
		
	    }
	    calcarc(*newarc);
	    startpoint.x = (short)(*newarc)->points[(*newarc)->number - 1].x;
	    startpoint.y = (short)(*newarc)->points[(*newarc)->number - 1].y;
	    decomposearc(*newpath);
	 }

	 else if (!strcmp(keyword, "curveto")) {
	    splineptr *newspline;
	    px = py = 0;

	    NEW_SPLINE(newspline, (*newpath));
	    (*newspline)->passed = NULL;
	    (*newspline)->cycle = NULL;
	    (*newspline)->width = 1.0;
	    (*newspline)->style = UNCLOSED;
	    (*newspline)->color = curcolor;

	    /* If the last point on the last path part was parameterized, then	*/
	    /* the first point of the spline must be, too.			*/

	    if (epptrx != NULL) {
	       eparamptr newepp = copyeparam(epptrx, (genericptr)(*newpath));
	       newepp->next = (*newpath)->passed;
	       (*newpath)->passed = newepp;
	       newepp->pdata.pathpt[1] = 0;
	       newepp->pdata.pathpt[0] = (*newpath)->parts - 1;
	    }
	    if (epptry != NULL) {
	       eparamptr newepp = copyeparam(epptry, (genericptr)(*newpath));
	       newepp->next = (*newpath)->passed;
	       (*newpath)->passed = newepp;
	       newepp->pdata.pathpt[1] = 0;
	       newepp->pdata.pathpt[0] = (*newpath)->parts - 1;
	    }

	    for (--keyptr; *keyptr == ' '; keyptr--);
	    for (; *keyptr != ' '; keyptr--);

	    /* check for "addtox" and "addtoy" parameter specification */
	    while (!strncmp(keyptr + 1, "addto", 5)) {
	       saveptr = keyptr + 1;

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);

	       /* Get parameter and its value */
	       if (*(saveptr + 5) == 'x')
	          varpscan(localdata, keyptr + 1, &px, (genericptr)*newspline,
				-1, offx, P_POSITION_X);
	       else
	          varpscan(localdata, keyptr + 1, &py, (genericptr)*newspline,
				-1, offy, P_POSITION_Y);

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);
	    }


	    lineptr = varpathscan(localdata, buffer, &(*newspline)->ctrl[1].x,
			(genericptr *)newspline, *newpath, 1, offx + px, P_POSITION_X,
			NULL);
	    lineptr = varpathscan(localdata, lineptr, &(*newspline)->ctrl[1].y,
			(genericptr *)newspline, *newpath, 1, offy + py, P_POSITION_Y,
			NULL);
	    lineptr = varpathscan(localdata, lineptr, &(*newspline)->ctrl[2].x,
			(genericptr *)newspline, *newpath, 2, offx + px, P_POSITION_X,
			NULL);
	    lineptr = varpathscan(localdata, lineptr, &(*newspline)->ctrl[2].y,
			(genericptr *)newspline, *newpath, 2, offy + py, P_POSITION_Y,
			NULL);
	    lineptr = varpathscan(localdata, lineptr, &(*newspline)->ctrl[3].x,
			(genericptr *)newspline, *newpath, 3, offx + px, P_POSITION_X,
			&epptrx);
	    lineptr = varpathscan(localdata, lineptr, &(*newspline)->ctrl[3].y,
			(genericptr *)newspline, *newpath, 3, offy + py, P_POSITION_Y,
			&epptry);

	    (*newspline)->ctrl[0].x = startpoint.x;
	    (*newspline)->ctrl[0].y = startpoint.y;

	    calcspline(*newspline);
	    startpoint.x = (*newspline)->ctrl[3].x;
	    startpoint.y = (*newspline)->ctrl[3].y;
	 }

         /* read arcs */

         else if (!strcmp(keyword, "xcarc")) {
	    arcptr *newarc;
	 
	    NEW_ARC(newarc, localdata);
	    (*newarc)->color = curcolor;
	    (*newarc)->passed = NULL;
	    (*newarc)->cycle = NULL;

	    /* backward compatibility */
	    if (compare_version(version, "1.5") < 0) {
	       sscanf(buffer, "%hd %hd %hd %f %f %f %hd", &(*newarc)->position.x,
	          &(*newarc)->position.y, &(*newarc)->radius, &(*newarc)->angle1,
	          &(*newarc)->angle2, &(*newarc)->width, &(*newarc)->style);
	       (*newarc)->position.x -= offx;
	       (*newarc)->position.y -= offy;
	    }
	    else {
	       lineptr = varscan(localdata, buffer, &(*newarc)->style,
				(genericptr)*newarc, P_STYLE);
	       lineptr = varfscan(localdata, lineptr, &(*newarc)->width,
				(genericptr)*newarc, P_LINEWIDTH);
	       lineptr = varpscan(localdata, lineptr, &(*newarc)->position.x,
				(genericptr)*newarc, 0, offx, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newarc)->position.y,
				(genericptr)*newarc, 0, offy, P_POSITION_Y);
	       lineptr = varscan(localdata, lineptr, &(*newarc)->radius,
				(genericptr)*newarc, P_RADIUS);
	       lineptr = varfscan(localdata, lineptr, &(*newarc)->angle1,
				(genericptr)*newarc, P_ANGLE1);
	       lineptr = varfscan(localdata, lineptr, &(*newarc)->angle2,
				(genericptr)*newarc, P_ANGLE2);
	    }

	    (*newarc)->yaxis = (*newarc)->radius;
	    calcarc(*newarc);
	    std_eparam((genericptr)(*newarc), colorkey);
         }

	 /* read ellipses */

         else if (!strcmp(keyword, "ellipse")) {
	    arcptr *newarc;
	 
	    NEW_ARC(newarc, localdata);

	    (*newarc)->color = curcolor;
	    (*newarc)->passed = NULL;
	    (*newarc)->cycle = NULL;

	    lineptr = varscan(localdata, buffer, &(*newarc)->style,
			(genericptr)*newarc, P_STYLE);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->width,
			(genericptr)*newarc, P_LINEWIDTH);
	    lineptr = varpscan(localdata, lineptr, &(*newarc)->position.x,
			(genericptr)*newarc, 0, offx, P_POSITION_X);
	    lineptr = varpscan(localdata, lineptr, &(*newarc)->position.y,
			(genericptr)*newarc, 0, offy, P_POSITION_Y);
	    lineptr = varscan(localdata, lineptr, &(*newarc)->radius,
			(genericptr)*newarc, P_RADIUS);
	    lineptr = varscan(localdata, lineptr, &(*newarc)->yaxis,
			(genericptr)*newarc, P_MINOR_AXIS);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->angle1,
			(genericptr)*newarc, P_ANGLE1);
	    lineptr = varfscan(localdata, lineptr, &(*newarc)->angle2,
			(genericptr)*newarc, P_ANGLE2);

	    calcarc(*newarc);
	    std_eparam((genericptr)(*newarc), colorkey);
         }

         /* read polygons */
	 /* (and wires---backward compatibility for v1.5 and earlier) */

         else if (!strcmp(keyword, "polygon") || !strcmp(keyword, "wire")) {
	    polyptr *newpoly;
	    pointlist newpoints;
	    px = py = 0;

	    NEW_POLY(newpoly, localdata);
	    lineptr = buffer;

	    (*newpoly)->passed = NULL;
	    (*newpoly)->cycle = NULL;

	    if (!strcmp(keyword, "wire")) {
	       (*newpoly)->number = 2;
	       (*newpoly)->width = 1.0;
	       (*newpoly)->style = UNCLOSED;
	    }
	    else {
	       /* backward compatibility */
	       if (compare_version(version, "1.5") < 0) {
	          for (--keyptr; *keyptr == ' '; keyptr--);
	          for (; *keyptr != ' '; keyptr--);
	          sscanf(keyptr, "%hd", &(*newpoly)->style);
	          for (--keyptr; *keyptr == ' '; keyptr--);
	          for (; *keyptr != ' '; keyptr--);
	          sscanf(keyptr, "%f", &(*newpoly)->width);
	       }
	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);
	       /* check for "addtox" and "addtoy" parameter specification */
	       while (!strncmp(keyptr + 1, "addto", 5)) {
		  saveptr = keyptr + 1;

	          for (--keyptr; *keyptr == ' '; keyptr--);
	          for (; *keyptr != ' '; keyptr--);

		  /* Get parameter and its value */
		  if (*(saveptr + 5) == 'x')
	             varpscan(localdata, keyptr + 1, &px, (genericptr)*newpoly,
				-1, offx, P_POSITION_X);
		  else
	             varpscan(localdata, keyptr + 1, &py, (genericptr)*newpoly,
				-1, offy, P_POSITION_Y);

	          for (--keyptr; *keyptr == ' '; keyptr--);
	          for (; *keyptr != ' '; keyptr--);
	       }
	       sscanf(keyptr, "%hd", &(*newpoly)->number);

	       if (compare_version(version, "1.5") >= 0) {
	          lineptr = varscan(localdata, lineptr, &(*newpoly)->style,
				(genericptr)*newpoly, P_STYLE);
	          lineptr = varfscan(localdata, lineptr, &(*newpoly)->width,
				(genericptr)*newpoly, P_LINEWIDTH);
	       }
	    }

	    if ((*newpoly)->style & BBOX)
	       (*newpoly)->color = BBOXCOLOR;
	    else
	       (*newpoly)->color = curcolor;
            (*newpoly)->points = (pointlist) malloc((*newpoly)->number * 
		   sizeof(XPoint));

            for (newpoints = (*newpoly)->points; newpoints < (*newpoly)->points
		+ (*newpoly)->number; newpoints++) {
	       lineptr = varpscan(localdata, lineptr, &newpoints->x,
			(genericptr)*newpoly, newpoints - (*newpoly)->points,
			offx + px, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &newpoints->y,
			(genericptr)*newpoly, newpoints - (*newpoly)->points,
			offy + py, P_POSITION_Y);
	    }
	    std_eparam((genericptr)(*newpoly), colorkey);
         }

	 /* read spline curves */

         else if (!strcmp(keyword, "spline")) {
            splineptr *newspline;
	    px = py = 0;

	    NEW_SPLINE(newspline, localdata);
	    (*newspline)->color = curcolor;
	    (*newspline)->passed = NULL;
	    (*newspline)->cycle = NULL;

	    /* backward compatibility */
	    if (compare_version(version, "1.5") < 0) {
               sscanf(buffer, "%f %hd %hd %hd %hd %hd %hd %hd %hd %hd", 
	          &(*newspline)->width, &(*newspline)->ctrl[1].x,
	          &(*newspline)->ctrl[1].y, &(*newspline)->ctrl[2].x,
	          &(*newspline)->ctrl[2].y, &(*newspline)->ctrl[3].x,
	          &(*newspline)->ctrl[3].y, &(*newspline)->ctrl[0].x,
	          &(*newspline)->ctrl[0].y, &(*newspline)->style);
	       (*newspline)->ctrl[1].x -= offx; (*newspline)->ctrl[2].x -= offx;
	       (*newspline)->ctrl[0].x -= offx;
	       (*newspline)->ctrl[3].x -= offx;
	       (*newspline)->ctrl[1].y -= offy; (*newspline)->ctrl[2].y -= offy;
	       (*newspline)->ctrl[3].y -= offy;
	       (*newspline)->ctrl[0].y -= offy;
	    }
	    else {

	       for (--keyptr; *keyptr == ' '; keyptr--);
	       for (; *keyptr != ' '; keyptr--);
	       /* check for "addtox" and "addtoy" parameter specification */
	       while (!strncmp(keyptr + 1, "addto", 5)) {
		  saveptr = keyptr + 1;

	          for (--keyptr; *keyptr == ' '; keyptr--);
	          for (; *keyptr != ' '; keyptr--);

		  /* Get parameter and its value */
		  if (*(saveptr + 5) == 'x')
	             varpscan(localdata, keyptr + 1, &px, (genericptr)*newspline,
				-1, offx, P_POSITION_X);
		  else
	             varpscan(localdata, keyptr + 1, &py, (genericptr)*newspline,
				-1, offy, P_POSITION_Y);

	          for (--keyptr; *keyptr == ' '; keyptr--);
	          for (; *keyptr != ' '; keyptr--);
	       }

	       lineptr = varscan(localdata, buffer, &(*newspline)->style,
				(genericptr)*newspline, P_STYLE);
	       lineptr = varfscan(localdata, lineptr, &(*newspline)->width,
				(genericptr)*newspline, P_LINEWIDTH);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[1].x,
				(genericptr)*newspline, 1, offx + px, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[1].y,
				(genericptr)*newspline, 1, offy + py, P_POSITION_Y);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[2].x,
				(genericptr)*newspline, 2, offx + px, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[2].y,
				(genericptr)*newspline, 2, offy + py, P_POSITION_Y);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[3].x,
				(genericptr)*newspline, 3, offx + px, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[3].y,
				(genericptr)*newspline, 3, offy + py, P_POSITION_Y);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[0].x,
				(genericptr)*newspline, 0, offx + px, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newspline)->ctrl[0].y,
				(genericptr)*newspline, 0, offy + py, P_POSITION_Y);

	       /* check for "addtox" and "addtoy" parameter specification */
	    }

	    calcspline(*newspline);
	    std_eparam((genericptr)(*newspline), colorkey);
         }

         /* read graphics image instances */

	 else if (!strcmp(keyword, "graphic")) {
            graphicptr *newgp;
	    Imagedata *img;

	    lineptr = buffer + 1;
	    for (i = 0; i < xobjs.images; i++) {
	       img = xobjs.imagelist + i;
	       if (!strncmp(img->filename, lineptr, strlen(img->filename))) {
		  NEW_GRAPHIC(newgp, localdata);
		  (*newgp)->color = curcolor;
		  (*newgp)->passed = NULL;
#ifndef HAVE_CAIRO
		  (*newgp)->clipmask = (Pixmap)NULL;
		  (*newgp)->target = NULL;
		  (*newgp)->valid = False;
#endif /* HAVE_CAIRO */
		  (*newgp)->source = img->image;
		  img->refcount++;
		  lineptr += strlen(img->filename) + 1;
		  break;
	       }
	    }
	    if (i == xobjs.images) {
	       /* Error:  Line points to a non-existant image (no data) */
	       /* See if we can load the image name as a filename, and	*/
	       /* if that fails, then we must throw an error and ignore	*/
	       /* the image element.					*/

	       graphicptr locgp;
	       char *sptr = strchr(lineptr, ' ');

	       if (sptr != NULL)
		  *sptr = '\0';
	       else
		  sptr = lineptr;
	       locgp = new_graphic(NULL, lineptr, 0, 0);

	       if (locgp == NULL) {
	          Fprintf(stderr, "Error:  No graphic data for \"%s\".\n",
			lineptr);
		  newgp = NULL;
	       }
	       else {
		  lineptr = sptr;
		  newgp = &locgp;
	       }
	    }

	    if ((newgp != NULL) && (*newgp != NULL)) {
	       lineptr = varfscan(localdata, lineptr, &(*newgp)->scale,
				(genericptr)*newgp, P_SCALE);
	       lineptr = varfscan(localdata, lineptr, &(*newgp)->rotation,
				(genericptr)*newgp, P_ROTATION);
	       lineptr = varpscan(localdata, lineptr, &(*newgp)->position.x,
				(genericptr)*newgp, 0, offx, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newgp)->position.y,
				(genericptr)*newgp, 0, offy, P_POSITION_Y);
	       std_eparam((genericptr)(*newgp), colorkey);
	    }
	 }

         /* read labels */

         else if (!strcmp(keyword, "fontset")) { 	/* old style */
            char tmpstring[100];
            int i;
            sscanf(buffer, "%f %*c%99s", &tmpscale, tmpstring);
            for (i = 0; i < fontcount; i++)
               if (!strcmp(tmpstring, fonts[i].psname)) {
		  tmpfont = i;
		  break;
	       }
	    if (i == fontcount) i = 0;	/* Why bother with anything fancy? */
         }

         else if (!strcmp(keyword, "label") || !strcmp(keyword, "pinlabel")
		|| !strcmp(keyword, "pinglobal") || !strcmp(keyword, "infolabel")) {

	    labelptr *newlabel;
	    stringpart *firstscale, *firstfont;

	    NEW_LABEL(newlabel, localdata);
	    (*newlabel)->color = curcolor;
	    (*newlabel)->string = NULL;
	    (*newlabel)->passed = NULL;
	    (*newlabel)->cycle = NULL;

	    /* scan backwards to get the number of substrings */
	    lineptr = keyptr - 1;
	    for (i = 0; i < ((compare_version(version, "2.3") < 0) ? 5 : 6); i++) {
	       for (; *lineptr == ' '; lineptr--);
	       for (; *lineptr != ' '; lineptr--);
	    }
	    if ((strchr(lineptr, '.') != NULL) && (compare_version(version, "2.3") < 0)) {
	       Fprintf(stderr, "Error:  File version claims to be %s,"
			" but has version %s labels\n", version, PROG_VERSION);
	       Fprintf(stderr, "Attempting to resolve problem by updating version.\n");
	       strcpy(version, PROG_VERSION);
	       for (; *lineptr == ' '; lineptr--);
	       for (; *lineptr != ' '; lineptr--);
	    }
	    /* no. segments is ignored---may be a derived quantity, anyway */
	    if (compare_version(version, "2.3") < 0) {
	       sscanf(lineptr, "%*s %hd %hf %hd %hd", &(*newlabel)->anchor,
		   &(*newlabel)->rotation, &(*newlabel)->position.x,
		   &(*newlabel)->position.y);
	       (*newlabel)->position.x -= offx; (*newlabel)->position.y -= offy;
	       *lineptr = '\0';
	    }
	    else {
	       *lineptr++ = '\0';
	       lineptr = advancetoken(lineptr);  /* skip string token */
	       lineptr = varscan(localdata, lineptr, &(*newlabel)->anchor,
				(genericptr)*newlabel, P_ANCHOR);
	       lineptr = varfscan(localdata, lineptr, &(*newlabel)->rotation,
				(genericptr)*newlabel, P_ROTATION);
	       lineptr = varfscan(localdata, lineptr, &(*newlabel)->scale,
				(genericptr)*newlabel, P_SCALE);
	       lineptr = varpscan(localdata, lineptr, &(*newlabel)->position.x,
				(genericptr)*newlabel, 0, offx, P_POSITION_X);
	       lineptr = varpscan(localdata, lineptr, &(*newlabel)->position.y,
				(genericptr)*newlabel, 0, offy, P_POSITION_Y);
	    }
	    if (compare_version(version, "2.4") < 0)
	       (*newlabel)->rotation = -(*newlabel)->rotation;
	    while ((*newlabel)->rotation < 0.0) (*newlabel)->rotation += 360.0;

	    (*newlabel)->pin = False;
	    if (strcmp(keyword, "label")) {	/* all the schematic types */
	       /* enable schematic capture if it is not already on. */
	       if (!strcmp(keyword, "pinlabel"))
		  (*newlabel)->pin = LOCAL;
	       else if (!strcmp(keyword, "pinglobal"))
		  (*newlabel)->pin = GLOBAL;
	       else if (!strcmp(keyword, "infolabel")) {
		  /* Do not turn top-level pages into symbols! */
		  /* Info labels on schematics are treated differently. */
		  if (localdata != topobject)
		     localdata->schemtype = FUNDAMENTAL;
		  (*newlabel)->pin = INFO;
		  if (curcolor == DEFAULTCOLOR)
		     (*newlabel)->color = INFOLABELCOLOR;
	       }
	    }

	    lineptr = buffer;	/* back to beginning of string */
	    if (!strncmp(lineptr, "mark", 4)) lineptr += 4;

	    readlabel(localdata, lineptr, &(*newlabel)->string);
	    CheckMarginStop(*newlabel, areawin->topinstance, FALSE);

	    if (compare_version(version, "2.3") < 0) {
	       /* Switch 1st scale designator to overall font scale */

	       firstscale = (*newlabel)->string->nextpart;
	       if (firstscale->type != FONT_SCALE) {
	          if (tmpscale != 0.0)
	             (*newlabel)->scale = 0.0;
	          else
	             (*newlabel)->scale = 1.0;
	       }
	       else {
	          (*newlabel)->scale = firstscale->data.scale;
	          deletestring(firstscale, &((*newlabel)->string),
				areawin->topinstance);
	       }
	    }

	    firstfont = (*newlabel)->string;
	    if ((firstfont == NULL) || (firstfont->type != FONT_NAME)) {
	       if (tmpfont == -1) {
	          Fprintf(stderr, "Error:  Label with no font designator?\n");
		  tmpfont = 0;
	       }
	       firstfont = makesegment(&((*newlabel)->string), (*newlabel)->string);  
	       firstfont->type = FONT_NAME;
	       firstfont->data.font = tmpfont;
	    }
	    cleanuplabel(&(*newlabel)->string);

	    std_eparam((genericptr)(*newlabel), colorkey);
         }

	 /* read symbol-to-schematic connection */

	 else if (!strcmp(keyword, "is_schematic")) {
	    char tempstr[50];
	    for (lineptr = buffer; *lineptr == ' '; lineptr++);
	    parse_ps_string(++lineptr, tempstr, 49, FALSE, FALSE);
	    checksym(localdata, tempstr);
	 }

         /* read bounding box (font files only)	*/

         else if (!strcmp(keyword, "bbox")) {
	    for (lineptr = buffer; *lineptr == ' '; lineptr++);
            if (*lineptr != '%') {
	       Wprintf("Illegal bbox.");
	       free(buffer);
               *retstr = '\0';
	       return True;
	    }
	    sscanf(++lineptr, "%hd %hd %hd %hd",
		&localdata->bbox.lowerleft.x, &localdata->bbox.lowerleft.y,
		&localdata->bbox.width, &localdata->bbox.height);

	    // If this is *not* a FONTLIB, then the font character symbols
	    // are being edited as a normal library, so copy the bounding
	    // box into a FIXEDBBOX-type box.

	    if (mode != FONTLIB) {
		polyptr *newpoly;
		pointlist newpoints;

		NEW_POLY(newpoly, localdata);
		(*newpoly)->passed = NULL;
		(*newpoly)->cycle = NULL;
		(*newpoly)->number = 4;
		(*newpoly)->width = 1.0;
		(*newpoly)->style = FIXEDBBOX;
		(*newpoly)->color = FIXEDBBOXCOLOR;
       		(*newpoly)->points = (pointlist) malloc(4 * sizeof(XPoint));
		newpoints = (*newpoly)->points;
		newpoints->x = localdata->bbox.lowerleft.x;
		newpoints->y = localdata->bbox.lowerleft.y;
		newpoints++;
		newpoints->x = localdata->bbox.lowerleft.x + localdata->bbox.width;
		newpoints->y = localdata->bbox.lowerleft.y;
		newpoints++;
		newpoints->x = localdata->bbox.lowerleft.x + localdata->bbox.width;
		newpoints->y = localdata->bbox.lowerleft.y + localdata->bbox.height;
		newpoints++;
		newpoints->x = localdata->bbox.lowerleft.x;
		newpoints->y = localdata->bbox.lowerleft.y + localdata->bbox.height;
		std_eparam((genericptr)(*newpoly), colorkey);
	    }
         }

	 /* read "hidden" attribute */

	 else if (!strcmp(keyword, "hidden")) {
	    localdata->hidden = True;
	 }

	 /* read "libinst" special instance of a library part */

	 else if (!strcmp(keyword, "libinst")) {

	    /* Read backwards from keyword to find name of object instanced. */
	    for (lineptr = keyptr; *lineptr != '/' && lineptr > buffer;
			lineptr--);
	    parse_ps_string(++lineptr, keyword, 79, FALSE, FALSE);
	    new_library_instance(mode - LIBRARY, keyword, buffer, defaulttech);
	 }

	 /* read objects */

         else if (!strcmp(keyword, "{")) {  /* This is an object definition */
	    objlistptr redef;
	    objectptr *newobject;

	    for (lineptr = buffer; *lineptr == ' '; lineptr++);
	    if (*lineptr++ != '/') {
	       /* This may be part of a label. . . treat as a continuation line */
	       temp = continueline(&buffer);
	       continue;
	    }
	    parse_ps_string(lineptr, keyword, 79, FALSE, FALSE);

	    newobject = new_library_object(mode, keyword, &redef, defaulttech);

	    if (objectread(ps, *newobject, 0, 0, mode, retstr, curcolor,
			defaulttech) == True) {
               strncpy(retstr, buffer, 150);
               retstr[149] = '\0';
	       free(buffer);
	       return True;
            }
	    else {
	       if (library_object_unique(mode, *newobject, redef))
	          add_object_to_library(mode, *newobject);
	    }
         }
         else if (!strcmp(keyword, "def")) {
            strncpy(retstr, buffer, 150);
            retstr[149] = '\0';
	    free (buffer);
	    return False; /* end of object def or end of object library */
	 }

	 else if (!strcmp(keyword, "loadfontencoding")) {
	    /* Deprecated, but retained for backward compatibility. */
	    /* Load from script, .xcircuitrc, or command line instead. */
	    for (lineptr = buffer; *lineptr != '%'; lineptr++);
	    sscanf (lineptr + 1, "%149s", _STR);
	    if (*(lineptr + 1) != '%') loadfontfile(_STR);
	 }
	 else if (!strcmp(keyword, "loadlibrary")) {
	    /* Deprecated, but retained for backward compatibility */
	    /* Load from script, .xcircuitrc, or command line instead. */
	    int ilib, tlib;

	    for (lineptr = buffer; *lineptr != '%'; lineptr++);
	    sscanf (++lineptr, "%149s", _STR);
	    while (isspace(*lineptr)) lineptr++;
	    while (!isspace(*++lineptr));
	    while (isspace(*++lineptr));
	    if (sscanf (lineptr, "%d", &ilib) > 0) {
	       while ((ilib - 2 + LIBRARY) > xobjs.numlibs) {
		  tlib = createlibrary(False);
		  if (tlib != xobjs.numlibs - 2 + LIBRARY) {
		     ilib = tlib;
		     break;
		  }
	       }
	       mode = ilib - 1 + LIBRARY;
	    }
	    loadlibrary(mode);
	 }
	 else if (!strcmp(keyword, "beginparm")) { /* parameterized object */
	    short tmpnum, i;
	    for (--keyptr; *keyptr == ' '; keyptr--);
	    for (; isdigit(*keyptr) && (keyptr >= buffer); keyptr--);
	    sscanf(keyptr, "%hd", &tmpnum);
	    lineptr = buffer;
	    while (isspace(*lineptr)) lineptr++;

	    if (tmpnum < 256) {		/* read parameter defaults in order */
	       stringpart *newpart;
	       /* oparamptr newops; (jdk) */

	       for (i = 0; i < tmpnum; i++) {
		  oparamptr newops;

	          newops = (oparamptr)malloc(sizeof(oparam));
		  newops->next = localdata->params;
		  localdata->params = newops;
		  newops->key = (char *)malloc(6);
		  sprintf(newops->key, "v%d", i + 1);

		  if (*lineptr == '(' || *lineptr == '{') {  /* type is XC_STRING */
		     char *linetmp, csave;
		     
		     newops->parameter.string = NULL;

		     /* get simple substring or set of substrings and commands */
		     linetmp = find_delimiter(lineptr);
		     csave = *(++linetmp);
		     *linetmp = '\0';
		     if (*lineptr == '{') lineptr++;
		     readlabel(localdata, lineptr, &(newops->parameter.string));

		     /* Add the ending part to the parameter string */
		     newpart = makesegment(&(newops->parameter.string), NULL);  
		     newpart->type = PARAM_END;
		     newpart->data.string = (u_char *)NULL;

		     newops->type = (u_char)XC_STRING;
		     newops->which = P_SUBSTRING;
		     /* Fprintf(stdout, "Parameter %d to object %s defaults "
				"to string \"%s\"\n", i + 1, localdata->name,
				ops->parameter.string); */
		     *linetmp = csave;
		     lineptr = linetmp;
		     while (isspace(*lineptr)) lineptr++;
		  }
		  else {	/* type is assumed to be XC_FLOAT */
		     newops->type = (u_char)XC_FLOAT;
	             sscanf(lineptr, "%g", &newops->parameter.fvalue);
		     /* Fprintf(stdout, "Parameter %s to object %s defaults to "
				"value %g\n", newops->key, localdata->name,
				newops->parameter.fvalue); */
		     lineptr = advancetoken(lineptr);
		  }
	       }
	    }
	 }
	 else if (!strcmp(keyword, "nonetwork")) {
	    localdata->valid = True;
	    localdata->schemtype = NONETWORK;
	 }
	 else if (!strcmp(keyword, "trivial")) {
	    localdata->schemtype = TRIVIAL;
	 }
         else if (!strcmp(keyword, "begingate")) {
	    localdata->params = NULL;
	    /* read dictionary of parameter key:value pairs */
	    readparams(NULL, NULL, localdata, buffer);
	 }

         else if (!strcmp(keyword, "%%Trailer")) break;
         else if (!strcmp(keyword, "EndLib")) break;
	 else if (!strcmp(keyword, "restore"));    /* handled at top */
	 else if (!strcmp(keyword, "grestore"));   /* ignore */
         else if (!strcmp(keyword, "endgate"));    /* also ignore */
	 else if (!strcmp(keyword, "xyarray"));	   /* ignore for now */
         else {
	    char *tmpptr, *libobjname, *objnamestart;
	    Boolean matchtech, found = False;

	    /* First, make sure this is not a general comment line */
	    /* Return if we have a page boundary	   	   */
	    /* Read an image if this is an imagedata line.	   */

	    for (tmpptr = buffer; isspace(*tmpptr); tmpptr++);
	    if (*tmpptr == '%') {
	       if (strstr(buffer, "%%Page:") == tmpptr) {
                  strncpy(retstr, buffer, 150);
                  retstr[149] = '\0';
		  free (buffer);
		  return True;
	       }
	       else if (strstr(buffer, "%imagedata") == tmpptr) {
		  int width, height;
		  sscanf(buffer, "%*s %d %d", &width, &height);
		  readimagedata(ps, width, height);
	       }
	       continue;
	    }

	    parse_ps_string(keyword, keyword, 79, FALSE, FALSE);
	    matchtech =  (strstr(keyword, "::") == NULL) ? FALSE : TRUE;

	    /* If the file contains a reference to a bare-word device	*/
	    /* without a technology prefix, then it is probably an	*/
	    /* older-version file.  If this is the case, then the file	*/
	    /* should define an unprefixed object, which will be given	*/
	    /* a null prefix (just "::" by itself).  Look for that	*/
	    /* match.							*/

	    /* (Assume that this line calls an object instance) */
	    /* Double loop through user libraries 		*/

            for (k = 0; k < ((mode == FONTLIB) ? 1 : xobjs.numlibs); k++) {
	       for (j = 0; j < ((mode == FONTLIB) ? xobjs.fontlib.number :
			xobjs.userlibs[k].number); j++) {

		  libobj = (mode == FONTLIB) ? xobjs.fontlib.library + j :
			xobjs.userlibs[k].library + j;

		  /* Objects which have a technology ("<lib>::<obj>")	*/
		  /* must compare exactly.  Objects which don't	can 	*/
		  /* only match an object of the same name with a null	*/
		  /* technology prefix.					*/

		  libobjname = (*libobj)->name;
		  if (!matchtech) {
		     objnamestart = strstr(libobjname, "::");
		     if (objnamestart != NULL) libobjname = objnamestart + 2;
		  }
	          if (!objnamecmp(keyword, libobjname)) {

		     /* If the name is not exactly the same (appended underscores) */
		     /* check if the name is on the list of aliases. */

		     if (strcmp(keyword, libobjname)) {
			Boolean is_alias = False;
			aliasptr ckalias = aliastop;
			slistptr sref;

			for (; ckalias != NULL; ckalias = ckalias->next) {
			   if (ckalias->baseobj == (*libobj)) {
			      sref = ckalias->aliases;
			      for (; sref != NULL; sref = sref->next) {
			         if (!strcmp(keyword, sref->alias)) {
				    is_alias = True;
				    break;
				 }
			      }
			      if (is_alias) break;
			   }
			}
			if (!is_alias) continue;
		     }

		     if (!matchtech && ((*libobj)->name != objnamestart))
			if (compare_version(version, "3.7") > 0) 
			   continue;	// no prefix in file must match
					// null prefix in library object.

		     found = True;
		     NEW_OBJINST(newinst, localdata);
		     (*newinst)->thisobject = *libobj;
		     (*newinst)->color = curcolor;
		     (*newinst)->params = NULL;
		     (*newinst)->passed = NULL;
		     (*newinst)->bbox.lowerleft.x = (*libobj)->bbox.lowerleft.x;
		     (*newinst)->bbox.lowerleft.y = (*libobj)->bbox.lowerleft.y;
		     (*newinst)->bbox.width = (*libobj)->bbox.width;
		     (*newinst)->bbox.height = (*libobj)->bbox.height;
		     (*newinst)->schembbox = NULL;

	             lineptr = varfscan(localdata, buffer, &(*newinst)->scale,
				(genericptr)*newinst, P_SCALE);

		     /* Format prior to xcircuit 3.7 did not have scale-	*/
		     /* invariant linewidth.  If scale is 1, though, keep it	*/
		     /* invariant so as not to overuse the scale-variance	*/
		     /* flag in the output file.				*/

		     if ((compare_version(version, "3.7") < 0) && ((*newinst)->scale != 1.0))
		        (*newinst)->style = NORMAL;
		     else
		        (*newinst)->style = LINE_INVARIANT;

	       	     lineptr = varfscan(localdata, lineptr, &(*newinst)->rotation,
				(genericptr)*newinst, P_ROTATION);
	             lineptr = varpscan(localdata, lineptr, &(*newinst)->position.x,
				(genericptr)*newinst, 0, offx, P_POSITION_X);
	             lineptr = varpscan(localdata, lineptr, &(*newinst)->position.y,
				(genericptr)*newinst, 0, offy, P_POSITION_Y);

		     /* Negative rotations = flip in x in version 2.3.6 and    */
		     /* earlier.  Later versions don't allow negative rotation */

	    	     if (compare_version(version, "2.4") < 0) {
                        if ((*newinst)->rotation < 0.0) {
			   (*newinst)->scale = -((*newinst)->scale);
			   (*newinst)->rotation += 1.0;
		        }
		        (*newinst)->rotation = -(*newinst)->rotation;
		     }

                     while ((*newinst)->rotation > 360.0)
			(*newinst)->rotation -= 360.0;
                     while ((*newinst)->rotation < 0.0)
			(*newinst)->rotation += 360.0;

		     std_eparam((genericptr)(*newinst), colorkey);

		     /* Does this instance contain parameters? */
		     readparams(localdata, *newinst, *libobj, buffer);

		     calcbboxinst(*newinst);

		     break;

	          } /* if !strcmp */
	       } /* for j loop */
	       if (found) break;
	    } /* for k loop */
	    if (!found)		/* will assume that we have a continuation line */
	       temp = continueline(&buffer);
         }
      }
   }
   strncpy(retstr, buffer, 150);
   retstr[149] = '\0';
   free(buffer);
   return True;
}

/*------------------------*/
/* Save a PostScript file */
/*------------------------*/

#ifdef TCL_WRAPPER

void setfile(char *filename, int mode)
{
   if ((filename == NULL) || (xobjs.pagelist[areawin->page]->filename == NULL)) {
      Wprintf("Error: No filename for schematic.");
      if (areawin->area && beeper) XBell(dpy, 100);
      return;
   }

   /* see if name has been changed in the buffer */

   if (strcmp(xobjs.pagelist[areawin->page]->filename, filename)) {
      Wprintf("Changing name of edit file.");
      free(xobjs.pagelist[areawin->page]->filename);
      xobjs.pagelist[areawin->page]->filename = strdup(filename);
   }

   if (strstr(xobjs.pagelist[areawin->page]->filename, "Page ") != NULL) {
      Wprintf("Warning: Enter a new name.");
      if (areawin->area && beeper) XBell(dpy, 100);
   }
   else {
      savefile(mode); 
      if (areawin->area && beeper) XBell(dpy, 100);
   }
}

#else  /* !TCL_WRAPPER */

void setfile(xcWidget button, xcWidget fnamewidget, caddr_t clientdata) 
{
   /* see if name has been changed in the buffer */

   sprintf(_STR2, "%.249s", (char *)XwTextCopyBuffer(fnamewidget));
   if (xobjs.pagelist[areawin->page]->filename == NULL) {
      xobjs.pagelist[areawin->page]->filename = strdup(_STR2);
   }
   else if (strcmp(xobjs.pagelist[areawin->page]->filename, _STR2)) {
      Wprintf("Changing name of edit file.");
      free(xobjs.pagelist[areawin->page]->filename);
      xobjs.pagelist[areawin->page]->filename = strdup(_STR2);
   }
   if (strstr(xobjs.pagelist[areawin->page]->filename, "Page ") != NULL) {
      Wprintf("Warning: Enter a new name.");
      if (areawin->area && beeper) XBell(dpy, 100);
   }
   else {

      Arg wargs[1];
      xcWidget db, di;

      savefile(CURRENT_PAGE); 

      /* Change "close" button to read "done" */

      di = xcParent(button);
      db = XtNameToWidget(di, "Close");
      XtSetArg(wargs[0], XtNlabel, "  Done  ");
      XtSetValues(db, wargs, 1);
      if (areawin->area && beeper) XBell(dpy, 100);
   }
}

#endif  /* TCL_WRAPPER */

/*--------------------------------------------------------------*/
/* Update number of changes for an object and initiate a temp	*/
/* file save if total number of unsaved changes exceeds 20.	*/
/*--------------------------------------------------------------*/

void incr_changes(objectptr thisobj)
{
   /* It is assumed that empty pages are meant to be that way */
   /* and are not to be saved, so changes are marked as zero. */

   if (thisobj->parts == 0) {
      thisobj->changes = 0;
      return;
   }

   /* Remove any pending timeout */

   if (xobjs.timeout_id != (xcIntervalId)NULL) {
      xcRemoveTimeOut(xobjs.timeout_id);
      xobjs.timeout_id = (xcIntervalId)NULL;
   }

   thisobj->changes++;

   /* When in "suspend" mode, we assume that we are reading commands from
    * a script, and therefore we should not continuously increment changes
    * and keep saving the backup file.
    */

   if (xobjs.suspend < 0)
      xobjs.new_changes++;

   if (xobjs.new_changes > MAXCHANGES) {
#ifdef TCL_WRAPPER
      savetemp(NULL);
#else
      savetemp(NULL, NULL);
#endif
   }

   /* Generate a new timeout */

   if (areawin->area)
	xobjs.timeout_id = xcAddTimeOut(app, 60000 * xobjs.save_interval,
 		savetemp, NULL);
}

/*--------------------------------------------------------------*/
/* tempfile save						*/
/*--------------------------------------------------------------*/

#ifdef TCL_WRAPPER

void savetemp(ClientData clientdata)
{

#else 

void savetemp(XtPointer clientdata, xcIntervalId *id)
{

#endif
   /* If areawin->area is NULL then xcircuit is running in	*/
   /* batch mode and should not make backups.			*/
   if (areawin->area == NULL) return;

   /* Remove the timeout ID.  If this routine is called from	*/
   /* incr_changes() instead of from the timeout interrupt	*/
   /* service routine, then this value will already be NULL.	*/

   xobjs.timeout_id = (xcIntervalId)NULL;

   /* First see if there are any unsaved changes in the file.	*/
   /* If not, then just reset the counter and continue.  	*/

   if (xobjs.new_changes > 0) {
      if (xobjs.tempfile == NULL)
      {
         int fd;
         char *template = (char *)malloc(20 + strlen(xobjs.tempdir));
	 int pid = (int)getpid();

         sprintf(template, "%s/XC%d.XXXXXX", xobjs.tempdir, pid);

#ifdef _MSC_VER
         fd = mktemp(template);
#else
         fd = mkstemp(template);
#endif
         if (fd == -1) {
	    Fprintf(stderr, "Error generating file for savetemp\n");
	    free(template);
         } 
         close(fd);
         xobjs.tempfile = strdup(template);
         free(template);
      }
      /* Show the "wristwatch" cursor, as graphic data may cause this	*/
      /* step to be inordinately long.					*/

      XDefineCursor(dpy, areawin->window, WAITFOR);
      savefile(ALL_PAGES);
      XDefineCursor(dpy, areawin->window, DEFAULTCURSOR);
      xobjs.new_changes = 0;	/* reset the count */
   }
}

/*----------------------------------------------------------------------*/
/* Set all objects in the list "wroteobjs" as having no unsaved changes */
/*----------------------------------------------------------------------*/

void setassaved(objectptr *wroteobjs, short written)
{
   int i;

   for (i = 0; i < written; i++)
      (*(wroteobjs + i))->changes = 0;
}

/*---------------------------------------------------------------*/
/* Save indicated library.  If libno is 0, save current page if	 */
/* the current page is a library.  If not, save the user library */
/*---------------------------------------------------------------*/

void savelibpopup(xcWidget button, char *technology, caddr_t nulldata)
{
   TechPtr nsptr;

#ifndef TCL_WRAPPER
   buttonsave *savebutton;
#endif

   nsptr = LookupTechnology(technology);

   if (nsptr != NULL) {
      if ((nsptr->flags & TECH_READONLY) != 0) {
         Wprintf("Library technology \"%s\" is read-only.", technology);
         return;
      }
   }

#ifndef TCL_WRAPPER
   savebutton = getgeneric(button, savelibpopup, technology);
   popupprompt(button, "Enter technology:", "\0", savelibrary,
	savebutton, NULL);
#endif
}

/*---------------------------------------------------------*/

#ifndef TCL_WRAPPER

void savelibrary(xcWidget w, char *technology)
{
   char outname[250];
   sscanf(_STR2, "%249s", outname);
   savetechnology(technology, outname);
}

#endif

/*---------------------------------------------------------*/

void savetechnology(char *technology, char *outname)
{
   FILE *ps;
   char *outptr, *validname, outfile[150];
   objectptr *wroteobjs, libobjptr, *optr, depobj;
   genericptr *gptr;
   liblistptr spec;
   short written;
   short *glist;
   char *uname = NULL;
   char *hostname = NULL;
   struct passwd *mypwentry = NULL;
   int i, j, ilib;
   TechPtr nsptr;
   char *loctechnology;

   // Don't use the string "(user)" as a technology name;  this is just
   // a GUI placeholder for a null string ("").  This shouldn't happen,
   // though, as technology should be NULL if the user technology is
   // selected.

   if (technology && (!strcmp(technology, "(user)")))
      nsptr = LookupTechnology(NULL);
   else
      nsptr = LookupTechnology(technology);

   if ((outptr = strrchr(outname, '/')) == NULL)
      outptr = outname;
   else
      outptr++;
   strcpy(outfile, outname);
   if (strchr(outptr, '.') == NULL) strcat(outfile, ".lps");

   xc_tilde_expand(outfile, 149);
   while(xc_variable_expand(outfile, 149));

   if (nsptr != NULL && nsptr->filename != NULL) {
      // To be pedantic, we should probably check that the inodes of the
      // files are different, to be sure we are avoiding an unintentional
      // over-write.

      if (!strcmp(outfile, nsptr->filename)) {

	 if ((nsptr->flags & TECH_READONLY) != 0) {
	    Wprintf("Technology file \"%s\" is read-only.", technology);
	    return;
	 }

	 if ((nsptr->flags & TECH_IMPORTED) != 0) {
	    Wprintf("Attempt to write a truncated technology file!");
	    return;
	 }
      }
   }

   ps = fopen(outfile, "wb");
   if (ps == NULL) {
      Wprintf("Can't open PS file.");
      if (nsptr && nsptr->filename && (!strcmp(nsptr->filename, outfile))) {
	 Wprintf("Marking technology \"%s\" as read-only.", technology);
	 nsptr->flags |= TECH_READONLY;
      }
      return;
   }

   /* Did the technology name change?  If so, register the new name.	*/
   /* Clear any "IMPORTED" or "READONLY" flags.				*/

   if (nsptr && nsptr->filename && strcmp(outfile, nsptr->filename)) {
      Wprintf("Technology filename changed from \"%s\" to \"%s\".",
		nsptr->filename, outfile);
      free(nsptr->filename);
      nsptr->filename = strdup(outfile);
      nsptr->flags &= ~(TECH_READONLY | TECH_IMPORTED);
   }
   else if (nsptr && !nsptr->filename) {
      nsptr->filename = strdup(outfile);
      nsptr->flags &= ~(TECH_READONLY | TECH_IMPORTED);
   }

   fprintf(ps, "%%! PostScript set of library objects for XCircuit\n");
   fprintf(ps, "%%  Version: %s\n", version);
   fprintf(ps, "%%  Library name is: %s\n",
		(technology == NULL) ? "(user)" : technology);
#ifdef _MSC_VER
   uname = getenv((const char *)"USERNAME");
#else
   uname = getenv((const char *)"USER");
   if (uname != NULL) mypwentry = getpwnam(uname);
#endif

   /* Check for both $HOST and $HOSTNAME environment variables.  Thanks */
   /* to frankie liu <frankliu@Stanford.EDU> for this fix.		*/
   
   if ((hostname = getenv((const char *)"HOSTNAME")) == NULL)
      if ((hostname = getenv((const char *)"HOST")) == NULL) {
	 if (gethostname(_STR, 149) != 0)
	    hostname = uname;
	 else
	    hostname = _STR;
      }

#ifndef _MSC_VER
   if (mypwentry != NULL)
      fprintf(ps, "%%  Author: %s <%s@%s>\n", mypwentry->pw_gecos, uname,
		hostname);
#endif

   fprintf(ps, "%%\n\n");

   /* Print lists of object dependencies, where they exist.		*/
   /* Note that objects can depend on objects in other technologies;	*/
   /* this is allowed.							*/

   wroteobjs = (objectptr *) malloc(sizeof(objectptr));
   for (ilib = 0; ilib < xobjs.numlibs; ilib++) {
      for (j = 0; j < xobjs.userlibs[ilib].number; j++) {

	 libobjptr = *(xobjs.userlibs[ilib].library + j);
         if (CompareTechnology(libobjptr, technology)) {
	    written = 0;

	    /* Search for all object definitions instantiated in this object, */
	    /* and add them to the dependency list (non-recursive).	   */

	    for (gptr = libobjptr->plist; gptr < libobjptr->plist
			+ libobjptr->parts; gptr++) {
	       if (IS_OBJINST(*gptr)) {
	          depobj = TOOBJINST(gptr)->thisobject;

	          /* Search among the list of objects already collected.	*/
	          /* If this object has been previously, then ignore it.	*/
	          /* Otherwise, update the list of object dependencies	*/

	          for (optr = wroteobjs; optr < wroteobjs + written; optr++)
		     if (*optr == depobj)
		        break;

	          if (optr == wroteobjs + written) {
		     wroteobjs = (objectptr *)realloc(wroteobjs, (written + 1) * 
				sizeof(objectptr));
		     *(wroteobjs + written) = depobj;
		     written++;
	          }
	       }
	    }
	    if (written > 0) {
	       fprintf(ps, "%% Depend %s", libobjptr->name);
	       for (i = 0; i < written; i++) {
	          depobj = *(wroteobjs + i);
	          fprintf(ps, " %s", depobj->name);
	       }
	       fprintf(ps, "\n");
	    }
         }
      }
   }

   fprintf(ps, "\n%% XCircuitLib library objects\n");

   /* Start by looking for any graphic images in the library objects	*/
   /* and saving the graphic image data at the top of the file.		*/

   glist = (short *)malloc(xobjs.images * sizeof(short));
   for (i = 0; i < xobjs.images; i++) glist[i] = 0;

   for (ilib = 0; ilib < xobjs.numlibs; ilib++) {
      for (spec = xobjs.userlibs[ilib].instlist; spec != NULL; spec = spec->next) {
	 libobjptr = spec->thisinst->thisobject;
         if (CompareTechnology(libobjptr, technology))
	    count_graphics(spec->thisinst->thisobject, glist);
      }
   }
   output_graphic_data(ps, glist);
   free(glist);

   /* list of library objects already written */

   wroteobjs = (objectptr *)realloc(wroteobjs, sizeof(objectptr));
   written = 0;

   /* write all of the object definitions used, bottom up, with virtual	*/
   /* instances in the correct placement.  The need to find virtual	*/
   /* instances is why we do a look through the library pages and not	*/
   /* the libraries themselves when looking for objects matching the	*/
   /* given technology.							*/

   for (ilib = 0; ilib < xobjs.numlibs; ilib++) {
      for (spec = xobjs.userlibs[ilib].instlist; spec != NULL; spec = spec->next) {
	 libobjptr = spec->thisinst->thisobject;
         if (CompareTechnology(libobjptr, technology)) {
            if (!spec->virtual) {
               printobjects(ps, spec->thisinst->thisobject, &wroteobjs,
			&written, DEFAULTCOLOR);
      	    }
            else {
	       if ((spec->thisinst->scale != 1.0) ||
			(spec->thisinst->rotation != 0.0)) {
	    	  fprintf(ps, "%3.3f %3.3f ", spec->thisinst->scale,
				spec->thisinst->rotation);
	       }
               printparams(ps, spec->thisinst, 0);
	       validname = create_valid_psname(spec->thisinst->thisobject->name, FALSE);
	       /* Names without technologies get '::' string (blank technology) */
	       if (technology == NULL)
                  fprintf(ps, "/::%s libinst\n", validname);
	       else
                  fprintf(ps, "/%s libinst\n", validname);
	       if ((spec->next != NULL) && (!(spec->next->virtual)))
	          fprintf(ps, "\n");
            }
         }
      }
   }

   setassaved(wroteobjs, written);
   if (nsptr) nsptr->flags &= (~TECH_CHANGED);
   xobjs.new_changes = countchanges(NULL);

   /* and the postlog */

   fprintf(ps, "\n%% EndLib\n");
   fclose(ps);
   if (technology != NULL)
      Wprintf("Library technology \"%s\" saved as file %s.",technology, outname);
   else
      Wprintf("Library technology saved as file %s.", outname);

   free(wroteobjs);
}

/*----------------------------------------------------------------------*/
/* Recursive routine to search the object hierarchy for fonts used	*/
/*----------------------------------------------------------------------*/

void findfonts(objectptr writepage, short *fontsused) {
   genericptr *dfp;
   stringpart *chp;
   int findex;

   for (dfp = writepage->plist; dfp < writepage->plist + writepage->parts; dfp++) {
      if (IS_LABEL(*dfp)) {
         for (chp = TOLABEL(dfp)->string; chp != NULL; chp = chp->nextpart) {
	    if (chp->type == FONT_NAME) {
	       findex = chp->data.font;
	       if (fontsused[findex] == 0) {
	          fontsused[findex] = 0x8000 | fonts[findex].flags;
	       }
	    }
	 }
      }
      else if (IS_OBJINST(*dfp)) {
	 findfonts(TOOBJINST(dfp)->thisobject, fontsused);
      }
   }
}

/*------------------------------------------------------*/
/* Write graphics image data to file "ps".  "glist" is	*/
/* a pointer to a vector of short integers, each one	*/
/* being an index into xobjs.images for an image that	*/
/* is to be output.					*/
/*------------------------------------------------------*/
 
void output_graphic_data(FILE *ps, short *glist)
{
   char *fptr, ascbuf[6];
   int i, j;
   for (i = 0; i < xobjs.images; i++) {
      Imagedata *img = xobjs.imagelist + i;
      int ilen, flen, k, m = 0, n, q = 0;
      u_char *filtbuf, *flatebuf;
      Boolean lastpix = False;
      union {
	u_long i;
	u_char b[4];
      } pixel;
      int width = xcImageGetWidth(img->image);
      int height = xcImageGetHeight(img->image);

      if (glist[i] == 0) continue;

      fprintf(ps, "%%imagedata %d %d\n", width, height);
      fprintf(ps, "currentfile /ASCII85Decode filter ");

#ifdef HAVE_LIBZ
      fprintf(ps, "/FlateDecode filter\n");
#endif

      fprintf(ps, "/ReusableStreamDecode filter\n");

      /* creating a stream buffer is wasteful if we're just using ASCII85	*/
      /* decoding but is a must for compression filters. 			*/

      ilen = 3 * width * height;
      filtbuf = (u_char *)malloc(ilen + 4);
      q = 0;

      for (j = 0; j < height; j++)
	 for (k = 0; k < width; k++) {
	    unsigned char r, g, b;
	    xcImageGetPixel(img->image, k, j, &r, &g, &b);
	    filtbuf[q++] = r;
	    filtbuf[q++] = g;
	    filtbuf[q++] = b;
	 }

      /* Extra encoding goes here */
#ifdef HAVE_LIBZ
      flen = ilen * 2;
      flatebuf = (char *)malloc(flen);
      ilen = large_deflate(flatebuf, flen, filtbuf, ilen);
      free(filtbuf);
#else
      flatebuf = filtbuf;
#endif
	    
      ascbuf[5] = '\0';
      pixel.i = 0;
      for (j = 0; j < ilen; j += 4) {
	 if ((j + 4) > ilen) lastpix = TRUE;
	 if (!lastpix && (flatebuf[j] + flatebuf[j + 1] + flatebuf[j + 2]
			+ flatebuf[j + 3] == 0)) {
	    fprintf(ps, "z");
	    m++;
	 }
	 else {
	    for (n = 0; n < 4; n++)
	       pixel.b[3 - n] = flatebuf[j + n];

	    ascbuf[0] = '!' + (pixel.i / 52200625);
	    pixel.i %= 52200625;
	    ascbuf[1] = '!' + (pixel.i / 614125);
	    pixel.i %= 614125;
	    ascbuf[2] = '!' + (pixel.i / 7225);
	    pixel.i %= 7225;
	    ascbuf[3] = '!' + (pixel.i / 85);
	    pixel.i %= 85;
	    ascbuf[4] = '!' + pixel.i;
	    if (lastpix)
	       for (n = 0; n < ilen + 1 - j; n++)
	          fprintf(ps, "%c", ascbuf[n]);
	    else
	       fprintf(ps, "%5s", ascbuf);
	    m += 5;
	 }
	 if (m > 75) {
	    fprintf(ps, "\n");
	    m = 0;
	 }
      }
      fprintf(ps, "~>\n");
      free(flatebuf);

      /* Remove any filesystem path information from the image name.	*/
      /* Otherwise, the slashes will cause PostScript to err.		*/

      fptr = strrchr(img->filename, '/');
      if (fptr == NULL)
	 fptr = img->filename;
      else
	 fptr++;
      fprintf(ps, "/%sdata exch def\n", fptr);
      fprintf(ps, "/%s <<\n", fptr);
      fprintf(ps, "  /ImageType 1 /Width %d /Height %d /BitsPerComponent 8\n",
	    width, height);
      fprintf(ps, "  /MultipleDataSources false\n");
      fprintf(ps, "  /Decode [0 1 0 1 0 1]\n");
      fprintf(ps, "  /ImageMatrix [1 0 0 -1 %d %d]\n",
	    width >> 1, height >> 1);
      fprintf(ps, "  /DataSource %sdata >> def\n\n", fptr);
   }
}

/*----------------------------------------------------------------------*/
/* Main file saving routine						*/
/*----------------------------------------------------------------------*/
/*	mode 		description					*/
/*----------------------------------------------------------------------*/
/*	ALL_PAGES	saves a crash recovery backup file		*/
/*	CURRENT_PAGE	saves all pages associated with the same	*/
/*			filename as the current page, and all		*/
/*			dependent schematics (which have their		*/
/*			filenames changed to match).			*/
/*	NO_SUBCIRCUITS	saves all pages associated with the same	*/
/*			filename as the current page, only.		*/
/*----------------------------------------------------------------------*/

void savefile(short mode) 
{
   FILE *ps, *pro, *enc;
   char outname[150], temp[150], prologue[150], *fname, *fptr, ascbuf[6];
   /* u_char decodebuf[6]; (jdk */
   short written, fontsused[256], i, page, curpage, multipage;
   short savepage, stcount, *pagelist, *glist;
   objectptr *wroteobjs;
   objinstptr writepage;
   int findex, j;
   time_t tdate;
   char *tmp_s;

   if (mode != ALL_PAGES) {
      /* doubly-protected file write: protect against errors during file write */
      fname = xobjs.pagelist[areawin->page]->filename;
      sprintf(outname, "%s~", fname);
      rename(fname, outname);
   }
   else {
      /* doubly-protected backup: protect against errors during file write */
      sprintf(outname, "%sB", xobjs.tempfile);
      rename(xobjs.tempfile, outname);
      fname = xobjs.tempfile;
   }

   if ((fptr = strrchr(fname, '/')) != NULL)
      fptr++;
   else
      fptr = fname;

   if ((mode != ALL_PAGES) && (strchr(fptr, '.') == NULL))
      sprintf(outname, "%s.ps", fname);
   else sprintf(outname, "%s", fname);

   xc_tilde_expand(outname, 149);
   while(xc_variable_expand(outname, 149));

   ps = fopen(outname, "wb");
   if (ps == NULL) {
      Wprintf("Can't open file %s for writing.", outname);
      return;
   }

   if ((mode != NO_SUBCIRCUITS) && (mode != ALL_PAGES))
      collectsubschems(areawin->page);

   /* Check for multiple-page output: get the number of pages;	*/
   /* ignore empty pages.					*/

   multipage = 0;

   if (mode == NO_SUBCIRCUITS)
      pagelist = pagetotals(areawin->page, INDEPENDENT);
   else if (mode == ALL_PAGES)
      pagelist = pagetotals(areawin->page, ALL_PAGES);
   else
      pagelist = pagetotals(areawin->page, TOTAL_PAGES);

   for (page = 0; page < xobjs.pages; page++)
      if (pagelist[page] > 0)
	  multipage++;

   if (multipage == 0) {
      Wprintf("Panic:  could not find this page in page list!");
      free (pagelist);
      fclose(ps);
      return;
   }

   /* Print the PostScript DSC Document Header */

   fprintf(ps, "%%!PS-Adobe-3.0");
   if (multipage == 1 && !(xobjs.pagelist[areawin->page]->pmode & 1))
      fprintf(ps, " EPSF-3.0\n");
   else
      fprintf(ps, "\n");
   fprintf(ps, "%%%%Title: %s\n", fptr);
   fprintf(ps, "%%%%Creator: XCircuit v%2.1f rev%d\n", PROG_VERSION, PROG_REVISION);
   tdate = time(NULL);
   fprintf(ps, "%%%%CreationDate: %s", asctime(localtime(&tdate)));
   fprintf(ps, "%%%%Pages: %d\n", multipage);

   /* This is just a default value; each bounding box is declared per	*/
   /* page by the DSC "PageBoundingBox" keyword.			*/
   /* However, encapsulated files adjust the bounding box to center on	*/
   /* the object, instead of centering the object on the page.		*/

   if (multipage == 1 && !(xobjs.pagelist[areawin->page]->pmode & 1)) {
      objectptr thisobj = xobjs.pagelist[areawin->page]->pageinst->thisobject;
      float psscale = getpsscale(xobjs.pagelist[areawin->page]->outscale,
			areawin->page);

      /* The top-level bounding box determines the size of an encapsulated */
      /* drawing, regardless of the PageBoundingBox numbers.  Therefore,   */
      /* we size this to bound just the picture by closing up the 1" (72   */
      /* PostScript units) margins, except for a tiny sliver of a margin   */
      /* (4 PostScript units) which covers a bit of sloppiness in the font */
      /* measurements.							   */
	
      fprintf(ps, "%%%%BoundingBox: 68 68 %d %d\n",
	 (int)((float)thisobj->bbox.width * psscale)
			+ xobjs.pagelist[areawin->page]->margins.x + 4,
	 (int)((float)thisobj->bbox.height * psscale)
			+ xobjs.pagelist[areawin->page]->margins.y + 4);
   }
   else if (xobjs.pagelist[0]->coordstyle == CM)
      fprintf(ps, "%%%%BoundingBox: 0 0 595 842\n");  /* A4 default (fixed by jdk) */
   else
      fprintf(ps, "%%%%BoundingBox: 0 0 612 792\n");  /* letter default */

   for (i = 0; i < fontcount; i++) fontsused[i] = 0;
   fprintf(ps, "%%%%DocumentNeededResources: font ");
   stcount = 32;

   /* find all of the fonts used in this document */
   /* log all fonts which are native PostScript   */

   for (curpage = 0; curpage < xobjs.pages; curpage++)
      if (pagelist[curpage] > 0) {
         writepage = xobjs.pagelist[curpage]->pageinst;
         findfonts(writepage->thisobject, fontsused);
      }
      
   for (i = 0; i < fontcount; i++) {
      if (fontsused[i] & 0x8000)
	 if ((fonts[i].flags & 0x8018) == 0x0) {
	    stcount += strlen(fonts[i].psname) + 1;
	    if (stcount > OUTPUTWIDTH) {
	       stcount = strlen(fonts[i].psname) + 11;
	       fprintf(ps, "\n%%%%+ font ");
	    }
	    fprintf(ps, "%s ", fonts[i].psname);
	 }
   }

   fprintf(ps, "\n%%%%EndComments\n");

   tmp_s = getenv((const char *)"XCIRCUIT_LIB_DIR");
   if (tmp_s != NULL) {
      sprintf(prologue, "%s/%s", tmp_s, PROLOGUE_FILE);
      pro = fopen(prologue, "r");
   }
   else
      pro = NULL;

   if (pro == NULL) {
      sprintf(prologue, "%s/%s", PROLOGUE_DIR, PROLOGUE_FILE);
      pro = fopen(prologue, "r");
      if (pro == NULL) {
         sprintf(prologue, "%s", PROLOGUE_FILE);
         pro = fopen(prologue, "r");
         if (pro == NULL) {
            Wprintf("Can't open prolog.");
	    free(pagelist);
	    fclose(ps);
            return;
	 }
      }
   }

   /* write the prolog to the output */

   for(;;) {
      if (fgets(temp, 149, pro) == NULL) break;
      if (!strncmp(temp, "%%EndProlog", 11)) break;
      fputs(temp, ps);
   }
   fclose(pro);

   /* Special font encodings not known to PostScript	*/
   /* (anything other than Standard and ISOLatin-1)	*/

   for (findex = 0; findex < fontcount; findex++) {
      if ((fontsused[findex] & 0xf80) == 0x400) {
	 /* Cyrillic (ISO8859-5) */
	 sprintf(prologue, "%s/%s", PROLOGUE_DIR, CYRILLIC_ENC_FILE);
	 pro = fopen(prologue, "r");
         if (pro == NULL) {
            sprintf(prologue, "%s", CYRILLIC_ENC_FILE);
            pro = fopen(prologue, "r");
            if (pro == NULL) {
               Wprintf("Warning:  Missing font encoding vectors.");
               Wprintf("Output may not print properly.");
	    }
         }
	 if (pro != NULL) {
            for(;;) {
               if (fgets(temp, 149, pro) == NULL) break;
               fputs(temp, ps);
            }
            fclose(pro);
	 }
      }
      else if ((fontsused[findex] & 0xf80) == 0x180) {
	 /* Eastern European (ISOLatin2) */
	 sprintf(prologue, "%s/%s", PROLOGUE_DIR, ISOLATIN2_ENC_FILE);
	 pro = fopen(prologue, "r");
         if (pro == NULL) {
            sprintf(prologue, "%s", ISOLATIN2_ENC_FILE);
            pro = fopen(prologue, "r");
            if (pro == NULL) {
               Wprintf("Warning:  Missing font encoding vectors.");
               Wprintf("Output may not print properly.");
	    }
         }
	 if (pro != NULL) {
            for(;;) {
               if (fgets(temp, 149, pro) == NULL) break;
               fputs(temp, ps);
            }
            fclose(pro);
	 }
      }
      else if ((fontsused[findex] & 0xf80) == 0x300) {
	 /* Turkish (ISOLatin5) */
	 sprintf(prologue, "%s/%s", PROLOGUE_DIR, ISOLATIN5_ENC_FILE);
	 pro = fopen(prologue, "r");
         if (pro == NULL) {
            sprintf(prologue, "%s", ISOLATIN5_ENC_FILE);
            pro = fopen(prologue, "r");
            if (pro == NULL) {
               Wprintf("Warning:  Missing font encoding vectors.");
               Wprintf("Output may not print properly.");
	    }
         }
	 if (pro != NULL) {
            for(;;) {
               if (fgets(temp, 149, pro) == NULL) break;
               fputs(temp, ps);
            }
            fclose(pro);
	 }
      }
   }
   
   /* Finish off prolog */
   fputs("%%EndProlog\n", ps);

   /* Special font handling */

   for (findex = 0; findex < fontcount; findex++) {

      /* Derived font slant */

      if ((fontsused[findex] & 0x032) == 0x032)
         fprintf(ps, "/%s /%s .167 fontslant\n\n",
		fonts[findex].psname, fonts[findex].family);

      /* Derived ISO-Latin1 encoding */

      if ((fontsused[findex] & 0xf80) == 0x100) {
	 char *fontorig = NULL;
	 short i;
	 /* find the original standard-encoded font (can be itself) */
	 for (i = 0; i < fontcount; i++) {
	    if (i == findex) continue;
	    if (!strcmp(fonts[i].family, fonts[findex].family) &&
		 ((fonts[i].flags & 0x03) == (fonts[findex].flags & 0x03))) {
	       fontorig = fonts[i].psname;
	       break;
	    }
	 }
	 if (fontorig == NULL) fontorig = fonts[findex].psname;
	 fprintf(ps, "/%s findfont dup length dict begin\n", fontorig);
	 fprintf(ps, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
	 fprintf(ps, "/Encoding ISOLatin1Encoding def currentdict end\n");
	 fprintf(ps, "/%s exch definefont pop\n\n", fonts[findex].psname);
      }

      /* Derived Cyrillic (ISO8859-5) encoding */

      if ((fontsused[findex] & 0xf80) == 0x400) {
	 char *fontorig = NULL;
	 short i;
	 /* find the original standard-encoded font (can be itself) */
	 for (i = 0; i < fontcount; i++) {
	    if (i == findex) continue;
	    if (!strcmp(fonts[i].family, fonts[findex].family) &&
		 ((fonts[i].flags & 0x03) == (fonts[findex].flags & 0x03))) {
	       fontorig = fonts[i].psname;
	       break;
	    }
	 }
	 if (fontorig == NULL) fontorig = fonts[findex].psname;
	 fprintf(ps, "/%s findfont dup length dict begin\n", fontorig);
	 fprintf(ps, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
	 fprintf(ps, "/Encoding ISO8859_5Encoding def currentdict end\n");
	 fprintf(ps, "/%s exch definefont pop\n\n", fonts[findex].psname);
      }

      /* ISO-Latin2 encoding */

      if ((fontsused[findex] & 0xf80) == 0x180) {
	 char *fontorig = NULL;
	 short i;
	 /* find the original standard-encoded font (can be itself) */
	 for (i = 0; i < fontcount; i++) {
	    if (i == findex) continue;
	    if (!strcmp(fonts[i].family, fonts[findex].family) &&
		 ((fonts[i].flags & 0x03) == (fonts[findex].flags & 0x03))) {
	       fontorig = fonts[i].psname;
	       break;
	    }
	 }
	 if (fontorig == NULL) fontorig = fonts[findex].psname;
	 fprintf(ps, "/%s findfont dup length dict begin\n", fontorig);
	 fprintf(ps, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
	 fprintf(ps, "/Encoding ISOLatin2Encoding def currentdict end\n");
	 fprintf(ps, "/%s exch definefont pop\n\n", fonts[findex].psname);
      }

      /* ISO-Latin5 encoding */

      if ((fontsused[findex] & 0xf80) == 0x300) {
	 char *fontorig = NULL;
	 short i;
	 /* find the original standard-encoded font (can be itself) */
	 for (i = 0; i < fontcount; i++) {
	    if (i == findex) continue;
	    if (!strcmp(fonts[i].family, fonts[findex].family) &&
		 ((fonts[i].flags & 0x03) == (fonts[findex].flags & 0x03))) {
	       fontorig = fonts[i].psname;
	       break;
	    }
	 }
	 if (fontorig == NULL) fontorig = fonts[findex].psname;
	 fprintf(ps, "/%s findfont dup length dict begin\n", fontorig);
	 fprintf(ps, "{1 index /FID ne {def} {pop pop} ifelse} forall\n");
	 fprintf(ps, "/Encoding ISOLatin5Encoding def currentdict end\n");
	 fprintf(ps, "/%s exch definefont pop\n\n", fonts[findex].psname);
      }

      /* To do:  Special encoding */

      if ((fontsused[findex] & 0xf80) == 0x80) {
      }

      /* To do:  Vectored (drawn) font */

      if (fontsused[findex] & 0x8) {
      }
   }

   /* List of objects already written */
   wroteobjs = (objectptr *) malloc (sizeof(objectptr));
   written = 0;

   fprintf(ps, "%% XCircuit output starts here.\n\n");
   fprintf(ps, "%%%%BeginSetup\n\n");

   /* Write out all of the images used */

   glist = collect_graphics(pagelist);
   output_graphic_data(ps, glist);
   free(glist);

   for (curpage = 0; curpage < xobjs.pages; curpage++) {
      if (pagelist[curpage] == 0) continue;

      /* Write all of the object definitions used, bottom up */
      printrefobjects(ps, xobjs.pagelist[curpage]->pageinst->thisobject,
		&wroteobjs, &written);
   }

   fprintf(ps, "\n%%%%EndSetup\n\n");

   page = 0;
   for (curpage = 0; curpage < xobjs.pages; curpage++) {
      if (pagelist[curpage] == 0) continue;

      /* Print the page header, all elements in the page, and page trailer */
      savepage = areawin->page;
      /* Set the current page for the duration of printing so that any	*/
      /* page parameters will be printed correctly.			*/
      areawin->page = curpage;
      printpageobject(ps, xobjs.pagelist[curpage]->pageinst->thisobject,
		++page, curpage);
      areawin->page = savepage;

      /* For crash recovery, log the filename for each page */
      if (mode == ALL_PAGES) {
	 fprintf(ps, "%% %s is_filename\n",
		(xobjs.pagelist[curpage]->filename == NULL) ?
		xobjs.pagelist[curpage]->pageinst->thisobject->name :
		xobjs.pagelist[curpage]->filename);
      }

      fprintf(ps, "\n");
      fflush(ps);
   }

   /* For crash recovery, save all objects that have been edited but are */
   /* not in the list of objects already saved.				 */

   if (mode == ALL_PAGES)
   {
      int i, j, k;
      objectptr thisobj;

      for (i = 0; i < xobjs.numlibs; i++) {
	 for (j = 0; j < xobjs.userlibs[i].number; j++) {
	    thisobj = *(xobjs.userlibs[i].library + j);
	    if (thisobj->changes > 0 ) {
	       for (k = 0; k < written; k++)
	          if (thisobj == *(wroteobjs + k)) break;
	       if (k == written)
      	          printobjects(ps, thisobj, &wroteobjs, &written, DEFAULTCOLOR);
	    }
	 }
      }
   }
   else {	/* No unsaved changes in these objects */
      setassaved(wroteobjs, written);
      for (i = 0; i < xobjs.pages; i++)
	 if (pagelist[i] > 0)
	    xobjs.pagelist[i]->pageinst->thisobject->changes = 0;
      xobjs.new_changes = countchanges(NULL);
   }

   /* Free allocated memory */
   free((char *)pagelist);
   free((char *)wroteobjs);

   /* Done! */

   fprintf(ps, "%%%%Trailer\n");
   fprintf(ps, "XCIRCsave restore\n");
   fprintf(ps, "%%%%EOF\n");
   fclose(ps);

   Wprintf("File %s saved (%d page%s).", fname, multipage,
		(multipage > 1 ? "s" : ""));

   if (mode == ALL_PAGES) {
      /* Remove the temporary redundant backup */
      sprintf(outname, "%sB", xobjs.tempfile);
      unlink(outname);
   }
   else if (!xobjs.retain_backup) {
      /* Remove the backup file */
      sprintf(outname, "%s~", fname);
      unlink(outname);
   }

   /* Write LATEX strings, if any are present */
   TopDoLatex();
}

/*----------------------------------------------------------------------*/
/* Given a color index, print the R, G, B values			*/
/*----------------------------------------------------------------------*/

int printRGBvalues(char *tstr, int index, const char *postfix)
{
   int i;
   if (index >= 0 && index < number_colors) {
      sprintf(tstr, "%4.3f %4.3f %4.3f %s",
		(float)colorlist[index].color.red   / 65535,
		(float)colorlist[index].color.green / 65535,
		(float)colorlist[index].color.blue  / 65535,
		postfix);
      return 0;
   }

   /* The program can reach this point for any color which is	*/
   /* not listed in the table.  This can happen when parameters	*/
   /* printed from printobjectparams object contain the string	*/
   /* "@p_color".  Therefore print the default top-level	*/
   /* default color, which is black.				*/

   /* If the index is *not* DEFAULTCOLOR (-1), return an error	*/
   /* code.							*/

   sprintf(tstr, "0 0 0 %s", postfix);
   return (index == DEFAULTCOLOR) ? 0 : -1;
}

/*----------------------------------------------------*/
/* Write string to PostScript string, ignoring NO_OPs */
/*----------------------------------------------------*/

char *nosprint(char *baseptr, int *margin, int *extsegs)
{
   int qtmp, slen = 100;
   char *sptr, *lptr = NULL, lsave, *sptr2;
   u_char *pptr, *qptr, *bptr;

   bptr = (u_char *)malloc(slen);	/* initial length 100 */
   qptr = bptr;

   while(1) {	/* loop for breaking up margin-limited text into words */

      if (*margin > 0) {
	 sptr = strrchr(baseptr, ' ');
	 if (sptr == NULL)
	    sptr = baseptr;
	 else {
	    if (*(sptr + 1) == '\0') {
	       while (*sptr == ' ') sptr--;
	       *(sptr + 1) = '\0';
	       sptr2 = strrchr(baseptr, ' ');
	       *(sptr + 1) = ' ';
	       if (sptr2 == NULL)
		  sptr = baseptr;
	       else
		  sptr = sptr2 + 1;
	    }
	    else
	       sptr++;
	 }
      }
      else
	 sptr = baseptr;

      *qptr++ = '(';

      /* Includes extended character set (non-ASCII) */

      for (pptr = sptr; pptr && *pptr != '\0'; pptr++) {
         /* Ensure enough space for the string, including everything */
         /* following the "for" loop */
         qtmp = qptr - bptr;
         if (qtmp + 7 >= slen) {
	    slen += 7;
	    bptr = (char *)realloc(bptr, slen);
	    qptr = bptr + qtmp;
         }

         /* Deal with non-printable characters and parentheses */
         if (*pptr > (char)126) {
	    sprintf(qptr, "\\%3o", (int)(*pptr));
	    qptr += 4; 
         }
         else {
            if ((*pptr == '(') || (*pptr == ')') || (*pptr == '\\'))
	       *qptr++ = '\\';
            *qptr++ = *pptr;
         }
      }
      if (qptr == bptr + 1) {	/* Empty string gets a NULL result, not "()" */
         qptr--;
      }
      else {
         *qptr++ = ')';
         *qptr++ = ' ';
      }

      if (lptr != NULL)
	 *lptr = lsave;

      if (sptr == baseptr)
	 break;
      else {
	 lptr = sptr;
	 lsave = *lptr;
	 *lptr = '\0';
	 (*extsegs)++;
      }
   }

   *qptr++ = '\0';
   return (char *)bptr;
}

/*--------------------------------------------------------------*/
/* Write label segments to the output (in reverse order)	*/
/*--------------------------------------------------------------*/

short writelabel(FILE *ps, stringpart *chrtop, short *stcount)
{
   short i, segs = 0;
   stringpart *chrptr;
   char **ostr = (char **)malloc(sizeof(char *));
   char *tmpstr;
   float lastscale = 1.0;
   int lastfont = -1;
   int margin = 0;
   int extsegs = 0;

   /* Write segments into string array, in forward order */

   for (chrptr = chrtop; chrptr != NULL; chrptr = chrptr->nextpart) {
      ostr = (char **)realloc(ostr, (segs + 1) * sizeof(char *));
      if (chrtop->type == PARAM_END) {	/* NULL parameter is empty string */
	 ostr[segs] = (char *)malloc(4);
	 strcpy(ostr[segs], "() ");
      }
      else {
	 tmpstr = writesegment(chrptr, &lastscale, &lastfont, &margin, &extsegs);
	 if (tmpstr[0] != '\0')
            ostr[segs] = tmpstr;
	 else
	    segs--;
      }
      segs++;
   }

   /* Write string array to output in reverse order */
   for (i = segs - 1; i >= 0; i--) {
      dostcount(ps, stcount, strlen(ostr[i]));
      fputs(ostr[i], ps);
      free(ostr[i]);
   }
   free(ostr);	 

   return segs + extsegs;
}

/*--------------------------------------------------------------*/
/* Write a single label segment to the output			*/
/* (Recursive, so we can write segments in the reverse order)	*/
/*--------------------------------------------------------------*/

char *writesegment(stringpart *chrptr, float *lastscale, int *lastfont, int *margin,
	int *extsegs)
{
   int type = chrptr->type;
   char *retstr, *validname;

   switch(type) {
      case PARAM_START:
	 validname = create_valid_psname(chrptr->data.string, TRUE);
	 sprintf(_STR, "%s ", validname);
	 break;
      case PARAM_END:
	 _STR[0] = '\0';
	 chrptr->nextpart = NULL;
	 break;
      case SUBSCRIPT:
	 sprintf(_STR, "{ss} ");
	 break;
      case SUPERSCRIPT:
	 sprintf(_STR, "{Ss} ");
	 break;
      case NORMALSCRIPT:
	 *lastscale = 1.0;
         sprintf(_STR, "{ns} ");
         break;
      case UNDERLINE:
         sprintf(_STR, "{ul} ");
         break;
      case OVERLINE:
         sprintf(_STR, "{ol} ");
         break;
      case NOLINE:
         sprintf(_STR, "{} ");
         break;
      case HALFSPACE:
         sprintf(_STR, "{hS} ");
         break;
      case QTRSPACE:
	 sprintf(_STR, "{qS} ");
	 break;
      case RETURN:
	 *lastscale = 1.0;
	 if (chrptr->data.flags == 0)
	    // Ignore automatically-generated line breaks
	    sprintf(_STR, "{CR} ");
	 else
	    sprintf(_STR, "");
	 break;
      case TABSTOP:
	 sprintf(_STR, "{Ts} ");
	 break;
      case TABFORWARD:
	 sprintf(_STR, "{Tf} ");
	 break;
      case TABBACKWARD:
	 sprintf(_STR, "{Tb} ");
	 break;
      case FONT_NAME:
	 /* If font specifier is followed by a scale specifier, then	*/
	 /* record the font change but defer the output.  Otherwise,	*/
	 /* output the font record now.					*/

	 if ((chrptr->nextpart == NULL) || (chrptr->nextpart->type != FONT_SCALE))
	 {
	    if (*lastscale == 1.0)
	       sprintf(_STR, "{/%s cf} ", fonts[chrptr->data.font].psname);
	    else
	       sprintf(_STR, "{/%s %5.3f cf} ", fonts[chrptr->data.font].psname,
			*lastscale);
	 }
	 else
	    _STR[0] = '\0';
	 *lastfont = chrptr->data.font;
	 break;
      case FONT_SCALE:
	 if (*lastfont == -1) {
	    Fprintf(stderr, "Warning:  Font may not be the one that was intended.\n");
	    *lastfont = 0;
	 }
	 *lastscale = chrptr->data.scale;
	 sprintf(_STR, "{/%s %5.3f cf} ", fonts[*lastfont].psname, *lastscale);
	 break;
      case FONT_COLOR:
	 strcpy(_STR, "{");
	 if (chrptr->data.color == DEFAULTCOLOR)
	    strcat(_STR, "sce} ");
	 else
	    if (printRGBvalues(_STR + 1, chrptr->data.color, "scb} ") < 0)
	       strcat(_STR, "sce} ");
	 break;
      case MARGINSTOP:
	 sprintf(_STR, "{%d MR} ", chrptr->data.width);
	 *margin = chrptr->data.width;
	 break;
      case KERN:
	 sprintf(_STR, "{%d %d Kn} ", chrptr->data.kern[0], chrptr->data.kern[1]);
	 break;
      case TEXT_STRING:
	 /* Everything except TEXT_STRING will always fit in the _STR fixed-	*/
	 /* length character array. 						*/
	 return nosprint(chrptr->data.string, margin, extsegs);
   }

   retstr = (char *)malloc(1 + strlen(_STR));
   strcpy(retstr, _STR);
   return retstr;
}

/*--------------------------------------------------------------*/
/* Routine to write all the label segments as stored in _STR	*/
/*--------------------------------------------------------------*/

int writelabelsegs(FILE *ps, short *stcount, stringpart *chrptr)
{
   Boolean ismultipart;
   int segs;

   if (chrptr == NULL) return 0;

   ismultipart = ((chrptr->nextpart != NULL) &&
	   (chrptr->nextpart->type != PARAM_END)) ? True : False;

   /* If there is only one part, but it is not a string or the	*/
   /* end of a parameter (empty parameter), then set multipart	*/
   /* anyway so we get the double brace {{ }}.	  		*/

   if ((!ismultipart) && (chrptr->type != TEXT_STRING) &&
		(chrptr->type != PARAM_END))
      ismultipart = True;

   /* nextpart is not NULL if there are multiple parts to the string */
   if (ismultipart) {
      fprintf(ps, "{");
      (*stcount)++;
   }
   segs = writelabel(ps, chrptr, stcount);

   if (ismultipart) {
      fprintf(ps, "} ");
      (*stcount) += 2;
   }
   return segs;
}

/*--------------------------------------------------------------*/
/* Write the dictionary of parameters belonging to an object	*/
/*--------------------------------------------------------------*/

void printobjectparams(FILE *ps, objectptr localdata)
{
   int segs;
   short stcount;
   oparamptr ops;
   char *ps_expr, *validkey;
   float fp;

   /* Check for parameters and default values */
   if (localdata->params == NULL) return;

   fprintf(ps, "<<");
   stcount = 2;

   for (ops = localdata->params; ops != NULL; ops = ops->next) {
      validkey = create_valid_psname(ops->key, TRUE);
      fprintf(ps, "/%s ", validkey);
      dostcount (ps, &stcount, strlen(validkey) + 2);

      switch (ops->type) {
	 case XC_EXPR:
	    ps_expr = evaluate_expr(localdata, ops, NULL);
	    if (ops->which == P_SUBSTRING || ops->which == P_EXPRESSION) {
	       dostcount(ps, &stcount, 3 + strlen(ps_expr));
	       fputs("(", ps);
	       fputs(ps_expr, ps);
	       fputs(") ", ps);
	    }
	    else if (ops->which == P_COLOR) {
	       int ccol;
	       /* Write R, G, B components for PostScript */
	       if (sscanf(ps_expr, "%d", &ccol) == 1) {
	          fputs("{", ps);
	          printRGBvalues(_STR, ccol, "} ");
	          dostcount(ps, &stcount, 1 + strlen(_STR));
	          fputs(_STR, ps);
	       }
	       else {
	          dostcount(ps, &stcount, 8);
	          fputs("{0 0 0} ", ps);
	       }
	    }
	    else if (sscanf(ps_expr, "%g", &fp) == 1) {
	       dostcount(ps, &stcount, 1 + strlen(ps_expr));
	       fputs(ps_expr, ps);
	       fputs(" ", ps);
	    }
	    else {	/* Expression evaluates to error in object */
	       dostcount(ps, &stcount, 2);
	       fputs("0 ", ps);
            }
	    dostcount(ps, &stcount, 7 + strlen(ops->parameter.expr));
	    fputs("(", ps);
	    fputs(ops->parameter.expr, ps);
	    fputs(") pop ", ps);
	    free(ps_expr);
	    break;
	 case XC_STRING:
	    segs = writelabelsegs(ps, &stcount, ops->parameter.string);
	    if (segs == 0) {
	       /* When writing object parameters, we cannot allow a */
	       /* NULL value.  Instead, print an empty string ().   */
	       dostcount(ps, &stcount, 3);
	       fputs("() ", ps);
	    }
	    break;
	 case XC_INT:
	    sprintf(_STR, "%d ", ops->parameter.ivalue);
	    dostcount(ps, &stcount, strlen(_STR));
	    fputs(_STR, ps);
	    break;
	 case XC_FLOAT:
	    sprintf(_STR, "%g ", ops->parameter.fvalue);
	    dostcount(ps, &stcount, strlen(_STR));
	    fputs(_STR, ps);
	    break;
      }
   }

   fprintf(ps, ">> ");
   dostcount (ps, &stcount, 3);
}

/*--------------------------------------------------------------*/
/* Write the list of parameters belonging to an object instance */
/*--------------------------------------------------------------*/

short printparams(FILE *ps, objinstptr sinst, short stcount)
{
   int segs;
   short loccount;
   oparamptr ops, objops;
   eparamptr epp;
   char *ps_expr, *validkey, *validref;
   short instances = 0;

   if (sinst->params == NULL) return stcount;

   for (ops = sinst->params; ops != NULL; ops = ops->next) {
      validref = strdup(create_valid_psname(ops->key, TRUE));

      /* Check for indirect parameter references */
      for (epp = sinst->passed; epp != NULL; epp = epp->next) {
	 if ((epp->flags & P_INDIRECT) && (epp->pdata.refkey != NULL)) {
	    if (!strcmp(epp->pdata.refkey, ops->key)) {
	       if (instances++ == 0) {
		  fprintf(ps, "<<");		/* begin PostScript dictionary */
		  loccount = stcount + 2;
	       }
	       dostcount(ps, &loccount, strlen(validref + 3));
	       fprintf(ps, "/%s ", validref);
	       dostcount(ps, &loccount, strlen(epp->key + 1));
	       validkey = create_valid_psname(epp->key, TRUE);
	       fprintf(ps, "%s ", validkey);
	       break;
	    }
	 }
      }
      if (epp == NULL) {	/* No indirection */
	 Boolean nondefault = TRUE;
	 char *deflt_expr = NULL;
	
	 /* For instance values that are expression results, ignore if	*/
	 /* the instance value is the same as the default value.	*/
	 /* Correction 9/08:  We can't second-guess expression results,	*/
	 /* in particular, this doesn't work for an expression like	*/
	 /* "page", where the local and default values will evaluate to	*/
	 /* the same result, when clearly each page is intended to have	*/
	 /* its own instance value, and "page" for an object is not a	*/
	 /* well-defined concept.					*/

//	 ps_expr = NULL;
//	 objops = match_param(sinst->thisobject, ops->key);
//	 if (objops && (objops->type == XC_EXPR)) {
//	    int i;
//	    float f;
//	    deflt_expr = evaluate_expr(sinst->thisobject, objops, NULL);
//	    switch (ops->type) {
//	       case XC_STRING:
//		  if (!textcomp(ops->parameter.string, deflt_expr, sinst))
//		     nondefault = FALSE;
//		  break;
//	       case XC_EXPR:
//	          ps_expr = evaluate_expr(sinst->thisobject, ops, sinst);
//		  if (!strcmp(ps_expr, deflt_expr)) nondefault = FALSE;
//		  break;
//	       case XC_INT:
//		  sscanf(deflt_expr, "%d", &i);
//		  if (i == ops->parameter.ivalue) nondefault = FALSE;
//		  break;
//	       case XC_FLOAT:
//		  sscanf(deflt_expr, "%g", &f);
//		  if (f == ops->parameter.fvalue) nondefault = FALSE;
//		  break;
//	    }
//	    if (deflt_expr) free(deflt_expr);
//	    if (nondefault == FALSE) {
//	       if (ps_expr) free(ps_expr);
//	       continue;
//	    }
//	 }

	 if (instances++ == 0) {
	    fprintf(ps, "<<");		/* begin PostScript dictionary */
	    loccount = stcount + 2;
	 }
         dostcount(ps, &loccount, strlen(validref) + 2);
         fprintf(ps, "/%s ", validref);

         switch (ops->type) {
	    case XC_STRING:
	       segs = writelabelsegs(ps, &loccount, ops->parameter.string);
	       if (segs == 0) {
		  /* When writing object parameters, we cannot allow a	*/
		  /* NULL value.  Instead, print an empty string ().	*/
		  dostcount(ps, &stcount, 3);
		  fputs("() ", ps);
	       }
	       break;
	    case XC_EXPR:
	       ps_expr = evaluate_expr(sinst->thisobject, ops, sinst);
	       dostcount(ps, &loccount, 3 + strlen(ps_expr));
	       fputs("(", ps);
	       fputs(ps_expr, ps);
	       fputs(") ", ps);
	       free(ps_expr);

	       /* The instance parameter expression may have the	*/ 
	       /* same expression as the object but a different		*/
	       /* result if the expression uses another parameter.	*/
	       /* Only print the expression itself if it's different	*/
	       /* from the object's expression.				*/

	       objops = match_param(sinst->thisobject, ops->key);
	       if (objops && strcmp(ops->parameter.expr, objops->parameter.expr)) {
	          dostcount(ps, &loccount, 3 + strlen(ops->parameter.expr));
	          fputs("(", ps);
	          fputs(ops->parameter.expr, ps);
	          fputs(") pop ", ps);
	       }
	       break;
	    case XC_INT:
	       if (ops->which == P_COLOR) {
		  /* Write R, G, B components */
		  _STR[0] = '{';
	          printRGBvalues(_STR + 1, ops->parameter.ivalue, "} ");
	       }
	       else
	          sprintf(_STR, "%d ", ops->parameter.ivalue);
	       dostcount(ps, &loccount, strlen(_STR));
	       fputs(_STR, ps);
	       break;
	    case XC_FLOAT:
	       sprintf(_STR, "%g ", ops->parameter.fvalue);
	       dostcount(ps, &loccount, strlen(_STR));
	       fputs(_STR, ps);
	       break;
	 }
      }
      free(validref);
   }
   if (instances > 0) {
      fprintf(ps, ">> ");		/* end PostScript dictionary */
      loccount += 3;
   }
   return loccount;
}

/*------------------------------------------------------------------*/
/* Macro for point output (calls varpcheck() on x and y components) */
/*------------------------------------------------------------------*/

#define xyvarcheck(z, n, t) \
    varpcheck(ps, (z).x, localdata, n, &stcount, *(t), P_POSITION_X); \
    varpcheck(ps, (z).y, localdata, n, &stcount, *(t), P_POSITION_Y)

#define xypathcheck(z, n, t, p) \
    varpathcheck(ps, (z).x, localdata, n, &stcount, t, TOPATH(p), P_POSITION_X); \
    varpathcheck(ps, (z).y, localdata, n, &stcount, t, TOPATH(p), P_POSITION_Y)
  
/*--------------------------------------------------------------*/
/* Main routine for writing the contents of a single object to	*/
/* output file "ps".						*/
/*--------------------------------------------------------------*/

void printOneObject(FILE *ps, objectptr localdata, int ccolor)
{
   int i, curcolor = ccolor;
   genericptr *savegen, *pgen;
   objinstptr sobj;
   graphicptr sg;
   Imagedata *img;
   pointlist savept;
   short stcount;
   short segs;
   Boolean has_parameter;
   char *fptr, *validname;

   /* first, get a total count of all objects and give warning if large */

   if ((is_page(localdata) == -1) && (localdata->parts > 255)) {
      Wprintf("Warning: \"%s\" may exceed printer's PS limit for definitions",
	    localdata->name);
   }
	   
   for (savegen = localdata->plist; savegen < localdata->plist +
		localdata->parts; savegen++) {

      /* Check if this color is parameterized */
      eparamptr epp;
      oparamptr ops;

      /* FIXEDBBOX style is handled in the object header and	*/
      /* the part should be skipped.				*/

      if (ELEMENTTYPE(*savegen) == POLYGON)
	 if (TOPOLY(savegen)->style & FIXEDBBOX)
	    continue;

      for (epp = (*savegen)->passed; epp != NULL; epp = epp->next) {
	 ops = match_param(localdata, epp->key);
	 if (ops != NULL && (ops->which == P_COLOR)) {
	    /* Ensure that the next element forces a color change */
	    curcolor = ERRORCOLOR;
	    sprintf(_STR, "%s scb\n", epp->key);
	    fputs(_STR, ps);
	    break;
	 }
      }

      /* Enforce the rule that clipmasks must always be DEFAULTCOLOR */

      switch(ELEMENTTYPE(*savegen)) {
	 case POLYGON: case SPLINE: case ARC: case PATH:
	    if (TOPOLY(savegen)->style & CLIPMASK)
	       (*savegen)->color = DEFAULTCOLOR;
	    break;
      }

      /* change current color if different */

      if ((epp == NULL) && ((*savegen)->color != curcolor)) {
	 if ((curcolor = (*savegen)->color) == DEFAULTCOLOR)
	    fprintf(ps, "sce\n");
	 else {
	    if (printRGBvalues(_STR, (*savegen)->color, "scb\n") < 0) {
	       fprintf(ps, "sce\n");
	       curcolor = DEFAULTCOLOR;
	    }
	    else
	       fputs(_STR, ps); 
	 }
      }

      stcount = 0;
      switch(ELEMENTTYPE(*savegen)) {

	 case(POLYGON):
	    varcheck(ps, TOPOLY(savegen)->style, localdata, &stcount,
			*savegen, P_STYLE);
	    varfcheck(ps, TOPOLY(savegen)->width, localdata, &stcount,
			*savegen, P_LINEWIDTH);
            for (savept = TOPOLY(savegen)->points; savept < TOPOLY(savegen)->
		      points + TOPOLY(savegen)->number; savept++) {
	       varpcheck(ps, savept->x, localdata,
			savept - TOPOLY(savegen)->points, &stcount, *savegen,
			P_POSITION_X);
	       varpcheck(ps, savept->y, localdata,
			savept - TOPOLY(savegen)->points, &stcount, *savegen,
			P_POSITION_Y);
	    }
	    sprintf(_STR, "%hd ", TOPOLY(savegen)->number);
	    dostcount (ps, &stcount, strlen(_STR));
	    fputs(_STR, ps);
	    if (varpcheck(ps, 0, localdata, -1, &stcount, *savegen,
			P_POSITION_X)) {
	       sprintf(_STR, "addtox ");
	       dostcount (ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    if (varpcheck(ps, 0, localdata, -1, &stcount, *savegen,
			P_POSITION_Y)) {
	       sprintf(_STR, "addtoy ");
	       dostcount (ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    sprintf(_STR, "polygon\n");
	    dostcount (ps, &stcount, strlen(_STR));
	    fputs(_STR, ps);
	    break;

	 case(PATH):
	    pgen = TOPATH(savegen)->plist;
	    switch(ELEMENTTYPE(*pgen)) {
	       case POLYGON:
		  xypathcheck(TOPOLY(pgen)->points[0], 0, pgen, savegen);
		  break;
	       case SPLINE:
		  xypathcheck(TOSPLINE(pgen)->ctrl[0], 0, pgen, savegen);
		  break;
	    }
	    dostcount(ps, &stcount, 9);
	    if (varpathcheck(ps, 0, localdata, -1, &stcount, pgen,
			TOPATH(savegen), P_POSITION_X)) {
	       sprintf(_STR, "addtox1 ");
	       dostcount (ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    if (varpathcheck(ps, 0, localdata, -1, &stcount, pgen,
			TOPATH(savegen), P_POSITION_Y)) {
	       sprintf(_STR, "addtoy1 ");
	       dostcount (ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    fprintf(ps, "beginpath\n");
	    for (pgen = TOPATH(savegen)->plist; pgen < TOPATH(savegen)->plist
			+ TOPATH(savegen)->parts; pgen++) {
	       switch(ELEMENTTYPE(*pgen)) {
		  case POLYGON:
               	     for (savept = TOPOLY(pgen)->points + TOPOLY(pgen)->number
				- 1; savept > TOPOLY(pgen)->points; savept--) {
			xypathcheck(*savept, savept - TOPOLY(pgen)->points, pgen,
				savegen);
	       	     }
	       	     sprintf(_STR, "%hd ", TOPOLY(pgen)->number - 1);
		     dostcount (ps, &stcount, strlen(_STR));
		     fputs(_STR, ps);
		     if (varpathcheck(ps, 0, localdata, -1, &stcount, pgen,
				TOPATH(savegen), P_POSITION_X)) {
	 	        sprintf(_STR, "addtox ");
	 	        dostcount (ps, &stcount, strlen(_STR));
		        fputs(_STR, ps);
	 	     }
	 	     if (varpathcheck(ps, 0, localdata, -1, &stcount, pgen,
				TOPATH(savegen), P_POSITION_Y)) {
	 	        sprintf(_STR, "addtoy ");
		        dostcount (ps, &stcount, strlen(_STR));
	 	        fputs(_STR, ps);
	 	     }
		     sprintf(_STR, "polyc\n");
	       	     dostcount (ps, &stcount, strlen(_STR));
	       	     fputs(_STR, ps);
	       	     break;
		  case SPLINE:
		     xypathcheck(TOSPLINE(pgen)->ctrl[1], 1, pgen, savegen);
		     xypathcheck(TOSPLINE(pgen)->ctrl[2], 2, pgen, savegen);
		     xypathcheck(TOSPLINE(pgen)->ctrl[3], 3, pgen, savegen);
	    	     if (varpathcheck(ps, 0, localdata, -1, &stcount, pgen,
				TOPATH(savegen), P_POSITION_X)) {
	     	        sprintf(_STR, "addtox3 ");
	     	        dostcount (ps, &stcount, strlen(_STR));
	     	        fputs(_STR, ps);
	     	     }
	     	     if (varpathcheck(ps, 0, localdata, -1, &stcount, pgen,
				TOPATH(savegen), P_POSITION_Y)) {
	     	        sprintf(_STR, "addtoy3 ");
	     	        dostcount (ps, &stcount, strlen(_STR));
	     	        fputs(_STR, ps);
	     	     }
		     fprintf(ps, "curveto\n");
		     break;
	       }
	    }
	    varcheck(ps, TOPATH(savegen)->style, localdata, &stcount,
			*savegen, P_STYLE);
	    varfcheck(ps, TOPATH(savegen)->width, localdata, &stcount,
			*savegen, P_LINEWIDTH);
	    fprintf(ps, "endpath\n");
	    break;

	 case(SPLINE):
	    varcheck(ps, TOSPLINE(savegen)->style, localdata, &stcount,
			*savegen, P_STYLE);
	    varfcheck(ps, TOSPLINE(savegen)->width, localdata, &stcount,
			*savegen, P_LINEWIDTH);
	    xyvarcheck(TOSPLINE(savegen)->ctrl[1], 1, savegen);
	    xyvarcheck(TOSPLINE(savegen)->ctrl[2], 2, savegen);
	    xyvarcheck(TOSPLINE(savegen)->ctrl[3], 3, savegen);
	    xyvarcheck(TOSPLINE(savegen)->ctrl[0], 0, savegen);
	    if (varpcheck(ps, 0, localdata, -1, &stcount, *savegen,
			P_POSITION_X)) {
	       sprintf(_STR, "addtox4 ");
	       dostcount (ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    if (varpcheck(ps, 0, localdata, -1, &stcount, *savegen,
			P_POSITION_Y)) {
	       sprintf(_STR, "addtoy4 ");
	       dostcount (ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    fprintf(ps, "spline\n");
	    break;

	 case(ARC):
	    varcheck(ps, TOARC(savegen)->style, localdata, &stcount,
			*savegen, P_STYLE);
	    varfcheck(ps, TOARC(savegen)->width, localdata, &stcount,
			*savegen, P_LINEWIDTH);
	    xyvarcheck(TOARC(savegen)->position, 0, savegen);
	    varcheck(ps, abs(TOARC(savegen)->radius), localdata, &stcount,
			*savegen, P_RADIUS);
	    if (abs(TOARC(savegen)->radius) == TOARC(savegen)->yaxis) {
	       varfcheck(ps, TOARC(savegen)->angle1, localdata, &stcount,
			*savegen, P_ANGLE1);
	       varfcheck(ps, TOARC(savegen)->angle2, localdata, &stcount,
			*savegen, P_ANGLE2);
	       fprintf(ps, "xcarc\n");
	    }
	    else {
	       varcheck(ps, abs(TOARC(savegen)->yaxis), localdata, &stcount,
			*savegen, P_MINOR_AXIS);
	       varfcheck(ps, TOARC(savegen)->angle1, localdata, &stcount,
			*savegen, P_ANGLE1);
	       varfcheck(ps, TOARC(savegen)->angle2, localdata, &stcount,
			*savegen, P_ANGLE2);
	       fprintf(ps, "ellipse\n");
	    }
	    break;

	 case(OBJINST):
	    sobj = TOOBJINST(savegen);
	    varfcheck(ps, sobj->scale, localdata, &stcount, *savegen, P_SCALE);
	    if (!(sobj->style & LINE_INVARIANT)) fprintf(ps, "/sv ");
	    varfcheck(ps, sobj->rotation, localdata, &stcount, *savegen, P_ROTATION);
	    xyvarcheck(sobj->position, 0, savegen);

	    opsubstitute(sobj->thisobject, sobj);
	    stcount = printparams(ps, sobj, stcount);

	    validname = create_valid_psname(sobj->thisobject->name, FALSE);

	    /* Names without technologies get a leading string '::' 	*/
	    /* (blank technology)					*/

	    if (strstr(validname, "::") == NULL)
	       fprintf(ps, "::%s\n", validname);
	    else
	       fprintf(ps, "%s\n", validname);
	    break;
            
	 case(GRAPHIC):
	    sg = TOGRAPHIC(savegen);
	    for (i = 0; i < xobjs.images; i++) {
	       img = xobjs.imagelist + i;
	       if (img->image == sg->source)
		   break;
	    }

	    fptr = strrchr(img->filename, '/');
	    if (fptr == NULL)
	       fptr = img->filename;
	    else
	       fptr++;
	    fprintf(ps, "/%s ", fptr);
	    stcount += (2 + strlen(fptr));

	    varfcheck(ps, sg->scale, localdata, &stcount, *savegen, P_SCALE);
	    varfcheck(ps, sg->rotation, localdata, &stcount, *savegen, P_ROTATION);
	    xyvarcheck(sg->position, 0, savegen);
	    fprintf(ps, "graphic\n");
	    break;

	 case(LABEL):

	    /* Don't save temporary labels from schematic capture system */
	    if (TOLABEL(savegen)->string->type != FONT_NAME) break;

	    /* Check for parameter --- must use "mark" to count # segments */
	    has_parameter = hasparameter(TOLABEL(savegen));

	    if (has_parameter) {
	       fprintf(ps, "mark ");
	       stcount += 5;
	    }

	    segs = writelabel(ps, TOLABEL(savegen)->string, &stcount);

	    if (segs > 0) {
	       if (has_parameter)
                  sprintf(_STR, "ctmk ");
	       else
                  sprintf(_STR, "%hd ", segs);
	       dostcount(ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	       varcheck(ps, TOLABEL(savegen)->anchor, localdata, &stcount,
			*savegen, P_ANCHOR);
	       varfcheck(ps, TOLABEL(savegen)->rotation, localdata, &stcount,
			*savegen, P_ROTATION);
	       varfcheck(ps, TOLABEL(savegen)->scale, localdata, &stcount,
			*savegen, P_SCALE);
	       xyvarcheck(TOLABEL(savegen)->position, 0, savegen);

	       switch(TOLABEL(savegen)->pin) {
		  case LOCAL:
		     strcpy(_STR, "pinlabel\n"); break;
		  case GLOBAL:
		     strcpy(_STR, "pinglobal\n"); break;
		  case INFO:
		     strcpy(_STR, "infolabel\n"); break;
		  default:
		     strcpy(_STR, "label\n");
	       }
	       dostcount(ps, &stcount, strlen(_STR));
	       fputs(_STR, ps);
	    }
	    break;
      }
   }
}

/*----------------------------------------------------------------------*/
/* Recursive routine to print out the library objects used in this	*/
/* drawing, starting at the bottom of the object hierarchy so that each	*/
/* object is defined before it is called.  A list of objects already	*/
/* written is maintained so that no object is written twice. 		*/
/*									*/
/* When object "localdata" is not a top-level page, call this routine	*/
/* with mpage=-1 (simpler than checking whether localdata is a page).	*/
/*----------------------------------------------------------------------*/

void printobjects(FILE *ps, objectptr localdata, objectptr **wrotelist,
	short *written, int ccolor)
{
   genericptr *gptr, *savegen;
   objectptr *optr;
   /* oparamptr ops; (jdk) */
   char *validname;
   int curcolor = ccolor;
   /* int libno; (jdk) */

   /* Search among the list of objects already written to the output	*/
   /* If this object has been written previously, then we ignore it.	*/

   for (optr = *wrotelist; optr < *wrotelist + *written; optr++)
      if (*optr == localdata)
	  return;

   /* If this page is a schematic, write out the definiton of any symbol */
   /* attached to it, because that symbol may not be used anywhere else. */

   if (localdata->symschem && (localdata->schemtype == PRIMARY))
      printobjects(ps, localdata->symschem, wrotelist, written, curcolor);

   /* Search for all object definitions instantiated in this object,	*/
   /* and (recursively) print them to the output.			*/

   for (gptr = localdata->plist; gptr < localdata->plist + localdata->parts; gptr++)
      if (IS_OBJINST(*gptr))
         printobjects(ps, TOOBJINST(gptr)->thisobject, wrotelist, written, curcolor);

   /* Update the list of objects already written to the output */

   *wrotelist = (objectptr *)realloc(*wrotelist, (*written + 1) * 
		sizeof(objectptr));
   *(*wrotelist + *written) = localdata;
   (*written)++;

   validname = create_valid_psname(localdata->name, FALSE);
   if (strstr(validname, "::") == NULL)
      fprintf(ps, "/::%s {\n", validname);
   else
      fprintf(ps, "/%s {\n", validname);

   /* Write a "bbox" record if there is an element with style FIXEDBBOX	*/
   /* This is the only time a "bbox" record is written for an object.	*/

   for (savegen = localdata->plist; savegen < localdata->plist +
		localdata->parts; savegen++) {
      if (ELEMENTTYPE(*savegen) == POLYGON) {
	 if (TOPOLY(savegen)->style & FIXEDBBOX) {
	    pointlist polypoints;
	    int width, height;
	    polypoints = TOPOLY(savegen)->points + 2;
	    width = polypoints->x;
	    height = polypoints->y;
	    polypoints = TOPOLY(savegen)->points;
	    width -= polypoints->x;
	    height -= polypoints->y;
	    fprintf(ps, "%% %d %d %d %d bbox\n",
			polypoints->x, polypoints->y,
			width, height);
	    break;
	 }
      }
   }

   if (localdata->hidden == True) fprintf(ps, "%% hidden\n");

   /* For symbols with schematics, and "trivial" schematics */
   if (localdata->symschem != NULL)
      fprintf(ps, "%% %s is_schematic\n", localdata->symschem->name);
   else if (localdata->schemtype == TRIVIAL)
      fprintf(ps, "%% trivial\n");
   else if (localdata->schemtype == NONETWORK)
      fprintf(ps, "%% nonetwork\n");

   printobjectparams(ps, localdata);
   fprintf(ps, "begingate\n");

   /* Write all the elements in order */

   opsubstitute(localdata, NULL);
   printOneObject(ps, localdata, curcolor);

   /* Write object (gate) trailer */

   fprintf(ps, "endgate\n} def\n\n");
}

/*--------------------------------------------------------------*/
/* Print a page header followed by everything in the page.	*/
/* this routine assumes that all objects used by the page have	*/
/* already been handled and written to the output.		*/
/*								*/
/* "page" is the page number, counting consecutively from one.	*/
/* "mpage" is the page number in xcircuit's pagelist structure.	*/
/*--------------------------------------------------------------*/

void printpageobject(FILE *ps, objectptr localdata, short page, short mpage)
{
  /* genericptr *gptr; (jdk) */
   XPoint origin, corner;
   objinstptr writepage;
   int width, height;
   float psnorm, psscale;
   float xmargin, ymargin;
   char *rootptr = NULL;
   polyptr framebox;

   /* Output page header information */

   if (xobjs.pagelist[mpage]->filename)
      rootptr = strrchr(xobjs.pagelist[mpage]->filename, '/');
   if (rootptr == NULL)
      rootptr = xobjs.pagelist[mpage]->filename;
   else rootptr++;

   writepage = xobjs.pagelist[mpage]->pageinst;

   psnorm = xobjs.pagelist[mpage]->outscale;
   psscale = getpsscale(psnorm, mpage);

   /* Determine the margins (offset of drawing from page corner)	*/
   /* If a bounding box has been declared in the drawing, it is		*/
   /* centered on the page.  Otherwise, the drawing itself is		*/
   /* centered on the page.  If encapsulated, the bounding box		*/
   /* encompasses only the object itself.				*/

   width = toplevelwidth(writepage, &origin.x);
   height = toplevelheight(writepage, &origin.y);

   corner.x = origin.x + width;
   corner.y = origin.y + height;

   if (xobjs.pagelist[mpage]->pmode & 1) {	/* full page */

      if (xobjs.pagelist[mpage]->orient == 90) {
	 xmargin = (xobjs.pagelist[mpage]->pagesize.x -
		((float)height * psscale)) / 2;
	 ymargin = (xobjs.pagelist[mpage]->pagesize.y -
		((float)width * psscale)) / 2;
      }
      else {
         xmargin = (xobjs.pagelist[mpage]->pagesize.x -
		((float)width * psscale)) / 2;
         ymargin = (xobjs.pagelist[mpage]->pagesize.y -
		((float)height * psscale)) / 2;
      }
   }
   else {	/* encapsulated --- should have 1" border so that any */
		/* drawing passed directly to a printer will not clip */
      xmargin = xobjs.pagelist[mpage]->margins.x;
      ymargin = xobjs.pagelist[mpage]->margins.y;
   }

   /* If a framebox is declared, then we adjust the page to be	*/
   /* centered on the framebox by translating through the	*/
   /* difference between the object center and the framebox	*/
   /* center.							*/

   if ((framebox = checkforbbox(localdata)) != NULL) {
      int i, fcentx = 0, fcenty = 0;

      for (i = 0; i < framebox->number; i++) {
	 fcentx += framebox->points[i].x;
	 fcenty += framebox->points[i].y;
      }
      fcentx /= framebox->number;
      fcenty /= framebox->number;

      xmargin += psscale * (float)(origin.x + (width >> 1) - fcentx);
      ymargin += psscale * (float)(origin.y + (height >> 1) - fcenty);
   }

   /* If the page label is just the root name of the file, or has been left	*/
   /* as "Page n" or "Page_n", just do the normal page numbering.  Otherwise,	*/
   /* write out the page label explicitly.					*/

   if ((rootptr == NULL) || (!strcmp(rootptr, localdata->name))
		|| (strchr(localdata->name, ' ') != NULL)
		|| (strstr(localdata->name, "Page_") != NULL))
      fprintf (ps, "%%%%Page: %d %d\n", page, page);
   else
      fprintf (ps, "%%%%Page: %s %d\n", localdata->name, page);

   if (xobjs.pagelist[mpage]->orient == 90)
      fprintf (ps, "%%%%PageOrientation: Landscape\n");
   else
      fprintf (ps, "%%%%PageOrientation: Portrait\n");

   if (xobjs.pagelist[mpage]->pmode & 1) { 	/* full page */
      fprintf(ps, "%%%%PageBoundingBox: 0 0 %d %d\n",
		xobjs.pagelist[mpage]->pagesize.x,
		xobjs.pagelist[mpage]->pagesize.y);
   }

   /* Encapsulated files do not get a PageBoundingBox line,	*/
   /* unless the bounding box was explicitly drawn.		*/

   else if (framebox != NULL) {
      fprintf(ps, "%%%%PageBoundingBox: %g %g %g %g\n",
	xmargin, ymargin,
	xmargin + psscale * (float)(width),
	ymargin + psscale * (float)(height));
   }
     
   fprintf (ps, "/pgsave save def bop\n");

   /* Top-page definitions */
   if (localdata->params != NULL) {
      printobjectparams(ps, localdata);
      fprintf(ps, "begin\n");
   }

   if (localdata->symschem != NULL) {
      if (is_page(localdata->symschem) == -1)
         fprintf(ps, "%% %s is_symbol\n", localdata->symschem->name);
      else if (localdata->schemtype == SECONDARY)
         fprintf(ps, "%% %s is_primary\n", localdata->symschem->name);
      else
	 Wprintf("Something is wrong. . . schematic \"%s\" is connected to"
			" schematic \"%s\" but is not declared secondary.\n",
			localdata->name, localdata->symschem->name);
   }

   /* Extend bounding box around schematic pins */
   extendschembbox(xobjs.pagelist[mpage]->pageinst, &origin, &corner);

   if (xobjs.pagelist[mpage]->drawingscale.x != 1
		|| xobjs.pagelist[mpage]->drawingscale.y != 1)
      fprintf(ps, "%% %hd:%hd drawingscale\n", xobjs.pagelist[mpage]->drawingscale.x,
	 	xobjs.pagelist[mpage]->drawingscale.y);

   if (xobjs.pagelist[mpage]->gridspace != 32
		|| xobjs.pagelist[mpage]->snapspace != 16)
      fprintf(ps, "%% %4.2f %4.2f gridspace\n", xobjs.pagelist[mpage]->gridspace,
		xobjs.pagelist[mpage]->snapspace);

   if (xobjs.pagelist[mpage]->background.name != (char *)NULL) {
     /* float iscale = (xobjs.pagelist[mpage]->coordstyle == CM) ? CMSCALE : INCHSCALE; (jdk) */
      if (xobjs.pagelist[mpage]->orient == 90)
	 fprintf(ps, "%5.4f %d %d 90 psinsertion\n", psnorm,
		  (int)(ymargin - xmargin),
		  -((int)((float)(corner.y - origin.y) * psscale) +
		  (int)(xmargin + ymargin)));
      else
	 fprintf(ps, "%5.4f %d %d 0 psinsertion\n", psnorm,
		(int)(xmargin / psscale) - origin.x,
		(int)(ymargin / psscale) - origin.y);
      savebackground(ps, xobjs.pagelist[mpage]->background.name);
      fprintf(ps, "\nend_insert\n");
   }

   if (xobjs.pagelist[mpage]->orient == 90)
      fprintf(ps, "90 rotate %d %d translate\n", (int)(ymargin - xmargin),
	     -((int)((float)(corner.y - origin.y) * psscale) + 
	     (int)(xmargin + ymargin)));

   fprintf(ps, "%5.4f ", psnorm);
   switch(xobjs.pagelist[mpage]->coordstyle) {
      case CM:
	 fprintf(ps, "cmscale\n");
	 break;
      default:
	 fprintf(ps, "inchscale\n");
	 break;
   }

   /* Final scale and translation */
   fprintf(ps, "%5.4f setlinewidth %d %d translate\n\n",
		1.3 * xobjs.pagelist[mpage]->wirewidth,
		(int)(xmargin / psscale) - origin.x,
		(int)(ymargin / psscale) - origin.y);

   /* Output all the elements in the page */
   printOneObject(ps, localdata, DEFAULTCOLOR);

   /* Page trailer */
   if (localdata->params != NULL) fprintf(ps, "end ");
   fprintf(ps, "pgsave restore showpage\n");
}

/*--------------------------------------------------------------*/
/* Print objects referenced from a particular page.  These get	*/
/* bundled together at the beginning of the output file under	*/
/* the DSC "Setup" section, so that the PostScript		*/
/* interpreter knows that these definitions may be used by any	*/
/* page.  This prevents ghostscript from producing an error	*/
/* when backing up in a multi-page document.			*/
/*--------------------------------------------------------------*/

void printrefobjects(FILE *ps, objectptr localdata, objectptr **wrotelist,
	short *written)
{
   genericptr *gptr;

   /* If this page is a schematic, write out the definiton of any symbol */
   /* attached to it, because that symbol may not be used anywhere else. */

   if (localdata->symschem && (localdata->schemtype == PRIMARY))
      printobjects(ps, localdata->symschem, wrotelist, written, DEFAULTCOLOR);

   /* Search for all object definitions instantiated on the page and	*/
   /* write them to the output.						*/

   for (gptr = localdata->plist; gptr < localdata->plist + localdata->parts; gptr++)
      if (IS_OBJINST(*gptr))
         printobjects(ps, TOOBJINST(gptr)->thisobject, wrotelist, written,
		DEFAULTCOLOR);
}

/*----------------------------------------------------------------------*/

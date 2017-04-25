/*-----------------------------------------------------------------------*/
/* formats.c --- input file support for xcircuit			 */
/* Copyright (c) 2001  Tim Edwards, Johns Hopkins University        	 */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

/*------------------------------------------------------------------------*/
/* Local includes                                                         */
/*------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*------------------------------------------------------------------------*/
/* Useful (local) defines						  */
/*------------------------------------------------------------------------*/

#define xmat(a)		(((a) - 3300) << 4)
#define ymat(a)		((3300 - (a)) << 4)
#define S_OBLIQUE 13	/* position of Symbol-Oblique in font array */

/*------------------------------------------------------------------------*/
/* External Variable definitions                                          */
/*------------------------------------------------------------------------*/

extern char _STR[150];
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern Display *dpy;

/*----------------------------------------------*/
#ifdef LGF

/*----------------------------------------------*/
/* loadlgf: Load an analog LGF file 		*/
/*	mode = 1: import into current page;	*/
/*	mode = 0 clear current page and load	*/
/*----------------------------------------------*/

void loadlgf(int mode)
{
   FILE *ps;
   char inname[150], temp[500], *pdchar;
   char **signals;
   short *signets;
   objectptr *libobj;
   genericptr *iolabel;
   int i, sigs;

   sscanf(_STR, "%149s", inname);

   ps = fopen(inname, "r");
   if (ps == NULL) {
      sprintf(inname, "%s.lgf", _STR);
      ps = fopen(inname, "r");
      if (ps == NULL) {
	 sprintf(inname, "%s.lfo", _STR);
	 ps = fopen(inname, "r");
	 if (ps == NULL) {
	    Wprintf("Can't open LGF file %s", inname);
	    return;
         }
      }
   }

   /* for PostScript file, remove ".lgf" or ".lfo" (to be replaced with ".ps") */

   if ((pdchar = strstr(inname, ".l")) != NULL) *pdchar = '\0';

   Wprintf("Loaded file: %s", inname);

   /* Make sure that LGF object library has been loaded by loading it now. */

   if (NameToLibrary(LGF_LIB) < 0) {
      int ilib;
      strcpy(_STR, LGF_LIB);
      ilib = createlibrary(FALSE);
      loadlibrary(ilib);
   }
   
   /* Read header information */

   if (fgets(temp, 149, ps) == NULL) {
      Wprintf("Error: end of file.");
      return;
   }
   for (pdchar = temp; *pdchar != '-' && *pdchar != '\n'; pdchar++);
   if (*pdchar == '\n') {
      Wprintf("Not an LGF file?");
      return;
   }
   if (*(++pdchar) != '5') {
      Wprintf("Don't know how to read version %c.", *pdchar);
      return;
   }
   if (fgets(temp, 149, ps) == NULL) {
      Wprintf("Error: end of file.");
      return;
   }
   for (pdchar = temp; *pdchar != 'f' && *pdchar != '\n'; pdchar++);
   for (; *pdchar != 's' && *pdchar != '\n'; pdchar++);
   if (*pdchar == '\n') {
      Wprintf("Something wrong with the LGF file?");
      return;
   }

   /* Done with header. . . okay to clear current page now unless importing */

   if (mode == 0) {
      reset(topobject, NORMAL);
      pagereset(areawin->page); 
   }

   /* Set up filename and object (page) name */

   xobjs.pagelist[areawin->page]->filename = (char *) realloc (
      xobjs.pagelist[areawin->page]->filename, (strlen(inname) + 1) * sizeof(char));
   strcpy(xobjs.pagelist[areawin->page]->filename, inname);

   /* If the filename has a path component, use only the root */

   if ((pdchar = strrchr(inname, '/')) != NULL)
      sprintf(topobject->name, "%s", pdchar + 1);
   else
      sprintf(topobject->name, "%s", inname);

   renamepage(areawin->page);
   printname(topobject);

   /* Read objects */

   for(;;) {
      char *lineptr, keyptr, tmpstring[256];
      int dval;
      short pvalx, pvaly, pvalx2, pvaly2;

      if (fgets(temp, 499, ps) == NULL) break;		/* End-Of-File */

      /* ignore whitespace */
      for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
      if (*lineptr == '\n') continue;  /* ignore blank lines */
      switch(keyptr = *lineptr) {

	 case '#':	/* comment */
	    break;

	 case 'n':	/* nodes */
	    sscanf(++lineptr, "%d", &dval); 	    
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in node section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');
	    }
	    break;

	 case 's': 	/* signal names --- save for future reference */

	    sscanf(++lineptr, "%d", &sigs);
	    signals = (char **) malloc(sigs * sizeof(char *));
	    signets = (short *) malloc(sigs * sizeof(short));
	    for (i = 0; i < sigs; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in signal section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');
	       
	       sscanf(lineptr, "%hd %249s", &signets[i], tmpstring);

	       signals[i] = (char *)malloc((strlen(tmpstring) + 1) * sizeof(char));
	       sprintf(signals[i], "%s", tmpstring);
            }
	    break;
	 
	 case 'l': {	/* labels */

	    labelptr *newlabel;
	    char *tstrp;

	    sscanf(++lineptr, "%d", &dval);
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in signal section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');
	       
	       /* Allocate label, and put node number into X value, to be replaced */
	       /* Flag it using an inappropriate rotation value (= 500) */

	       sscanf(lineptr, "%hd %hd", &pvalx, &pvaly);

	       /* Get rid of newline character, if any */

	       ridnewline(lineptr);

	       /* forward to the label part of the input line */

	       tstrp = lineptr - 1;
	       while (isdigit(*(++tstrp)));
	       while (isspace(*(++tstrp)));
	       while (isdigit(*(++tstrp)));
	       while (isspace(*(++tstrp)));
	       while (isdigit(*(++tstrp)));
	       while (isspace(*(++tstrp)));

	       if (tstrp != NULL) {	/* could be a blank line */
		  stringpart *strptr;

		  NEW_LABEL(newlabel, topobject);

		  labeldefaults(*newlabel, False, xmat(pvalx), ymat(pvaly));
	          (*newlabel)->anchor = TOP | NOTBOTTOM;
		  (*newlabel)->color = DEFAULTCOLOR;
		  (*newlabel)->string->data.font = 0;
		  strptr = makesegment(&((*newlabel)->string), NULL);
		  strptr->type = TEXT_STRING;
		  strptr->data.string = (char *)malloc(1 + strlen(tstrp));
		  strcpy(strptr->data.string, tstrp);
		  (*newlabel)->pin = NORMAL;
	       }
            }}
	    break;

	case 'w': {	/* wires, implemented as single-segment polygons */
	    polyptr *newwire;
	    XPoint  *tmppnts;

	    sscanf(++lineptr, "%d", &dval);
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in wire section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');

	       /* Allocate wire */

	       NEW_POLY(newwire, topobject);

	       sscanf(lineptr, "%hd %hd %hd %hd", &pvalx, &pvaly, &pvalx2, &pvaly2);
	       (*newwire)->number = 2;
	       (*newwire)->points = (XPoint *)malloc(2 * sizeof(XPoint));
	       (*newwire)->width = 1.0;
	       (*newwire)->style = UNCLOSED;
	       (*newwire)->color = DEFAULTCOLOR;
	       (*newwire)->passed = NULL;
	       tmppnts = (*newwire)->points;
	       tmppnts->x = xmat(pvalx);
	       tmppnts->y = ymat(pvaly);
	       (++tmppnts)->x = xmat(pvalx2);
	       tmppnts->y = ymat(pvaly2);

	    }}
	    break;

	case 'p': 	/* solder dot */
	    sscanf(++lineptr, "%d", &dval);
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in solder dot section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');

	       /* Allocate arc */

	       sscanf(lineptr, "%hd %hd", &pvalx, &pvaly);
	       drawdot(xmat(pvalx), ymat(pvaly));
	    }
	    break;

	case 'b': {	/* boxes */
	    polyptr *newpoly;
	    pointlist newpoints;

	    sscanf(++lineptr, "%d", &dval);
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in box section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');

	       NEW_POLY(newpoly, topobject);

	       (*newpoly)->style = DASHED;
	       (*newpoly)->color = DEFAULTCOLOR;
	       (*newpoly)->width = 1.0;
	       (*newpoly)->number = 4;
               (*newpoly)->points = (pointlist) malloc(4 * sizeof(XPoint));
	       (*newpoly)->passed = NULL;

               newpoints = (*newpoly)->points;
	       sscanf(lineptr, "%hd %hd %hd %hd", &pvalx, &pvaly, &pvalx2, &pvaly2);
	       newpoints->x = xmat(pvalx);
	       newpoints->y = ymat(pvaly);
	       (newpoints + 1)->x = xmat(pvalx2);
	       (newpoints + 2)->y = ymat(pvaly2);

	       (newpoints + 2)->x = (newpoints + 1)->x;
	       (newpoints + 3)->x = newpoints->x;
	       (newpoints + 1)->y = newpoints->y;
	       (newpoints + 3)->y = (newpoints + 2)->y;
	    }}
	    break;

	case 'g': {	/* gates */

	    objinstptr *newinst;
	    labelptr *newlabel;
	    int j, k, hval, flip;

	    sscanf(++lineptr, "%d", &dval);
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in gates section");
		     return;
	          }
		  for (lineptr = temp; *lineptr != '\n'; lineptr++); *lineptr = '\0';
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\0'; lineptr++);
	       } while (*lineptr == '\0');

	       /* double loop through user libraries */

	       for (j = 0; j < xobjs.numlibs; j++) {
		  for (k = 0; k < xobjs.userlibs[j].number; k++) {
		     libobj = xobjs.userlibs[j].library + k;
	             if (!strcmp(lineptr, (*libobj)->name)) break;
		  }
	          if (k < xobjs.userlibs[j].number) break;
	       }
	       strcpy(tmpstring, lineptr);

	       /* read gate definition */

	       if (fgets(temp, 499, ps) == NULL) {
		  Wprintf("End of file during gate read");
		  return;
	       }
      	       for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);

	       if (j < xobjs.numlibs || k < xobjs.userlibs[xobjs.numlibs - 1].number) {

		  NEW_OBJINST(newinst, topobject);

	          sscanf(lineptr, "%hd %hd %hd %*d %*d %*d %d", &pvalx, &pvaly,
			&pvalx2, &hval); 

		  flip = (pvalx2 >= 4) ? 1 : 0;
		  if (!strcmp(tmpstring, "FROM")) flip = 1 - flip;

		  (*newinst)->position.x = xmat(pvalx);
		  (*newinst)->position.y = ymat(pvaly);
		  (*newinst)->scale = 1.0;
		  (*newinst)->color = DEFAULTCOLOR;
		  (*newinst)->params = NULL;
	          (*newinst)->passed = NULL;

		  if (pvalx2 & 0x01) pvalx2 ^= 0x02;
                  if (pvalx2 >= 4)
		     (*newinst)->rotation = (float)(-((pvalx2 - 4) * 90) + 1);
                  else
		     (*newinst)->rotation = (float)((pvalx2 * 90) + 1);
		  (*newinst)->thisobject = *libobj;
		  (*newinst)->bbox.lowerleft.x = (*libobj)->bbox.lowerleft.x;
		  (*newinst)->bbox.lowerleft.y = (*libobj)->bbox.lowerleft.y;
		  (*newinst)->bbox.width = (*libobj)->bbox.width;
		  (*newinst)->bbox.height = (*libobj)->bbox.height;

	          /* Add label to "TO" and "FROM" */

	          if (!strcmp(tmpstring, "FROM") || !strcmp(tmpstring, "TO")) {
	   	     int nval;

		     hval--;
		     fgets(temp, 499, ps);
		     sscanf(temp, "%d", &nval);

		     for (k = 0; k < sigs; k++)
		        if (signets[k] == nval) {
			   stringpart *strptr;

			   NEW_LABEL(newlabel, topobject);
			   /* reconnect newinst if displaced by realloc() */
			   newinst = (objinstptr *)(topobject->plist
				+ topobject->parts - 2);

			   labeldefaults(*newlabel, False, (*newinst)->position.x,
				(*newinst)->position.y);
			   (*newlabel)->color = DEFAULTCOLOR;
			   (*newlabel)->pin = LOCAL;
			   (*newlabel)->color = LOCALPINCOLOR;
			   if (!strcmp(tmpstring, "TO"))
			      (*newlabel)->position.x += ((flip) ? 48 : -48);
			   else
			      (*newlabel)->position.x += ((flip) ? 54 : -54);

			   (*newlabel)->anchor = NOTBOTTOM;
			   if (flip) (*newlabel)->anchor |= (RIGHT | NOTLEFT);
			   (*newlabel)->string->data.font = 0;
			   strptr = makesegment(&((*newlabel)->string), NULL);
			   strptr->type = TEXT_STRING;
			   strptr->data.string = (char *)malloc(1 + strlen(signals[k]));
			   strcpy(strptr->data.string, signals[k]);
			   break;
		        }
	          }
	       }

	       /* read through list of attributes */

	       else {
	          sscanf(lineptr, "%*d %*d %*d %*d %*d %*d %d", &hval);
	          Wprintf("No library object %s", tmpstring);
	       }

	       for (j = 0; j < hval + 1; j++) {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("Unexpected end of file");
		     return;
	          }
	       }
	       /* read to next blank line */
	       do {
		  if (fgets(temp, 499, ps) == NULL) break;
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr != '\n');
	    }}
	    break;

	case 'h': {	/* history */
	    int j, hval;

	    sscanf(++lineptr, "%d", &dval);
	    for (i = 0; i < dval; i++) {
	       do {
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("End of file in history section");
		     return;
	          }
      	          for (lineptr = temp; isspace(*lineptr) && *lineptr != '\n'; lineptr++);
	       } while (*lineptr == '\n');

	       /* read through history */

	       sscanf(lineptr, "%*d %d", &hval);
	       for (j = 0; j < hval; j++)
	          if (fgets(temp, 499, ps) == NULL) {
		     Wprintf("Unexpected end of file");
		     return;
	          }
	    }}
	    break;

	case '.':	/* blank, don't use for EOF */
	    break;

	default:
	    Wprintf("Don't understand statement '%c'", *lineptr);
	    break;
      }
   }

   /* check for unattached labels and delete them */

   for (iolabel = topobject->plist; iolabel < topobject->plist +
	   topobject->parts; iolabel++)
      if (IS_LABEL(*iolabel)) {
         if (TOLABEL(iolabel)->rotation == 500.0) {
	    genericptr *tmplabel;

	    free(TOLABEL(iolabel)->string);
	    free(*iolabel);
            for (tmplabel = iolabel + 1; tmplabel < topobject->plist +
                topobject->parts; tmplabel++) *(tmplabel - 1) = *tmplabel;
            topobject->parts--;
	    iolabel--;
	 }
      }

   calcbbox(areawin->topinstance);
   centerview(areawin->topinstance);

   for (i = 0; i < sigs; i++) free(signals[i]);
   free(signals);
   free(signets);
}

#endif
/* (LGF) */

/*-------------------------*/
#ifdef MATLAB_4X

/*------------------------*/
/* Load a Matlab .ps file */
/*------------------------*/
/*------------------------------------------------------------------*/
/* This is unfinished. . . needs a lot of thought and a lot of work */
/*------------------------------------------------------------------*/

void loadmat4(caddr_t nullval)
{
   char inname[150], *temp, *buffer, keyword[30], percentc, *pdchar;
   int bufsize = 256;
   short curcolor = DEFAULTCOLOR;
   char colorstr[100][5];
   short matcolors = 0;
   float curwidth = 1.0;
   int tmpstyle = UNCLOSED;

   sscanf(_STR, "%149s", inname);

   ps = fopen(inname, "r");
   if (ps == NULL) {
      sprintf(inname, "%s.ps", _STR);
      ps = fopen(inname, "r");
      if (ps == NULL) {
	 sprintf(inname, "%s.eps", _STR);
	 ps = fopen(inname, "r");
	 if (ps == NULL) {
	    Wprintf("Can't open Matlab PostScript file %s", inname);
	    return;
         }
      }
   }

   /* Keep same filename---overwriting file is end-user's own risk */

   if ((pdchar = strstr(_STR, ".ps")) != NULL) *pdchar = '\0';
   sprintf(topobject->name, "%s", _STR);
   Wprintf("Loaded file: %s", inname);
   renamepage(areawin->page);
   printname(topobject);

   /* Create input string buffer */

   buffer = (char *)malloc(bufsize * sizeof(char));
   temp = buffer;

   /* Read header information */

   if (fgets(temp, 149, ps) == NULL) {
      Wprintf("Error: end of file.");
      return;
   }
   if (*temp != '%' || *(temp + 1) != '!') {
      Wprintf("Not a PostScript file?");
      return;
   }
   if (fgets(temp, 149, ps) == NULL) {
      Wprintf("Error: end of file.");
      return;
   }
   if (!strstr(temp, "MATLAB")) {
      Wprintf("Not a Matlab PostScript file?");
      return;
   }

   /* Read through to Page start */

   do {
      if (fgets(temp, 149, ps) == NULL) {
         Wprintf("Error: no pages in input.");
         return;
      }
      if (strstr(temp, "%%Page:") != NULL) break;
   } while (1);

   /* Read objects */

   do {
      char *lineptr, keyptr;

      if (fgets(temp, 255, ps) == NULL) break;		/* End-Of-File */
      temp = buffer;

      /* scan from the end;  ignore blank lines. */

      for (lineptr = buffer; (*lineptr != '\n') && (*lineptr != '\0'); lineptr++);

      /* ignore any blank lines and PostScript comment lines */

      if (lineptr != buffer && *buffer != '%') {
         for (keyptr = lineptr - 1; isspace(*keyptr) && keyptr != buffer; keyptr--);
         for (; !isspace(*keyptr) && keyptr != buffer; keyptr--);
         sscanf(keyptr, "%29s", keyword);

	 if (!strcmp(keyword, "showpage")) {
            free(buffer);
            return False;  /* end of page */
         }

	 else if (!strcmp(keyword, "bdef")) {   /* new color definition */
	    char *bb;
	    float red, green, blue;
	    if ((bb = strchr(buffer, '{')) != NULL) {
	       sscanf(bb + 1, "%f %f %f", &red, &green, &blue);
	       curcolor = rgb_alloccolor((int)(red * 65535), (int)(green * 65535),
			(int)(blue * 65535));
	    }
	    if ((bb = strchr(buffer, '/')) != NULL) {
	       sscanf(bb, "%4s", &colorstr[matcolors]);
	       matcolors++;
	    }
	 } 

	 else if (!strcmp(keyword, "w")) {	/* linewidth */
	    float tmpwidth;
	    sscanf(buffer, "%f", &tmpwidth)
	 }
	 else if (!strcmp(keyword, "DO")) {	/* style */
	    tmpstyle = DOTTED | UNCLOSED;
	 }
	 else if (!strcmp(keyword, "SO")) {	/* style */
	    tmpstyle = UNCLOSED;
	 }
	 else if (!strcmp(keyword, "DA")) {	/* style */
	    tmpstyle = DASHED | UNCLOSED;
	 }
	 else if (!strcmp(keyword, "FMSR")) ;   /* ignore font spec for now */
	 else if (!strcmp(keyword, "j")) ;   /* ignore line join */
	 else if (!strcmp(keyword, "def")) ;   /* ignore */
	 else if (!strcmp(keyword, "dictionary")) ;   /* ignore */
	 else if (!strcmp(keyword, "np")) ;   /* ignore clip paths */
	 else {  /* continuation line ? */
	    for (lineptr = buffer; (*lineptr != '\n') && (*lineptr != '\0');
			lineptr++);
	    if (*lineptr == '\n') *lineptr = ' ';

	    bufsize = (int)(lineptr - buffer) + 256;
	    buffer = (char *)realloc(buffer, bufsize * sizeof(char));
	    temp = buffer + (bufsize - 256);
         }
      }
   } while (1);

   free(buffer);
}

#endif
/* (MATLAB_4X) */
/*---------------------------------------------------------------------------*/

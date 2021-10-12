/*--------------------------------------------------------------*/
/* tclxcircuit.c:						*/
/*	Tcl routines for xcircuit command-line functions	*/
/* Copyright (c) 2003  Tim Edwards, Johns Hopkins University    */
/* Copyright (c) 2004  Tim Edwards, MultiGiG, Inc.		*/
/*--------------------------------------------------------------*/

#if defined(TCL_WRAPPER) && !defined(HAVE_PYTHON)

#include <stdio.h>
#include <stdarg.h>	/* for va_copy() */
#include <stdlib.h>	/* for atoi() and others */
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <tk.h>

#ifdef HAVE_CAIRO
#include <cairo/cairo-xlib.h>
#endif

#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#include "xcircuit.h"
#include "colordefs.h"
#include "menudep.h"
#include "prototypes.h"

Tcl_HashTable XcTagTable;

extern Tcl_Interp *xcinterp;
extern Tcl_Interp *consoleinterp;
extern Display *dpy;
extern Colormap cmap;
extern Pixmap   STIPPLE[STIPPLES];  /* Polygon fill-style stipple patterns */
extern char _STR[150], _STR2[250];
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern int screenDPI;
extern int number_colors;
extern colorindex *colorlist;
extern Cursor appcursors[NUM_CURSORS];
extern ApplicationData appdata;
extern fontinfo *fonts;
extern short fontcount;
extern u_char param_select[];
extern keybinding *keylist;
extern Boolean spice_end;
extern short flstart;
extern int pressmode;
extern u_char undo_collect;

char STIPDATA[STIPPLES][4] = {
   "\000\004\000\001",
   "\000\005\000\012",
   "\001\012\005\010",
   "\005\012\005\012",
   "\016\005\012\007",
   "\017\012\017\005",
   "\017\012\017\016",
   "\000\000\000\000"
};

short flags = -1;

#define LIBOVERRIDE     1
#define LIBLOADED       2
#define COLOROVERRIDE   4
#define FONTOVERRIDE    8
#define KEYOVERRIDE     16

/*-----------------------*/
/* Tcl 8.4 compatibility */
/*-----------------------*/

#ifndef CONST84
#define CONST84
#endif

/*----------------------------------------------------------------------*/
/* Procedure for waiting on X to map a window				*/
/* This code copied from Tk sources, where it is used for the "tkwait"	*/
/* command.								*/
/*----------------------------------------------------------------------*/

static void
WaitVisibilityProc(ClientData clientData, XEvent *eventPtr)
{
    int *donePtr = (int *) clientData;

    if (eventPtr->type == VisibilityNotify) {
        *donePtr = 1;
    }
    if (eventPtr->type == DestroyNotify) {
        *donePtr = 2;
    }
}

/*----------------------------------------------------------------------*/
/* Deal with systems which don't define va_copy().			*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_VA_COPY
  #ifdef HAVE___VA_COPY
    #define va_copy(a, b) __va_copy(a, b)
  #else
    #define va_copy(a, b) a = b
  #endif
#endif

#ifdef ASG
   extern int SetDebugLevel(int *level);
#endif

/*----------------------------------------------------------------------*/
/* Reimplement strdup() to use Tcl_Alloc().				*/
/* Note that "strdup" is defined as "Tcl_Strdup" in xcircuit.h.		*/
/*----------------------------------------------------------------------*/

char *Tcl_Strdup(const char *s)
{
   char *snew;
   int slen;

   slen = 1 + strlen(s);
   snew = Tcl_Alloc(slen);
   if (snew != NULL)
      memcpy(snew, s, slen);

   return snew;
}

/*----------------------------------------------------------------------*/
/* Reimplement vfprintf() as a call to Tcl_Eval().			*/
/*----------------------------------------------------------------------*/

void tcl_vprintf(FILE *f, const char *fmt, va_list args_in)
{
   va_list args;
   static char outstr[128] = "puts -nonewline std";
   char *outptr, *bigstr = NULL, *finalstr = NULL;
   int i, nchars, result, escapes = 0;

   /* If we are printing an error message, we want to bring attention	*/
   /* to it by mapping the console window and raising it, as necessary.	*/
   /* I'd rather do this internally than by Tcl_Eval(), but I can't	*/
   /* find the right window ID to map!					*/

   if ((f == stderr) && (consoleinterp != xcinterp)) {
      Tk_Window tkwind;
      tkwind = Tk_MainWindow(consoleinterp);
      if ((tkwind != NULL) && (!Tk_IsMapped(tkwind)))
	 result = Tcl_Eval(consoleinterp, "wm deiconify .\n");
      result = Tcl_Eval(consoleinterp, "raise .\n");
   }

   strcpy (outstr + 19, (f == stderr) ? "err \"" : "out \"");
   outptr = outstr;

   /* This mess circumvents problems with systems which do not have	*/
   /* va_copy() defined.  Some define __va_copy();  otherwise we must	*/
   /* assume that args = args_in is valid.				*/

   va_copy(args, args_in);
   nchars = vsnprintf(outptr + 24, 102, fmt, args);
   va_end(args);

   if (nchars >= 102) {
      va_copy(args, args_in);
      bigstr = Tcl_Alloc(nchars + 26);
      strncpy(bigstr, outptr, 24);
      outptr = bigstr;
      vsnprintf(outptr + 24, nchars + 2, fmt, args);
      va_end(args);
    }
    else if (nchars == -1) nchars = 126;

    for (i = 24; *(outptr + i) != '\0'; i++) {
       if (*(outptr + i) == '\"' || *(outptr + i) == '[' ||
	  	*(outptr + i) == ']' || *(outptr + i) == '\\')
	  escapes++;
    }

    if (escapes > 0) {
      finalstr = Tcl_Alloc(nchars + escapes + 26);
      strncpy(finalstr, outptr, 24);
      escapes = 0;
      for (i = 24; *(outptr + i) != '\0'; i++) {
	  if (*(outptr + i) == '\"' || *(outptr + i) == '[' ||
	    		*(outptr + i) == ']' || *(outptr + i) == '\\') {
	     *(finalstr + i + escapes) = '\\';
	     escapes++;
	  }
	  *(finalstr + i + escapes) = *(outptr + i);
      }
      outptr = finalstr;
    }

    *(outptr + 24 + nchars + escapes) = '\"';
    *(outptr + 25 + nchars + escapes) = '\0';

    result = Tcl_Eval(consoleinterp, outptr);

    if (bigstr != NULL) Tcl_Free(bigstr);
    if (finalstr != NULL) Tcl_Free(finalstr);
}

/*------------------------------------------------------*/
/* Console output flushing which goes along with the	*/
/* routine tcl_vprintf() above.				*/
/*------------------------------------------------------*/

void tcl_stdflush(FILE *f)
{
   Tcl_SavedResult state;
   static char stdstr[] = "::flush stdxxx";
   char *stdptr = stdstr + 11;

   if ((f != stderr) && (f != stdout)) {
      fflush(f);
   }
   else {
      Tcl_SaveResult(xcinterp, &state);
      strcpy(stdptr, (f == stderr) ? "err" : "out");
      Tcl_Eval(xcinterp, stdstr);
      Tcl_RestoreResult(xcinterp, &state);
   }
}

/*----------------------------------------------------------------------*/
/* Reimplement fprintf() as a call to Tcl_Eval().			*/
/* Make sure that files (not stdout or stderr) get treated normally.	*/
/*----------------------------------------------------------------------*/

void tcl_printf(FILE *f, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  if ((f != stderr) && (f != stdout))
     vfprintf(f, format, ap);
  else
     tcl_vprintf(f, format, ap);
  va_end(ap);
}

/*----------------------------------------------------------------------*/
/* Fill in standard areas of a key event structure.  This includes	*/
/* everything necessary except type, keycode, and state (although	*/
/* state defaults to zero).  This is also good for button events, which	*/
/* share the same structure as key events (except that keycode is	*/
/* changed to button).							*/
/*----------------------------------------------------------------------*/

void make_new_event(XKeyEvent *event)
{
   XPoint newpos, wpoint;

   newpos = UGetCursorPos();
   user_to_window(newpos, &wpoint);
   event->x = wpoint.x;
   event->y = wpoint.y;

   event->same_screen = TRUE;
   event->send_event = TRUE;
   event->display = dpy;
   event->window = Tk_WindowId(areawin->area);

   event->state = 0;
}

/*----------------------------------------------------------------------*/
/* Implement tag callbacks on functions					*/
/* Find any tags associated with a command and execute them.		*/
/*----------------------------------------------------------------------*/

int XcTagCallback(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int objidx, result = TCL_OK;
    char *postcmd, *substcmd, *newcmd, *sptr, *sres;
    char *croot = Tcl_GetString(objv[0]);
    Tcl_HashEntry *entry;
    Tcl_SavedResult state;
    int reset = FALSE;
    int i, llen;

    /* Skip over technology qualifier, if any */

    if (!strncmp(croot, "::", 2)) croot += 2;
    if (!strncmp(croot, "xcircuit::", 10)) croot += 10;

    entry = Tcl_FindHashEntry(&XcTagTable, croot);
    postcmd = (entry) ? (char *)Tcl_GetHashValue(entry) : NULL;

    if (postcmd)
    {
	substcmd = (char *)Tcl_Alloc(strlen(postcmd) + 1);
	strcpy(substcmd, postcmd);
	sptr = substcmd;

	/*--------------------------------------------------------------*/
	/* Parse "postcmd" for Tk-substitution escapes			*/
	/* Allowed escapes are:						*/
	/* 	%W	substitute the tk path of the calling window	*/
	/*	%r	substitute the previous Tcl result string	*/
	/*	%R	substitute the previous Tcl result string and	*/
	/*		reset the Tcl result.				*/
	/*	%[0-5]  substitute the argument to the original command	*/
	/*	%N	substitute all arguments as a list		*/
	/*	%%	substitute a single percent character		*/
	/* 	%#	substitute the number of arguments passed	*/
	/*	%*	(all others) no action: print as-is.		*/
	/*--------------------------------------------------------------*/

	while ((sptr = strchr(sptr, '%')) != NULL)
	{
	    switch (*(sptr + 1))
	    {
		case 'W': {
		    char *tkpath = NULL;
		    Tk_Window tkwind = Tk_MainWindow(interp);
		    if (tkwind != NULL) tkpath = Tk_PathName(tkwind);
		    if (tkpath == NULL)
			newcmd = (char *)Tcl_Alloc(strlen(substcmd));
		    else
			newcmd = (char *)Tcl_Alloc(strlen(substcmd) + strlen(tkpath));

		    strcpy(newcmd, substcmd);

		    if (tkpath == NULL)
			strcpy(newcmd + (int)(sptr - substcmd), sptr + 2);
		    else
		    {
			strcpy(newcmd + (int)(sptr - substcmd), tkpath);
			strcat(newcmd, sptr + 2);
		    }
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    } break;

		case 'R':
		    reset = TRUE;
		case 'r':
		    sres = (char *)Tcl_GetStringResult(interp);
		    newcmd = (char *)Tcl_Alloc(strlen(substcmd)
				+ strlen(sres) + 1);
		    strcpy(newcmd, substcmd);
		    sprintf(newcmd + (int)(sptr - substcmd), "\"%s\"", sres);
		    strcat(newcmd, sptr + 2);
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    break;

		case '#':
		    if (objc < 100) {
		       newcmd = (char *)Tcl_Alloc(strlen(substcmd) + 3);
		       strcpy(newcmd, substcmd);
		       sprintf(newcmd + (int)(sptr - substcmd), "%d", objc);
		       strcat(newcmd, sptr + 2);
		       Tcl_Free(substcmd);
		       substcmd = newcmd;
		       sptr = substcmd;
		    }
		    break;

		case '0': case '1': case '2': case '3': case '4': case '5':
		    objidx = (int)(*(sptr + 1) - '0');
		    if ((objidx >= 0) && (objidx < objc))
		    {
		        newcmd = (char *)Tcl_Alloc(strlen(substcmd)
				+ strlen(Tcl_GetString(objv[objidx])) + 1);
		        strcpy(newcmd, substcmd);
			strcpy(newcmd + (int)(sptr - substcmd),
				Tcl_GetString(objv[objidx]));
			strcat(newcmd, sptr + 2);
			Tcl_Free(substcmd);
			substcmd = newcmd;
			sptr = substcmd;
		    }
		    else if (objidx >= objc)
		    {
		        newcmd = (char *)Tcl_Alloc(strlen(substcmd) + 1);
		        strcpy(newcmd, substcmd);
			strcpy(newcmd + (int)(sptr - substcmd), sptr + 2);
			Tcl_Free(substcmd);
			substcmd = newcmd;
			sptr = substcmd;
		    }
		    else sptr++;
		    break;

		case 'N':
		    llen = 1;
		    for (i = 1; i < objc; i++)
		       llen += (1 + strlen(Tcl_GetString(objv[i])));
		    newcmd = (char *)Tcl_Alloc(strlen(substcmd) + llen);
		    strcpy(newcmd, substcmd);
		    strcpy(newcmd + (int)(sptr - substcmd), "{");
		    for (i = 1; i < objc; i++) {
		       strcat(newcmd, Tcl_GetString(objv[i]));
		       if (i < (objc - 1))
			  strcat(newcmd, " ");
		    }
		    strcat(newcmd, "}");
		    strcat(newcmd, sptr + 2);
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    break;

		case '%':
		    newcmd = (char *)Tcl_Alloc(strlen(substcmd) + 1);
		    strcpy(newcmd, substcmd);
		    strcpy(newcmd + (int)(sptr - substcmd), sptr + 1);
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    break;

		default:
		    sptr++;
		    break;
	    }
	}

	/* Fprintf(stderr, "Substituted tag callback is \"%s\"\n", substcmd); */
	/* Flush(stderr); */

	Tcl_SaveResult(interp, &state);
	result = Tcl_Eval(interp, substcmd);
	if ((result == TCL_OK) && (reset == FALSE))
	    Tcl_RestoreResult(interp, &state);
	else
	    Tcl_DiscardResult(&state);

	Tcl_Free(substcmd);
    }
    return result;
}

/*--------------------------------------------------------------*/
/* XcInternalTagCall ---						*/
/*								*/
/* Execute the tag callback for a command without actually	*/
/* evaluating the command itself.  The command and arguments	*/
/* are passed as a variable number or char * arguments, since	*/
/* usually this routine will called with constant arguments	*/
/* (e.g., XcInternalTagCall(interp, 2, "set", "color");)		*/
/*								*/
/* objv declared static because this routine is used a lot	*/
/* (e.g., during select/unselect operations).			*/
/*--------------------------------------------------------------*/

int XcInternalTagCall(Tcl_Interp *interp, int argc, ...)
{
   int i;
   static Tcl_Obj **objv = NULL;
   char *aptr;
   va_list ap;


   if (objv == (Tcl_Obj **)NULL)
      objv = (Tcl_Obj **)malloc(argc * sizeof(Tcl_Obj *));
   else
      objv = (Tcl_Obj **)realloc(objv, argc * sizeof(Tcl_Obj *));

   va_start(ap, argc);
   for (i = 0; i < argc; i++) {
      aptr = va_arg(ap, char *);
      /* We are depending on Tcl's heap allocation of objects	*/
      /* so that we do not have to manage memory for these	*/
      /* string representations. . .				*/

      objv[i] = Tcl_NewStringObj(aptr, -1);
   }
   va_end(ap);

   return XcTagCallback(interp, argc, objv);
}

/*--------------------------------------------------------------*/
/* Return the event mode					*/
/* Event mode can be set in specific cases.			*/
/*--------------------------------------------------------------*/

int xctcl_eventmode(ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
   static char *modeNames[] = {
	"normal", "undo", "move", "copy", "pan",
	"selarea", "rescale", "catalog", "cattext",
	"fontcat", "efontcat", "text", "wire", "box",
	"arc", "spline", "etext", "epoly", "earc",
	"espline", "epath", "einst", "assoc", "catmove",
	NULL
   };

   /* This routine is diagnostic only */

   if (objc != 1) return TCL_ERROR;

   Tcl_SetResult(interp, modeNames[eventmode], NULL);
   return TCL_OK;
}

/*--------------------------------------------------------------*/
/* Add a command tag callback					*/
/*--------------------------------------------------------------*/

int xctcl_tag(ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    Tcl_HashEntry *entry;
    char *hstring;
    int new;

    if (objc != 2 && objc != 3)
	return TCL_ERROR;

    entry = Tcl_CreateHashEntry(&XcTagTable, Tcl_GetString(objv[1]), &new);
    if (entry == NULL) return TCL_ERROR;

    hstring = (char *)Tcl_GetHashValue(entry);
    if (objc == 2)
    {
	Tcl_SetResult(interp, hstring, NULL);
	return TCL_OK;
    }

    if (strlen(Tcl_GetString(objv[2])) == 0)
    {
	Tcl_DeleteHashEntry(entry);
    }
    else
    {
	hstring = strdup(Tcl_GetString(objv[2]));
	Tcl_SetHashValue(entry, hstring);
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* Turn a selection list into a Tcl List object (may be empty list)	*/
/*----------------------------------------------------------------------*/

Tcl_Obj *SelectToTclList(Tcl_Interp *interp, short *slist, int snum)
{
   int i;
   Tcl_Obj *objPtr, *listPtr;

   if (snum == 1) {
      objPtr = Tcl_NewHandleObj(SELTOGENERIC(slist));
      return objPtr;
   }

   listPtr = Tcl_NewListObj(0, NULL);
   for (i = 0; i < snum; i++) {
      objPtr = Tcl_NewHandleObj(SELTOGENERIC(slist + i));
      Tcl_ListObjAppendElement(interp, listPtr, objPtr);
   }
   return listPtr;
}

/*----------------------------------------------------------------------*/
/* Get an x,y position (as an XPoint structure) from a list of size 2	*/
/*----------------------------------------------------------------------*/

int GetPositionFromList(Tcl_Interp *interp, Tcl_Obj *list, XPoint *rpoint)
{
   int result, numobjs;
   Tcl_Obj *lobj, *tobj;
   int pos;

   if (!strcmp(Tcl_GetString(list), "here")) {
      if (rpoint) *rpoint = UGetCursorPos();
      return TCL_OK;
   }
   result = Tcl_ListObjLength(interp, list, &numobjs);
   if (result != TCL_OK) return result;

   if (numobjs == 1) {
      /* Try decomposing the object into a list */
      result = Tcl_ListObjIndex(interp, list, 0, &tobj);
      if (result == TCL_OK) {
         result = Tcl_ListObjLength(interp, tobj, &numobjs);
	 if (numobjs == 2)
	    list = tobj;
      }
      if (result != TCL_OK) Tcl_ResetResult(interp);
   }
   if (numobjs != 2) {
      Tcl_SetResult(interp, "list must contain x y positions", NULL);
      return TCL_ERROR;
   }
   result = Tcl_ListObjIndex(interp, list, 0, &lobj);
   if (result != TCL_OK) return result;
   result = Tcl_GetIntFromObj(interp, lobj, &pos);
   if (result != TCL_OK) return result;
   if (rpoint) rpoint->x = pos;

   result = Tcl_ListObjIndex(interp, list, 1, &lobj);
   if (result != TCL_OK) return result;
   result = Tcl_GetIntFromObj(interp, lobj, &pos);
   if (result != TCL_OK) return result;
   if (rpoint) rpoint->y = pos;

   return TCL_OK;
}

/*--------------------------------------------------------------*/
/* Convert color index to a list of 3 elements			*/
/* We assume that this color exists in the color table.		*/
/*--------------------------------------------------------------*/

Tcl_Obj *TclIndexToRGB(int cidx)
{
   Tcl_Obj *RGBTuple;

   if (cidx < 0) {	/* Handle "default color" */
      return Tcl_NewStringObj("Default", 7);
   }
   else if (cidx >= number_colors) {
      Tcl_SetResult(xcinterp, "Bad color index", NULL);
      return NULL;
   }

   RGBTuple = Tcl_NewListObj(0, NULL);
   Tcl_ListObjAppendElement(xcinterp, RGBTuple,
	Tcl_NewIntObj((int)(colorlist[cidx].color.red / 256)));
   Tcl_ListObjAppendElement(xcinterp, RGBTuple,
	Tcl_NewIntObj((int)(colorlist[cidx].color.green / 256)));
   Tcl_ListObjAppendElement(xcinterp, RGBTuple,
	Tcl_NewIntObj((int)(colorlist[cidx].color.blue / 256)));
   return RGBTuple;
}


/*--------------------------------------------------------------*/
/* Convert a stringpart* to a Tcl list object 			*/
/*--------------------------------------------------------------*/

Tcl_Obj *TclGetStringParts(stringpart *thisstring)
{
   Tcl_Obj *lstr, *sdict, *stup;
   int i;
   stringpart *strptr;

   lstr = Tcl_NewListObj(0, NULL);
   for (strptr = thisstring, i = 0; strptr != NULL;
      strptr = strptr->nextpart, i++) {
      switch(strptr->type) {
	 case TEXT_STRING:
	    sdict = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(xcinterp, sdict, Tcl_NewStringObj("Text", 4));
	    Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewStringObj(strptr->data.string,
			strlen(strptr->data.string)));
	    Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    break;
	 case PARAM_START:
	    sdict = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(xcinterp, sdict, Tcl_NewStringObj("Parameter", 9));
	    Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewStringObj(strptr->data.string,
			strlen(strptr->data.string)));
	    Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    break;
	 case PARAM_END:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("End Parameter", 13));
	    break;
	 case FONT_NAME:
	    sdict = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(xcinterp, sdict, Tcl_NewStringObj("Font", 4));
	    Tcl_ListObjAppendElement(xcinterp, sdict,
		  Tcl_NewStringObj(fonts[strptr->data.font].psname,
		  strlen(fonts[strptr->data.font].psname)));
	    Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    break;
	 case FONT_SCALE:
	    sdict = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewStringObj("Font Scale", 10));
	    Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewDoubleObj((double)strptr->data.scale));
	    Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    break;
	 case KERN:
	    sdict = Tcl_NewListObj(0, NULL);
	    stup = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(xcinterp, stup,
			Tcl_NewIntObj((int)strptr->data.kern[0]));
	    Tcl_ListObjAppendElement(xcinterp, stup,
			Tcl_NewIntObj((int)strptr->data.kern[1]));

	    Tcl_ListObjAppendElement(xcinterp, sdict, Tcl_NewStringObj("Kern", 4));
	    Tcl_ListObjAppendElement(xcinterp, sdict, stup);
	    Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    break;
	 case FONT_COLOR:
	    stup = TclIndexToRGB(strptr->data.color);
	    if (stup != NULL) {
	       sdict = Tcl_NewListObj(0, NULL);
	       Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewStringObj("Color", 5));
	       Tcl_ListObjAppendElement(xcinterp, sdict, stup);
	       Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    }
	    break;
	 case MARGINSTOP:
	    sdict = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewStringObj("Margin Stop", 11));
	    Tcl_ListObjAppendElement(xcinterp, sdict,
			Tcl_NewIntObj((int)strptr->data.width));
	    Tcl_ListObjAppendElement(xcinterp, lstr, sdict);
	    break;
	 case TABSTOP:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Tab Stop", 8));
	    break;
	 case TABFORWARD:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Tab Forward", 11));
	    break;
	 case TABBACKWARD:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Tab Backward", 12));
	    break;
	 case RETURN:
	    // Don't show automatically interted line breaks
	    if (strptr->data.flags == 0)
	       Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Return", 6));
	    break;
	 case SUBSCRIPT:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Subscript", 9));
	    break;
	 case SUPERSCRIPT:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Superscript", 11));
	    break;
	 case NORMALSCRIPT:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Normalscript", 12));
	    break;
	 case UNDERLINE:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Underline", 9));
	    break;
	 case OVERLINE:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Overline", 8));
	    break;
	 case NOLINE:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("No Line", 7));
	    break;
	 case HALFSPACE:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Half Space", 10));
	    break;
	 case QTRSPACE:
	    Tcl_ListObjAppendElement(xcinterp, lstr,
			Tcl_NewStringObj("Quarter Space", 13));
	    break;
      }
   }
   return lstr;
}

/*----------------------------------------------------------------------*/
/* Get a stringpart linked list from a Tcl list				*/
/*----------------------------------------------------------------------*/

int GetXCStringFromList(Tcl_Interp *interp, Tcl_Obj *list, stringpart **rstring)
{
   int result, j, k, numobjs, idx, numparts, ptype, ival;
   Tcl_Obj *lobj, *pobj, *tobj, *t2obj;
   stringpart *newpart;
   char *fname;
   double fscale;

   static char *partTypes[] = {"Text", "Subscript", "Superscript",
	"Normalscript", "Underline", "Overline", "No Line", "Tab Stop",
	"Tab Forward", "Tab Backward", "Half Space", "Quarter Space",
	"Return", "Font", "Font Scale", "Color", "Margin Stop", "Kern",
	"Parameter", "End Parameter", "Special", NULL};

   static int partTypesIdx[] = {TEXT_STRING, SUBSCRIPT, SUPERSCRIPT,
	NORMALSCRIPT, UNDERLINE, OVERLINE, NOLINE, TABSTOP, TABFORWARD,
	TABBACKWARD, HALFSPACE, QTRSPACE, RETURN, FONT_NAME, FONT_SCALE,
	FONT_COLOR, MARGINSTOP, KERN, PARAM_START, PARAM_END, SPECIAL};

   /* No place to put result! */
   if (rstring == NULL) return TCL_ERROR;

   result = Tcl_ListObjLength(interp, list, &numobjs);
   if (result != TCL_OK) return result;

   newpart = NULL;
   for (j = 0; j < numobjs; j++) {
      result = Tcl_ListObjIndex(interp, list, j, &lobj);
      if (result != TCL_OK) return result;

      result = Tcl_ListObjLength(interp, lobj, &numparts);
      if (result != TCL_OK) return result;

      result = Tcl_ListObjIndex(interp, lobj, 0, &pobj);
      if (result != TCL_OK) return result;

      /* Must define TCL_EXACT in flags, or else, for instance, "u" gets */
      /* interpreted as "underline", which is usually not intended.	 */

      if (pobj == NULL)
	 return TCL_ERROR;
      else if (Tcl_GetIndexFromObj(interp, pobj, (CONST84 char **)partTypes,
		"string part types", TCL_EXACT, &idx) != TCL_OK) {
	 Tcl_ResetResult(interp);
	 idx = -1;

	 // If there's only one object and the first item doesn't match
	 // a stringpart itentifying word, then assume that "list" is a
	 // single text string.

	 if (numobjs == 1)
	    tobj = list;
	 else
	    result = Tcl_ListObjIndex(interp, lobj, 0, &tobj);
      }
      else {
	 result = Tcl_ListObjIndex(interp, lobj, (numparts > 1) ? 1 : 0, &tobj);
      }
      if (result != TCL_OK) return result;

      if (idx < 0) {
	 if ((newpart == NULL) || (newpart->type != TEXT_STRING))
	    idx = 0;
	 else {
	    /* We have an implicit text string which should be appended */
	    /* to the previous text string with a space character.	*/
	    newpart->data.string = (char *)realloc(newpart->data.string,
		strlen(newpart->data.string) + strlen(Tcl_GetString(tobj))
		+ 2);
	    strcat(newpart->data.string, " ");
	    strcat(newpart->data.string, Tcl_GetString(tobj));
	    continue;
         }
      }
      ptype = partTypesIdx[idx];

      newpart = makesegment(rstring, NULL);
      newpart->nextpart = NULL;
      newpart->type = ptype;

      switch(ptype) {
	 case TEXT_STRING:
	 case PARAM_START:
	    newpart->data.string = strdup(Tcl_GetString(tobj));
	    break;
	 case FONT_NAME:
	    fname = Tcl_GetString(tobj);
	    for (k = 0; k < fontcount; k++) {
	       if (!strcmp(fonts[k].psname, fname)) {
		  newpart->data.font = k;
		  break;
	       }
	    }
	    if (k == fontcount) {
	       Tcl_SetResult(interp, "Bad font name", NULL);
	       return TCL_ERROR;
	    }
	    break;
	 case FONT_SCALE:
	    result = Tcl_GetDoubleFromObj(interp, tobj, &fscale);
	    if (result != TCL_OK) return result;
	    newpart->data.scale = (float)fscale;
	    break;
	 case MARGINSTOP:
	    result = Tcl_GetIntFromObj(interp, tobj, &ival);
	    if (result != TCL_OK) return result;
	    newpart->data.width = ival;
	    break;
	 case KERN:
	    result = Tcl_ListObjLength(interp, tobj, &numparts);
	    if (result != TCL_OK) return result;
	    if (numparts != 2) {
	       Tcl_SetResult(interp, "Bad kern list:  need 2 values", NULL);
	       return TCL_ERROR;
	    }
	    result = Tcl_ListObjIndex(interp, tobj, 0, &t2obj);
	    if (result != TCL_OK) return result;
	    result = Tcl_GetIntFromObj(interp, t2obj, &ival);
	    if (result != TCL_OK) return result;
	    newpart->data.kern[0] = (short)ival;

	    result = Tcl_ListObjIndex(interp, tobj, 1, &t2obj);
	    if (result != TCL_OK) return result;
	    result = Tcl_GetIntFromObj(interp, t2obj, &ival);
	    if (result != TCL_OK) return result;
	    newpart->data.kern[1] = (short)ival;

	    break;
	 case FONT_COLOR:
	    /* Not implemented:  Need TclRGBToIndex() function */
	    break;

	 /* All other types have no arguments */
      }
   }
   return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* Handle (integer representation of internal xcircuit object) checking	*/
/* if "checkobject" is NULL, then                                       */
/*----------------------------------------------------------------------*/

genericptr *CheckHandle(pointertype eaddr, objectptr checkobject)
{
   genericptr *gelem;
   int i, j;
   objectptr thisobj;
   Library *thislib;

   if (checkobject != NULL) {
      for (gelem = checkobject->plist; gelem < checkobject->plist +
		checkobject->parts; gelem++)
	 if ((pointertype)(*gelem) == eaddr) goto exists;
      return NULL;
   }

   /* Look through all the pages. */

   for (i = 0; i < xobjs.pages; i++) {
      if (xobjs.pagelist[i]->pageinst == NULL) continue;
      thisobj = xobjs.pagelist[i]->pageinst->thisobject;
      for (gelem = thisobj->plist; gelem < thisobj->plist + thisobj->parts; gelem++)
         if ((pointertype)(*gelem) == eaddr) goto exists;
   }

   /* Not found?  Maybe in a library */

   for (i = 0; i < xobjs.numlibs; i++) {
      thislib = xobjs.userlibs + i;
      for (j = 0; j < thislib->number; j++) {
         thisobj = thislib->library[j];
         for (gelem = thisobj->plist; gelem < thisobj->plist + thisobj->parts; gelem++)
            if ((pointertype)(*gelem) == eaddr) goto exists;
      }
   }

   /* Either in the delete list (where we don't want to go) or	*/
   /* is an invalid number.					*/
   return NULL;

exists:
   return gelem;
}

/*----------------------------------------------------------------------*/
/* Find the index into the "plist" list of elements			*/
/* Part number must be of a type in "mask" or no selection occurs.	*/
/* return values:  -1 = no object found, -2 = found, but wrong type	*/
/*----------------------------------------------------------------------*/

short GetPartNumber(genericptr egen, objectptr checkobject, int mask)
{
   genericptr *gelem;
   objectptr thisobject = checkobject;
   int i;

   if (checkobject == NULL) thisobject = topobject;

   for (i = 0, gelem = thisobject->plist; gelem < thisobject->plist +
		thisobject->parts; gelem++, i++) {
      if ((*gelem) == egen) {
	 if ((*gelem)->type & mask)
	    return i;
	 else
	    return -2;
      }
   }
   return -1;
}

/*----------------------------------------------------------------------*/
/* This routine is used by a number of menu functions.  It looks for	*/
/* the arguments "selected" or an integer (object handle).  If the	*/
/* argument is a valid object handle, it is added to the select list.	*/
/* The argument can be a list of handles, of which each is checked and	*/
/* added to the select list.						*/
/* "extra" indicates the number of required arguments beyond 2.		*/
/* "next" returns the integer of the argument after the handle, or the	*/
/* argument after the command, if there is no handle.  If the handle is	*/
/* specified as a hierarchical list of element handles then		*/
/* areawin->hierstack contains the hierarchy of object instances.	*/
/*----------------------------------------------------------------------*/

int ParseElementArguments(Tcl_Interp *interp, int objc,
		Tcl_Obj *CONST objv[], int *next, int mask) {

   short *newselect;
   char *argstr;
   int i, j, result, numobjs;
   pointertype ehandle;
   Tcl_Obj *lobj;
   int extra = 0, goodobjs = 0;

   if (next != NULL) {
      extra = *next;
      *next = 1;
   }

   if ((objc > (2 + extra)) || (objc == 1)) {
      Tcl_WrongNumArgs(interp, 1, objv, "[selected | <element_handle>] <option>");
      return TCL_ERROR;
   }
   else if (objc == 1) {
      *next = 0;
      return TCL_OK;
   }
   else {
      argstr = Tcl_GetString(objv[1]);

      if (strcmp(argstr, "selected")) {

         /* check for object handle (special type) */

         result = Tcl_ListObjLength(interp, objv[1], &numobjs);
         if (result != TCL_OK) return result;
	 goodobjs = 0;

	 /* Non-integer, non-list types: assume operation is to be applied */
	 /* to currently selected elements, and return to caller.	   */

	 if (numobjs == 1) {
	    result = Tcl_GetHandleFromObj(interp, objv[1], (void *)&ehandle);
	    if (result != TCL_OK) {
	       Tcl_ResetResult(interp);
	       return TCL_OK;
	    }
	 }
	 if (numobjs == 0) {
	    Tcl_SetResult(interp, "No elements.", NULL);
	    return TCL_ERROR;
	 }
	 else
	    newselect = (short *)malloc(numobjs * sizeof(short));

	 /* Prepare a new selection, in case the new selection is	*/
	 /* smaller than the original selection, but don't blanket	*/
	 /* delete an existing selection, which will destroy cycle	*/
	 /* information.						*/

	 for (j = 0; j < numobjs; j++) {
            result = Tcl_ListObjIndex(interp, objv[1], j, &lobj);
            if (result != TCL_OK) {
	       free(newselect);
	       return result;
	    }
	    result = Tcl_GetHandleFromObj(interp, lobj, (void *)&ehandle);
            if (result != TCL_OK) {
	       free(newselect);
	       return result;
	    }
	    if (areawin->hierstack != NULL)
	       i = GetPartNumber((genericptr)ehandle,
			areawin->hierstack->thisinst->thisobject, mask);
	    else
               i = GetPartNumber((genericptr)ehandle, topobject, mask);

            if (i == -1) {
	       free_stack(&areawin->hierstack);
	       Tcl_SetResult(interp, "No such element exists.", NULL);
	       free(newselect);
	       return TCL_ERROR;
            }
	    else if (i >= 0) {
               *(newselect + goodobjs) = i;
	       if (next != NULL) *next = 2;
	       goodobjs++;
	    }
	 }
	 if (goodobjs == 0) {
	    Tcl_SetResult(interp, "No element matches required type.", NULL);
	    unselect_all();
	    free(newselect);
	    return TCL_ERROR;
	 }
	 else {
	    selection aselect, bselect;

	    /* To avoid unnecessarily blasting the existing selection	*/
	    /* and its cycles, we compare the two selection lists.	*/
	    /* This is not an excuse for not fixing the selection list	*/
	    /* mess in general!						*/

	    aselect.selectlist = newselect;
	    aselect.selects = goodobjs;
	    bselect.selectlist = areawin->selectlist;
	    bselect.selects = areawin->selects;
	    if (compareselection(&aselect, &bselect)) {
	       free(newselect);
	    }
	    else {
	       unselect_all();
	       areawin->selects = goodobjs;
	       areawin->selectlist = newselect;
	    }
	 }

         draw_normal_selected(topobject, areawin->topinstance);
      }
      else if (next != NULL) *next = 2;
   }
   return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* Generate a transformation matrix according to the object instance	*/
/* hierarchy left on the hierstack.					*/
/*----------------------------------------------------------------------*/

void MakeHierCTM(Matrix *hierCTM)
{
   objinstptr thisinst;
   pushlistptr cs;

   UResetCTM(hierCTM);
   for (cs = areawin->hierstack; cs != NULL; cs = cs->next) {
      thisinst = cs->thisinst;
      UMultCTM(hierCTM, thisinst->position, thisinst->scale, thisinst->rotation);
   }
}

/*----------------------------------------------------------------------*/
/* This routine is similar to ParseElementArguments.  It looks for a	*/
/* page number or page name in the second argument position.  If it	*/
/* finds one, it sets the page number in the return value.  Otherwise,	*/
/* it sets the return value to the value of areawin->page.		*/
/*----------------------------------------------------------------------*/

int ParsePageArguments(Tcl_Interp *interp, int objc,
		Tcl_Obj *CONST objv[], int *next, int *pageret) {

   char *pagename;
   int i, page, result;
   Tcl_Obj *objPtr;

   if (next != NULL) *next = 1;
   if (pageret != NULL) *pageret = areawin->page;  /* default */

   if ((objc == 1) || ((objc == 2) && !strcmp(Tcl_GetString(objv[1]), ""))) {
      objPtr = Tcl_NewIntObj(areawin->page + 1);
      Tcl_SetObjResult(interp, objPtr);
      if (next) *next = -1;
      return TCL_OK;
   }
   else {
      pagename = Tcl_GetString(objv[1]);
      if (strcmp(pagename, "directory")) {

         /* check for page number (integer) */

	 result = Tcl_GetIntFromObj(interp, objv[1], &page);
	 if (result != TCL_OK) {
	    Tcl_ResetResult(interp);

	    /* check for page name (string) */

	    for (i = 0; i < xobjs.pages; i++) {
	       if (xobjs.pagelist[i]->pageinst == NULL) continue;
	       if (!strcmp(pagename, xobjs.pagelist[i]->pageinst->thisobject->name)) {
		  if (pageret) *pageret = i;
		  break;
	       }
	    }
	    if (i == xobjs.pages) {
	       if (next != NULL) *next = 0;
	    }
	 }
         else {
	    if (page < 1) {
	       Tcl_SetResult(interp, "Illegal page number: zero or negative", NULL);
	       return TCL_ERROR;
	    }
	    else if (page > xobjs.pages) {
	       Tcl_SetResult(interp, "Illegal page number: page does not exist", NULL);
	       if (pageret) *pageret = (page - 1);
	       return TCL_ERROR;
	    }
	    else if (pageret) *pageret = (page - 1);
	 }
      }
      else {
	 *next = 0;
      }
   }
   return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* This routine is similar to ParsePageArguments.  It looks for a	*/
/* library number or library name in the second argument position.  If 	*/
/* it finds one, it sets the page number in the return value.		*/
/* Otherwise, if a library page is currently being viewed, it sets the	*/
/* return value to that library.  Otherwise, it sets the return value	*/
/* to the User Library.							*/
/*----------------------------------------------------------------------*/

int ParseLibArguments(Tcl_Interp *interp, int objc,
		Tcl_Obj *CONST objv[], int *next, int *libret) {

  char *libname;
  int library, result;
   Tcl_Obj *objPtr;

   if (next != NULL) *next = 1;

   if (objc == 1) {
      library = is_library(topobject);
      if (library < 0) {
	 Tcl_SetResult(interp, "No current library.", NULL);
	 return TCL_ERROR;
      }
      objPtr = Tcl_NewIntObj(library + 1);
      Tcl_SetObjResult(interp, objPtr);
      if (next) *next = -1;
      return TCL_OK;
   }
   else {
      libname = Tcl_GetString(objv[1]);
      if (strcmp(libname, "directory")) {

         /* check for library number (integer) or name */

	 result = Tcl_GetIntFromObj(interp, objv[1], &library);
	 if (result != TCL_OK) {
	    Tcl_ResetResult(xcinterp);
	    *libret = NameToLibrary(libname);
	    if (*libret < 0) {
	       *libret = -1;
	       if (next != NULL) *next = 0;
	    }
	 }
         else {
	    if (library < 1) {
	       Tcl_SetResult(interp, "Illegal library number: zero or negative", NULL);
	       return TCL_ERROR;
	    }
	    else if (library > xobjs.numlibs) {
	       Tcl_SetResult(interp, "Illegal library number: library "
			"does not exist", NULL);
	       return TCL_ERROR;
	    }
	    else *libret = (library - 1);
	 }
      }
      else *next = 0;
   }
   return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* Schematic and symbol creation and association			*/
/*----------------------------------------------------------------------*/

int xctcl_symschem(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int i, idx, result, stype;
   objectptr otherobj = NULL;
   char *objname;

   static char *subCmds[] = {
      "associate", "disassociate", "make", "goto", "get", "type", NULL
   };
   enum SubIdx {
      AssocIdx, DisAssocIdx, MakeIdx, GoToIdx, NameIdx, TypeIdx
   };

   /* The order of these must match the definitions in xcircuit.h */
   static char *schemTypes[] = {
     "primary", "secondary", "trivial", "symbol", "fundamental",
     "nonetwork", NULL /* (jdk) */
   };

   if (objc == 1 || objc > 4) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }
   else if ((result = Tcl_GetIndexFromObj(interp, objv[1],
		(CONST84 char **)subCmds, "option", 0, &idx)) != TCL_OK) {
      return result;
   }

   switch(idx) {
      case AssocIdx:
	 if (objc == 3) {

	    /* To do: accept name for association */
	    objname = Tcl_GetString(objv[2]);

	    if (topobject->schemtype == PRIMARY) {

	       /* Name has to be that of a library object */

	       otherobj = NameToObject(Tcl_GetString(objv[2]), NULL, FALSE);
	       if (otherobj == NULL) {
	          Tcl_SetResult(interp, "Name is not a known object", NULL);
		  return TCL_ERROR;
	       }
	    }
	    else {

	       /* Name has to be that of a page label */

	       objectptr pageobj;
	       for (i = 0; i < xobjs.pages; i++) {
		  pageobj = xobjs.pagelist[i]->pageinst->thisobject;
		  if (!strcmp(objname, pageobj->name)) {
		     otherobj = pageobj;
		     break;
		  }
	       }
	       if (otherobj == NULL)
	       {
	          Tcl_SetResult(interp, "Name is not a known page label", NULL);
		  return TCL_ERROR;
	       }
	    }
	    if (schemassoc(topobject, otherobj) == False)
	       return TCL_ERROR;
	 }
	 else
	    startschemassoc(NULL, 0, NULL);
	 break;
      case DisAssocIdx:
	 schemdisassoc();
	 break;
      case MakeIdx:
	 if (topobject->symschem != NULL)
	    Wprintf("Error:  Schematic already has an associated symbol.");
	 else if (topobject->schemtype != PRIMARY)
	    Wprintf("Error:  Current page is not a primary schematic.");
	 else if (!strncmp(topobject->name, "Page ", 5))
	    Wprintf("Error:  Schematic page must have a valid name.");
	 else {
	    int libnum = -1;
	    if (objc >= 3) {

	       objname = Tcl_GetString(objv[2]);

	       if (objc == 4) {
		  ParseLibArguments(xcinterp, 2, &objv[2], NULL, &libnum);
		  if (libnum < 0) {
	             Tcl_SetResult(interp, "Invalid library name.", NULL);
		     return TCL_ERROR;
		  }
	       }
	    }
	    else {
	       /* Use this error condition to generate the popup prompt */
	       Tcl_SetResult(interp, "Must supply a name for the page", NULL);
	       return TCL_ERROR;
	    }
	    swapschem(1, libnum, objname);
	    return TCL_OK;
	 }
	 return TCL_ERROR;
	 break;
      case GoToIdx:
	 /* This is supposed to specifically go to the specified type,	*/
	 /* so don't call swapschem to change views if we're already	*/
	 /* on the right view.						*/

	 if (topobject->schemtype == PRIMARY || topobject->schemtype == SECONDARY) {
	    if (!strncmp(Tcl_GetString(objv[0]), "sym", 3)) {
	       swapschem(0, -1, NULL);
	    }
	 }
	 else {
	    if (!strncmp(Tcl_GetString(objv[0]), "sch", 3)) {
	       swapschem(0, -1, NULL);
	    }
	 }
	 break;
      case NameIdx:
	 if (topobject->symschem != NULL)
	    Tcl_AppendElement(interp, topobject->symschem->name);
	 break;
      case TypeIdx:
	 if (objc == 3) {
	    if (topobject->schemtype == PRIMARY || topobject->schemtype == SECONDARY) {
	       Tcl_SetResult(interp, "Make object to change from schematic to symbol",
			NULL);
	       return TCL_ERROR;
	    }
	    if ((result = Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)schemTypes, "schematic types",
			0, &stype)) != TCL_OK)
	       return result;
	    if (stype == PRIMARY || stype == SECONDARY) {
	       Tcl_SetResult(interp, "Cannot change symbol into a schematic", NULL);
	       return TCL_ERROR;
	    }
	    topobject->schemtype = stype;
	    if (topobject->symschem) schemdisassoc();
	 }
	 else
	    Tcl_AppendElement(interp, schemTypes[topobject->schemtype]);

	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Generate netlist into a Tcl hierarchical list			*/
/* (plus other netlist functions)					*/
/*----------------------------------------------------------------------*/

int xctcl_netlist(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   Tcl_Obj *rdict;
   int idx, result, mpage, spage, bvar, j;
   Boolean valid, quiet;
   char *option, *extension, *mode = NULL;
   pushlistptr stack;
   objectptr master, slave;
   objinstptr schemtopinst;

   static char *subCmds[] = {
      "write", "highlight", "unhighlight", "goto", "get", "select", "parse",
      "position", "make", "connect", "unconnect", "autonumber", "ratsnest",
      "update", NULL
   };
   enum SubIdx {
      WriteIdx, HighLightIdx, UnHighLightIdx, GoToIdx, GetIdx, SelectIdx,
	ParseIdx, PositionIdx, MakeIdx, ConnectIdx, UnConnectIdx,
	AutoNumberIdx, RatsNestIdx, UpdateIdx
   };

   if (objc == 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }
   else if ((result = Tcl_GetIndexFromObj(interp, objv[1],
		(CONST84 char **)subCmds, "option", 0, &idx)) != TCL_OK) {
      return result;
   }

   /* Look for the "-quiet" option (more options processed by "netlist get") */

   j = 1;
   quiet = FALSE;
   while (option = Tcl_GetString(objv[objc - (j++)]), option[0] == '-') {
      if (!strncmp(option + 1, "quiet", 5))
	 quiet = TRUE;
   }

   /* Make sure a valid netlist exists for the current schematic */
   /* for those commands which require a valid netlist (non-ASG	 */
   /* functions).  Some functions (e.g., "parse") require that	 */
   /* the next object up in the hierarchy have a valid netlist,	 */
   /* if we have descended to the current symbol from there.	 */

   valid = False;
   switch(idx) {
      case RatsNestIdx:
	 /* Specifically avoid calling updatenets() */
	 if ((topobject->labels != NULL) || (topobject->polygons != NULL))
	    valid = True;
	 break;
   }

   if (!valid) {
      objinstptr tinst;

      /* Ignore libraries */
      if (is_library(topobject) >= 0 || (eventmode == CATALOG_MODE))
	 return TCL_ERROR;

      if ((topobject->schemtype) != PRIMARY && (areawin->stack != NULL))
         tinst = areawin->stack->thisinst;
      else
         tinst = areawin->topinstance;

      if ((result = updatenets(tinst, quiet)) < 0) {
	 Tcl_SetResult(interp, "Check circuit for infinite recursion.", NULL);
	 return TCL_ERROR;
      }
      else if (result == 0) {
	 Tcl_SetResult(interp, "No netlist.", NULL);
	 return TCL_ERROR;
      }
   }

   switch(idx) {
      case WriteIdx:		/* write netlist formats */
         if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "write format [extension] "
			"[spice_end] [-option]");
	    return TCL_ERROR;
	 }

	 /* Check for forcing option */

	 option = Tcl_GetString(objv[objc - 1]);
	 if (option[0] == '-')
	 {
	    option++;
	    if (!strncmp(option, "flat", 4) || !strncmp(option, "pseu", 4))
	    {
		mode = (char *)malloc(5 + strlen(Tcl_GetString(objv[2])));
		option[4] = '\0';
		sprintf(mode, "%s%s", option, Tcl_GetString(objv[2]));
	    }
	    else if (strncmp(option, "hier", 4))
	    {
		Tcl_SetResult(interp, "Unknown netlist option.", NULL);
		return TCL_ERROR;
	    }
	    objc--;
	 }

	 if ((result = Tcl_GetBooleanFromObj(interp, objv[objc - 1], &bvar))
		!= TCL_OK) {
	    spice_end = True;
	    Tcl_ResetResult(interp);
	 }
	 else {
	    spice_end = (Boolean)bvar;
	    objc--;
	 }

	 /* If no extension is specified, the extension is the same as	*/
	 /* the format name.						*/

	 if (objc == 3)
	    extension = Tcl_GetString(objv[2]);
	 else
	    extension = Tcl_GetString(objv[3]);
	 writenet(topobject, (mode == NULL) ? Tcl_GetString(objv[2]) : mode,
			extension);
	 if (mode != NULL) free(mode);
	 break;

      case GoToIdx:	/* go to top-level page having specified name */
         if (objc != 2 && objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "goto [hierarchical-network-name]");
	    return TCL_ERROR;
	 }

	 /* Find the top of the schematic hierarchy, regardless of	*/
	 /* where the current page is in it.				*/

	 if (areawin->stack == NULL)
	    schemtopinst = areawin->topinstance;
	 else {
	    pushlistptr sstack = areawin->stack;
	    while (sstack->next != NULL) sstack = sstack->next;
	    schemtopinst = sstack->thisinst;
	 }

	 stack = NULL;
	 push_stack(&stack, schemtopinst, NULL);
	 valid = TRUE;
	 if (objc == 3)
	    valid = HierNameToObject(schemtopinst, Tcl_GetString(objv[2]), &stack);

	 if (valid) {
	     /* Add the current edit object to the push stack, then append */
	     /* the new push stack 					   */
	     free_stack(&areawin->stack);
	     topobject->viewscale = areawin->vscale;
	     topobject->pcorner = areawin->pcorner;
	     areawin->topinstance = stack->thisinst;
	     pop_stack(&stack);
	     areawin->stack = stack;
	     setpage(TRUE);
	     transferselects();
	     refresh(NULL, NULL, NULL);
	     setsymschem();

	     /* If the current object is a symbol that has a schematic,	*/
	     /* go to the schematic.					*/

	     if (topobject->schemtype != PRIMARY && topobject->symschem != NULL)
		swapschem(0, -1, NULL);
	 }
	 else {
	    Tcl_SetResult(interp, "Not a valid network.", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case GetIdx: {	/* return hierarchical name of selected network */
	 int stype, netid, lbus;
	 Boolean uplevel, hier, canon;
	 char *prefix = NULL;
	 Matrix locctm;
	 short *newselect;
	 Genericlist *netlist;
	 CalllistPtr calls;
	 objinstptr refinstance;
	 objectptr refobject;
	 XPoint refpoint, *refptptr;
	 stringpart *ppin;
	 char *snew;
	 buslist *sbus;
	 Tcl_Obj *tlist;

	 option = Tcl_GetString(objv[objc - 1]);
	 uplevel = FALSE;
	 hier = FALSE;
	 canon = FALSE;
	 quiet = FALSE;
	 while (option[0] == '-') {
	    if (!strncmp(option + 1, "up", 2)) {
	       uplevel = TRUE;
	    }
	    else if (!strncmp(option + 1, "hier", 4)) {
	       hier = TRUE;
	    }
	    else if (!strncmp(option + 1, "canon", 5)) {
	       canon = TRUE;
	    }
	    else if (!strncmp(option + 1, "quiet", 5)) {
	       quiet = TRUE;
	    }
	    else if (sscanf(option, "%hd", &refpoint.x) == 1) {
	       break;	/* This is probably a negative point position! */
	    }
	    objc--;
	    option = Tcl_GetString(objv[objc - 1]);
	 }

	 refinstance = (areawin->hierstack) ?  areawin->hierstack->thisinst
		: areawin->topinstance;

	 if (uplevel) {
	    if (areawin->hierstack == NULL) {
	       if (areawin->stack == NULL) {
		  if (quiet) return TCL_OK;
	          Fprintf(stderr, "Option \"up\" used, but current page is the"
			" top of the schematic\n");
	          return TCL_ERROR;
	       }
	       else {
	          UResetCTM(&locctm);
	          UPreMultCTM(&locctm, refinstance->position, refinstance->scale,
			refinstance->rotation);
	          refinstance = areawin->stack->thisinst;
	          refobject = refinstance->thisobject;
	       }
	    }
	    else {
	       if (areawin->hierstack->next == NULL) {
		  if (quiet) return TCL_OK;
	          Fprintf(stderr, "Option \"up\" used, but current page is the"
			" top of the drawing stack\n");
	          return TCL_ERROR;
	       }
	       else {
	          UResetCTM(&locctm);
	          UPreMultCTM(&locctm, refinstance->position, refinstance->scale,
			refinstance->rotation);
	          refinstance = areawin->hierstack->next->thisinst;
	          refobject = refinstance->thisobject;
	       }
	    }
	 }
	 else {
	    refobject = topobject;
	 }
         if ((objc != 2) && (objc != 3)) {
	    Tcl_WrongNumArgs(interp, 1, objv,
			"get [selected|here|<name>] [-up][-hier][-canon][-quiet]");
	    return TCL_ERROR;
	 }
	 if ((objc == 3) && !strcmp(Tcl_GetString(objv[2]), "here")) {
	    /* If "here", make a selection. */
            areawin->save = UGetCursorPos();
            newselect = select_element(POLYGON | LABEL | OBJINST);
	    objc--;
	 }
	 if ((objc == 2) || (!strcmp(Tcl_GetString(objv[2]), "selected"))) {
	    /* If no argument, or "selected", use the selected element */
            newselect = areawin->selectlist;
	    if (areawin->selects == 0) {
	       if (hier) {
		  Tcl_SetResult(interp, GetHierarchy(&areawin->stack, canon),
				TCL_DYNAMIC);
		  break;
	       }
	       else {
	          Fprintf(stderr, "Either select an element or use \"-hier\"\n");
	          return TCL_ERROR;
	       }
	    }
            if (areawin->selects != 1) {
	       Fprintf(stderr, "Choose only one network element\n");
	       return TCL_ERROR;
	    }
	    else {
	       stype = SELECTTYPE(newselect);
	       if (stype == LABEL) {
	          labelptr nlabel = SELTOLABEL(newselect);
		  refptptr = &(nlabel->position);
		  if ((nlabel->pin != LOCAL) && (nlabel->pin != GLOBAL)) {
		     Fprintf(stderr, "Selected label is not a pin\n");
		     return TCL_ERROR;
		  }
	       }
	       else if (stype == POLYGON) {
	          polyptr npoly = SELTOPOLY(newselect);
		  refptptr = npoly->points;
		  if (nonnetwork(npoly)) {
		     Fprintf(stderr, "Selected polygon is not a wire\n");
		     return TCL_ERROR;
		  }
	       }
	       else if (stype == OBJINST) {
		  objinstptr ninst = SELTOOBJINST(newselect);
		  char *devptr;

		  for (calls = topobject->calls; calls != NULL; calls = calls->next)
		     if (calls->callinst == ninst)
		        break;
		  if (calls == NULL) {
		     Fprintf(stderr, "Selected instance is not a circuit component\n");
		     return TCL_ERROR;
		  }
		  else if (calls->devindex == -1) {
		     cleartraversed(topobject);
		     resolve_indices(topobject, FALSE);
		  }
		  push_stack(&areawin->stack, ninst, NULL);
		  prefix = GetHierarchy(&areawin->stack, canon);
		  pop_stack(&areawin->stack);
		  if (prefix == NULL) break;
		  devptr = prefix;
		  if (!hier) {
		     devptr = strrchr(prefix, '/');
		     if (devptr == NULL)
			devptr = prefix;
		     else
			devptr++;
		  }
		  Tcl_SetResult(interp, devptr, TCL_VOLATILE);
 		  free(prefix);
		  break;
	       }
	    }
	 }
	 else if ((objc == 3) && (result = GetPositionFromList(interp, objv[2],
		&refpoint)) == TCL_OK) {
	    /* Find net at indicated position in reference object.	*/
	    /* This allows us to query points without generating a pin	*/
	    /* at the position, which can alter the netlist under	*/
	    /* observation.						*/
	    refptptr = &refpoint;
	 }
	 else {
	    /* If a name, find the pin label element matching the name */
	    int x, y;
	    objinstptr instofname = (areawin->hierstack) ?
			areawin->hierstack->thisinst :
			areawin->topinstance;

	    Tcl_ResetResult(interp);

	    if (NameToPinLocation(instofname, Tcl_GetString(objv[2]),
			&x, &y) == 0) {
	       refpoint.x = x;		/* conversion from int to short */
	       refpoint.y = y;
	       refptptr = &refpoint;
	    }
	    else {
	       /* This is not necessarily an error.  Use "-quiet" to shut it up */
	       if (quiet) return TCL_OK;
	       Tcl_SetResult(interp, "Cannot find position for pin ", NULL);
	       Tcl_AppendElement(interp, Tcl_GetString(objv[2]));
	       return TCL_ERROR;
	    }
	 }

	 /* Now that we have a reference point, convert it to a netlist */
	 if (uplevel) {
	    UTransformbyCTM(&locctm, refptptr, &refpoint, 1);
	    refptptr = &refpoint;
	 }
	 netlist = pointtonet(refobject, refinstance, refptptr);
	 if (netlist == NULL) {
	    if (quiet) return TCL_OK;
	    Fprintf(stderr, "Error:  No network found!\n");
	    return TCL_ERROR;
	 }

	 /* If refobject is a secondary schematic, we need to find the	*/
	 /* corresponding primary page to call nettopin().		*/
         master = (refobject->schemtype == SECONDARY) ?
		refobject->symschem : refobject;

	 /* Now that we have a netlist, convert it to a name		*/
	 /* Need to get prefix from the current call stack so we	*/
	 /* can represent flat names as well as hierarchical names.	*/

	 if (hier) {
	    int plen;
	    prefix = GetHierarchy(&areawin->stack, canon);
	    if (prefix) {
	       plen = strlen(prefix);
	       if (*(prefix + plen - 1) != '/') {
	          prefix = realloc(prefix, plen + 2);
	          strcat(prefix, "/");
	       }
	    }
	 }

	 if (netlist->subnets == 0) {
	    netid = netlist->net.id;
	    ppin = nettopin(netid, master, (prefix == NULL) ? "" : prefix);
	    snew = textprint(ppin, refinstance);
	    Tcl_SetResult(interp, snew, TCL_DYNAMIC);
	 }
	 else if (netlist->subnets == 1) {

	    /* Need to get prefix from the current call stack! */
	    sbus = netlist->net.list;
	    netid = sbus->netid;
	    ppin = nettopin(netid, master, (prefix == NULL) ? "" : prefix);
	    snew = textprintsubnet(ppin, refinstance, sbus->subnetid);
	    Tcl_SetResult(interp, snew, TCL_DYNAMIC);
	 }
	 else {
	    tlist = Tcl_NewListObj(0, NULL);
	    for (lbus = 0; lbus < netlist->subnets; lbus++) {
	       sbus = netlist->net.list + lbus;
	       netid = sbus->netid;
	       ppin = nettopin(netid, master, (prefix == NULL) ? "" : prefix);
	       snew = textprintsubnet(ppin, refinstance, sbus->subnetid);
	       Tcl_ListObjAppendElement(interp, tlist, Tcl_NewStringObj(snew, -1));
	       Tcl_SetObjResult(interp, tlist);
	       free(snew);
	    }
	 }
	 if (prefix != NULL) free(prefix);
	 } break;

      case ParseIdx: {		/* generate output from info labels */
	 char *mode, *snew;
	 objectptr cfrom;

	 if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "parse <mode>");
	    return TCL_ERROR;
	 }
	 mode = Tcl_GetString(objv[2]);
	 master = topobject;
	 if ((master->schemtype == SECONDARY) && (master->symschem != NULL))
	    master = master->symschem;

	 if (master->schemtype != PRIMARY && areawin->stack != NULL) {
	    cfrom = areawin->stack->thisinst->thisobject;
	    snew = parseinfo(cfrom, master, cfrom->calls, NULL, mode, FALSE, TRUE);
	 }
	 else {
	    Calllist loccalls;

	    loccalls.cschem = NULL;
	    loccalls.callobj = master;
	    loccalls.callinst = areawin->topinstance;
	    loccalls.devindex = -1;
	    loccalls.ports = NULL;
	    loccalls.next = NULL;

	    snew = parseinfo(NULL, master, &loccalls, NULL, mode, FALSE, TRUE);
	 }
	 Tcl_SetResult(interp, snew, TCL_DYNAMIC);

	 } break;

      case UnConnectIdx:	/* disassociate the page with another one */
         if ((objc != 2) && (objc != 3)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "unconnect [<secondary>]");
	    return TCL_ERROR;
	 }
	 else if (objc == 3) {
	    result = Tcl_GetIntFromObj(interp, objv[2], &spage);
	    if (result != TCL_OK) {
	       Tcl_ResetResult(interp);
	       slave = NameToPageObject(Tcl_GetString(objv[2]), NULL, &spage);
	    }
	    else {
	       if (spage >= xobjs.pages) {
		  Tcl_SetResult(interp, "Bad page number for secondary schematic", NULL);
		  return TCL_ERROR;
	       }
	       slave = xobjs.pagelist[spage]->pageinst->thisobject;
	    }
	    if ((slave == NULL) || (is_page(slave) < 0)) {
	       Tcl_SetResult(interp, "Error determining secondary schematic", NULL);
	       return TCL_ERROR;
	    }
	 }
	 else {
	    slave = topobject;
	    spage = areawin->page;
	 }
	 if (slave->symschem == NULL || slave->symschem->schemtype !=
		PRIMARY) {
	    Tcl_SetResult(interp, "Page is not a secondary schematic", NULL);
	    return TCL_ERROR;
	 }

	 destroynets(slave->symschem);
	 slave->schemtype = PRIMARY;
	 slave->symschem = NULL;
	 break;

      case ConnectIdx:		/* associate the page with another one */
         if ((objc != 3) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "connect <primary> [<secondary>]");
	    return TCL_ERROR;
	 }
	 else if (objc == 4) {
	    result = Tcl_GetIntFromObj(interp, objv[3], &spage);
	    if (result != TCL_OK) {
	       Tcl_ResetResult(interp);
	       slave = NameToPageObject(Tcl_GetString(objv[3]), NULL, &spage);
	    }
	    else {
	       if (spage >= xobjs.pages) {
		  Tcl_SetResult(interp, "Bad page number for secondary schematic", NULL);
		  return TCL_ERROR;
	       }
	       slave = xobjs.pagelist[spage]->pageinst->thisobject;
	    }
	    if ((slave == NULL) || (is_page(slave) < 0)) {
	       Tcl_SetResult(interp, "Error determining secondary schematic", NULL);
	       return TCL_ERROR;
	    }
	 }
	 else {
	    slave = topobject;
	    spage = areawin->page;
	    destroynets(slave);
	 }

	 result = Tcl_GetIntFromObj(interp, objv[2], &mpage);
	 if (result != TCL_OK) {
	    Tcl_ResetResult(interp);
	    master = NameToPageObject(Tcl_GetString(objv[2]), NULL, &mpage);
	 }
	 else
	    mpage--;

	 if ((mpage >= xobjs.pages) || (xobjs.pagelist[mpage]->pageinst == NULL)) {
	    Tcl_SetResult(interp, "Bad page number for master schematic", NULL);
	    return TCL_ERROR;
	 }
	 else if (mpage == areawin->page) {
	    Tcl_SetResult(interp, "Attempt to specify schematic "
				"as its own master", NULL);
	    return TCL_ERROR;
	 }
	 if (xobjs.pagelist[mpage]->pageinst->thisobject->symschem == slave) {
	    Tcl_SetResult(interp, "Attempt to create recursive "
				"primary/secondary schematic relationship", NULL);
	    return TCL_ERROR;
	 }
	 master = xobjs.pagelist[mpage]->pageinst->thisobject;
	 destroynets(master);

	 if ((master == NULL) || (is_page(master) < 0)) {
	    Tcl_SetResult(interp, "Error determining master schematic", NULL);
	    return TCL_ERROR;
	 }

	 slave->schemtype = SECONDARY;
	 slave->symschem = master;
	 break;

      case UnHighLightIdx:	/* remove network connectivity highlight */
         if (objc == 2) {
	    highlightnetlist(topobject, areawin->topinstance, 0);
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "(no options)");
	    return TCL_ERROR;
	 }
	 break;

      case HighLightIdx:	/* highlight network connectivity */
         if (objc == 2) {
	    startconnect(NULL, NULL, NULL);
	    break;
	 }
	 /* drop through */
      case PositionIdx:
      case SelectIdx:		/* select the first element in the indicated net */
	 {
	    int netid, lbus, i;
	    XPoint newpos, *netpos;
	    char *tname;
	    Genericlist *lnets, *netlist;
	    buslist *sbus;
	    LabellistPtr llist;
	    PolylistPtr plist;
	    short *newselect;

	    if (objc < 3) {
	       Tcl_WrongNumArgs(interp, 1, objv, "network");
	       return TCL_ERROR;
            }

	    result = GetPositionFromList(interp, objv[2], &newpos);
	    if (result == TCL_OK) {	/* find net at indicated position */
	       areawin->save = newpos;
	       connectivity(NULL, NULL, NULL);
	       /* should there be any result here? */
	       break;
	    }
	    else {			/* assume objv[2] is net name */
	       Tcl_ResetResult(interp);
	       tname = Tcl_GetString(objv[2]);
	       lnets = nametonet(topobject, areawin->topinstance, tname);
	       if (lnets == NULL) {
		  Tcl_SetResult(interp, "No such network ", NULL);
	          Tcl_AppendElement(interp, tname);
		  break;
	       }
	       switch (idx) {
		  case HighLightIdx:
		     netlist = (Genericlist *)malloc(sizeof(Genericlist));

		     /* Erase any existing highlights first */
		     highlightnetlist(topobject, areawin->topinstance, 0);
		     netlist->subnets = 0;
		     copy_bus(netlist, lnets);
		     topobject->highlight.netlist = netlist;
		     topobject->highlight.thisinst = areawin->topinstance;
		     highlightnetlist(topobject, areawin->topinstance, 1);
		     if (netlist->subnets == 0) {
		        netid = netlist->net.id;
		        Tcl_SetObjResult(interp,  Tcl_NewIntObj(netlist->net.id));
		     }
		     else {
		        rdict = Tcl_NewListObj(0, NULL);
			for (lbus = 0; lbus < netlist->subnets; lbus++) {
			   sbus = netlist->net.list + lbus;
			   netid = sbus->netid;
		           Tcl_ListObjAppendElement(interp, rdict, Tcl_NewIntObj(netid));
			}
		        Tcl_SetObjResult(interp, rdict);
	             }
		     break;

		  /* Return a position belonging to the net.  If this is a bus, */
		  /* we return the position of the 1st subnet.  At some point,	*/
		  /* this should be expanded to return a point per subnet.	*/

		  case PositionIdx:
		     if (lnets->subnets == 0)
			netid = lnets->net.id;
		     else
			netid = (lnets->net.list)->netid;

		     netpos = NetToPosition(lnets->net.id, topobject);
		     rdict = Tcl_NewListObj(0, NULL);
		     Tcl_ListObjAppendElement(interp, rdict, Tcl_NewIntObj(netpos->x));
		     Tcl_ListObjAppendElement(interp, rdict, Tcl_NewIntObj(netpos->y));
		     Tcl_SetObjResult(interp, rdict);
		     break;

		  /* Select everything in the network.  To-do:  allow specific	*/
		  /* selection of labels, wires, or a single element in the net	*/

		  case SelectIdx:
		     unselect_all();
		     rdict = Tcl_NewListObj(0, NULL);
		     for (llist = topobject->labels; llist != NULL;
				llist = llist->next) {
			if (match_buses((Genericlist *)llist, (Genericlist *)lnets, 0)) {
		           i = GetPartNumber((genericptr)llist->label, topobject, LABEL);
		           if (i >= 0) {
			      newselect = allocselect();
			      *newselect = i;
		              Tcl_ListObjAppendElement(interp, rdict,
					Tcl_NewHandleObj((genericptr)llist->label));
			   }
			}
		     }
		     for (plist = topobject->polygons; plist != NULL;
				plist = plist->next) {
			if (match_buses((Genericlist *)plist, (Genericlist *)lnets, 0)) {
		           i = GetPartNumber((genericptr)plist->poly, topobject, POLYGON);
		           if (i >= 0) {
			      newselect = allocselect();
			      *newselect = i;
		              Tcl_ListObjAppendElement(interp, rdict,
					Tcl_NewHandleObj((genericptr)plist->poly));
			   }
			}
		     }
		     Tcl_SetObjResult(interp, rdict);
		     refresh(NULL, NULL, NULL);
		     break;
	       }
	    }
	 } break;

      case UpdateIdx:		/* destroy and regenerate the current netlist */
	 destroynets(areawin->topinstance->thisobject);
	 if ((result = updatenets(areawin->topinstance, quiet)) < 0) {
	    Tcl_SetResult(interp, "Check circuit for infinite recursion.", NULL);
	    return TCL_ERROR;
	 }
	 else if (result == 0) {
	    Tcl_SetResult(interp, "Failure to generate a network.", NULL);
	    return TCL_ERROR;
         }
	 break;

      case MakeIdx:		/* generate Tcl-list netlist */
	 rdict = Tcl_NewListObj(0, NULL);
	 Tcl_ListObjAppendElement(interp, rdict, Tcl_NewStringObj("globals", 7));
	 Tcl_ListObjAppendElement(interp, rdict, tclglobals(areawin->topinstance));
	 Tcl_ListObjAppendElement(interp, rdict, Tcl_NewStringObj("circuit", 7));
	 Tcl_ListObjAppendElement(interp, rdict, tcltoplevel(areawin->topinstance));

	 Tcl_SetObjResult(interp, rdict);
	 break;

      case AutoNumberIdx:	/* auto-number circuit components */
	 if (checkvalid(topobject) == -1) {
	    destroynets(topobject);
	    createnets(areawin->topinstance, FALSE);
	 }
	 else {
	    cleartraversed(topobject);
	    clear_indices(topobject);
	 }
	 if ((objc == 3) && !strcmp(Tcl_GetString(objv[2]), "-forget")) {
	    cleartraversed(topobject);
	    unnumber(topobject);
	 }
	 else {
	    cleartraversed(topobject);
	    resolve_indices(topobject, FALSE);  /* Do fixed assignments first */
	    cleartraversed(topobject);
	    resolve_indices(topobject, TRUE);   /* Now do the auto-numbering */
	 }
	 break;

      case RatsNestIdx:
	 /* Experimental netlist stuff! */
	 ratsnest(areawin->topinstance);
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Return current position						*/
/*----------------------------------------------------------------------*/

int xctcl_here(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   Tcl_Obj *listPtr, *objPtr;
   XPoint newpos;

   if (objc != 1) {
      Tcl_WrongNumArgs(interp, 0, objv, "(no arguments)");
      return TCL_ERROR;
   }
   newpos = UGetCursorPos();

   listPtr = Tcl_NewListObj(0, NULL);
   objPtr = Tcl_NewIntObj((int)newpos.x);
   Tcl_ListObjAppendElement(interp, listPtr, objPtr);

   objPtr = Tcl_NewIntObj((int)newpos.y);
   Tcl_ListObjAppendElement(interp, listPtr, objPtr);

   Tcl_SetObjResult(interp, listPtr);

   return XcTagCallback(interp, objc, objv);
}


/*----------------------------------------------------------------------*/
/* Argument-converting wrappers from Tcl command callback to xcircuit	*/
/*----------------------------------------------------------------------*/

int xctcl_pan(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result, idx;
   double frac = 0.0;
   XPoint newpos, wpoint;
   static char *directions[] = {"here", "left", "right", "up", "down",
		"center", "follow", NULL};
   enum DirIdx {
      DirHere, DirLeft, DirRight, DirUp, DirDown, DirCenter, DirFollow
   };

   if (objc != 2 && objc != 3) {
      Tcl_WrongNumArgs(interp, 0, objv, "option ?arg ...?");
      return TCL_ERROR;
   }

   /* Check against keywords */

   if (Tcl_GetIndexFromObj(interp, objv[1], (CONST84 char **)directions,
		"option", 0, &idx) != TCL_OK) {
      result = GetPositionFromList(interp, objv[1], &newpos);
      if (result != TCL_OK) return result;
      idx = 5;
   }
   else
      newpos = UGetCursorPos();

   user_to_window(newpos, &wpoint);

   switch(idx) {
      case DirHere:
      case DirCenter:
      case DirFollow:
	 if (objc != 2) {
            Tcl_WrongNumArgs(interp, 0, objv, "(no arguments)");
	 }
	 break;
      default:
	 if (objc == 2) frac = 0.3;
	 else
	    Tcl_GetDoubleFromObj(interp, objv[2], &frac);
   }

   panbutton((u_int)idx, wpoint.x, wpoint.y, (float)frac);
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_zoom(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result, idx;
   float save;
   double factor;
   XPoint newpos, wpoint;

   static char *subCmds[] = {"in", "out", "view", "factor", NULL};
   enum SubIdx {
      InIdx, OutIdx, ViewIdx, FactorIdx
   };

   newpos = UGetCursorPos();
   user_to_window(newpos, &wpoint);

   if (objc == 1)
      zoomview(NULL, NULL, NULL);
   else if ((result = Tcl_GetDoubleFromObj(interp, objv[1], &factor)) != TCL_OK)
   {
      Tcl_ResetResult(interp);
      if (Tcl_GetIndexFromObj(interp, objv[1], (CONST84 char **)subCmds,
		"option", 0, &idx) != TCL_OK) {
	 Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	 return TCL_ERROR;
      }
      switch(idx) {
	 case InIdx:
	    zoominrefresh(wpoint.x, wpoint.y);
	    break;
	 case OutIdx:
	    zoomoutrefresh(wpoint.x, wpoint.y);
	    break;
	 case ViewIdx:
	    zoomview(NULL, NULL, NULL);
	    break;
	 case FactorIdx:
	    if (objc == 2) {
	       Tcl_Obj *objPtr = Tcl_NewDoubleObj((double)areawin->zoomfactor);
	       Tcl_SetObjResult(interp, objPtr);
	       break;
	    }
	    else if (objc != 3) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    if (!strcmp(Tcl_GetString(objv[2]), "default"))
	       factor = SCALEFAC;
	    else {
	       result = Tcl_GetDoubleFromObj(interp, objv[2], &factor);
	       if (result != TCL_OK) return result;
	       if (factor <= 0) {
	          Tcl_SetResult(interp, "Negative/Zero zoom factors not allowed.",
			NULL);
	          return TCL_ERROR;
	       }
	       if (factor < 1.0) factor = 1.0 / factor;
	    }
	    if ((float)factor == areawin->zoomfactor) break;
	    Wprintf("Zoom factor changed from %2.1f to %2.1f",
		areawin->zoomfactor, (float)factor);
	    areawin->zoomfactor = (float) factor;
	    break;
      }
   }
   else {
      save = areawin->zoomfactor;

      if (factor < 1.0) {
         areawin->zoomfactor = (float)(1.0 / factor);
         zoomout(wpoint.x, wpoint.y);
      }
      else {
         areawin->zoomfactor = (float)factor;
         zoomin(wpoint.x, wpoint.y);
      }
      refresh(NULL, NULL, NULL);
      areawin->zoomfactor = save;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Get a color, either by name or by integer index.			*/
/* If "append" is TRUE, then if the color is not in the existing list	*/
/* of colors, it will be added to the list.				*/
/*----------------------------------------------------------------------*/

int GetColorFromObj(Tcl_Interp *interp, Tcl_Obj *obj, int *cindex, Boolean append)
{
   char *cname;
   int result;

   if (cindex == NULL) return TCL_ERROR;

   cname = Tcl_GetString(obj);
   if (!strcmp(cname, "inherit")) {
      *cindex = DEFAULTCOLOR;
   }
   else {
      result = Tcl_GetIntFromObj(interp, obj, cindex);
      if (result != TCL_OK) {
	 Tcl_ResetResult(interp);
	 *cindex = query_named_color(cname);
	 if (*cindex == BADCOLOR) {
	    *cindex = ERRORCOLOR;
	    Tcl_SetResult(interp, "Unknown color name ", NULL);
	    Tcl_AppendElement(interp, cname);
	    return TCL_ERROR;
	 }
	 else if (*cindex == ERRORCOLOR) {
	    if (append)
	       *cindex = addnewcolorentry(xc_alloccolor(cname));
	    else {
	       Tcl_SetResult(interp, "Color ", NULL);
	       Tcl_AppendElement(interp, cname);
	       Tcl_AppendElement(interp, "is not in the color table.");
	       return TCL_ERROR;
	    }
	 }
	 return TCL_OK;
      }

      if ((*cindex >= number_colors) || (*cindex < DEFAULTCOLOR)) {
	 Tcl_SetResult(interp, "Color index out of range", NULL);
	 return TCL_ERROR;
      }
   }
   return TCL_OK;
}

/*----------------------------------------------------------------------*/

int xctcl_color(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result, nidx, cindex, ccol, idx, i;
   char *colorname, *option;

   static char *subCmds[] = {"set", "index", "value", "get", "add",
		"override", NULL};
   enum SubIdx { SetIdx, IndexIdx, ValueIdx, GetIdx, AddIdx, OverrideIdx };

   nidx = 2;
   result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds, "option", 0,
		&idx)) != TCL_OK)
      return result;

   switch (idx) {
      case SetIdx:
         if ((objc - nidx) == 2) {
            result = GetColorFromObj(interp, objv[nidx + 1], &cindex, TRUE);
            if (result != TCL_OK) return result;
            setcolor((Tk_Window)clientData, cindex);
	    /* Tag callback performed by setcolormarks() via setcolor() */
	    return TCL_OK;
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "set <color> | inherit");
	    return TCL_ERROR;
	 }
         break;

      case IndexIdx:
	 /* Return the index of the color.  For use with parameterized color */
         if ((objc - nidx) == 2) {
            result = GetColorFromObj(interp, objv[nidx + 1], &cindex, TRUE);
            if (result != TCL_OK) return result;
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(cindex));
	    return TCL_OK;
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "index <color> | inherit");
	    return TCL_ERROR;
	 }
         break;

      case ValueIdx:
	 /* Return the value of the color as an {R G B} list */
         if ((objc - nidx) == 2) {
            result = GetColorFromObj(interp, objv[nidx + 1], &cindex, TRUE);
            if (result != TCL_OK) return result;
	    else if (cindex < 0 || cindex >= number_colors) {
	       Tcl_SetResult(interp, "Color index out of range", NULL);
	       return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, TclIndexToRGB(cindex));
	    return TCL_OK;
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "value <color>");
	    return TCL_ERROR;
	 }
         break;

      case GetIdx:
	 /* Check for "-all" switch */
	 if ((objc - nidx) == 2) {
	    option = Tcl_GetString(objv[nidx + 1]);
	    if (!strncmp(option, "-all", 4)) {
	       for (i = NUMBER_OF_COLORS; i < number_colors; i++) {
		  char colorstr[14];
		  sprintf(colorstr, "#%04x%04x%04x",
		     colorlist[i].color.red,
		     colorlist[i].color.green,
		     colorlist[i].color.blue);
		  Tcl_AppendElement(interp, colorstr);
	       }
	    }
	    else {
	       Tcl_WrongNumArgs(interp, 1, objv, "get [-all]");
	       return TCL_ERROR;
	    }
	    break;
	 }

	 if (areawin->selects > 0) {	/* operation on element */
	    genericptr genobj = SELTOGENERIC(areawin->selectlist);
	    ccol = (int)genobj->color;
	 }
	 else			/* global setting */
	    ccol = areawin->color;

	 /* Find and return the index of the color */
	 if (ccol == DEFAULTCOLOR)
	     Tcl_SetObjResult(interp, Tcl_NewStringObj("inherit", 7));
	 else {
	    for (i = NUMBER_OF_COLORS; i < number_colors; i++)
	       if (colorlist[i].color.pixel == ccol)
	          break;
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(i));
	 }
	 break;

      case AddIdx:
         if ((objc - nidx) == 2) {
	    colorname = Tcl_GetString(objv[nidx + 1]);
	    if (strlen(colorname) == 0) return TCL_ERROR;
	    cindex = addnewcolorentry(xc_alloccolor(colorname));
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(cindex));
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "add <color_name>");
	    return TCL_ERROR;
	 }
	 break;

      case OverrideIdx:
	 flags |= COLOROVERRIDE;
	 return TCL_OK;			/* no tag callback */
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_delete(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result = ParseElementArguments(interp, objc, objv, NULL, ALL_TYPES);

   if (result != TCL_OK) return result;

   /* delete element (call library delete if in catalog) */
   if (areawin->selects > 0) {
      if (eventmode == CATALOG_MODE)
         catdelete();
      else
         deletebutton(0, 0);	/* Note: arguments are not used */
   }

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Note that when using "undo series", it is the responsibility of the	*/
/* caller to make sure that every "start" is matched by an "end".	*/
/*----------------------------------------------------------------------*/

int xctcl_undo(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   if ((objc == 3) && !strcmp(Tcl_GetString(objv[1]), "series")) {

      if (!strcmp(Tcl_GetString(objv[2]), "start")) {
	 if (undo_collect < 255) undo_collect++;
      }
      else if (!strcmp(Tcl_GetString(objv[2]), "end")) {
	 if (undo_collect > 0) undo_collect--;
	 undo_finish_series();
      }
      else if (!strcmp(Tcl_GetString(objv[2]), "cancel")) {
	 undo_collect = (u_char)0;
	 undo_finish_series();
      }
      else {
         Tcl_SetResult(interp, "Usage: undo series <start|end|cancel>", NULL);
         return TCL_ERROR;
      }
   }
   else if (objc == 1) {
      undo_action();
   }
   else {
      Tcl_WrongNumArgs(interp, 1, objv, "[series <start|end>");
      return TCL_ERROR;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_redo(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   if (objc != 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "(no arguments)");
      return TCL_ERROR;
   }
   redo_action();
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_move(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   XPoint position;
   int nidx = 3;
   int result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);

   if (result != TCL_OK) return result;

   if (areawin->selects == 0) {
      Tcl_SetResult(interp, "Error in move setup:  nothing selected.", NULL);
      return TCL_ERROR;
   }

   if ((objc - nidx) == 0) {
      eventmode = MOVE_MODE;
      u2u_snap(&areawin->save);
      Tk_CreateEventHandler(areawin->area, PointerMotionMask,
		(Tk_EventProc *)xctk_drag, NULL);
   }
   else if ((objc - nidx) >= 1) {
      if ((objc - nidx) == 2) {
	 if (!strcmp(Tcl_GetString(objv[nidx]), "relative")) {
	    if ((result = GetPositionFromList(interp, objv[nidx + 1],
			&position)) != TCL_OK) {
	       Tcl_SetResult(interp, "Position must be {x y} list", NULL);
	       return TCL_ERROR;
	    }
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "relative {x y}");
	    return TCL_ERROR;
	 }
      }
      else {
	 if ((result = GetPositionFromList(interp, objv[nidx],
			&position)) != TCL_OK) {
	    Tcl_SetResult(interp, "Position must be {x y} list", NULL);
	    return TCL_ERROR;
	 }
         position.x -= areawin->save.x;
         position.y -= areawin->save.y;
      }
      placeselects(position.x, position.y, NULL);
   }
   else {
      Tcl_WrongNumArgs(interp, 1, objv, "[relative] {x y}");
      return TCL_ERROR;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_copy(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   XPoint position;
   Tcl_Obj *listPtr;
   int nidx = 3;
   int result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);

   if (result != TCL_OK) return result;

   if ((objc - nidx) == 0) {
      if (areawin->selects > 0) {
	 createcopies();
	 copydrag();
      }
   }
   else if ((objc - nidx) >= 1) {
      if (areawin->selects == 0) {
         Tcl_SetResult(interp, "Error in copy:  nothing selected.", NULL);
         return TCL_ERROR;
      }
      if ((objc - nidx) == 2) {
	 if (!strcmp(Tcl_GetString(objv[nidx]), "relative")) {
	    if ((result = GetPositionFromList(interp, objv[nidx + 1],
			&position)) != TCL_OK) {
	       Tcl_SetResult(interp, "Position must be {x y} list", NULL);
	       return TCL_ERROR;
	    }
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "relative {x y}");
	    return TCL_ERROR;
	 }
      }
      else {
	 if ((result = GetPositionFromList(interp, objv[nidx],
			&position)) != TCL_OK) {
	    Tcl_SetResult(interp, "Position must be {x y} list", NULL);
	    return TCL_ERROR;
	 }
         position.x -= areawin->save.x;
         position.y -= areawin->save.y;
      }
      createcopies();

      listPtr = SelectToTclList(interp, areawin->selectlist, areawin->selects);
      Tcl_SetObjResult(interp, listPtr);

      placeselects(position.x, position.y, NULL);
   }
   else {
      Tcl_WrongNumArgs(interp, 1, objv, "[relative] {x y}");
      return TCL_ERROR;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_flip(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   char *teststr;
   int nidx = 2;
   int result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);
   XPoint position;

   if (result != TCL_OK) return result;

   if ((objc - nidx) == 2) {
      if ((result = GetPositionFromList(interp, objv[nidx + 1],
			&position)) != TCL_OK)
	 return result;
   }
   else if ((objc - nidx) == 1) {
      if (areawin->selects > 1)
	 position = UGetCursorPos();
   }
   else {
      Tcl_WrongNumArgs(interp, 1, objv, "horizontal|vertical [<center>]");
      return TCL_ERROR;
   }

   teststr = Tcl_GetString(objv[nidx]);

   switch(teststr[0]) {
      case 'h': case 'H':
         elementflip(&position);
	 break;
      case 'v': case 'V':
         elementvflip(&position);
	 break;
      default:
	 Tcl_SetResult(interp, "Error: options are horizontal or vertical", NULL);
         return TCL_ERROR;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_rotate(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int rval, nidx = 2;
   int result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);
   XPoint position;

   if (result != TCL_OK) return result;

   /* No options --- return the rotation value(s) */
   if ((objc - nidx) == 0) {
      int i, numfound = 0;
      Tcl_Obj *listPtr, *objPtr;
      for (i = 0; i < areawin->selects; i++) {
	 objPtr = NULL;
	 if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
	    objinstptr pinst = SELTOOBJINST(areawin->selectlist + i);
	    objPtr = Tcl_NewDoubleObj((double)(pinst->rotation));
	 }
	 else if (SELECTTYPE(areawin->selectlist + i) == LABEL) {
	    labelptr plab = SELTOLABEL(areawin->selectlist + i);
	    objPtr = Tcl_NewDoubleObj((double)(plab->rotation));
	 }
	 else if (SELECTTYPE(areawin->selectlist + i) == GRAPHIC) {
	    graphicptr gp = SELTOGRAPHIC(areawin->selectlist + i);
	    objPtr = Tcl_NewDoubleObj((double)(gp->rotation));
	 }
	 if (objPtr != NULL) {
	    if (numfound > 0)
	       Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    if ((++numfound) == 1)
	       listPtr = objPtr;
	 }
      }
      switch (numfound) {
	 case 0:
	    Tcl_SetResult(interp, "Error: no object instances, graphic "
			"images, or labels selected", NULL);
	    return TCL_ERROR;
	    break;
	 case 1:
	    Tcl_SetObjResult(interp, objPtr);
	    break;
	 default:
	    Tcl_SetObjResult(interp, listPtr);
	    break;
      }
      return XcTagCallback(interp, objc, objv);
   }

   result = Tcl_GetIntFromObj(interp, objv[nidx], &rval);
   if (result != TCL_OK) return result;

   if ((objc - nidx) == 2) {
      if ((result = GetPositionFromList(interp, objv[nidx + 1],
			&position)) != TCL_OK)
	 return result;
      else {
	 elementrotate(rval, &position);
         return XcTagCallback(interp, objc, objv);
      }
   }
   else if ((objc - nidx) == 1) {
      position = UGetCursorPos();
      elementrotate(rval, &position);
      return XcTagCallback(interp, objc, objv);
   }

   Tcl_WrongNumArgs(interp, 1, objv, "<angle> [<center>]");
   return TCL_ERROR;
}

/*----------------------------------------------------------------------*/

int xctcl_edit(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result = ParseElementArguments(interp, objc, objv, NULL, ALL_TYPES);

   if (result != TCL_OK) return result;

   /* To be done---edit element */

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Support procedure for xctcl_param:  Given a pointer to a parameter,	*/
/* return the value of the parameter as a pointer to a Tcl object.	*/
/* This takes care of the fact that the parameter value can be a	*/
/* string, integer, or float, depending on the parameter type.		*/
/*									*/
/* If "verbatim" is true, then expression parameters return the string	*/
/* representation of the expression, not the result, and indirect	*/
/* parameters return the parameter name referenced, not the value.	*/
/*									*/
/* refinst, if non-NULL, is the instance containing ops, used when	*/
/* "verbatim" is true and the parameter is indirectly referenced.	*/
/*----------------------------------------------------------------------*/

Tcl_Obj *GetParameterValue(objectptr refobj, oparamptr ops, Boolean verbatim,
		objinstptr refinst)
{
   Tcl_Obj *robj;
   char *refkey;

   if (verbatim && (refinst != NULL) &&
		((refkey = find_indirect_param(refinst, ops->key)) != NULL)) {
      robj = Tcl_NewStringObj(refkey, strlen(refkey));
      return robj;
   }

   switch (ops->type) {
      case XC_STRING:
	 robj = TclGetStringParts(ops->parameter.string);
	 break;
      case XC_EXPR:
	 if (verbatim)
	    robj = Tcl_NewStringObj(ops->parameter.expr,
			strlen(ops->parameter.expr));
	 else
	    robj = evaluate_raw(refobj, ops, refinst, NULL);
	 break;
      case XC_INT:
	 robj = Tcl_NewIntObj(ops->parameter.ivalue);
	 break;
      case XC_FLOAT:
	 robj = Tcl_NewDoubleObj((double)ops->parameter.fvalue);
	 break;
   }
   return robj;
}

/*----------------------------------------------------------------------*/
/* Given a pointer to a parameter and a Tcl object, set the parameter	*/
/* to the value of the object.  Return the standard Tcl return type	*/
/*									*/
/* If searchinst is non-NULL, then it refers to the level above in the	*/
/* hierarchy, and we are supposed to set an indirect reference.		*/
/*----------------------------------------------------------------------*/

int SetParameterValue(Tcl_Interp *interp, oparamptr ops, Tcl_Obj *objv)
{
   int result, ivalue;
   double dvalue;
   stringpart *strptr = NULL, *newpart;

   if (ops == NULL) {
      Tcl_SetResult(interp, "Cannot set parameter value", NULL);
      return TCL_ERROR;
   }
   switch (ops->type) {
      case XC_FLOAT:
	 result = Tcl_GetDoubleFromObj(interp, objv, &dvalue);
	 if (result != TCL_OK) return result;
	 ops->parameter.fvalue = (float)dvalue;
	 break;
      case XC_INT:
	 result = Tcl_GetIntFromObj(interp, objv, &ivalue);
	 if (result != TCL_OK) return result;
	 ops->parameter.ivalue = ivalue;
	 break;
      case XC_EXPR:
	 ops->parameter.expr = strdup(Tcl_GetString(objv));
	 break;
      case XC_STRING:
	 result = GetXCStringFromList(interp, objv, &strptr);
	 if (result != TCL_OK) return result;
	 freelabel(ops->parameter.string);
	 /* Must add a "param end" */
         newpart = makesegment(&strptr, NULL);
         newpart->nextpart = NULL;
         newpart->type = PARAM_END;
	 newpart->data.string = (u_char *)NULL;
	 ops->parameter.string = strptr;
	 break;
   }
   return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* Translate the numeric parameter types to a string that the Tcl	*/
/* "parameter" routine will recognize from the command line.		*/
/*----------------------------------------------------------------------*/

char *
translateparamtype(int type)
{
   const char *param_types[] = {"numeric", "substring", "x position",
        "y position", "style", "anchoring", "start angle", "end angle",
        "radius", "minor axis", "rotation", "scale", "linewidth", "color",
	 "expression", "position", NULL};

   if (type < 0) return NULL;
   return (char *)param_types[type];
}

/*----------------------------------------------------------------------*/
/* Parameter command:							*/
/*									*/
/* Normally, a selected element will produce a list of backwards-	*/
/* referenced parameters (eparam).  However, it is useful to pick up	*/
/* the forwards-referenced parameters of an object instance, so that	*/
/* parameters can be modified from the level above (e.g., to change	*/
/* circuit component values, component indices, etc.).  The optional	*/
/* final argument "-forward" can be used to access this mode.		*/
/*----------------------------------------------------------------------*/

int xctcl_param(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int i, j, value, idx, nidx = 4;
   int result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);
   oparamptr ops, instops;
   oparam temps;
   eparamptr epp;
   genericptr thiselem = NULL;
   Tcl_Obj *plist, *kpair, *exprres;
   objinstptr refinst;
   objectptr refobj;
   char *dash_opt;
   Boolean verbatim = FALSE, indirection = FALSE, forwarding = FALSE;

   static char *subCmds[] = {"allowed", "get", "type", "default", "set", "make",
	"replace", "forget", "delete", NULL};
   enum SubIdx {
      AllowedIdx, GetIdx, TypeIdx, DefaultIdx, SetIdx, MakeIdx, ReplaceIdx,
      ForgetIdx, DeleteIdx
   };

   /* The order of these type names must match the enumeration in xcircuit.h	*/

   static char *param_types[] = {"numeric", "substring", "x position",
        "y position", "style", "anchoring", "start angle", "end angle",
        "radius", "minor axis", "rotation", "scale", "linewidth", "color",
	 "expression", "position", NULL};  /* (jdk) */

   /* The first object instance in the select list becomes "thiselem",	*/
   /* if such exists.  Otherwise, it remains null.			*/

   for (j = 0; j < areawin->selects; j++) {
      if (SELECTTYPE(areawin->selectlist + j) == OBJINST) {
	 thiselem = SELTOGENERIC(areawin->selectlist + j);
	 break;
      }
   }

   if (objc - nidx < 1)
      idx = GetIdx;
   else {
      dash_opt = Tcl_GetString(objv[nidx]);
      if (*dash_opt == '-')
	 idx = GetIdx;
      else {
	if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds, "option", 0, &idx)) != TCL_OK)
	   return result;
      }
   }

   /* Use the topobject by default */
   refinst = areawin->topinstance;
   refobj = topobject;

   /* command-line switches */

   dash_opt = Tcl_GetString(objv[objc - 1]);
   while (*dash_opt == '-') {

      /* If an object instance is selected, we list backwards-referenced */
      /* (eparam) parameters, unless the command ends in "-forward".	 */

      if (!strncmp(dash_opt + 1, "forw", 4)) {
	 switch (idx) {
	    case SetIdx:
	    case GetIdx:
	    case TypeIdx:
	    case MakeIdx:
	    case DeleteIdx:
	    case ForgetIdx:
	    case DefaultIdx:
	       if (thiselem && IS_OBJINST(thiselem)) {
		  refinst = (objinstptr)thiselem;
		  refobj = refinst->thisobject;
		  thiselem = NULL;
		  forwarding = TRUE;
	       }
	    break;
	 }
      }
      else if (!strncmp(dash_opt + 1, "verb", 4)) {
	 verbatim = TRUE;
      }
      else if (!strncmp(dash_opt + 1, "ind", 3)) {
	 indirection = TRUE;
      }

      objc--;
      if (objc == 0) {
	 Tcl_SetResult(interp, "Must have a valid option", NULL);
	 return TCL_ERROR;
      }
      dash_opt = Tcl_GetString(objv[objc - 1]);
   }


   switch (idx) {
      case AllowedIdx:
	 for (i = 0; i < (sizeof(param_types) / sizeof(char *)); i++)
	    if ((thiselem == NULL) || (param_select[i] & thiselem->type))
	       Tcl_AppendElement(interp, param_types[i]);

         break;

      case GetIdx:
      case TypeIdx:

	 if (objc == nidx + 2) {

	    /* Check argument against all parameter keys */
	    ops = find_param(refinst, Tcl_GetString(objv[nidx + 1]));
	    if (ops == NULL) {
	       /* Otherwise, the argument must be a parameter type. */
               if ((result = Tcl_GetIndexFromObj(interp, objv[nidx + 1],
		   	(CONST84 char **)param_types, "parameter type",
			0, &value)) != TCL_OK) {
	          Tcl_SetResult(interp, "Must have a valid key or parameter type",
			NULL);
	          return result;
	       }
	    }

	    /* Return the value of the indicated parameter  */

	    plist = Tcl_NewListObj(0, NULL);
	    if (thiselem == NULL) {
	       if (ops != NULL) {
		  if (idx == GetIdx)
		     Tcl_ListObjAppendElement(interp, plist,
			   	GetParameterValue(refobj, ops, verbatim, refinst));
		  else
	             Tcl_ListObjAppendElement(interp, plist,
				Tcl_NewStringObj(param_types[ops->which],
				strlen(param_types[ops->which])));
	       }
	       else {
		  for (ops = refobj->params; ops != NULL; ops = ops->next) {
		     instops = find_param(refinst, ops->key);
		     if (instops->which == value) {
	       	        kpair = Tcl_NewListObj(0, NULL);
	                Tcl_ListObjAppendElement(interp, kpair,
			   	Tcl_NewStringObj(instops->key, strlen(instops->key)));
			if (idx == GetIdx)
		           Tcl_ListObjAppendElement(interp, kpair,
				   	GetParameterValue(refobj, instops, verbatim,
							refinst));
			else
		           Tcl_ListObjAppendElement(interp, kpair,
					Tcl_NewStringObj(param_types[instops->which],
					strlen(param_types[instops->which])));
	                Tcl_ListObjAppendElement(interp, plist, kpair);
		     }
	          }
	       }
	    }
	    else {
	       for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
		  instops = find_param(refinst, epp->key);
		  if (instops->which == value) {
		     if (idx == GetIdx)
		        Tcl_ListObjAppendElement(interp, plist,
				GetParameterValue(refobj, instops, verbatim, refinst));
		     else
		        Tcl_ListObjAppendElement(interp, plist,
				Tcl_NewStringObj(param_types[instops->which],
				strlen(param_types[instops->which])));
		  }
	       }

	       /* Search label for parameterized substrings.  These are	*/
	       /* backwards-referenced parameters, although they are 	*/
	       /* not stored in the eparam record of the label.		*/

	       if ((value == P_SUBSTRING) && IS_LABEL(thiselem)) {
		  stringpart *cstr;
		  labelptr clab = (labelptr)thiselem;
		  for (cstr = clab->string; cstr != NULL; cstr = cstr->nextpart) {
		     if (cstr->type == PARAM_START) {
	       	        kpair = Tcl_NewListObj(0, NULL);
			ops = find_param(refinst, cstr->data.string);
	                Tcl_ListObjAppendElement(interp, kpair,
			   	Tcl_NewStringObj(ops->key, strlen(ops->key)));
			if (idx == GetIdx)
		           Tcl_ListObjAppendElement(interp, kpair,
					GetParameterValue(refobj, ops, verbatim,
					refinst));
			else
		           Tcl_ListObjAppendElement(interp, kpair,
					Tcl_NewStringObj(param_types[ops->which],
					strlen(param_types[ops->which])));
	                Tcl_ListObjAppendElement(interp, plist, kpair);
		     }
		  }
	       }
	    }
	    Tcl_SetObjResult(interp, plist);
	 }
	 else {
	    plist = Tcl_NewListObj(0, NULL);
	    if (thiselem == NULL) {
	       for (ops = refobj->params; ops != NULL; ops = ops->next) {
	       	  kpair = Tcl_NewListObj(0, NULL);
	          Tcl_ListObjAppendElement(interp, kpair,
		     Tcl_NewStringObj(ops->key, strlen(ops->key)));
		  if (idx == GetIdx) {
		     instops = find_param(refinst, ops->key);
		     Tcl_ListObjAppendElement(interp, kpair,
				GetParameterValue(refobj, instops, verbatim, refinst));
		  }
		  else
	             Tcl_ListObjAppendElement(interp, kpair,
				Tcl_NewStringObj(param_types[ops->which],
				strlen(param_types[ops->which])));
	          Tcl_ListObjAppendElement(interp, plist, kpair);
	       }
	    }
	    else {
	       for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
		  kpair = Tcl_NewListObj(0, NULL);
		  ops = find_param(refinst, epp->key);
	          Tcl_ListObjAppendElement(interp, kpair,
			Tcl_NewStringObj(ops->key, strlen(ops->key)));
		  if (idx == GetIdx)
		     Tcl_ListObjAppendElement(interp, kpair,
				GetParameterValue(refobj, ops, verbatim, refinst));
		  else
	             Tcl_ListObjAppendElement(interp, kpair,
			   Tcl_NewStringObj(param_types[ops->which],
			   strlen(param_types[ops->which])));
	          Tcl_ListObjAppendElement(interp, plist, kpair);
	       }

	       /* Search label for parameterized substrings.  These are	*/
	       /* backwards-referenced parameters, although they are 	*/
	       /* not stored in the eparam record of the label.		*/

	       if (IS_LABEL(thiselem)) {
		  stringpart *cstr;
		  labelptr clab = (labelptr)thiselem;
		  for (cstr = clab->string; cstr != NULL; cstr = cstr->nextpart) {
		     if (cstr->type == PARAM_START) {
	       	        kpair = Tcl_NewListObj(0, NULL);
			ops = find_param(refinst, cstr->data.string);
	                Tcl_ListObjAppendElement(interp, kpair,
			   	Tcl_NewStringObj(ops->key, strlen(ops->key)));
			if (idx == GetIdx)
		           Tcl_ListObjAppendElement(interp, kpair,
					GetParameterValue(refobj, ops, verbatim,
					refinst));
			else
		           Tcl_ListObjAppendElement(interp, kpair,
					Tcl_NewStringObj(param_types[ops->which],
					strlen(param_types[ops->which])));
	                Tcl_ListObjAppendElement(interp, plist, kpair);
		     }
		  }
	       }
	    }
	    Tcl_SetObjResult(interp, plist);
	 }
         break;

      case DefaultIdx:
	 if (objc == nidx + 2) {
	    /* Check against keys */
	    ops = match_param(refobj, Tcl_GetString(objv[nidx + 1]));
	    if (ops == NULL) {
               if ((result = Tcl_GetIndexFromObj(interp, objv[nidx + 1],
			(CONST84 char **)param_types, "parameter type",
			0, &value)) != TCL_OK) {
	          Tcl_SetResult(interp, "Must have a valid key or parameter type",
			NULL);
	          return result;
	       }
	    }
	    else {		/* get default value(s) */
	       plist = Tcl_NewListObj(0, NULL);
	       if (thiselem == NULL) {
		  if (ops != NULL) {
		     Tcl_ListObjAppendElement(interp, plist,
				GetParameterValue(refobj, ops, verbatim, refinst));
		  }
		  else {
		     for (ops = refobj->params; ops != NULL; ops = ops->next) {
		        if (ops->which == value) {
		           Tcl_ListObjAppendElement(interp, plist,
				GetParameterValue(refobj, ops, verbatim, refinst));
			}
		     }
	          }
	       }
	       else {
		  for (epp = thiselem->passed; epp != NULL; epp = epp->next) {
		     ops = match_param(refobj, epp->key);
		     if (ops->which == value) {
		        Tcl_ListObjAppendElement(interp, plist,
				GetParameterValue(refobj, ops, verbatim, refinst));
		     }
		  }

		  /* search label for parameterized substrings */

		  if ((value == P_SUBSTRING) && IS_LABEL(thiselem)) {
		     stringpart *cstr;
		     labelptr clab = (labelptr)thiselem;
		     for (cstr = clab->string; cstr != NULL; cstr = cstr->nextpart) {
			if (cstr->type == PARAM_START) {
			   ops = match_param(refobj, cstr->data.string);
			   if (ops != NULL)
		              Tcl_ListObjAppendElement(interp, plist,
					GetParameterValue(refobj, ops, verbatim,
					refinst));
			}
		     }
		  }
	       }
	       Tcl_SetObjResult(interp, plist);
	    }
	 }
	 else if (objc == nidx + 1) {	/* list all parameters and their defaults */
	    plist = Tcl_NewListObj(0, NULL);
	    for (ops = refobj->params; ops != NULL; ops = ops->next) {
	       kpair = Tcl_NewListObj(0, NULL);
	       Tcl_ListObjAppendElement(interp, kpair,
			Tcl_NewStringObj(ops->key, strlen(ops->key)));
	       Tcl_ListObjAppendElement(interp, kpair,
			GetParameterValue(refobj, ops, verbatim, refinst));
	       Tcl_ListObjAppendElement(interp, plist, kpair);
	    }
	    Tcl_SetObjResult(interp, plist);
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "default <type|key> [<value>]");
	    return TCL_ERROR;
	 }
	 break;

      case SetIdx:			/* currently, instances only. . .*/
	 if (objc == nidx + 3) {	/* possibly to be expanded. . .	 */
	    char *key = Tcl_GetString(objv[nidx + 1]);
	    objinstptr searchinst = NULL;

	    /* Allow option "set" to act on more than one selection */

	    if (areawin->selects == 0) goto keycheck;

	    while (j < areawin->selects) {

	       refinst = SELTOOBJINST(areawin->selectlist + j);
	       refobj = refinst->thisobject;

	       /* Check against keys */
keycheck:
	       instops = match_instance_param(refinst, key);
	       ops = match_param(refobj, key);
	       if (instops == NULL) {
	          if (ops == NULL) {
		     if (!forwarding || (areawin->selects <= 1)) {
			Tcl_SetResult(interp, "Invalid key ", NULL);
			Tcl_AppendElement(interp, key);
			return TCL_ERROR;
		     }
		     else
			goto next_param;
	          }
	          copyparams(refinst, refinst);
	          instops = match_instance_param(refinst, key);
	       }
	       else if (ops->type == XC_EXPR) {
	          /* If the expression is currently the default expression	*/
	          /* but the instance value is holding the last evaluated	*/
	          /* result, then we have to delete and regenerate the		*/
	          /* existing instance parameter ("verbatim" assumed even	*/
	          /* if not declared because you can't change the result	*/
	          /* of the expression).					*/

	          free_instance_param(refinst, instops);
	          instops = copyparameter(ops);
	          instops->next = refinst->params;
	          refinst->params = instops;
	       }
	       if (indirection) {
	          char *refkey = Tcl_GetString(objv[nidx + 2]);

	          if (refinst != areawin->topinstance)
		     searchinst = areawin->topinstance;
	          else if (areawin->stack) {
		     searchinst = areawin->stack->thisinst;
	          }
	          else {
		     resolveparams(refinst);
		     Tcl_SetResult(interp, "On top-level page:  "
				"no indirection possible!", NULL);
		     return TCL_ERROR;
	          }
	          if (match_param(searchinst->thisobject, refkey) == NULL) {
		     resolveparams(refinst);
	             Tcl_SetResult(interp, "Invalid indirect reference key", NULL);
	             return TCL_ERROR;
	          }
	          /* Create an eparam record in the instance */
	          epp = make_new_eparam(refkey);
		  epp->flags |= P_INDIRECT;
	          epp->pdata.refkey = strdup(key);
	          epp->next = refinst->passed;
	          refinst->passed = epp;
	       }
	       else
	          SetParameterValue(interp, instops, objv[nidx + 2]);
	       resolveparams(refinst);

	       /* Check if there are more selections to modify */

next_param:
	       if (!forwarding) break;
	       while (++j != areawin->selects)
		  if (SELECTTYPE(areawin->selectlist + j) == OBJINST)
		     break;
	    }

	    /* Redraw everything (this could be finessed. . .) */
	    areawin->redraw_needed = True;
	    drawarea(areawin->area, (caddr_t)NULL, (caddr_t)NULL);
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "set <key>");
	    return TCL_ERROR;
	 }
         break;

      case MakeIdx:
	 if (objc >= (nidx + 2) && objc <= (nidx + 4)) {
            if ((result = Tcl_GetIndexFromObj(interp, objv[nidx + 1],
			(CONST84 char **)param_types, "parameter type",
			0, &value)) != TCL_OK)
	       return result;

	    if ((value == P_SUBSTRING) && (objc == (nidx + 4))) {
	       stringpart *strptr = NULL, *newpart;
	       result = GetXCStringFromList(interp, objv[nidx + 3], &strptr);
	       if (result != TCL_ERROR) {
	          if (makestringparam(refobj, Tcl_GetString(objv[nidx + 2]),
				strptr) == -1)
		     return TCL_ERROR;
		  /* Add the "parameter end" marker to this string */
		  newpart = makesegment(&strptr, NULL);
		  newpart->nextpart = NULL;
		  newpart->type = PARAM_END;
		  newpart->data.string = (u_char *)NULL;
	       }
	    }
	    else if (value == P_SUBSTRING) {
	       /* Get parameter value from selection */
	       startparam((Tk_Window)clientData, (pointertype)value,
			(caddr_t)Tcl_GetString(objv[nidx + 2]));
	    }
	    else if ((value == P_EXPRESSION) && (objc == (nidx + 4))) {
 	       temps.type = XC_EXPR;
	       temps.parameter.expr = Tcl_GetString(objv[nidx + 3]);
	       exprres = evaluate_raw(refobj, &temps, refinst, &result);

	       if (result != TCL_OK) {
		  Tcl_SetResult(xcinterp, "Bad result from expression!", NULL);
		  /* Not fatal to have a bad expression result. . . */
		  /* return result; */
	       }
	       if (makeexprparam(refobj, Tcl_GetString(objv[nidx + 2]),
				temps.parameter.expr, P_EXPRESSION) == NULL)
		  return TCL_ERROR;
	    }

	    /* All other types are parsed as either a numeric value	*/
	    /* (integer or float), or an expression that evaluates	*/
	    /* to a numeric value.					*/

	    else if (((value == P_NUMERIC) && (objc == (nidx + 4))) ||
			objc == (nidx + 3)) {
	       double tmpdbl;

	       i = (value == P_NUMERIC) ? 3 : 2;

	       result = Tcl_GetDoubleFromObj(interp, objv[nidx + i], &tmpdbl);
	       if (result != TCL_ERROR) {
		  if (makefloatparam(refobj, Tcl_GetString(objv[nidx + i - 1]),
				(float)tmpdbl) == -1)
		     return TCL_ERROR;
	       }
	       else {
		  char *newkey;

		  /* This may be an expression.   Do a quick check to	*/
		  /* see if the	string can be evaluated as a Tcl	*/
		  /* expression.  If it returns a valid numeric result,	*/
		  /* then accept the expression.			*/

		  Tcl_ResetResult(interp);
		  temps.type = XC_EXPR;
		  temps.parameter.expr = Tcl_GetString(objv[nidx + i]);

		  exprres = evaluate_raw(refobj, &temps, refinst, &result);
		  if (result != TCL_OK) {
		     Tcl_SetResult(xcinterp, "Bad result from expression!", NULL);
		     return result;
		  }
		  result = Tcl_GetDoubleFromObj(interp, exprres, &tmpdbl);
		  if (result != TCL_ERROR) {
		     if ((newkey = makeexprparam(refobj, (value == P_NUMERIC) ?
				Tcl_GetString(objv[nidx + i - 1]) : NULL,
				temps.parameter.expr, value)) == NULL)
			return TCL_ERROR;
		     else if (value != P_NUMERIC) {
			/* Link the expression parameter to the element */
			/* To-do:  Handle cycles (one extra argument)	*/
			genericptr pgen;
			for (i = 0; i < areawin->selects; i++) {
			   pgen = SELTOGENERIC(areawin->selectlist + i);
			   makenumericalp(&pgen, value, newkey, 0);
			}
		     }
		  }
		  else {
		     Tcl_SetResult(xcinterp, "Expression evaluates to "
				"non-numeric type!", NULL);
		     return result;
		  }
	       }
	    }
	    else if (((value != P_NUMERIC) && (objc == (nidx + 4))) ||
			objc == (nidx + 3)) {
	       int cycle;
	       i = objc - 1;
	       if (value == P_POSITION || value == P_POSITION_X ||
			value == P_POSITION_Y) {
		  if (objc == nidx + 4) {
		     result = Tcl_GetIntFromObj(interp, objv[i - 1], &cycle);
		     if (result == TCL_ERROR) {
		        Tcl_ResetResult(interp);
                        startparam((Tk_Window)clientData, (pointertype)value,
				Tcl_GetString(objv[i]));
		     }
	   	     else {
		        parameterize(value, NULL, (short)cycle);
		     }
	          }
		  else {
		     Tcl_WrongNumArgs(interp, 1, objv, "make position cycle <value>");
		     return TCL_ERROR;
		  }
	       }
	       else {
		  if (objc == nidx + 3)
                     startparam((Tk_Window)clientData, (pointertype)value,
				Tcl_GetString(objv[i]));
		  else {
		     Tcl_WrongNumArgs(interp, 1, objv, "make <numeric_type> <value>");
		     return TCL_ERROR;
		  }
	       }
	    }
	    else {
	       if ((value == P_SUBSTRING) || (value == P_NUMERIC) ||
			(value == P_EXPRESSION)) {
		  Tcl_WrongNumArgs(interp, 1, objv,
				"make substring|numeric|expression <key>");
		  return TCL_ERROR;
	       }
	       else
                  startparam((Tk_Window)clientData, (pointertype)value, NULL);
	    }
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "make <type> [<key>]");
	    return TCL_ERROR;
	 }
         break;

      case ReplaceIdx:
	 /* Calls unparameterize---replaces text with the instance value, */
	 /* or replaces a numeric parameter with the instance values by   */
	 /* unparameterizing the element.  Don't use with parameter keys. */

	 if (objc == nidx + 2) {
            if ((result = Tcl_GetIndexFromObj(interp, objv[nidx + 1],
			(CONST84 char **)param_types, "parameter type",
			0, &value)) != TCL_OK)
	       return result;
            unparameterize(value);
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "replace <type>");
	    return TCL_ERROR;
	 }
         break;

      case DeleteIdx:
      case ForgetIdx:

	 if (objc == nidx + 2) {
	    /* Check against keys */
	    ops = match_param(refobj, Tcl_GetString(objv[nidx + 1]));
	    if (ops == NULL) {
	       Tcl_SetResult(interp, "Invalid parameter key", NULL);
	       return TCL_ERROR;
	    }
	    else {
	       free_object_param(refobj, ops);
	       /* Redraw everything */
	       drawarea(areawin->area, (caddr_t)NULL, (caddr_t)NULL);
	    }
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "forget <key>");
	    return TCL_ERROR;
	 }
         break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_select(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   char *argstr;
   short *newselect;
   int selected_prior, selected_new, nidx, result;
   Tcl_Obj *listPtr;
   XPoint newpos;

   if (objc == 1) {
      /* Special case: "select" by itself returns the number of	*/
      /* selected objects.					*/
      Tcl_SetObjResult(interp, Tcl_NewIntObj((int)areawin->selects));
      return XcTagCallback(interp, objc, objv);
   }
   else {
      nidx = 1;
      result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);
      if (result != TCL_OK) return result;
   }

   if (objc != 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "here | get | <element_handle>");
      return TCL_ERROR;
   }

   if (nidx == 1) {
      argstr = Tcl_GetString(objv[1]);
      if (!strcmp(argstr, "here")) {
         newpos = UGetCursorPos();
         areawin->save = newpos;
         selected_prior = areawin->selects;
         newselect = select_element(ALL_TYPES);
         selected_new = areawin->selects - selected_prior;
      }
      else if (!strcmp(argstr, "get")) {
         newselect = areawin->selectlist;
         selected_new = areawin->selects;
      }
      else {
         Tcl_WrongNumArgs(interp, 1, objv, "here | get | <object_handle>");
	 return TCL_ERROR;
      }

      listPtr = SelectToTclList(interp, newselect, selected_new);
      Tcl_SetObjResult(interp, listPtr);
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_deselect(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int i, j, k, result, numobjs;
   pointertype ehandle;
   char *argstr;
   Tcl_Obj *lobj;

   if (objc > 3) {
      Tcl_WrongNumArgs(interp, 1, objv, "[element_handle]");
      return TCL_ERROR;
   }
   else if (objc == 3 || (objc == 2 && !strcmp(Tcl_GetString(objv[0]), "deselect"))) {

      argstr = Tcl_GetString(objv[1]);
      if (strcmp(argstr, "selected")) {

         /* check for object handles (integer list) */

         result = Tcl_ListObjLength(interp, objv[1], &numobjs);
         if (result != TCL_OK) return result;

	 for (j = 0; j < numobjs; j++) {
            result = Tcl_ListObjIndex(interp, objv[1], j, &lobj);
            if (result != TCL_OK) return result;
	    result = Tcl_GetHandleFromObj(interp, lobj, (void *)&ehandle);
            if (result != TCL_OK) return result;
            i = GetPartNumber((genericptr)ehandle, topobject, ALL_TYPES);
            if (i == -1) {
	       Tcl_SetResult(interp, "No such element exists.", NULL);
	       return TCL_ERROR;
            }
	    for (i = 0; i < areawin->selects; i++) {
	       short *newselect = areawin->selectlist + i;
	       if ((genericptr)ehandle == SELTOGENERIC(newselect)) {
		  XTopSetForeground(SELTOCOLOR(newselect));
		  geneasydraw(*newselect, DEFAULTCOLOR, topobject,
			areawin->topinstance);

		  areawin->selects--;
		  for (k = i; k < areawin->selects; k++)
		      *(areawin->selectlist + k) = *(areawin->selectlist + k + 1);
		  if (areawin->selects == 0) {
		     free(areawin->selectlist);
		     freeselects();  	/* specifically, free hierstack */
		  }
	       }
	    }
	 }
      }
      else
	 unselect_all();
   }
   else
      startdesel((Tk_Window)clientData, NULL, NULL);

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_push(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result = ParseElementArguments(interp, objc, objv, NULL, OBJINST);

   if (result != TCL_OK) return result;

   pushobject(NULL);

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_pop(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   if (objc != 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "(no arguments)");
      return TCL_ERROR;
   }
   popobject((Tk_Window)clientData, 0, NULL);

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Object queries							*/
/*----------------------------------------------------------------------*/

int xctcl_object(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
  int i, j, idx, result, nidx, libno;
   genericptr egen;
   Tcl_Obj **newobjv, *ilist, *plist, *hobj;
   pointertype ehandle;
   objinstptr thisinst;
   Boolean forceempty = FALSE;

   static char *subCmds[] = {"make", "name", "parts", "library",
	"handle", "hide", "unhide", "bbox", NULL};
   enum SubIdx {
      MakeIdx, NameIdx, PartsIdx, LibraryIdx, HandleIdx, HideIdx,
	UnhideIdx, BBoxIdx
   };

   /* Check for option "-force" (create an object even if it has no contents) */
   if (!strncmp(Tcl_GetString(objv[objc - 1]), "-forc", 5)) {
      forceempty = TRUE;
      objc--;
   }

   /* (revision) "object handle <name>" returns a handle (or null), so	*/
   /* all commands can unambiguously operate on a handle (or nothing)	*/
   /* in the second position.						*/

   nidx = 0;

   /* 2nd argument may be a handle, object name, or nothing.	 */
   /* If nothing, the instance of the top-level page is assumed. */

   if (objc < 2) {
      Tcl_WrongNumArgs(interp, 0, objv, "object [handle] <option> ...");
      return TCL_ERROR;
   }

   result = Tcl_GetHandleFromObj(interp, objv[1], (void *)&ehandle);
   if (result != TCL_OK) {
      Tcl_ResetResult(interp);
      ehandle = (pointertype)(areawin->topinstance);
   }
   else {
      nidx = 1;
      objc--;
   }
   egen = (genericptr)ehandle;

   if (ELEMENTTYPE(egen) != OBJINST) {
      Tcl_SetResult(interp, "handle does not point to an object instance!", NULL);
      return TCL_ERROR;
   }
   if (objc < 2) {
      Tcl_WrongNumArgs(interp, 0, objv, "object <handle> <option> ...");
      return TCL_ERROR;
   }
   thisinst = (objinstptr)egen;

   if ((result = Tcl_GetIndexFromObj(interp, objv[1 + nidx],
		(CONST84 char **)subCmds, "option", 0, &idx)) != TCL_OK)
      return result;

   switch (idx) {
      case LibraryIdx:
      case HideIdx:
      case UnhideIdx:

	 if ((libno = libfindobject(thisinst->thisobject, &j)) < 0) {
	    Tcl_SetResult(interp, "No such object.", NULL);
	    return TCL_ERROR;
	 }
	 break;
   }

   switch (idx) {
      case BBoxIdx:
	 ilist = Tcl_NewListObj(0, NULL);
	 hobj = Tcl_NewIntObj((int)thisinst->thisobject->bbox.lowerleft.x);
	 Tcl_ListObjAppendElement(interp, ilist, hobj);
	 hobj = Tcl_NewIntObj((int)thisinst->thisobject->bbox.lowerleft.y);
	 Tcl_ListObjAppendElement(interp, ilist, hobj);
	 hobj = Tcl_NewIntObj((int)(thisinst->thisobject->bbox.lowerleft.x +
		thisinst->thisobject->bbox.width));
	 Tcl_ListObjAppendElement(interp, ilist, hobj);
	 hobj = Tcl_NewIntObj((int)(thisinst->thisobject->bbox.lowerleft.y +
		thisinst->thisobject->bbox.height));
	 Tcl_ListObjAppendElement(interp, ilist, hobj);
	 Tcl_SetObjResult(interp, ilist);
	 break;

      case HandleIdx:
	 if ((objc == 3) && (!NameToObject(Tcl_GetString(objv[nidx + 2]),
			(objinstptr *)&ehandle, TRUE))) {
	    Tcl_SetResult(interp, "Object is not loaded.", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetObjResult(interp, Tcl_NewHandleObj((genericptr)ehandle));
	 }
         break;

      case LibraryIdx:
	 if (objc == 3) {
	    int libtarget;
	    if (ParseLibArguments(xcinterp, 2, &objv[objc - 2 + nidx], NULL,
			&libtarget) == TCL_ERROR)
	       return TCL_ERROR;
	    else if (libno != libtarget) {
	       libmoveobject(thisinst->thisobject, libtarget);
	       /* Regenerate the source and target library pages */
	       composelib(libno + LIBRARY);
	       composelib(libtarget + LIBRARY);
	    }
	 }
	 Tcl_SetObjResult(interp, Tcl_NewIntObj(libno + 1));
	 break;

      case HideIdx:
	 thisinst->thisobject->hidden = True;
	 composelib(libno + LIBRARY);
         break;

      case UnhideIdx:
	 thisinst->thisobject->hidden = False;
	 composelib(libno + LIBRARY);
         break;

      case MakeIdx:

	 if ((areawin->selects == 0) && (nidx == 0)) {
	    /* h = object make "name" [{element_list}] [library]*/
	    newobjv = (Tcl_Obj **)(&objv[2]);
	    result = ParseElementArguments(interp, objc - 2, newobjv, NULL, ALL_TYPES);
	    if (forceempty && result != TCL_OK) Tcl_ResetResult(interp);
	    else if (!forceempty && result == TCL_OK && areawin->selects == 0)
	    {
		Tcl_SetResult(interp, "Cannot create empty object.  Use "
			"\"-force\" option.", NULL);
		return TCL_ERROR;
	    }
	    else if (result != TCL_OK) return result;
	 }
	 else if (nidx == 1) {
	    Tcl_SetResult(interp, "\"object <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "make <name> [element_list] [<library>]");
	    return TCL_ERROR;
	 }
	 if (objc >= 4)
	    ParseLibArguments(xcinterp, 2, &objv[objc - 2], NULL, &libno);
	 else
	    libno = -1;
	 thisinst = domakeobject(libno, Tcl_GetString(objv[nidx + 2]), forceempty);
	 Tcl_SetObjResult(interp, Tcl_NewHandleObj(thisinst));
	 break;

      case NameIdx:
	 if (nidx == 1 || areawin->selects == 0) {
	    if (objc == 3) {
	       sprintf(thisinst->thisobject->name, "%s", Tcl_GetString(objv[nidx + 2]));
	       checkname(thisinst->thisobject);
	    }
	    Tcl_AppendElement(interp, thisinst->thisobject->name);
	 }
	 else {
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  thisinst = SELTOOBJINST(areawin->selectlist + i);
	          Tcl_AppendElement(interp, thisinst->thisobject->name);
	       }
	    }
	 }
	 break;
      case PartsIdx:
	 /* Make a list of the handles of all parts in the object */
	 if (nidx == 1 || areawin->selects == 0) {
	    plist = Tcl_NewListObj(0, NULL);
	    for (j = 0; j < thisinst->thisobject->parts; j++) {
	       hobj = Tcl_NewHandleObj(*(thisinst->thisobject->plist + j));
	       Tcl_ListObjAppendElement(interp, plist, hobj);
	    }
	    Tcl_SetObjResult(interp, plist);
	 }
	 else {
	    ilist = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  objinstptr thisinst = SELTOOBJINST(areawin->selectlist + i);
	          Tcl_ListObjAppendElement(interp, ilist,
			Tcl_NewStringObj(thisinst->thisobject->name,
			strlen(thisinst->thisobject->name)));
		  plist = Tcl_NewListObj(0, NULL);
		  for (j = 0; j < thisinst->thisobject->parts; j++) {
		     hobj = Tcl_NewHandleObj(*(thisinst->thisobject->plist + j));
		     Tcl_ListObjAppendElement(interp, plist, hobj);
		  }
		  Tcl_ListObjAppendElement(interp, ilist, plist);
	       }
	    }
	    Tcl_SetObjResult(interp, ilist);
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Get anchoring (or associated fields) global setting, or apply	*/
/* to selected labels.							*/
/*----------------------------------------------------------------------*/

int
getanchoring(Tcl_Interp *interp, short bitfield)
{
   int i, rval;
   labelptr tlab;

   if (areawin->selects == 0) {
      if (bitfield & RIGHT) {
	 Tcl_AppendElement(interp, (areawin->anchor & RIGHT) ?
		"right" : (areawin->anchor & NOTLEFT) ? "center" : "left");
      }
      else if (bitfield & TOP) {
	 Tcl_AppendElement(interp, (areawin->anchor & TOP) ?
		"top" : (areawin->anchor & NOTBOTTOM) ? "middle" : "bottom");
      }
      else if (bitfield & JUSTIFYRIGHT) {
	 Tcl_AppendElement(interp, (areawin->anchor & JUSTIFYRIGHT) ? "right" :
		(areawin->anchor & TEXTCENTERED) ? "center" :
		(areawin->anchor & JUSTIFYBOTH) ? "both" :
		"left");
      }
      else {
	 Tcl_AppendElement(interp, (areawin->anchor & bitfield) ?
			"true" : "false");
      }
      return (areawin->anchor & bitfield);
   }
   for (i = 0; i < areawin->selects; i++) {
      if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
      tlab = SELTOLABEL(areawin->selectlist + i);
      if (bitfield == PINVISIBLE && tlab->pin == NORMAL) continue;
      if (bitfield & RIGHT) {
	 Tcl_AppendElement(interp, (tlab->anchor & RIGHT) ?
		"right" : (tlab->anchor & NOTLEFT) ? "center" : "left");
      }
      else if (bitfield & TOP) {
	 Tcl_AppendElement(interp, (tlab->anchor & TOP) ?
		"top" : (tlab->anchor & NOTBOTTOM) ? "middle" : "bottom");
      }
      else if (bitfield & JUSTIFYRIGHT) {
	 Tcl_AppendElement(interp, (tlab->anchor & JUSTIFYRIGHT) ? "right" :
		(tlab->anchor & TEXTCENTERED) ? "center" :
		(tlab->anchor & JUSTIFYBOTH) ? "both" :
		"left");
      }
      else {
         Tcl_AppendElement(interp, (tlab->anchor & bitfield) ?  "true" : "false");
      }
      rval = tlab->anchor;
   }
   return (rval & bitfield);
}


/*----------------------------------------------------------------------*/
/* Set anchoring (and associated fields) global setting, or apply	*/
/* to selected labels.							*/
/*----------------------------------------------------------------------*/

void
setanchoring(short bitfield, short value)
{
   int i;
   labelptr tlab;

   if (areawin->selects == 0) {
      areawin->anchor &= (~bitfield);
      if (value > 0) areawin->anchor |= value;
      return;
   }
   for (i = 0; i < areawin->selects; i++) {
      if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
      tlab = SELTOLABEL(areawin->selectlist + i);
      if (bitfield == PINVISIBLE && tlab->pin == NORMAL) continue;
      tlab->anchor &= (~bitfield);
      if (value > 0) tlab->anchor |= value;
   }
}

/*----------------------------------------------------------------------*/
/* Translate the label encoding bits to a string that the Tcl routine	*/
/* will recognize from the command line.				*/
/*									*/
/* (note to self---is there a good way to not have to declare these	*/
/* constant character arrays twice in two different routines?)		*/
/*----------------------------------------------------------------------*/

char *
translateencoding(int psfont)
{
   const char *encValues[] = {"Standard", "special", "ISOLatin1",
	"ISOLatin2", "ISOLatin3", "ISOLatin4", "ISOLatin5",
	"ISOLatin6", "ISO8859-5", NULL};
   int i;

   i = (fonts[psfont].flags & 0xf80) >> 7;
   if (i < 0) return NULL;
   return (char *)encValues[i];
}

/*----------------------------------------------------------------------*/
/* Translate the label style bits to a string that the Tcl routine	*/
/* will recognize from the command line.				*/
/*----------------------------------------------------------------------*/

char *
translatestyle(int psfont)
{
   const char *styValues[] = {"normal", "bold", "italic", "bolditalic", NULL};
   int i;

   i = fonts[psfont].flags & 0x3;
   if (i < 0) return NULL;
   return (char *)styValues[i];
}

/*----------------------------------------------------------------------*/
/* Individual element handling.						*/
/*----------------------------------------------------------------------*/

int xctcl_label(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int i, idx, idx2, nidx, result, value, jval, jval2;
   double tmpdbl;
   char *tmpstr;
   Tcl_Obj *objPtr, *listPtr;
   labelptr tlab;

   static char *subCmds[] = {"make", "type", "insert", "anchor", "justify",
	"flipinvariant", "visible", "font", "scale", "encoding", "style",
	"family", "substring", "text", "latex", "list", "replace", "position",
	NULL};
   enum SubIdx {
      MakeIdx, TypeIdx, InsertIdx, AnchorIdx, JustifyIdx, FlipIdx, VisibleIdx,
	FontIdx, ScaleIdx, EncodingIdx, StyleIdx, FamilyIdx, SubstringIdx,
	TextIdx, LaTeXIdx, ListIdx, ReplaceIdx, PositionIdx
   };

   /* These must match the order of string part types defined in xcircuit.h */
   static char *subsubCmds[] = {"text", "subscript", "superscript",
	"normalscript", "underline", "overline", "noline", "stop",
	"forward", "backward", "halfspace", "quarterspace", "return",
	"name", "scale", "color", "margin", "kern", "parameter",
	"special", NULL};

   static char *pinTypeNames[] = {"normal", "text", "local", "pin", "global",
	"info", "netlist", NULL};

   static int pinTypes[] = {NORMAL, NORMAL, LOCAL, LOCAL, GLOBAL, INFO, INFO};

   static char *anchorValues[] = {"left", "center", "right", "top", "middle",
	"bottom", NULL};

   static char *justifyValues[] = {"left", "center", "right", "both", NULL};

   const char *styValues[] = {"normal", "bold", "italic", "bolditalic", NULL};

   const char *encValues[] = {"Standard", "special", "ISOLatin1",
	"ISOLatin2", "ISOLatin3", "ISOLatin4", "ISOLatin5",
	"ISOLatin6", "ISO8859-5", NULL};

   /* Tk "label" has been renamed to "tcl_label", but we want to	*/
   /* consider the "label" command to be overloaded, such that the	*/
   /* command "label" may be used without reference to technology.	*/

   Tcl_Obj **newobjv = (Tcl_Obj **)Tcl_Alloc(objc * sizeof(Tcl_Obj *));

   newobjv[0] = Tcl_NewStringObj("tcl_label", 9);
   Tcl_IncrRefCount(newobjv[0]);
   for (i = 1; i < objc; i++) {
      if (Tcl_IsShared(objv[i]))
         newobjv[i] = Tcl_DuplicateObj(objv[i]);
      else
         newobjv[i] = objv[i];
      Tcl_IncrRefCount(newobjv[i]);
   }

   result = Tcl_EvalObjv(interp, objc, newobjv, 0);

   for (i = 0; i < objc; i++)
      Tcl_DecrRefCount(newobjv[i]);
   Tcl_Free((char *)newobjv);

   if (result == TCL_OK) return result;
   Tcl_ResetResult(interp);

   /* Now, assuming that Tcl didn't like the syntax, we continue on with */
   /* our own version.							 */

   nidx = 4;
   result = ParseElementArguments(interp, objc, objv, &nidx, LABEL);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds, "option", 0, &idx)) != TCL_OK)
      return result;

   /* If there are no selections at this point, check if the command is */
   /* appropriate for setting a default value.				*/

   switch (idx) {
      case MakeIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    if (objc != 2) {
	       result = Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)pinTypeNames, "pin type", 0, &idx2);
	       if (result != TCL_OK) {
	          if (objc == 3) return result;
	          else {
		     Tcl_ResetResult(interp);
		     idx2 = 0;
		  }
	       }
	       else {
	          nidx++;
		  idx2 = pinTypes[idx2];  /* idx2 now matches defs in xcircuit.h */
	       }
	    }
	    if ((objc != 4) && (objc != 5)) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    else {
	       labelptr newlab;
	       stringpart *strptr = NULL;
	       XPoint position;

	       if ((result = GetXCStringFromList(interp, objv[nidx + 1],
			&strptr)) != TCL_OK)
		  return result;

	       /* Should probably have some mechanism to create an empty */
	       /* string from a script, even though empty strings are	 */
	       /* disallowed from the GUI.				 */

	       if (strptr == NULL) {
		  Tcl_SetResult(interp, "Empty string.  No element created.", NULL);
		  break;
	       }
	       if ((objc - nidx) <= 2) {
		  Tcl_WrongNumArgs(interp, 3, objv, "<text> {position}");
		  return TCL_ERROR;
	       }

	       if ((result = GetPositionFromList(interp, objv[nidx + 2],
			&position)) != TCL_OK)
		  return result;

	       newlab = new_label(NULL, strptr, idx2, position.x, position.y,
			(u_char)1);
	       singlebbox((genericptr *)&newlab);
	       objPtr = Tcl_NewHandleObj(newlab);
	       Tcl_SetObjResult(interp, objPtr);
	    }
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"label <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetResult(interp, "No selections allowed", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case ScaleIdx:
	 if (objc == 2) {
	    if ((areawin->selects == 0) && (nidx == 1) &&
		eventmode != TEXT_MODE && eventmode != ETEXT_MODE) {
	       objPtr = Tcl_NewDoubleObj((double)areawin->textscale);
	       Tcl_SetObjResult(interp, objPtr);
	    }
	    else {
	       float *floatptr;
	       gettextsize(&floatptr);
	       objPtr = Tcl_NewDoubleObj((double)((float)(*floatptr)));
	       Tcl_SetObjResult(interp, objPtr);
	    }
	 }
	 else if (objc >= 3) {
	    result = Tcl_GetDoubleFromObj(interp, objv[nidx + 1], &tmpdbl);
	    if (result != TCL_OK) return result;
	    if (tmpdbl <= 0.0) {
	       Tcl_SetResult(interp, "Illegal scale value", NULL);
               return TCL_ERROR;
	    }

	    if ((areawin->selects == 0) && (nidx == 1) && (eventmode != TEXT_MODE)
			&& (eventmode != ETEXT_MODE))
	       areawin->textscale = (float)tmpdbl;
	    else
	       changetextscale((float)tmpdbl);
	 }
	 break;

      case FontIdx:
	 if (objc == 2) {
	    tmpstr = fonts[areawin->psfont].psname;
	    objPtr = Tcl_NewStringObj(tmpstr, strlen(tmpstr));
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else {
	    tmpstr = Tcl_GetString(objv[2]);
	    for (i = 0; i < fontcount; i++)
	       if (!strcmp(fonts[i].psname, tmpstr)) break;
	    setfont((Tk_Window)clientData, (u_int)i, NULL);
	 }
	 break;

      case FamilyIdx:

	 /* Check for "-all" switch */
	 if ((objc - nidx) == 2) {
	    tmpstr = Tcl_GetString(objv[nidx + 1]);
	    if (!strncmp(tmpstr, "-all", 4)) {

	       /* Create a list of all font families.  This does a simple */
	       /* check against contiguous entries, but the result is not */
	       /* guaranteed to be a list of unique entries (i.e., the	  */
	       /* calling script should sort the list)			  */

	       for (i = 0; i < fontcount; i++) {
		  if (i == 0 || strcmp(fonts[i].family, fonts[i-1].family))
		     Tcl_AppendElement(interp, fonts[i].family);
	       }
	       break;
	    }
	 }

	 if (objc == 2) {
	    tmpstr = fonts[areawin->psfont].family;
	    objPtr = Tcl_NewStringObj(tmpstr, strlen(tmpstr));
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else {
	    tmpstr = Tcl_GetString(objv[2]);
	    for (i = 0; i < fontcount; i++)
	       if (!strcmp(fonts[i].family, tmpstr)) break;
	    setfont((Tk_Window)clientData, (u_int)i, NULL);
	 }
	 break;

      case EncodingIdx:
	 if (objc == 2) {
	    tmpstr = translateencoding(areawin->psfont);
	    objPtr = Tcl_NewStringObj(tmpstr, -1);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)encValues, "encodings", 0,
			&idx2) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    fontencoding((Tk_Window)clientData, idx2, NULL);
	    refresh(NULL, NULL, NULL);
	 }
	 break;

      case StyleIdx:
	 if (objc == 2) {
	    tmpstr = translatestyle(areawin->psfont);
	    objPtr = Tcl_NewStringObj(tmpstr, -1);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)styValues,
			"styles", 0, &idx2) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    fontstyle((Tk_Window)clientData, idx2, NULL);
	 }
	 break;

      case TypeIdx:	/* Change type of label */
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have a label selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 1) {	/* Return pin type(s) */
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
	       tlab = SELTOLABEL(areawin->selectlist + i);
	       for (idx2 = 0; idx2 < sizeof(pinTypeNames); idx2++) {
		  if (tlab->pin == pinTypes[idx2]) {
	             Tcl_AppendElement(interp, pinTypeNames[idx2]);
		     break;
		  }
	       }
	    }
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[nidx + 1],
			(CONST84 char **)pinTypeNames,
			"pin types", 0, &idx2) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
	       tlab = SELTOLABEL(areawin->selectlist + i);
	       tlab->pin = pinTypes[idx2];
	       pinconvert(tlab, tlab->pin);
	       setobjecttype(topobject);
	    }
	 }
	 break;

      case InsertIdx:	/* Text insertion */
	 if (nidx != 1) {
	    Tcl_SetResult(interp, "Insertion into handle or selection"
			" not supported (yet)", NULL);
	    return TCL_ERROR;
	 }
	 if (eventmode != TEXT_MODE && eventmode != ETEXT_MODE) {
	    Tcl_SetResult(interp, "Must be in edit mode to insert into label.",
			NULL);
	    return TCL_ERROR;
	 }
         if (objc <= nidx + 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "insert_type");
	    return TCL_ERROR;
	 }
	 if (Tcl_GetIndexFromObj(interp, objv[nidx + 1],
			(CONST84 char **)subsubCmds,
			"insertions", 0, &idx2) != TCL_OK) {
	    return TCL_ERROR;
	 }
	 if ((idx2 > TEXT_STRING) && (idx2 < FONT_NAME) && (objc - nidx == 2)) {
	    labeltext(idx2, (char *)1);
	 }
	 else if (idx2 == MARGINSTOP) {
	    if (objc - nidx == 3) {
	       result = Tcl_GetIntFromObj(interp, objv[nidx + 2], &value);
	       if (result != TCL_OK) return result;
 	    }
	    else value = 1;
	    labeltext(idx2, (char *)&value);
	 }
	 else if ((idx2 == PARAM_START) && (objc - nidx == 3)) {
	    labeltext(idx2, Tcl_GetString(objv[nidx + 2]));
	 }
	 else if ((idx2 == FONT_COLOR) && (objc - nidx == 3)) {
	    result = GetColorFromObj(interp, objv[nidx + 2], &value, TRUE);
	    if (result != TCL_OK) return result;
	    labeltext(idx2, (char *)&value);
	 }
	 else if ((idx2 == FONT_NAME) && (objc - nidx == 3)) {
	    tmpstr = Tcl_GetString(objv[nidx + 2]);
	    for (i = 0; i < fontcount; i++)
	       if (!strcmp(fonts[i].psname, tmpstr)) break;
	    if (i == fontcount) {
	       Tcl_SetResult(interp, "Invalid font name.", NULL);
	       return TCL_ERROR;
	    }
	    else
	       labeltext(idx2, (char *)&i);
	 }
	 else if ((idx2 == FONT_SCALE) && (objc - nidx == 3)) {
	    float fvalue;
	    double dvalue;
	    result = Tcl_GetDoubleFromObj(interp, objv[nidx + 2], &dvalue);
	    if (result != TCL_OK) return result;
	    fvalue = (float)dvalue;
	    labeltext(idx2, (char *)&fvalue);
	 }
	 else if ((idx2 == KERN) && (objc - nidx == 3)) {
	    strcpy(_STR2, Tcl_GetString(objv[nidx + 2]));
	    setkern(NULL, NULL);
	 }
	 else if ((idx2 == TEXT_STRING) && (objc - nidx == 3)) {
	    char *substring = Tcl_GetString(objv[nidx + 2]);
	    for (i = 0; i < strlen(substring); i++) {
	       /* Special handling allows newlines from cutbuffer selections */
	       /* to be translated into embedded carriage returns.	     */
	       if (substring[i] == '\012')
	          labeltext(RETURN, (char *)1);
	       else
	          labeltext(substring[i], NULL);
	     }
	 }

	 /* PARAM_END in xcircuit.h is actually mapped to the same */
	 /* position as "special" in subsubCommands[] above; don't */
	 /* be confused. . .					   */

	 else if ((idx2 == PARAM_END) && (objc - nidx == 2)) {
	    dospecial();
	 }
	 else if ((idx2 == PARAM_END) && (objc - nidx == 3)) {
	    result = Tcl_GetIntFromObj(interp, objv[nidx + 2], &value);
	    if (result != TCL_OK) return result;
	    labeltext(value, NULL);
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 2, objv, "insertion_type ?arg ...?");
	    return TCL_ERROR;
	 }
	 break;

      case SubstringIdx:
	 objPtr = Tcl_NewListObj(0, NULL);
	 if (areawin != NULL && areawin->selects == 1) {
	    if (SELECTTYPE(areawin->selectlist) == LABEL) {
	       Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewIntObj(areawin->textend));
	       Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewIntObj(areawin->textpos));
	    }
	 }
	 Tcl_SetObjResult(interp, objPtr);
	 break;

   /* Fixed issue where LaTeX mode wasn't assigned to labels */
   /* by Agustín Campeny, April 2020                         */

      case VisibleIdx:	/* Change visibility of pin */
	 if (objc == nidx + 1)
	    jval = getanchoring(interp, PINVISIBLE);
	 else {
	    if ((result = Tcl_GetBooleanFromObj(interp, objv[nidx + 1],
			&value)) != TCL_OK)
	       return result;
   	  setanchoring(PINVISIBLE, (value) ? PINVISIBLE : NORMAL);
	 }
	 break;

      case FlipIdx:
	 if (objc == nidx + 1)
	    jval = getanchoring(interp, FLIPINV);
	 else {
	    if ((result = Tcl_GetBooleanFromObj(interp, objv[nidx + 1],
			&value)) != TCL_OK)
	       return result;
   	  setanchoring(FLIPINV, (value) ? FLIPINV : NORMAL);
	 }
	 break;

      case LaTeXIdx:
	 if (objc == nidx + 1)
	    jval = getanchoring(interp, LATEXLABEL);
	 else {
	    if ((result = Tcl_GetBooleanFromObj(interp, objv[nidx + 1],
			&value)) != TCL_OK)
	       return result;
   	  setanchoring(LATEXLABEL, (value) ? LATEXLABEL : NORMAL);
	 }
	 break;

      case JustifyIdx:
	 if (objc == nidx + 1) {
	    jval = getanchoring(interp, JUSTIFYRIGHT | JUSTIFYBOTH | TEXTCENTERED);
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[nidx + 1],
		(CONST84 char **)justifyValues,
		"justification", 0, &idx2) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    switch (idx2) {
	       case 0: value = NORMAL; break;
	       case 1: value = TEXTCENTERED; break;
	       case 2: value = JUSTIFYRIGHT; break;
	       case 3: value = JUSTIFYBOTH; break;
	    }
		  setanchoring(JUSTIFYRIGHT | JUSTIFYBOTH | TEXTCENTERED, value);
		  refresh(NULL, NULL, NULL);
	 }
	 break;

      case AnchorIdx:
	 if (objc == nidx + 1) {
	    jval = getanchoring(interp, RIGHT | NOTLEFT);
	    jval2 = getanchoring(interp, TOP | NOTBOTTOM);
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[nidx + 1],
		(CONST84 char **)anchorValues,
		"anchoring", 0, &idx2) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    switch (idx2) {
	       case 0: value = NORMAL; break;
	       case 1: value = NOTLEFT; break;
	       case 2: value = NOTLEFT | RIGHT; break;
	       case 3: value = NOTBOTTOM | TOP; break;
	       case 4: value = NOTBOTTOM; break;
	       case 5: value = NORMAL; break;
	    }
	    switch (idx2) {
	       case 0: case 1: case 2:
		  setanchoring(RIGHT | NOTLEFT, value);
		  refresh(NULL, NULL, NULL);
	          break;
	       case 3: case 4: case 5:
		  setanchoring(TOP | NOTBOTTOM, value);
		  refresh(NULL, NULL, NULL);
	          break;
	    }
	 }
	 break;

      case TextIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have a label selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 1) {	/* Return label as printable string */
	    char *tstr;
	    objPtr = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
	       tlab = SELTOLABEL(areawin->selectlist + i);
	       tstr = textprint(tlab->string, areawin->topinstance);
	       Tcl_ListObjAppendElement(interp, objPtr,
			Tcl_NewStringObj(tstr, strlen(tstr)));
	       free(tstr);
	    }
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 break;

      case ListIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have a label selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 1) {	/* Return label as printable string */
	    listPtr = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
	       tlab = SELTOLABEL(areawin->selectlist + i);
	       objPtr = TclGetStringParts(tlab->string);
	       Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    }
	    Tcl_SetObjResult(interp, listPtr);
	 }
	 break;

      case ReplaceIdx:	/* the opposite of "list" */
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have a label selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 2) {	/* Replace string from list */
	    stringpart *strptr = NULL;

	    if ((result = GetXCStringFromList(interp, objv[nidx + 1],
			&strptr)) != TCL_OK)
	       return result;

	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
	       tlab = SELTOLABEL(areawin->selectlist + i);
	       register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance, tlab);
	       freelabel(tlab->string);
	       tlab->string = stringcopy(strptr);
	    }
	    freelabel(strptr);
	    undo_finish_series();
	    refresh(NULL, NULL, NULL);
	 }
	 break;

      case PositionIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have a label selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 1) {	/* Return position of label */
	    Tcl_Obj *cpair;

	    listPtr = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != LABEL) continue;
	       tlab = SELTOLABEL(areawin->selectlist + i);
	       cpair = Tcl_NewListObj(0, NULL);
	       objPtr = Tcl_NewIntObj((int)tlab->position.x);
	       Tcl_ListObjAppendElement(interp, cpair, objPtr);
	       objPtr = Tcl_NewIntObj((int)tlab->position.y);
	       Tcl_ListObjAppendElement(interp, cpair, objPtr);
	       Tcl_ListObjAppendElement(interp, listPtr, cpair);
	    }
	    Tcl_SetObjResult(interp, listPtr);
	 }
	 else if (objc == nidx + 2) {	/* Change position of label */
	    XPoint position;

	    if ((areawin->selects != 1) || (SELECTTYPE(areawin->selectlist)
			!= LABEL)) {
	       Tcl_SetResult(interp, "Must have exactly one selected label", NULL);
	       return TCL_ERROR;
	    }
	    if ((result = GetPositionFromList(interp, objv[nidx + 1],
			&position)) != TCL_OK)
	       return result;

	    tlab = SELTOLABEL(areawin->selectlist);
	    tlab->position.x = position.x;
	    tlab->position.y = position.y;
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Element Fill Styles							*/
/*----------------------------------------------------------------------*/

int xctcl_dofill(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   u_int value;
   int i, idx, result, rval = -1;

   static char *Styles[] = {"opaque", "transparent", "filled", "unfilled",
	"solid", NULL};
   enum StylesIdx {
      OpaqueIdx, TransparentIdx, FilledIdx, UnfilledIdx, SolidIdx
   };

   if (objc == 1) {
      value = areawin->style;
      Tcl_AppendElement(interp, ((value & OPAQUE) ? "opaque" : "transparent"));
      if (value & FILLED) {
         Tcl_AppendElement(interp, "filled");
	 switch (value & FILLSOLID) {
	    case 0:
               Tcl_AppendElement(interp, "12"); break;
	    case STIP0:
               Tcl_AppendElement(interp, "25"); break;
	    case STIP1:
               Tcl_AppendElement(interp, "37"); break;
	    case STIP1 | STIP0:
               Tcl_AppendElement(interp, "50"); break;
	    case STIP2:
               Tcl_AppendElement(interp, "62"); break;
	    case STIP2 | STIP0:
               Tcl_AppendElement(interp, "75"); break;
	    case STIP2 | STIP1:
               Tcl_AppendElement(interp, "87"); break;
	    case FILLSOLID:
               Tcl_AppendElement(interp, "solid"); break;
	 }
      }
      else {
	 Tcl_AppendElement(interp, "unfilled");
      }
      return TCL_OK;
   }

   for (i = 1; i < objc; i++) {
      if (Tcl_GetIndexFromObj(interp, objv[i],
			(CONST84 char **)Styles, "fill styles",
			0, &idx) != TCL_OK) {
	 Tcl_ResetResult(interp);
         result = Tcl_GetIntFromObj(interp, objv[i], &value);
         if (result != TCL_OK) {
	    Tcl_SetResult(interp, "Expected fill style or fillfactor 0 to 100", NULL);
	    return result;
	 }
	 else {
            if (value >= 0 && value < 6) value = FILLSOLID;
            else if (value >= 6 && value < 19) value = FILLED;
            else if (value >= 19 && value < 31) value = FILLED | STIP0;
            else if (value >= 31 && value < 44) value = FILLED | STIP1;
            else if (value >= 44 && value < 56) value = FILLED | STIP0 | STIP1;
            else if (value >= 56 && value < 69) value = FILLED | STIP2;
            else if (value >= 69 && value < 81) value = FILLED | STIP2 | STIP0;
            else if (value >= 81 && value < 94) value = FILLED | STIP2 | STIP1;
            else if (value >= 94 && value <= 100) value = FILLED | FILLSOLID;
            else {
               Tcl_SetResult(interp, "Fill value should be 0 to 100", NULL);
               return TCL_ERROR;
            }
            rval = setelementstyle((Tk_Window)clientData, (pointertype)value,
		FILLED | FILLSOLID);
	 }
      }
      else {
         switch(idx) {
	    case OpaqueIdx:
               rval = setelementstyle((Tk_Window)clientData, OPAQUE, OPAQUE);
	       break;
	    case TransparentIdx:
               rval = setelementstyle((Tk_Window)clientData, NORMAL, OPAQUE);
	       break;
	    case UnfilledIdx:
               rval = setelementstyle((Tk_Window)clientData, FILLSOLID,
			FILLED | FILLSOLID);
	       break;
	    case SolidIdx:
               rval = setelementstyle((Tk_Window)clientData, FILLED | FILLSOLID,
			FILLED | FILLSOLID);
	       break;
	    case FilledIdx:
	       break;
	 }
      }
   }
   if (rval < 0)
      return TCL_ERROR;

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Element border styles						*/
/*----------------------------------------------------------------------*/

int xctcl_doborder(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result, i, idx, value, rval = -1;
   u_short mask;
   double wvalue;

   static char *borderStyles[] = {"solid", "dashed", "dotted", "none",
	"unbordered", "unclosed", "closed", "bbox", "set", "get", "square",
	"round", "clipmask", NULL};
   enum StyIdx {
	SolidIdx, DashedIdx, DottedIdx, NoneIdx, UnborderedIdx,
	UnclosedIdx, ClosedIdx, BBoxIdx, SetIdx, GetIdx, SquareIdx,
	RoundIdx, ClipMaskIdx
   };

   if (objc == 1) {
      Tcl_Obj *listPtr;
      listPtr = Tcl_NewListObj(0, NULL);
      value = areawin->style;
      wvalue = (double)areawin->linewidth;
      switch (value & (DASHED | DOTTED | NOBORDER | SQUARECAP)) {
	 case NORMAL:
	    Tcl_ListObjAppendElement(interp, listPtr,
			Tcl_NewStringObj("solid", 5)); break;
	 case DASHED:
	    Tcl_ListObjAppendElement(interp, listPtr,
			Tcl_NewStringObj("dashed", 6)); break;
	 case DOTTED:
	    Tcl_ListObjAppendElement(interp, listPtr,
			Tcl_NewStringObj("dotted", 6)); break;
	 case NOBORDER:
	    Tcl_ListObjAppendElement(interp, listPtr,
			Tcl_NewStringObj("unbordered", 10)); break;
	 case SQUARECAP:
	    Tcl_ListObjAppendElement(interp, listPtr,
			Tcl_NewStringObj("square-endcaps", 10)); break;
      }
      if (value & UNCLOSED)
         Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("unclosed", 8));
      else
         Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("closed", 6));

      if (value & BBOX)
         Tcl_ListObjAppendElement(interp, listPtr,
		Tcl_NewStringObj("bounding box", 12));

      if (value & CLIPMASK)
         Tcl_ListObjAppendElement(interp, listPtr,
		Tcl_NewStringObj("clipmask", 8));

      Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewDoubleObj(wvalue));
      Tcl_SetObjResult(interp, listPtr);
      return TCL_OK;
   }

   for (i = 1; i < objc; i++) {
      result = Tcl_GetIndexFromObj(interp, objv[i],
		 (CONST84 char **)borderStyles,
		"border style", 0, &idx);
      if (result != TCL_OK)
	 return result;

      switch (idx) {
         case GetIdx:
	    {
	       int j, numfound = 0;
	       genericptr setel;
	       Tcl_Obj *objPtr, *listPtr = NULL;

	       for (j = 0; j < areawin->selects; j++) {
	          setel = SELTOGENERIC(areawin->selectlist + j);
	          if (IS_ARC(setel) || IS_POLYGON(setel) ||
			IS_SPLINE(setel) || IS_PATH(setel)) {
	             switch(ELEMENTTYPE(setel)) {
		        case ARC: wvalue = ((arcptr)setel)->width; break;
		        case POLYGON: wvalue = ((polyptr)setel)->width; break;
		        case SPLINE: wvalue = ((splineptr)setel)->width; break;
		        case PATH: wvalue = ((pathptr)setel)->width; break;
	             }
		     if ((++numfound) == 2) {
			listPtr = Tcl_NewListObj(0, NULL);
		        Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		     }
		     objPtr = Tcl_NewDoubleObj(wvalue);
		     if (numfound > 1)
		        Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	          }
	       }
	       switch (numfound) {
	          case 0:
		     objPtr = Tcl_NewDoubleObj(areawin->linewidth);
		     /* fall through */
	          case 1:
	             Tcl_SetObjResult(interp, objPtr);
		     break;
	          default:
	             Tcl_SetObjResult(interp, listPtr);
		     break;
	       }
	    }
	    break;
         case SetIdx:
	    if ((objc - i) != 2) {
	       Tcl_SetResult(interp, "Error: no linewidth given.", NULL);
	       return TCL_ERROR;
	    }
	    result = Tcl_GetDoubleFromObj(interp, objv[++i], &wvalue);
	    if (result == TCL_OK) {
	       sprintf(_STR2, "%f", wvalue);
	       setwwidth((Tk_Window)clientData, NULL);
	    }
	    else {
	       Tcl_SetResult(interp, "Error: invalid border linewidth.", NULL);
	       return TCL_ERROR;
	    }
	    break;
         case SolidIdx: value = NORMAL; mask = DASHED | DOTTED | NOBORDER; break;
         case DashedIdx: value = DASHED; mask = DASHED | DOTTED | NOBORDER; break;
         case DottedIdx: value = DOTTED; mask = DASHED | DOTTED | NOBORDER; break;
         case NoneIdx: case UnborderedIdx:
	    value = NOBORDER; mask = DASHED | DOTTED | NOBORDER; break;
         case UnclosedIdx: value = UNCLOSED; mask = UNCLOSED; break;
         case ClosedIdx: value = NORMAL; mask = UNCLOSED; break;
	 case SquareIdx: value = SQUARECAP; mask = SQUARECAP; break;
	 case RoundIdx: value = NORMAL; mask = SQUARECAP; break;
         case BBoxIdx:
	    mask = BBOX;
	    if ((objc - i) < 2) value = BBOX;
	    else {
	       char *yesno = Tcl_GetString(objv[++i]);
	       value = (tolower(yesno[0]) == 'y' || tolower(yesno[0]) == 't') ?
		   BBOX : NORMAL;
	    }
	    break;
         case ClipMaskIdx:
	    mask = CLIPMASK;
	    if ((objc - i) < 2) value = CLIPMASK;
	    else {
	       char *yesno = Tcl_GetString(objv[++i]);
	       value = (tolower(yesno[0]) == 'y' || tolower(yesno[0]) == 't') ?
		   CLIPMASK : NORMAL;
	    }
	    break;
      }
      if (idx != SetIdx && idx != GetIdx)
         rval = setelementstyle((Tk_Window)clientData, (u_short)value, mask);
   }

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_polygon(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int idx, nidx, result, npoints, j;
   polyptr newpoly, ppoly;
   XPoint ppt;
   pointlist points;
   Tcl_Obj *objPtr, *coord, *cpair, **newobjv;
   Boolean is_box = FALSE;
   Matrix hierCTM;

   static char *subCmds[] = {"make", "border", "fill", "points", "number", NULL};
   enum SubIdx {
	MakeIdx, BorderIdx, FillIdx, PointsIdx, NumberIdx
   };

   nidx = 255;
   result = ParseElementArguments(interp, objc, objv, &nidx, POLYGON);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx)) != TCL_OK)
      return result;

   switch (idx) {
      case MakeIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    if (objc < 5) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    if (!strcmp(Tcl_GetString(objv[2]), "box")) {
	       npoints = objc - 3;
	       is_box = TRUE;
	       if (npoints != 4 && npoints != 2) {
		  Tcl_SetResult(interp, "Box must have 2 or 4 points", NULL);
		  return TCL_ERROR;
	       }
	    }
	    else {
	       result = Tcl_GetIntFromObj(interp, objv[2], &npoints);
	       if (result != TCL_OK) return result;
	    }
	    if (objc != npoints + 3) {
	       Tcl_WrongNumArgs(interp, 1, objv, "N {x1 y1}...{xN yN}");
	       return TCL_ERROR;
	    }
	    points = (pointlist)malloc(npoints * sizeof(XPoint));
	    for (j = 0; j < npoints; j++) {
	       result = GetPositionFromList(interp, objv[3 + j], &ppt);
	       if (result == TCL_OK) {
	          points[j].x = ppt.x;
	          points[j].y = ppt.y;
	       }
	    }
	    if (is_box && (npoints == 2)) {
	       npoints = 4;
	       points = (pointlist)realloc(points, npoints * sizeof(XPoint));
	       points[2].x = points[1].x;
	       points[2].y = points[1].y;
	       points[1].y = points[0].y;
	       points[3].x = points[0].x;
	       points[3].y = points[2].y;
	    }
	    newpoly = new_polygon(NULL, &points, npoints);
	    if (!is_box) newpoly->style |= UNCLOSED;
	    singlebbox((genericptr *)&newpoly);

	    objPtr = Tcl_NewHandleObj(newpoly);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"polygon <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetResult(interp, "No selections allowed", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case BorderIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_doborder(clientData, interp, objc - nidx, newobjv);
	 break;

      case FillIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_dofill(clientData, interp, objc - nidx, newobjv);
	 break;

      case NumberIdx:
	 if (areawin->selects != 1) {
	    Tcl_SetResult(interp, "Must have exactly one selection to "
		"query points", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    if (SELECTTYPE(areawin->selectlist) != POLYGON) {
		Tcl_SetResult(interp, "Selected element is not a polygon", NULL);
		return TCL_ERROR;
	    }
	    else
	       ppoly = SELTOPOLY(areawin->selectlist);

	    if ((objc - nidx) == 1) {
	       objPtr = Tcl_NewIntObj(ppoly->number);
	       Tcl_SetObjResult(interp, objPtr);
	    }
	    else
	    {
		Tcl_SetResult(interp, "Cannot change number of points.\n", NULL);
		return TCL_ERROR;
	    }
	 }
	 break;

      case PointsIdx:
	 if (areawin->selects != 1) {
	    Tcl_SetResult(interp, "Must have exactly one selection to "
		"query or manipulate points", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    ppoly = SELTOPOLY(areawin->selectlist);
	    MakeHierCTM(&hierCTM);
	    if (ppoly->type != POLYGON) {
		Tcl_SetResult(interp, "Selected element is not a polygon", NULL);
		return TCL_ERROR;
	    }
	    points = ppoly->points;

	    if ((objc - nidx) == 1)	/* Return a list of all points */
	    {
	       objPtr = Tcl_NewListObj(0, NULL);
	       for (npoints = 0; npoints < ppoly->number; npoints++) {
		  cpair = Tcl_NewListObj(0, NULL);
		  UTransformbyCTM(&hierCTM, points + npoints, &ppt, 1);
	          coord = Tcl_NewIntObj((int)ppt.x);
	          Tcl_ListObjAppendElement(interp, cpair, coord);
	          coord = Tcl_NewIntObj((int)ppt.y);
	          Tcl_ListObjAppendElement(interp, cpair, coord);
	          Tcl_ListObjAppendElement(interp, objPtr, cpair);
	       }
	       Tcl_SetObjResult(interp, objPtr);
	    }
	    else if ((objc - nidx) == 2)  /* Return a specific point */
	    {
	       result = Tcl_GetIntFromObj(interp, objv[2], &npoints);
	       if (result != TCL_OK) return result;
	       if (npoints >= ppoly->number) {
		  Tcl_SetResult(interp, "Point number out of range", NULL);
		  return TCL_ERROR;
	       }
	       objPtr = Tcl_NewListObj(0, NULL);
	       UTransformbyCTM(&hierCTM, points + npoints, &ppt, 1);
	       coord = Tcl_NewIntObj((int)ppt.x);
	       Tcl_ListObjAppendElement(interp, objPtr, coord);
	       coord = Tcl_NewIntObj((int)ppt.y);
	       Tcl_ListObjAppendElement(interp, objPtr, coord);
	       Tcl_SetObjResult(interp, objPtr);
	    }
	    else
	    {
		Tcl_SetResult(interp, "Individual point setting unimplemented\n", NULL);
		return TCL_ERROR;
	    }
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_spline(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int idx, nidx, result, j, npoints;
   splineptr newspline, pspline;
   XPoint ppt, ctrlpoints[4];
   Tcl_Obj *objPtr, *cpair, *coord, **newobjv;
   Matrix hierCTM;

   static char *subCmds[] = {"make", "border", "fill", "points", NULL};
   enum SubIdx {
	MakeIdx, BorderIdx, FillIdx, PointsIdx
   };

   nidx = 5;
   result = ParseElementArguments(interp, objc, objv, &nidx, SPLINE);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx)) != TCL_OK)
      return result;

   /* h = spline make {x1 y1} ... {x4 y4} */

   switch (idx) {
      case MakeIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    if (objc != 6) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    for (j = 0; j < 4; j++) {
	       result = GetPositionFromList(interp, objv[2 + j], &ppt);
	       if (result == TCL_OK) {
	          ctrlpoints[j].x = ppt.x;
	          ctrlpoints[j].y = ppt.y;
	       }
	    }
	    newspline = new_spline(NULL, ctrlpoints);
	    singlebbox((genericptr *)&newspline);

	    objPtr = Tcl_NewHandleObj(newspline);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else if (areawin->selects == 1) {
	    if (ELEMENTTYPE(*(topobject->plist + (*areawin->selectlist))) == POLYGON) {
	       converttocurve();
	    }
	    else {
	       Tcl_SetResult(interp, "\"spline make\": must have a polygon selected",
			NULL);
	       return TCL_ERROR;
	    }
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"spline <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetResult(interp, "No selections allowed except single polygon", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case BorderIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_doborder(clientData, interp, objc - nidx, newobjv);
	 break;

      case FillIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_dofill(clientData, interp, objc - nidx, newobjv);
	 break;

      case PointsIdx:
	 if (areawin->selects != 1) {
	    Tcl_SetResult(interp, "Must have exactly one selection to "
		"query or manipulate points", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    /* check for ESPLINE mode? */
	    if (SELECTTYPE(areawin->selectlist) != SPLINE) {
		Tcl_SetResult(interp, "Selected element is not a spline", NULL);
		return TCL_ERROR;
	    }
	    else
	       pspline = SELTOSPLINE(areawin->selectlist);

	    MakeHierCTM(&hierCTM);

	    if ((objc - nidx) == 1)	/* Return a list of all points */
	    {
	       objPtr = Tcl_NewListObj(0, NULL);
	       for (npoints = 0; npoints < 4; npoints++) {
		  cpair = Tcl_NewListObj(0, NULL);
		  UTransformbyCTM(&hierCTM, pspline->ctrl + npoints, &ppt, 1);
	          coord = Tcl_NewIntObj((int)ppt.x);
	          Tcl_ListObjAppendElement(interp, cpair, coord);
	          coord = Tcl_NewIntObj((int)ppt.y);
	          Tcl_ListObjAppendElement(interp, cpair, coord);
	          Tcl_ListObjAppendElement(interp, objPtr, cpair);
	       }
	       Tcl_SetObjResult(interp, objPtr);
	    }
	    else if ((objc - nidx) == 2)  /* Return a specific point */
	    {
	       result = Tcl_GetIntFromObj(interp, objv[objc - nidx + 1], &npoints);
	       if (result != TCL_OK) return result;
	       if (npoints >= 4) {
		  Tcl_SetResult(interp, "Point number out of range", NULL);
		  return TCL_ERROR;
	       }
	       objPtr = Tcl_NewListObj(0, NULL);
	       UTransformbyCTM(&hierCTM, pspline->ctrl + npoints, &ppt, 1);
	       coord = Tcl_NewIntObj((int)ppt.x);
	       Tcl_ListObjAppendElement(interp, objPtr, coord);
	       coord = Tcl_NewIntObj((int)ppt.y);
	       Tcl_ListObjAppendElement(interp, objPtr, coord);
	       Tcl_SetObjResult(interp, objPtr);
	    }
	    else
	    {
		Tcl_SetResult(interp, "Individual control point setting "
				"unimplemented\n", NULL);
		return TCL_ERROR;
	    }
	 }
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_graphic(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
  int i, idx, nidx, result;
   double dvalue;
   graphicptr newgp, gp;
   XPoint ppt;
   Tcl_Obj *objPtr, *listPtr;
   char *filename;

   static char *subCmds[] = {"make", "scale", "position", NULL};
   enum SubIdx {
	MakeIdx, ScaleIdx, PositionIdx
   };

   nidx = 7;
   result = ParseElementArguments(interp, objc, objv, &nidx, GRAPHIC);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx)) != TCL_OK)
      return result;

   switch (idx) {
      case MakeIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    if ((objc != 5) && (objc != 7)) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }

	    filename = Tcl_GetString(objv[2]);

	    result = GetPositionFromList(interp, objv[3], &ppt);
	    if (result != TCL_OK) return result;

	    result = Tcl_GetDoubleFromObj(interp, objv[4], &dvalue);
	    if (result != TCL_OK) return result;

	    if (!strcmp(filename, "gradient")) {
	       if (objc == 7) {
		  int c1, c2;
                  result = GetColorFromObj(interp, objv[5], &c1, TRUE);
		  if (result != TCL_OK) return result;
                  result = GetColorFromObj(interp, objv[6], &c2, TRUE);
		  if (result != TCL_OK) return result;
	          newgp = gradient_field(NULL, ppt.x, ppt.y, c1, c2);
	       }
	       else
	          newgp = gradient_field(NULL, ppt.x, ppt.y, 0, 1);
	    }
	    else if (objc != 5) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    else
	       newgp = new_graphic(NULL, filename, ppt.x, ppt.y);

	    if (newgp == NULL) return TCL_ERROR;

	    newgp->scale = (float)dvalue;
	    singlebbox((genericptr *)&newgp);

	    objPtr = Tcl_NewHandleObj(newgp);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"graphic <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetResult(interp, "No selections allowed", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case ScaleIdx:
      case PositionIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have a graphic selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 1) {	/* Return position of graphic origin */
	    Tcl_Obj *cpair;
	    graphicptr gp;

	    listPtr = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != GRAPHIC) continue;
	       gp = SELTOGRAPHIC(areawin->selectlist + i);

	       switch (idx) {
		  case ScaleIdx:
		     objPtr = Tcl_NewDoubleObj(gp->scale);
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		     break;
		  case PositionIdx:
		     cpair = Tcl_NewListObj(0, NULL);
		     objPtr = Tcl_NewIntObj((int)gp->position.x);
		     Tcl_ListObjAppendElement(interp, cpair, objPtr);
		     objPtr = Tcl_NewIntObj((int)gp->position.y);
		     Tcl_ListObjAppendElement(interp, cpair, objPtr);
		     Tcl_ListObjAppendElement(interp, listPtr, cpair);
		     break;
	       }
	    }
	    Tcl_SetObjResult(interp, listPtr);
	 }
	 else if (objc == nidx + 2) {	/* Change position or scale */
	    if (idx == ScaleIdx) {
	       result = Tcl_GetDoubleFromObj(interp, objv[nidx + 1], &dvalue);
	       if (result == TCL_OK) {
		  for (i = 0; i < areawin->selects; i++) {
		     float oldscale;

		     if (SELECTTYPE(areawin->selectlist + i) != GRAPHIC) continue;
		     gp = SELTOGRAPHIC(areawin->selectlist + i);
		     oldscale = gp->scale;
		     gp->scale = (float)dvalue;
		     if (gp->scale != oldscale) {
#ifndef HAVE_CAIRO
		        gp->valid = False;
#endif /* !HAVE_CAIRO */
		        drawarea(areawin->area, (caddr_t)clientData, (caddr_t)NULL);
		        calcbboxvalues(areawin->topinstance,
				topobject->plist + *(areawin->selectlist + i));
			register_for_undo(XCF_Rescale, UNDO_MORE, areawin->topinstance,
				(genericptr)gp, (double)oldscale);
		     }
		  }
		  undo_finish_series();
	       }
	    }
	    else {
	       result = GetPositionFromList(interp, objv[nidx + 1], &ppt);
	       if (result == TCL_OK) {
		  for (i = 0; i < areawin->selects; i++) {
		     if (SELECTTYPE(areawin->selectlist + i) != GRAPHIC) continue;
		     gp = SELTOGRAPHIC(areawin->selectlist + i);
		     gp->position.x = ppt.x;
		     gp->position.y = ppt.y;
		     calcbboxvalues(areawin->topinstance,
				topobject->plist + *(areawin->selectlist + i));
		  }
	       }
	    }
	    updatepagebounds(topobject);
	    incr_changes(topobject);
 	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_arc(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
  int idx, nidx, result, value;
   double angle;
   arcptr newarc;
   XPoint ppt;
   Tcl_Obj *objPtr, *listPtr, **newobjv;

   static char *subCmds[] = {"make", "border", "fill", "radius", "minor",
	"angle", "position", NULL};
   enum SubIdx {
	MakeIdx, BorderIdx, FillIdx, RadiusIdx, MinorIdx, AngleIdx,
	PositionIdx
   };

   nidx = 7;
   result = ParseElementArguments(interp, objc, objv, &nidx, ARC);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx)) != TCL_OK)
      return result;

   switch (idx) {
      case MakeIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    if ((objc < 4) || (objc > 7)) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    result = GetPositionFromList(interp, objv[2], &ppt);
	    if (result != TCL_OK) return result;

	    result = Tcl_GetIntFromObj(interp, objv[3], &value);
	    if (result != TCL_OK) return result;

	    newarc = new_arc(NULL, value, ppt.x, ppt.y);

	    switch (objc) {
	       case 6:
	          result = Tcl_GetDoubleFromObj(interp, objv[4], &angle);
		  if (result == TCL_OK) newarc->angle1 = (float)angle;
	          result = Tcl_GetDoubleFromObj(interp, objv[5], &angle);
		  if (result == TCL_OK) newarc->angle2 = (float)angle;
	 	  break;
	       case 7:
	          result = Tcl_GetDoubleFromObj(interp, objv[5], &angle);
		  if (result == TCL_OK) newarc->angle1 = (float)angle;
	          result = Tcl_GetDoubleFromObj(interp, objv[6], &angle);
		  if (result == TCL_OK) newarc->angle2 = (float)angle;
	       case 5:
	          result = Tcl_GetIntFromObj(interp, objv[4], &value);
		  if (result == TCL_OK) newarc->yaxis = value;
	 	  break;
	    }
	    if (objc >= 6) {
	       /* Check that angle2 > angle1.  Swap if necessary. */
	       if (newarc->angle2 < newarc->angle1) {
		  int tmp = newarc->angle2;
		  newarc->angle2 = newarc->angle1;
		  newarc->angle1 = tmp;
	       }

	       /* Check for 0 length chords (assume full circle was intended) */
	       if (newarc->angle1 == newarc->angle2) {
		  Tcl_SetResult(interp, "Changed zero-length arc chord!\n", NULL);
		  newarc->angle2 = newarc->angle1 + 360;
	       }

	       /* Normalize */
	       if (newarc->angle1 >= 360) {
		  newarc->angle1 -= 360;
		  newarc->angle2 -= 360;
	       }
	       else if (newarc->angle2 <= 0) {
		  newarc->angle1 += 360;
		  newarc->angle2 += 360;
	       }
	    }
	    if (objc >= 5) {
	       calcarc(newarc);
	       singlebbox((genericptr *)&newarc);
	    }
	    objPtr = Tcl_NewHandleObj(newarc);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"arc <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetResult(interp, "No selections allowed", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case BorderIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_doborder(clientData, interp, objc - nidx, newobjv);
	 break;

      case FillIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_dofill(clientData, interp, objc - nidx, newobjv);
	 break;

      case RadiusIdx:
      case MinorIdx:
      case AngleIdx:
      case PositionIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    Tcl_SetResult(interp, "Must have an arc selection.", NULL);
	    return TCL_ERROR;
	 }
	 if (objc == nidx + 1) {	/* Return position of arc center */
	    Tcl_Obj *cpair;
	    int i;
	    arcptr parc;

	    listPtr = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) != ARC) continue;
	       parc = SELTOARC(areawin->selectlist + i);

	       switch (idx) {
		  case RadiusIdx:
		     objPtr = Tcl_NewIntObj(parc->radius);
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		     break;
		  case MinorIdx:
		     objPtr = Tcl_NewIntObj(parc->yaxis);
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		     break;
		  case AngleIdx:
		     cpair = Tcl_NewListObj(0, NULL);
		     objPtr = Tcl_NewDoubleObj(parc->angle1);
		     Tcl_ListObjAppendElement(interp, cpair, objPtr);
		     objPtr = Tcl_NewDoubleObj(parc->angle2);
		     Tcl_ListObjAppendElement(interp, cpair, objPtr);
		     Tcl_ListObjAppendElement(interp, listPtr, cpair);
		     break;
		  case PositionIdx:
		     cpair = Tcl_NewListObj(0, NULL);
		     objPtr = Tcl_NewIntObj((int)parc->position.x);
		     Tcl_ListObjAppendElement(interp, cpair, objPtr);
		     objPtr = Tcl_NewIntObj((int)parc->position.y);
		     Tcl_ListObjAppendElement(interp, cpair, objPtr);
		     Tcl_ListObjAppendElement(interp, listPtr, cpair);
		     break;
	       }
	    }
	    Tcl_SetObjResult(interp, listPtr);
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_path(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int idx, nidx, result, j, i;
   genericptr newgen, *eptr;
   pathptr ppath;
   Tcl_Obj *elist, *objPtr, *cpair, *coord, **newobjv;
   XPoint ppt;
   Matrix hierCTM;

   static char *subCmds[] = {"join", "make", "border", "fill", "point", "unjoin",
		"points", NULL};
   enum SubIdx {
	JoinIdx, MakeIdx, BorderIdx, FillIdx, PointIdx, UnJoinIdx, PointsIdx
   };

   nidx = 5;
   result = ParseElementArguments(interp, objc, objv, &nidx, PATH);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx)) != TCL_OK)
      return result;

   switch (idx) {
      case MakeIdx: case JoinIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    /* h = path make {element_list} */
	    newobjv = (Tcl_Obj **)(&objv[1]);
	    result = ParseElementArguments(interp, objc - 1, newobjv, NULL,
			POLYGON | ARC | SPLINE | PATH);
	    if (result != TCL_OK) return result;
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"path <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 /* h = path make */
	 join();
	 newgen = *(topobject->plist + topobject->parts - 1);
	 objPtr = Tcl_NewHandleObj(newgen);
	 Tcl_SetObjResult(interp, objPtr);
	 break;

      case BorderIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_doborder(clientData, interp, objc - nidx, newobjv);
	 break;

      case FillIdx:
	 newobjv = (Tcl_Obj **)(&objv[nidx]);
	 result = xctcl_dofill(clientData, interp, objc - nidx, newobjv);
	 break;

      case PointIdx:
	 Tcl_SetResult(interp, "Unimplemented function.", NULL);
	 return TCL_ERROR;
	 break;

      case UnJoinIdx:
	 unjoin();
	 /* Would be nice to return the list of constituent elements. . . */
	 break;

      case PointsIdx:
	 /* Make a list of the polygon and spline elements in the path, */
	 /* returning a nested list enumerating the points.  This is	*/
	 /* ad-hoc, as it does not match any other method of returning	*/
	 /* point information about a part.  This is because returning	*/
	 /* a handle list is useless, since the handles cannot be	*/
	 /* accessed directly.						*/

	 if (areawin->selects != 1) {
	    Tcl_SetResult(interp, "Must have exactly one selection to "
		"query parts", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    if (SELECTTYPE(areawin->selectlist) != PATH) {
		Tcl_SetResult(interp, "Selected element is not a path", NULL);
		return TCL_ERROR;
	    }
	    else
	       ppath = SELTOPATH(areawin->selectlist);

	    MakeHierCTM(&hierCTM);

	    objPtr = Tcl_NewListObj(0, NULL);
	    for (j = 0; j < ppath->parts; j++) {
	       eptr = (genericptr *)(ppath->plist + j);
	       elist = Tcl_NewListObj(0, NULL);
	       if ((*eptr)->type == POLYGON) {
		  polyptr ppoly;
		  ppoly = (polyptr)(*eptr);
	          Tcl_ListObjAppendElement(interp, elist,
				Tcl_NewStringObj("polygon", -1));
		  for (i = 0; i < ppoly->number; i++) {
		     cpair = Tcl_NewListObj(0, NULL);
		     UTransformbyCTM(&hierCTM, ppoly->points + i, &ppt, 1);
	             coord = Tcl_NewIntObj((int)ppt.x);
	             Tcl_ListObjAppendElement(interp, cpair, coord);
	             coord = Tcl_NewIntObj((int)ppt.y);
	             Tcl_ListObjAppendElement(interp, cpair, coord);
	             Tcl_ListObjAppendElement(interp, elist, cpair);
		  }
	       }
	       else {
		  splineptr pspline;
		  pspline = (splineptr)(*eptr);
	          Tcl_ListObjAppendElement(interp, elist,
				Tcl_NewStringObj("spline", -1));
		  for (i = 0; i < 4; i++) {
		     cpair = Tcl_NewListObj(0, NULL);
		     UTransformbyCTM(&hierCTM, pspline->ctrl + i, &ppt, 1);
	             coord = Tcl_NewIntObj((int)ppt.x);
	             Tcl_ListObjAppendElement(interp, cpair, coord);
	             coord = Tcl_NewIntObj((int)ppt.y);
	             Tcl_ListObjAppendElement(interp, cpair, coord);
	             Tcl_ListObjAppendElement(interp, elist, cpair);
		  }
	       }
	       Tcl_ListObjAppendElement(interp, objPtr, elist);
	    }
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_instance(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
  int i, numfound, idx, nidx, result;
   objectptr pobj;
   objinstptr pinst, newinst;
   short *newselect;
   XPoint newpos, ppt;
   Tcl_Obj *objPtr;
   Matrix hierCTM;

   static char *subCmds[] = {"make", "object", "scale", "center", "linewidth",
			"bbox", "netlist", NULL};
   enum SubIdx {
	MakeIdx, ObjectIdx, ScaleIdx, CenterIdx, LineWidthIdx, BBoxIdx, NetListIdx
   };

   static char *lwsubCmds[] = {"scale_variant", "variant", "scale_invariant",
			"invariant", NULL};

   nidx = 3;
   result = ParseElementArguments(interp, objc, objv, &nidx, OBJINST);
   if (result != TCL_OK) return result;

   if ((result = Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx)) != TCL_OK)
      return result;

   switch (idx) {
      case MakeIdx:
	 if ((areawin->selects == 0) && (nidx == 1)) {
	    if (objc == 3) {
	       pobj = NameToObject(Tcl_GetString(objv[2]), &pinst, False);
	       if (pobj == NULL) {
		  Tcl_SetResult(interp, "no such object ", NULL);
		  Tcl_AppendResult(interp, Tcl_GetString(objv[2]), NULL);
		  return TCL_ERROR;
	       }
	       newpos = UGetCursorPos();
	       u2u_snap(&newpos);
	       newinst = new_objinst(NULL, pinst, newpos.x, newpos.y);
	       newinst->color = areawin->color;
	       newselect = allocselect();
	       *newselect = (short)(topobject->parts - 1);
	       draw_normal_selected(topobject, areawin->topinstance);
	       eventmode = COPY_MODE;
	       Tk_CreateEventHandler(areawin->area, PointerMotionMask,
			(Tk_EventProc *)xctk_drag, NULL);
	       return XcTagCallback(interp, objc, objv);
	    }
	    else if (objc != 4) {
	       Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	       return TCL_ERROR;
	    }
	    pobj = NameToObject(Tcl_GetString(objv[2]), &pinst, False);
	    if (pobj == NULL) {
	       Tcl_SetResult(interp, "no such object ", NULL);
	       Tcl_AppendResult(interp, Tcl_GetString(objv[2]), NULL);
	       return TCL_ERROR;
	    }
	    result = GetPositionFromList(interp, objv[3], &newpos);
	    if (result != TCL_OK) return result;

	    newinst = new_objinst(NULL, pinst, newpos.x, newpos.y);
	    newinst->color = areawin->color;
	    singlebbox((genericptr *)&newinst);
	    objPtr = Tcl_NewHandleObj(newinst);
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else if (nidx == 2) {
	    Tcl_SetResult(interp, "\"instance <handle> make\" is illegal", NULL);
	    return TCL_ERROR;
	 }
	 else {
	    Tcl_SetResult(interp, "No selections allowed.", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case ObjectIdx:
	 if ((objc - nidx) == 1) {
	    Tcl_Obj *listPtr;
	    numfound = 0;
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  objPtr = Tcl_NewStringObj(pinst->thisobject->name, -1);
		  if (numfound > 0)
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		  if ((++numfound) == 1)
		     listPtr = objPtr;
	       }
	    }
	    switch (numfound) {
	       case 0:
		  Tcl_SetResult(interp, "Error: no object instances selected", NULL);
		  return TCL_ERROR;
		  break;
	       case 1:
	          Tcl_SetObjResult(interp, objPtr);
		  break;
	       default:
	          Tcl_SetObjResult(interp, listPtr);
		  break;
	    }
	 }
	 else {
	    Tcl_Obj *listPtr;
	    int listlen;
	    objectptr pobj;

	    /* If the number of additional arguments matches the number	*/
	    /* of selected items, or if there is one additional item	*/
	    /* that is a list with a number of items equal to the	*/
	    /* number of selected items, then change each element to	*/
	    /* the corresponding object in the list.  If there is only	*/
	    /* one additional item, change all elements to that object.	*/

	    if ((objc - nidx) == 1 + areawin->selects) {
	       // Change each element in turn to the corresponding object
	       // taken from the command arguments
	       for (i = 0; i < areawin->selects; i++) {
		  pobj = NameToObject(Tcl_GetString(objv[2 + i]), NULL, FALSE);
	          if (pobj == NULL) {
	             Tcl_SetResult(interp, "Name is not a known object", NULL);
		     return TCL_ERROR;
	          }
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  pinst->thisobject = pobj;
		  calcbboxinst(pinst);
	       }
	    }
	    else if ((objc - nidx) == 2) {
	       result = Tcl_ListObjLength(interp, objv[2], &listlen);
	       if (result != TCL_OK) return result;
	       if (listlen == 1) {
		  // Check if the indicated object exists
		  pobj = NameToObject(Tcl_GetString(objv[2]), NULL, FALSE);
	          if (pobj == NULL) {
	             Tcl_SetResult(interp, "Name is not a known object", NULL);
		     return TCL_ERROR;
	          }

		  // Change all selected elements to the object specified
	          for (i = 0; i < areawin->selects; i++) {
		     pinst = SELTOOBJINST(areawin->selectlist + i);
		     pinst->thisobject = pobj;
		     calcbboxinst(pinst);
	          }
	       }
	       else if (listlen != areawin->selects) {
		  Tcl_SetResult(interp, "Error: list length does not match"
				"the number of selected elements.", NULL);
		  return TCL_ERROR;
	       }
	       else {
		  // Change each element in turn to the corresponding object
		  // in the list
	          for (i = 0; i < areawin->selects; i++) {
		     result = Tcl_ListObjIndex(interp, objv[2], i, &listPtr);
		     if (result != TCL_OK) return result;

		     pobj = NameToObject(Tcl_GetString(listPtr), NULL, FALSE);
	             if (pobj == NULL) {
	                Tcl_SetResult(interp, "Name is not a known object", NULL);
		        return TCL_ERROR;
	             }
		     pinst = SELTOOBJINST(areawin->selectlist + i);
		     pinst->thisobject = pobj;
		     calcbboxinst(pinst);
		  }
	       }
	    }
	    drawarea(areawin->area, NULL, NULL);
	 }
	 break;

      case ScaleIdx:
	 if ((objc - nidx) == 1) {
	    Tcl_Obj *listPtr;
	    numfound = 0;
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  objPtr = Tcl_NewDoubleObj(pinst->scale);
		  if (numfound > 0)
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		  if ((++numfound) == 1)
		     listPtr = objPtr;
	       }
	    }
	    switch (numfound) {
	       case 0:
		  Tcl_SetResult(interp, "Error: no object instances selected", NULL);
		  return TCL_ERROR;
		  break;
	       case 1:
	          Tcl_SetObjResult(interp, objPtr);
		  break;
	       default:
	          Tcl_SetObjResult(interp, listPtr);
		  break;
	    }
	 }
	 else {
	    strcpy(_STR2, Tcl_GetString(objv[2]));
	    setosize((Tk_Window)clientData, NULL);
	 }
	 break;

      case CenterIdx:

	 if ((objc - nidx) == 1) {
	    Tcl_Obj *listPtr, *coord;
	    numfound = 0;
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  MakeHierCTM(&hierCTM);
		  objPtr = Tcl_NewListObj(0, NULL);
	          UTransformbyCTM(&hierCTM, &pinst->position, &ppt, 1);
		  coord = Tcl_NewIntObj((int)ppt.x);
		  Tcl_ListObjAppendElement(interp, objPtr, coord);
		  coord = Tcl_NewIntObj((int)ppt.y);
		  Tcl_ListObjAppendElement(interp, objPtr, coord);
		  if (numfound > 0)
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		  if ((++numfound) == 1)
		     listPtr = objPtr;
	       }
	    }
	    switch (numfound) {
	       case 0:
		  Tcl_SetResult(interp, "Error: no object instances selected", NULL);
		  return TCL_ERROR;
		  break;
	       case 1:
	          Tcl_SetObjResult(interp, objPtr);
		  break;
	       default:
	          Tcl_SetObjResult(interp, listPtr);
		  break;
	    }
	 }
	 else if (((objc - nidx) == 2) && (areawin->selects == 1)) {
	    result = GetPositionFromList(interp, objv[objc - 1], &newpos);
	    if (result != TCL_OK) return result;
	    if (SELECTTYPE(areawin->selectlist) == OBJINST) {
	       pinst = SELTOOBJINST(areawin->selectlist);
	       MakeHierCTM(&hierCTM);
	       UTransformbyCTM(&hierCTM, &newpos, &pinst->position, 1);
	    }
	 }
	 else {
	    Tcl_SetResult(interp, "Usage: instance center {x y}; only one"
			"instance should be selected.", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case LineWidthIdx:
	 if ((objc - nidx) == 1) {
	    Tcl_Obj *listPtr;
	    numfound = 0;
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  if (pinst->style & LINE_INVARIANT)
		     objPtr = Tcl_NewStringObj("scale_invariant", -1);
		  else
		     objPtr = Tcl_NewStringObj("scale_variant", -1);
		  if (numfound > 0)
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		  if ((++numfound) == 1)
		     listPtr = objPtr;
	       }
	    }
	    switch (numfound) {
	       case 0:
		  Tcl_SetResult(interp, "Error: no object instances selected", NULL);
		  return TCL_ERROR;
		  break;
	       case 1:
	          Tcl_SetObjResult(interp, objPtr);
		  break;
	       default:
	          Tcl_SetObjResult(interp, listPtr);
		  break;
	    }
	 }
	 else {
	    int subidx;
            if ((result = Tcl_GetIndexFromObj(interp, objv[nidx + 1],
			(CONST84 char **)lwsubCmds,
			"value", 0, &subidx)) == TCL_OK) {
	       for (i = 0; i < areawin->selects; i++) {
	          if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
	             pinst = SELTOOBJINST(areawin->selectlist + i);
		     if (subidx < 2)
		        pinst->style &= ~LINE_INVARIANT;
		     else
		        pinst->style |= LINE_INVARIANT;
		  }
	       }
	    }
	 }
	 break;

      case NetListIdx:
	 if ((objc - nidx) == 1) {
	    Tcl_Obj *listPtr;
	    numfound = 0;
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  objPtr = Tcl_NewBooleanObj((pinst->style & INST_NONETLIST) ?
			    FALSE : TRUE);
		  if (numfound > 0)
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		  if ((++numfound) == 1)
		     listPtr = objPtr;
	       }
	    }
	    switch (numfound) {
	       case 0:
		  Tcl_SetResult(interp, "Error: no object instances selected", NULL);
		  return TCL_ERROR;
		  break;
	       case 1:
	          Tcl_SetObjResult(interp, objPtr);
		  break;
	       default:
	          Tcl_SetObjResult(interp, listPtr);
		  break;
	    }
	 }
	 else {
	    int value;
	    if ((result = Tcl_GetBooleanFromObj(interp, objv[nidx + 1], &value))
			== TCL_OK) {
	       for (i = 0; i < areawin->selects; i++) {
	          if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
	             pinst = SELTOOBJINST(areawin->selectlist + i);
		     if (value)
		        pinst->style &= ~INST_NONETLIST;
		     else
		        pinst->style |= INST_NONETLIST;
		  }
	       }
	    }
	 }
	 break;

      case BBoxIdx:
	 if ((objc - nidx) == 1) {
	    Tcl_Obj *listPtr, *coord;
	    numfound = 0;
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  objPtr = Tcl_NewListObj(0, NULL);
		  coord = Tcl_NewIntObj((int)pinst->bbox.lowerleft.x);
		  Tcl_ListObjAppendElement(interp, objPtr, coord);
		  coord = Tcl_NewIntObj((int)pinst->bbox.lowerleft.y);
		  Tcl_ListObjAppendElement(interp, objPtr, coord);
		  coord = Tcl_NewIntObj((int)(pinst->bbox.lowerleft.x +
				pinst->bbox.width));
		  Tcl_ListObjAppendElement(interp, objPtr, coord);
		  coord = Tcl_NewIntObj((int)(pinst->bbox.lowerleft.y +
				pinst->bbox.height));
		  Tcl_ListObjAppendElement(interp, objPtr, coord);
		  if (numfound > 0)
		     Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		  if ((++numfound) == 1)
		     listPtr = objPtr;
	       }
	    }
	    switch (numfound) {
	       case 0:
		  Tcl_SetResult(interp, "Error: no object instances selected", NULL);
		  return TCL_ERROR;
		  break;
	       case 1:
	          Tcl_SetObjResult(interp, objPtr);
		  break;
	       default:
	          Tcl_SetObjResult(interp, listPtr);
		  break;
	    }
	 }
	 else {
	    /* e.g., "instance bbox recompute" */
	    for (i = 0; i < areawin->selects; i++) {
	       if (SELECTTYPE(areawin->selectlist + i) == OBJINST) {
		  pinst = SELTOOBJINST(areawin->selectlist + i);
		  calcbbox(pinst);
	       }
	    }
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* "element" configures properties of elements.  Note that if the 	*/
/* second argument is not an element handle (pointer), then operations	*/
/* will be applied to all selected elements.  If there is no element	*/
/* handle and no objects are selected, the operation will be applied	*/
/* to default settings, like the "xcircuit::set" command.		*/
/*----------------------------------------------------------------------*/

int xctcl_element(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result, nidx, idx, i, flags;
   Tcl_Obj *listPtr;
   Tcl_Obj **newobjv;
   int newobjc;
   genericptr egen;
   short *newselect, *tempselect, *orderlist;

   /* Commands */
   static char *subCmds[] = {
      "delete", "copy", "flip", "rotate", "edit", "select", "snap", "move",
	"color", "parameters", "raise", "lower", "exchange", "hide", "show",
	"handle", "deselect", NULL
   };
   enum SubIdx {
      DeleteIdx, CopyIdx, FlipIdx, RotateIdx, EditIdx, 	SelectIdx, SnapIdx,
	MoveIdx, ColorIdx, ParamIdx, RaiseIdx, LowerIdx, ExchangeIdx,
	HideIdx, ShowIdx, HandleIdx, DeselectIdx
   };

   static char *etypes[] = {
	"Label", "Polygon", "Bezier Curve", "Object Instance", "Path",
	"Arc", "Graphic", NULL  /* (jdk) */
   };

   /* Before doing a standard parse, we need to check for the single case */
   /* "element X deselect"; otherwise, calling ParseElementArguements()  */
   /* is going to destroy the selection list.				  */

   if ((objc == 3) && (!strcmp(Tcl_GetString(objv[2]), "deselect"))) {
      result = xctcl_deselect(clientData, interp, objc, objv);
      return result;
   }

   /* All other commands are dispatched to individual element commands	*/
   /* for the indicated element or for each selected element.		*/

   nidx = 7;
   result = ParseElementArguments(interp, objc, objv, &nidx, ALL_TYPES);
   if (result != TCL_OK) return result;

   if ((objc - nidx) < 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }

   if (!strcmp(Tcl_GetString(objv[nidx]), "type")) {
      /* Return a list of types of the selected elements */

      if (areawin->selects > 1)
	 listPtr = Tcl_NewListObj(0, NULL);

      for (i = 0; i < areawin->selects; i++) {
	 Tcl_Obj *objPtr;
	 int idx2, type = SELECTTYPE(areawin->selectlist + i);
	 switch (type) {
	    case LABEL: idx2 = 0; break;
	    case POLYGON: idx2 = 1; break;
	    case SPLINE: idx2 = 2; break;
	    case OBJINST: idx2 = 3; break;
	    case PATH: idx2 = 4; break;
	    case ARC: idx2 = 5; break;
	    case GRAPHIC: idx2 = 6; break;
	    default: return TCL_ERROR;
	 }
	 objPtr = Tcl_NewStringObj(etypes[idx2], strlen(etypes[idx2]));
	 if (areawin->selects == 1) {
	    Tcl_SetObjResult(interp, objPtr);
	    return TCL_OK;
	 }
	 else {
	    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	 }
	 Tcl_SetObjResult(interp, listPtr);
      }
      return XcTagCallback(interp, objc, objv);
   }
   else if (!strcmp(Tcl_GetString(objv[nidx]), "handle")) {
      /* Return a list of handles of the selected elements */

      listPtr = SelectToTclList(interp, areawin->selectlist, areawin->selects);
      Tcl_SetObjResult(interp, listPtr);
      return XcTagCallback(interp, objc, objv);
   }

   if (Tcl_GetIndexFromObj(interp, objv[nidx],
		(CONST84 char **)subCmds,
		"option", 0, &idx) == TCL_OK) {

      newobjv = (Tcl_Obj **)(&objv[nidx]);
      newobjc = objc - nidx;

      /* Shift the argument list and call the indicated function. */

      switch(idx) {
	 case DeleteIdx:
	    result = xctcl_delete(clientData, interp, newobjc, newobjv);
	    break;
	 case CopyIdx:
	    result = xctcl_copy(clientData, interp, newobjc, newobjv);
	    break;
	 case FlipIdx:
	    result = xctcl_flip(clientData, interp, newobjc, newobjv);
	    break;
	 case RotateIdx:
	    result = xctcl_rotate(clientData, interp, newobjc, newobjv);
	    break;
	 case EditIdx:
	    result = xctcl_edit(clientData, interp, newobjc, newobjv);
	    break;
	 case ParamIdx:
	    result = xctcl_param(clientData, interp, newobjc, newobjv);
	    break;
	 case HideIdx:
	    for (i = 0; i < areawin->selects; i++) {
	       newselect = areawin->selectlist + i;
	       egen = SELTOGENERIC(newselect);
	       egen->type |= DRAW_HIDE;
	    }
	    refresh(NULL, NULL, NULL);
	    break;
	 case ShowIdx:
	    if (newobjc == 2) {
	       if (!strcmp(Tcl_GetString(newobjv[1]), "all")) {
		  for (i = 0; i < topobject->parts; i++) {
		     egen = *(topobject->plist + i);
		     egen->type &= (~DRAW_HIDE);
		  }
	       }
	    }
	    else {
	       for (i = 0; i < areawin->selects; i++) {
		  newselect = areawin->selectlist + i;
		  egen = SELTOGENERIC(newselect);
		  egen->type &= (~DRAW_HIDE);
	       }
	    }
	    refresh(NULL, NULL, NULL);
	    break;
	 case SelectIdx:
	    if (newobjc == 2) {
	       if (!strncmp(Tcl_GetString(newobjv[1]), "hide", 4)) {
		  for (i = 0; i < areawin->selects; i++) {
		     newselect = areawin->selectlist + i;
		     egen = SELTOGENERIC(newselect);
		     egen->type |= SELECT_HIDE;
		  }
	       }
	       else if (!strncmp(Tcl_GetString(newobjv[1]), "allow", 5)) {
		  for (i = 0; i < topobject->parts; i++) {
		     egen = *(topobject->plist + i);
		     egen->type &= (~SELECT_HIDE);
		  }
	       }
	       else {
		  Tcl_SetResult(interp, "Select options are \"hide\" "
				"and \"allow\"", NULL);
		  return TCL_ERROR;
	       }
	    }
	    /* If nidx == 2, then we've already done the selection! */
	    else if (nidx == 1)
	       result = xctcl_select(clientData, interp, newobjc, newobjv);
	    else
	       result = TCL_OK;
	    break;
	 case DeselectIdx:
	    /* case nidx == 2 was already taken care of. case nidx == 1 */
	    /* implies "deselect all".					*/
	    unselect_all();
	    result = TCL_OK;
	    break;
	 case ColorIdx:
	    result = xctcl_color(clientData, interp, newobjc, newobjv);
	    break;
	 case SnapIdx:
	    snapelement();
	    break;
	 case ExchangeIdx:
	    exchange();
	    break;
	 case LowerIdx:

	    /* Improved method thanks to Dimitri Princen */

	    /* First move the selected parts to the bottom.  This sets	*/
	    /* all the values pointed by (selectlist + i) to zero, and	*/
	    /* inverts the order between the selected elements.		*/
	    /* Finally *tempselect += i inverts the original numbering,	*/
	    /* so the second loop inverts the placing again, regaining	*/
	    /* the correct order (and writes it so).			*/
	    /*								*/
	    /* RaiseIdx works similar but starts from the top.		*/

	    if (newobjc == 2) {
	       if (!strcmp(Tcl_GetString(newobjv[1]), "all")) {
	          orderlist = (short *)malloc(topobject->parts * sizeof(short));
	          for (i = 0; i < topobject->parts; i++) *(orderlist + i) = i;

	          for (i = 0; i < areawin->selects; i++) {
	             tempselect = areawin->selectlist + i;
  	             xc_bottom(tempselect, orderlist);
  	             *tempselect += i;
  	          }
  	          for (i = 0; i < areawin->selects; i++) {
  	             tempselect = areawin->selectlist + i;
  	             xc_bottom(tempselect, orderlist);
  	             *tempselect += (areawin->selects - 1 - i);
  	          }
	          register_for_undo(XCF_Reorder, UNDO_MORE, areawin->topinstance,
			orderlist, topobject->parts);
	       }
	    }
	    else {
  	       xc_lower();
	    }
	    break;

	 case RaiseIdx:

	    /* Improved method thanks to Dimitri Princen */

	    if (newobjc == 2) {
	       if (!strcmp(Tcl_GetString(newobjv[1]), "all")) {
	          orderlist = (short *)malloc(topobject->parts * sizeof(short));
	          for (i = 0; i < topobject->parts; i++) *(orderlist + i) = i;

 	          for (i = areawin->selects - 1; i >= 0 ; i--) {
 	             tempselect = areawin->selectlist + i;
 	             xc_top(tempselect, orderlist);
 	             *tempselect -= (areawin->selects - 1 - i);
 	          }
 	          for (i = areawin->selects - 1; i >= 0 ; i--) {
 	             tempselect = areawin->selectlist + i;
 	             xc_top(tempselect, orderlist);
 	             *tempselect -= i;
 	          }
	          register_for_undo(XCF_Reorder, UNDO_MORE, areawin->topinstance,
			orderlist, topobject->parts);
	       }
	    }
	    else {
 	       xc_raise();
	    }
	    break;

	 case MoveIdx:
	    result = xctcl_move(clientData, interp, newobjc, newobjv);
	    break;
      }
      return result;
   }

   /* Call each individual element function.				*/
   /* Each function is responsible for filtering the select list to	*/
   /* choose only the appropriate elements.  However, we first check	*/
   /* if at least one of that type exists in the list, so the function	*/
   /* won't return an error.						*/

   Tcl_ResetResult(interp);

   newobjv = (Tcl_Obj **)(&objv[nidx - 1]);
   newobjc = objc - nidx + 1;

   flags = 0;
   for (i = 0; i < areawin->selects; i++)
      flags |= SELECTTYPE(areawin->selectlist + i);

   if (flags & LABEL) {
      result = xctcl_label(clientData, interp, newobjc, newobjv);
      if (result != TCL_OK) return result;
   }
   if (flags & POLYGON) {
      result = xctcl_polygon(clientData, interp, newobjc, newobjv);
      if (result != TCL_OK) return result;
   }
   if (flags & OBJINST) {
      result = xctcl_instance(clientData, interp, newobjc, newobjv);
      if (result != TCL_OK) return result;
   }
   if (flags & SPLINE) {
      result = xctcl_spline(clientData, interp, newobjc, newobjv);
      if (result != TCL_OK) return result;
   }
   if (flags & PATH) {
      result = xctcl_path(clientData, interp, newobjc, newobjv);
      if (result != TCL_OK) return result;
   }
   if (flags & ARC) {
      result = xctcl_arc(clientData, interp, newobjc, newobjv);
   }
   if (flags & GRAPHIC) {
      result = xctcl_graphic(clientData, interp, newobjc, newobjv);
   }
   return result;
}

/*----------------------------------------------------------------------*/
/* "config" manipulates a whole bunch of option settings.		*/
/*----------------------------------------------------------------------*/

int xctcl_config(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int tmpint, i;
   int result, idx;
   char *tmpstr, buffer[30], **sptr;
   Pagedata *curpage;

   static char *boxsubCmds[] = {"manhattan", "rhomboidx", "rhomboidy",
	"rhomboida", "normal", NULL};
   static char *pathsubCmds[] = {"tangents", "normal", NULL};
   static char *coordsubCmds[] = {"decimal inches", "fractional inches",
	"centimeters", "internal units", NULL};
   static char *filterTypes[] = {"instances", "labels", "polygons", "arcs",
	"splines", "paths", "graphics", NULL};
   static char *searchOpts[] = {"files", "lib", "libs", "library", "libraries", NULL};

   static char *subCmds[] = {
      "axis", "axes", "grid", "snap", "bbox", "editinplace",
	"pinpositions", "pinattach", "clipmasks", "boxedit", "pathedit", "linewidth",
	"colorscheme", "coordstyle", "drawingscale", "manhattan", "centering",
	"filter", "buschar", "backup", "search", "focus", "init",
	"delete", "windownames", "hold", "database", "suspend",
	"technologies", "fontnames", "debug", NULL
   };
   enum SubIdx {
      AxisIdx, AxesIdx, GridIdx, SnapIdx, BBoxIdx, EditInPlaceIdx,
	PinPosIdx, PinAttachIdx, ShowClipIdx, BoxEditIdx, PathEditIdx, LineWidthIdx,
	ColorSchemeIdx, CoordStyleIdx, ScaleIdx, ManhattanIdx, CenteringIdx,
	FilterIdx, BusCharIdx, BackupIdx, SearchIdx, FocusIdx,
	InitIdx, DeleteIdx, WindowNamesIdx, HoldIdx, DatabaseIdx,
	SuspendIdx, TechnologysIdx, FontNamesIdx, DebugIdx
   };

   if ((objc == 1) || (objc > 5)) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }
   if (Tcl_GetIndexFromObj(interp, objv[1],
		(CONST84 char **)subCmds,
		"option", 0, &idx) != TCL_OK) {
      return TCL_ERROR;
   }

   /* Set curpage for those routines that need it */

   switch(idx) {
      case GridIdx:
      case SnapIdx:
      case LineWidthIdx:
      case CoordStyleIdx:
      case ScaleIdx:
	 if (areawin == NULL) {
	    Tcl_SetResult(interp, "No current window set, assuming default\n",
			NULL);
	    curpage = xobjs.pagelist[0];
	    if (curpage == NULL) return TCL_ERROR;
	 }
	 else
	    curpage = xobjs.pagelist[areawin->page];
	 break;
   }

   /* Check number of arguments wholesale (to be done) */

   switch(idx) {
      case SuspendIdx:
	 if (objc == 2) {
	    switch (xobjs.suspend) {
	       case -1:
	          Tcl_SetResult(interp, "normal drawing", NULL);
		  break;
	       case 0:
	          Tcl_SetResult(interp, "drawing suspended", NULL);
		  break;
	       case 1:
	          Tcl_SetResult(interp, "refresh pending", NULL);
		  break;
	       case 2:
	          Tcl_SetResult(interp, "drawing locked", NULL);
		  break;
	    }
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    if (tmpint == 0) {

	       /* Pending drawing */

	       if (xobjs.suspend == 1) {
	          xobjs.suspend = -1;
		  refresh(NULL, NULL, NULL);
	       }
	       else
	          xobjs.suspend = -1;
	    }
	    else {
	       /* Calling "config suspend true" twice effectively	*/
	       /* locks the graphics in a state that can only be	*/
	       /* removed by a call to "config suspend false".		*/
	       if (xobjs.suspend >= 0)
	          xobjs.suspend = 2;
	       else
	          xobjs.suspend = 0;
	    }
	 }
	 break;

      case DatabaseIdx:
	 /* Regenerate the database of colors, fonts, etc. from Tk options */
	 if (objc == 3) {
	    Tk_Window tkwind, tktop;

	    tktop = Tk_MainWindow(interp);
	    tkwind = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), tktop);
	    build_app_database(tkwind);
	    setcolorscheme(!areawin->invert);
	 }
	 break;

      case FontNamesIdx:
	 /* To do:  Return a list of known font names.  The Tk wrapper uses */
	 /* this list to regenerate the font menu for each new window.	    */
	 break;

      case WindowNamesIdx:
	 /* Generate and return a list of existing window names */

	 if (objc == 2) {
	    XCWindowData *winptr;
	    for (winptr = xobjs.windowlist; winptr != NULL; winptr = winptr->next)
	       Tcl_AppendElement(interp, Tk_PathName(winptr->area));
	 }
	 break;

      case DeleteIdx:
	 if (objc == 3) {
	    XCWindowData *winptr;
	    Tk_Window tkwind, tktop;

	    tktop = Tk_MainWindow(interp);
	    tkwind = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), tktop);
	    for (winptr = xobjs.windowlist; winptr != NULL; winptr = winptr->next) {
	       if (winptr->area == tkwind) {
		   delete_window(winptr);
		   break;
	       }
	    }
	    if (winptr == NULL) {
	       Tcl_SetResult(interp, "No such window\n", NULL);
	       return TCL_ERROR;
	    }
	 }
	 break;

      case DebugIdx:
#ifdef ASG
	 if (objc == 3) {
	    result = Tcl_GetIntFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    SetDebugLevel(&tmpint);
	 }
	 else {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(SetDebugLevel(NULL)));
	 }
#endif
	 break;


      case InitIdx:
	 /* Create a data structure for a new drawing window. */
	 /* Give it the same page number and view as the current window */

	 if (objc == 3) {
	    XCWindowData *newwin, *savewin;
	    savewin = areawin;	// In case focus callback overwrites areawin.
	    newwin = GUI_init(objc - 2, objv + 2);
	    if (newwin != NULL) {
	       newwin->page = savewin->page;
	       newwin->vscale = savewin->vscale;
	       newwin->pcorner = savewin->pcorner;
	       newwin->topinstance = savewin->topinstance;
	    }
	    else {
	       Tcl_SetResult(interp, "Unable to create new window structure\n", NULL);
	       return TCL_ERROR;
	    }
	 }
	 break;

      case FocusIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, Tk_PathName(areawin->area), NULL);
	 }
	 else if (objc == 3) {
	    Tk_Window tkwind, tktop;
	    XCWindowData *winptr;
	    XPoint locsave;

	    tktop = Tk_MainWindow(interp);
	    tkwind = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), tktop);
	    /* (Diagnostic) */
	    /* printf("Focusing: %s\n", Tcl_GetString(objv[2])); */
	    for (winptr = xobjs.windowlist; winptr != NULL; winptr = winptr->next) {
	       if (winptr->area == tkwind) {
		  int savemode;
		  objectptr savestack;

		  if (areawin == winptr) break;
		  else if (areawin == NULL) {
		     areawin = winptr;
		     break;
		  }
		  if ((eventmode == MOVE_MODE || eventmode == COPY_MODE) &&
				winptr->editstack->parts == 0) {
		     locsave = areawin->save;
		     delete_for_xfer(NORMAL, areawin->selectlist, areawin->selects);
		     /* Swap editstacks */
		     savestack = winptr->editstack;
		     winptr->editstack = areawin->editstack;
		     areawin->editstack = savestack;
		     savemode = eventmode;
		     eventmode = NORMAL_MODE;

		     /* Change event handlers */
		     xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
				(xcEventHandler)xctk_drag, NULL);
		     drawarea(areawin->area, NULL, NULL);
		     Tk_CreateEventHandler(winptr->area, PointerMotionMask,
				(Tk_EventProc *)xctk_drag, NULL);

		     /* Set new window */
		     areawin = winptr;
		     eventmode = savemode;
		     areawin->save = locsave;
		     transferselects();
		     drawarea(areawin->area, NULL, NULL);
		  }
		  else
		     areawin = winptr;
		  break;
	       }
	    }
	    if (winptr == NULL) {
	       Tcl_SetResult(interp, "No such xcircuit drawing window\n", NULL);
	       return TCL_ERROR;
	    }
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 2, objv, "[window]");
	    return TCL_ERROR;
	 }
	 break;

      case AxisIdx: case AxesIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->axeson) ? "true" : "false", NULL);
	    break;
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    areawin->axeson = (Boolean) tmpint;
	 }
	 break;

      case GridIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->gridon) ? "true" : "false", NULL);
	    break;
	 }
	 else {
	    if (!strncmp("spac", Tcl_GetString(objv[2]), 4)) {
	       if (objc == 3) {
		  measurestr((float)curpage->gridspace, buffer);
		  Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, strlen(buffer)));
		  break;
	       }
	       else {
	          strcpy(_STR2, Tcl_GetString(objv[3]));
	          setgrid(NULL, &(curpage->gridspace));
	       }
	    }
	    else {
	       result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	       if (result != TCL_OK) return result;
	       areawin->gridon = (Boolean) tmpint;
	    }
	 }
	 break;

      case SnapIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->snapto) ? "true" : "false", NULL);
	 }
	 else {
	    if (!strncmp("spac", Tcl_GetString(objv[2]), 4)) {
	       if (objc == 3) {
		  measurestr((float)curpage->snapspace, buffer);
		  Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, strlen(buffer)));
		  break;
	       }
	       else {
	          strcpy(_STR2, Tcl_GetString(objv[3]));
	          setgrid(NULL, &(curpage->snapspace));
	       }
	    }
	    else {
	       result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	       if (result != TCL_OK) return result;
	       areawin->snapto = (Boolean) tmpint;
	    }
	 }
	 break;

      case BoxEditIdx:
	 if (objc == 2) {
	    switch (areawin->boxedit) {
	       case MANHATTAN: idx = 0; break;
	       case RHOMBOIDX: idx = 1; break;
	       case RHOMBOIDY: idx = 2; break;
	       case RHOMBOIDA: idx = 3; break;
	       case NORMAL: idx = 4; break;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(boxsubCmds[idx],
		strlen(boxsubCmds[idx])));
	 }
	 else if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "boxedit ?arg ...?");
	    return TCL_ERROR;
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)boxsubCmds,
			"option", 0, &idx) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    switch (idx) {
	       case 0: tmpint = MANHATTAN; break;
	       case 1: tmpint = RHOMBOIDX; break;
	       case 2: tmpint = RHOMBOIDY; break;
	       case 3: tmpint = RHOMBOIDA; break;
	       case 4: tmpint = NORMAL; break;
	    }
	    areawin->boxedit = tmpint;
	 }
	 break;

      case PathEditIdx:
	 if (objc == 2) {
	    switch (areawin->pathedit) {
	       case TANGENTS: idx = 0; break;
	       case NORMAL: idx = 1; break;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(pathsubCmds[idx],
		strlen(pathsubCmds[idx])));
	 }
	 else if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "pathedit ?arg ...?");
	    return TCL_ERROR;
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)pathsubCmds,
			"option", 0, &idx) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    switch (idx) {
	       case 0: tmpint = TANGENTS; break;
	       case 1: tmpint = NORMAL; break;
	    }
	    areawin->pathedit = tmpint;
	 }
	 break;

      case LineWidthIdx:
	 if (objc == 2) {
	    Tcl_SetObjResult(interp,
		Tcl_NewDoubleObj((double)curpage->wirewidth / 2.0));
	 }
	 else if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, "linewidth");
	    return TCL_ERROR;
	 }
	 else {
	    strcpy(_STR2, Tcl_GetString(objv[2]));
	    setwidth(NULL, &(curpage->wirewidth));
	 }
	 break;

      case BBoxIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->bboxon) ? "visible" : "invisible", NULL);
	 }
	 else {
	    tmpstr = Tcl_GetString(objv[2]);
	    if (strstr(tmpstr, "visible"))
	       tmpint = (tmpstr[0] == 'i') ? False : True;
	    else {
	       result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	       if (result != TCL_OK) return result;
	    }
	    areawin->bboxon = (Boolean) tmpint;
	 }
	 break;

      case HoldIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (xobjs.hold) ? "true" : "false", NULL);
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    xobjs.hold = (Boolean) tmpint;
	 }
	 break;

      case EditInPlaceIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->editinplace) ? "true" : "false", NULL);
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    areawin->editinplace = (Boolean) tmpint;
	 }
	 break;

      case ShowClipIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->showclipmasks) ? "show" : "hide", NULL);
	 }
	 else {
	    tmpstr = Tcl_GetString(objv[2]);
	    if (!strcmp(tmpstr, "show"))
	       tmpint = True;
	    else if (!strcmp(tmpstr, "hide"))
	       tmpint = False;
	    else {
	       result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	       if (result != TCL_OK) return result;
	    }
	    areawin->showclipmasks = (Boolean) tmpint;
	 }
	 break;

      case PinPosIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->pinpointon) ? "visible" : "invisible", NULL);
	 }
	 else {
	    tmpstr = Tcl_GetString(objv[2]);
	    if (strstr(tmpstr, "visible"))
	       tmpint = (tmpstr[0] == 'i') ? False : True;
	    else {
	       result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	       if (result != TCL_OK) return result;
	    }
	    areawin->pinpointon = (Boolean) tmpint;
	 }
	 break;

      case PinAttachIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->pinattach) ? "true" : "false", NULL);
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    areawin->pinattach = (Boolean) tmpint;
	 }
	 break;

      case ColorSchemeIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->invert) ? "inverse" : "normal", NULL);
	 }
	 else {
	    tmpstr = Tcl_GetString(objv[2]);
	    if (!strcmp(tmpstr, "normal") || !strcmp(tmpstr, "standard"))
	       tmpint = False;
	    else if (!strcmp(tmpstr, "inverse") || !strcmp(tmpstr, "alternate"))
	       tmpint = True;
	    else {
	       result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	       if (result != TCL_OK) return result;
	    }
	    areawin->invert = (Boolean) tmpint;
	    setcolorscheme(!areawin->invert);
	 }
	 break;

      case CoordStyleIdx:
	 if (objc == 2) {
	    switch (curpage->coordstyle) {
	       case DEC_INCH: idx = 0; break;
	       case FRAC_INCH: idx = 1; break;
	       case CM: idx = 2; break;
	       case INTERNAL: idx = 3; break;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(coordsubCmds[idx],
		strlen(coordsubCmds[idx])));
	 }
	 else if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "coordstyle ?arg ...?");
	    return TCL_ERROR;
	 }
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)coordsubCmds,
			"option", 0, &idx) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    switch (idx) {
	       case 0: tmpint = DEC_INCH; break;
	       case 1: tmpint = FRAC_INCH; break;
	       case 2: tmpint = CM; break;
	       case 3: tmpint = INTERNAL; break;
	    }
	    getgridtype(NULL, tmpint, NULL);
	 }
	 break;

      case ScaleIdx:
	 if (objc == 2) {
	    Tcl_Obj *objPtr = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(interp, objPtr,
	 	Tcl_NewIntObj((int)curpage->drawingscale.x));
	    Tcl_ListObjAppendElement(interp, objPtr,
	 	Tcl_NewStringObj(":", 1));
	    Tcl_ListObjAppendElement(interp, objPtr,
	 	Tcl_NewIntObj((int)curpage->drawingscale.y));
	    Tcl_SetObjResult(interp, objPtr);
	 }
	 else if (objc == 3) {
	    strcpy(_STR2, Tcl_GetString(objv[2]));
	    setdscale(NULL, &(curpage->drawingscale));
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 2, objv, "drawingscale ?arg ...?");
	    return TCL_ERROR;
	 }
	 break;

      case TechnologysIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (xobjs.showtech) ? "true" : "false", NULL);
	 }
	 else {
	    short libnum;

	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    if (xobjs.showtech != (Boolean) tmpint) {
	       xobjs.showtech = (Boolean) tmpint;

	       /* When namespaces are included, the length of the printed */
	       /* name may cause names to overlap, so recompose each	  */
	       /* library when the showtech flag is changed.		  */
	       for (libnum = 0; libnum < xobjs.numlibs; libnum++)
		  composelib(LIBRARY + libnum);

	       if (eventmode == CATALOG_MODE) refresh(NULL, NULL, NULL);
            }
	 }
	 break;

      case ManhattanIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->manhatn) ? "true" : "false", NULL);
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    areawin->manhatn = (Boolean) tmpint;
	 }
	 break;

      case CenteringIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (areawin->center) ? "true" : "false", NULL);
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    areawin->center = (Boolean) tmpint;
	 }
	 break;

      case FilterIdx:
	 if (objc == 2) {
	    for (i = 0; i < 6; i++) {
	       tmpint = 1 << i;
	       if (areawin->filter & tmpint) {
		  Tcl_AppendElement(interp, filterTypes[i]);
	       }
	    }
	 }
	 else if (objc >= 3) {
	    if (Tcl_GetIndexFromObj(interp, objv[2],
			(CONST84 char **)filterTypes,
			"filter_type", 0, &tmpint) != TCL_OK) {
	       return TCL_ERROR;
	    }
	    if (objc == 3) {
	       if (areawin->filter & (1 << tmpint))
		  Tcl_SetResult(interp, "true", NULL);
	       else
		  Tcl_SetResult(interp, "false", NULL);
	    }
	    else {
	       int ftype = 1 << tmpint;
	       if (!strcmp(Tcl_GetString(objv[3]), "true"))
	          areawin->filter |= ftype;
	       else
	          areawin->filter &= (~ftype);
	    }
	 }
	 break;

      case BusCharIdx:
	 if (objc == 2) {
	    buffer[0] = '\\';
	    buffer[1] = areawin->buschar;
	    buffer[2] = '\0';
	    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
	 }
	 else if (objc == 3) {
	    tmpstr = Tcl_GetString(objv[2]);
	    areawin->buschar = (tmpstr[0] == '\\') ? tmpstr[1] : tmpstr[0];
	 }
	 break;

      case BackupIdx:
	 if (objc == 2) {
	    Tcl_SetResult(interp, (xobjs.retain_backup) ? "true" : "false", NULL);
	 }
	 else {
	    result = Tcl_GetBooleanFromObj(interp, objv[2], &tmpint);
	    if (result != TCL_OK) return result;
	    xobjs.retain_backup = (Boolean) tmpint;
	 }
	 break;

      case SearchIdx:
	 if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "search files|libraries ?arg ...?");
	    return TCL_ERROR;
	 }
	 if (Tcl_GetIndexFromObj(interp, objv[2],
		(CONST84 char **)searchOpts, "options", 0, &idx) != TCL_OK) {
	    return TCL_ERROR;
	 }
	 sptr = (idx == 0) ? &xobjs.filesearchpath : &xobjs.libsearchpath;
	 if (objc == 3) {
	    if (*sptr != NULL) Tcl_SetResult(interp, *sptr, TCL_VOLATILE);
	 }
	 else {
	    if (*sptr != NULL) free(*sptr);
	    *sptr = NULL;
	    tmpstr = Tcl_GetString(objv[3]);
	    if (strlen(tmpstr) > 0)
	       *sptr = strdup(Tcl_GetString(objv[3]));
	 }
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_promptsavepage(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int page = areawin->page;
   int result;
   Pagedata *curpage;
   objectptr pageobj;
   struct stat statbuf;

   /* save page popup */

   if (objc > 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "[page_number]");
      return TCL_ERROR;
   }
   else if (objc == 2) {
      result = Tcl_GetIntFromObj(interp, objv[1], &page);
      if (result != TCL_OK) return result;
   }
   else page = areawin->page;

   curpage = xobjs.pagelist[page];
   if (curpage->pageinst == NULL) {
      Tcl_SetResult(interp, "Page does not exist. . . cannot save.", NULL);
      return TCL_ERROR;
   }
   pageobj = curpage->pageinst->thisobject;

   /* recompute bounding box and auto-scale, if set */

   calcbbox(xobjs.pagelist[page]->pageinst);
   if (curpage->pmode & 2) autoscale(page);

   /* get file information, if filename is set */

   if (curpage->filename != NULL) {
      if (strstr(curpage->filename, ".") == NULL)
         sprintf(_STR2, "%s.ps", curpage->filename);
      else sprintf(_STR2, "%s", curpage->filename);
      if (stat(_STR2, &statbuf) == 0) {
         Wprintf("  Warning:  File exists");
      }
      else {
         if (errno == ENOTDIR)
            Wprintf("Error:  Incorrect pathname");
         else if (errno == EACCES)
            Wprintf("Error:  Path not readable");
         else
            W3printf("  ");
      }
   }
   Tcl_SetObjResult(interp, Tcl_NewIntObj((int)page));

   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_quit(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   Boolean is_intr = False;

   /* quit, without checks */
   if (objc != 1) {
      if (strncasecmp(Tcl_GetString(objv[0]), "intr", 4))
         is_intr = True;
      else {
         Tcl_WrongNumArgs(interp, 1, objv, "(no arguments)");
         return TCL_ERROR;
      }
   }
   quit(areawin->area, NULL);

   if (consoleinterp == interp)
      Tcl_Exit(XcTagCallback(interp, objc, objv));
   else {
      /* Ham-fisted, but prevents hanging on Ctrl-C kill */
      if (is_intr) exit(1);
      Tcl_Eval(interp, "catch {tkcon eval exit}\n");
   }

   return TCL_OK;	/* Not reached */
}

/*----------------------------------------------------------------------*/

int xctcl_promptquit(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result;

   /* quit, with checks */
   if (objc != 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "(no arguments)");
      return TCL_ERROR;
   }
   if (areawin != NULL) {
      result = quitcheck(areawin->area, NULL, NULL);
      if (result == 1) {
	 /* Immediate exit */
         if (consoleinterp == interp)
            Tcl_Exit(XcTagCallback(interp, objc, objv));
         else
            Tcl_Eval(interp, "catch {tkcon eval exit}\n");
      }
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_refresh(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   /* refresh */
   if (objc != 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "(no arguments)");
      return TCL_ERROR;
   }
   areawin->redraw_needed = True;
   drawarea(areawin->area, (caddr_t)clientData, (caddr_t)NULL);
   if (areawin->scrollbarh)
      drawhbar(areawin->scrollbarh, NULL, NULL);
   if (areawin->scrollbarv)
      drawvbar(areawin->scrollbarv, NULL, NULL);
   printname(topobject);
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Load a schematic that belongs to a symbol referenced by the current	*/
/* schematic by loading the file pointed to by the "link" parameter	*/
/* in the symbol.							*/
/*									*/
/* Return 1 on success, 0 if the link has already been loaded, and -1	*/
/* on failure to find, open, or read the link's schematic.		*/
/*----------------------------------------------------------------------*/

int loadlinkfile(objinstptr tinst, char *filename, int target, Boolean do_load)
{
   int j, savepage;
   FILE *ps;
   char file_return[150];
   int result;
   Boolean fgood;

   /* Shorthand: "%n" can be used to indicate that the link filename is	*/
   /* the same as the name of the object, minus technology prefix.	*/
   /* While unlikely to be used, "%N" includes the technology prefix.	*/

   if (!strcmp(filename, "%n")) {
      char *suffix = strstr(tinst->thisobject->name, "::");
      if (suffix == NULL)
	 suffix = tinst->thisobject->name;
      else
	 suffix += 2;
      strcpy(_STR, suffix);
   }
   else if (!strcmp(filename, "%N"))
      strcpy(_STR, tinst->thisobject->name);
   else
      strcpy(_STR, filename);

   /* When loading links, we want to avoid	*/
   /* loading the same file more than once, so	*/
   /* compare filename against all existing	*/
   /* page filenames.  Also compare links; any	*/
   /* page with a link to the same object is a	*/
   /* duplicate.				*/

   ps = fileopen(_STR, ".ps", file_return, 149);
   if (ps != NULL) {
      fgood = TRUE;
      fclose(ps);
   }
   else
      fgood = FALSE;

   for (j = 0; j < xobjs.pages; j++) {
      if (xobjs.pagelist[j]->filename == NULL)
	 continue;
      else if (!strcmp(file_return, xobjs.pagelist[j]->filename))
	 break;
      else if ((strlen(xobjs.pagelist[j]->filename) > 0) &&
		!strcmp(file_return + strlen(file_return) - 3, ".ps")
		&& !strncmp(xobjs.pagelist[j]->filename, file_return,
		strlen(file_return) - 3))
	 break;
      else if ((xobjs.pagelist[j]->pageinst != NULL) && (tinst->thisobject ==
		xobjs.pagelist[j]->pageinst->thisobject->symschem))
	 break;
    }
    if (j < xobjs.pages) {

       /* Duplicate page.  Don't load it, but make sure that an association	*/
       /* exists between the symbol and schematic.				*/

       if (tinst->thisobject->symschem == NULL) {
          tinst->thisobject->symschem =
			xobjs.pagelist[j]->pageinst->thisobject;
	  if (xobjs.pagelist[j]->pageinst->thisobject->symschem == NULL)
	        xobjs.pagelist[j]->pageinst->thisobject->symschem = tinst->thisobject;
       }
       return 0;
    }

    if (fgood == FALSE) {
       Fprintf(stderr, "Failed to open dependency \"%s\"\n", _STR);
       return -1;
    }

    /* Report that a pending link exists, but do not load it. */
    if (!do_load) return 1;

    savepage = areawin->page;
    while (areawin->page < xobjs.pages &&
   	   xobjs.pagelist[areawin->page]->pageinst != NULL &&
	    xobjs.pagelist[areawin->page]->pageinst->thisobject->parts > 0)
       areawin->page++;

    changepage(areawin->page);
    result = (loadfile(0, (target >= 0) ? target + LIBRARY : -1) == TRUE) ? 1 : -1;

    /* Make symschem link if not done by loadfile() */

    if (tinst->thisobject->symschem == NULL) {
       tinst->thisobject->symschem =
		xobjs.pagelist[areawin->page]->pageinst->thisobject;

       /* Many symbols may link to one schematic, but a schematic can	*/
       /* only link to one symbol (the first one associated).		*/

       if (xobjs.pagelist[areawin->page]->pageinst->thisobject->symschem == NULL)
	  xobjs.pagelist[areawin->page]->pageinst->thisobject->symschem
			= tinst->thisobject;
    }
    changepage(savepage);
    return result;
}

/*----------------------------------------------------------------------*/

int xctcl_page(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int result, idx, nidx, aval, i, locidx;
   int cpage, multi, savepage, pageno = -1, linktype, importtype;
   char *filename, *froot, *astr;
   Tcl_Obj *objPtr;
   double newheight, newwidth, newscale;
   float oldscale;
   int newrot, newmode;
   objectptr pageobj;
   oparamptr ops;
   char *oldstr, *newstr, *key, *argv;
   Pagedata *curpage, *lpage;
   short *pagelist;
   u_short changes;
   int target = -1;
   Boolean forcepage = FALSE;

   char *subCmds[] = {
	"load", "list", "import", "save", "saveonly", "make", "directory",
	"reset", "links", "fit", "filename", "label", "scale", "width",
	"height", "size", "margins", "bbox", "goto", "orientation",
	"encapsulation", "handle", "update", "changes", NULL
   };
   enum SubIdx {
	LoadIdx, ListIdx, ImportIdx, SaveIdx, SaveOnlyIdx, MakeIdx, DirIdx,
	ResetIdx, LinksIdx, FitIdx, FileIdx, LabelIdx, ScaleIdx,
	WidthIdx, HeightIdx, SizeIdx, MarginsIdx, BBoxIdx, GoToIdx,
	OrientIdx, EPSIdx, HandleIdx, UpdateIdx, ChangesIdx
   };

   char *importTypes[] = {"xcircuit", "postscript", "background", "spice", NULL};
   enum ImportTypes {
	XCircuitIdx, PostScriptIdx, BackGroundIdx, SPICEIdx
   };

   char *linkTypes[] = {"independent", "dependent", "total", "linked",
		"pagedependent", "all", "pending", "sheet", "load", NULL};
   enum LinkTypes {
	IndepIdx, DepIdx, TotalIdx, LinkedIdx, PageDepIdx, AllIdx, PendingIdx,
	SheetIdx, LinkLoadIdx
   };
   char *psTypes[] = {"eps", "full", NULL};

   if (areawin == NULL) {
      Tcl_SetResult(interp, "No database!", NULL);
      return TCL_ERROR;
   }
   savepage = areawin->page;

   /* Check for option "-force" (create page if it doesn't exist) */
   if (!strncmp(Tcl_GetString(objv[objc - 1]), "-forc", 5)) {
      forcepage = TRUE;
      objc--;
   }

   result = ParsePageArguments(interp, objc, objv, &nidx, &pageno);
   if ((result != TCL_OK) || (nidx < 0)) {
      if (forcepage && (pageno == xobjs.pages)) {
	 /* For now, allow a page to be created only if the page number	*/
	 /* is one higher than the current last page.			*/
	 Tcl_ResetResult(interp);
	 idx = MakeIdx;
	 nidx = 0;
	 pageno = areawin->page;	/* so we don't get a segfault */
      }
      else
	 return result;
   }
   else if (nidx == 1 && objc == 2) {
      idx = GoToIdx;
   }
   else if (Tcl_GetIndexFromObj(interp, objv[1 + nidx],
		(CONST84 char **)subCmds, "option", 0, &idx) != TCL_OK) {
      return result;
   }

   result = TCL_OK;

   curpage = xobjs.pagelist[pageno];

   if (curpage->pageinst != NULL)
      pageobj = curpage->pageinst->thisobject;
   else {
      if (idx != LoadIdx && idx != MakeIdx && idx != DirIdx && idx != GoToIdx) {
	 Tcl_SetResult(interp, "Cannot do function on non-initialized page.", NULL);
	 return TCL_ERROR;
      }
   }

   switch (idx) {
      case HandleIdx:
	 /* return handle of page instance */
	 objPtr = Tcl_NewHandleObj(curpage->pageinst);
	 Tcl_SetObjResult(interp, objPtr);
	 break;

      case ResetIdx:
	 /* clear page */
	 resetbutton(NULL, (pointertype)(pageno + 1), NULL);
	 break;

      case ListIdx:
	 /* return a list of all non-empty pages */
	 objPtr = Tcl_NewListObj(0, NULL);
	 for (i = 0; i < xobjs.pages; i++) {
	     lpage = xobjs.pagelist[i];
	     if ((lpage != NULL) && (lpage->pageinst != NULL)) {
	        Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewIntObj(i + 1));
	     }
	 }
	 Tcl_SetObjResult(interp, objPtr);
	 break;

      case LoadIdx:
	 TechReplaceSave();
	 sprintf(_STR2, "%s", Tcl_GetString(objv[2 + nidx]));
	 for (i = 3 + nidx; i < objc; i++) {
	    argv = Tcl_GetString(objv[i]);
	    if ((*argv == '-') && !strncmp(argv, "-repl", 5)) {
	       if (i < objc - 1) {
		  char *techstr = Tcl_GetString(objv[i + 1]);
		  if (!strcmp(techstr, "all") || !strcmp(techstr, "any"))
		     TechReplaceAll();
		  else if (!strcmp(techstr, "none")) TechReplaceNone();
		  else {
		     TechPtr nsptr = LookupTechnology(techstr);
		     if (nsptr != NULL) nsptr->flags |= TECH_REPLACE;
		  }
		  i++;
	       }
	       else
	          TechReplaceAll();	/* replace ALL */
	    }
	    else if ((*argv == '-') && !strncmp(argv, "-targ", 5)) {
	       if (i < objc - 1) {
		  ParseLibArguments(interp, 2, &objv[i], NULL, &target);
		  i++;
	       }
	    }
	    else {
	       strcat(_STR2, ",");
	       strcat(_STR2, argv);
	    }
	 }

	 if (savepage != pageno) newpage(pageno);
	 startloadfile((target >= 0) ? target + LIBRARY : -1);
	 if (savepage != pageno) newpage(savepage);
	 TechReplaceRestore();
	 break;

      case ImportIdx:
	 if ((objc - nidx) < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    return TCL_ERROR;
	 }

	 if (Tcl_GetIndexFromObj(interp, objv[2 + nidx],
			(CONST84 char **)importTypes, "file type",
			0, &importtype) != TCL_OK)
	    return TCL_ERROR;

	 /* First check the number of arguments, which varies by option. */

	 switch (importtype) {

	    /* Xcircuit imports may specify any number of files > 1.	*/

	    case XCircuitIdx:
	       if ((objc - nidx) == 3) {
		  Tcl_SetResult(interp, "Must specify a filename to import!", NULL);
		  return TCL_ERROR;
	       }
	       break;

	    /* Postscript imports may specify 1 or 0 files.  0 causes	*/
	    /* the function to report back what file is the background.	*/

	    case PostScriptIdx:
	    case BackGroundIdx:
	       if ((objc - nidx) != 3 && (objc - nidx) != 4) {
		  Tcl_SetResult(interp, "Can only specify one filename "
			"for background", NULL);
		  return TCL_ERROR;
	       }

	    /* All other import types must specify exactly one filename. */

	    default:
	       if ((objc - nidx) != 4) {
		  Tcl_SetResult(interp, "Must specify one filename "
			"for import", NULL);
		  return TCL_ERROR;
	       }
	       break;
	 }

	 /* Now process the option */

	 switch (importtype) {
	    case XCircuitIdx:
	       sprintf(_STR2, "%s", Tcl_GetString(objv[3 + nidx]));
	       for (i = 4; i < objc; i++) {
		  strcat(_STR2, ",");
		  strcat(_STR2, Tcl_GetString(objv[i + nidx]));
	       }
	       if (savepage != pageno) newpage(pageno);
	       importfile();
	       if (savepage != pageno) newpage(savepage);
	       break;
	    case PostScriptIdx:		/* replaces "background" */
	    case BackGroundIdx:
	       if (objc - nidx == 2) {
		  objPtr = Tcl_NewStringObj(curpage->background.name,
			strlen(curpage->background.name));
		  Tcl_SetObjResult(interp, objPtr);
		  return XcTagCallback(interp, objc, objv);
	       }
	       sprintf(_STR2, "%s", Tcl_GetString(objv[3 + nidx]));
	       if (savepage != pageno) newpage(pageno);
	       loadbackground();
	       if (savepage != pageno) newpage(savepage);
	       break;

	    case SPICEIdx:
#ifdef ASG
	       /* Make sure that the ASG library is present */

	       if (NameToLibrary(ASG_SPICE_LIB) < 0) {
		  short ilib;

	          strcpy(_STR, ASG_SPICE_LIB);
		  ilib = createlibrary(FALSE);
		  if (loadlibrary(ilib) == FALSE) {
		     Tcl_SetResult(interp, "Error loading library.\n", NULL);
		     return TCL_ERROR;
		  }

	       }

	       sprintf(_STR2, Tcl_GetString(objv[3 + nidx]));
	       if (savepage != pageno) newpage(pageno);
	       importspice();
	       if (savepage != pageno) newpage(savepage);
#else
	       Tcl_SetResult(interp, "ASG not compiled in;  "
			"function is unavailable.\n", NULL);
	       return TCL_ERROR;
#endif
	       break;
	 }

	 /* Redraw */
	 drawarea(areawin->area, NULL, NULL);
	 break;

      case MakeIdx:
	 if (nidx == 1) {
	    Tcl_SetResult(interp, "syntax is: \"page make [<name>]\"", NULL);
	    return TCL_ERROR;
	 }
	 if (objc != 2 && objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "make [<name>]");
	    return TCL_ERROR;
	 }
	 newpage((short)255);
	 if (objc == 3) {
	    curpage = xobjs.pagelist[areawin->page];
	    strcpy(curpage->pageinst->thisobject->name,
		Tcl_GetString(objv[2]));
	 }
	 updatepagelib(PAGELIB, areawin->page);
	 printname(topobject);
	 break;
      case SaveOnlyIdx:
      case SaveIdx:
	 if (objc - nidx > 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "[filename]");
	    return TCL_ERROR;
	 }
	 else if (objc - nidx == 3) {
	    filename = Tcl_GetString(objv[nidx + 2]);
	    if (strcmp(filename, curpage->filename)) {
	       Wprintf("Warning:  Filename is \"%s\" but will be "
		   "saved as \"%s\"\n", curpage->filename, filename);
	    }
	 }
	 else if (curpage->filename == NULL) {
	    Fprintf(stderr, "Warning:  Filename created to match object name\n");
	    filename = curpage->pageinst->thisobject->name;
	 }
	 else
	    filename = curpage->filename;

	 if (savepage != pageno) newpage(pageno);
	 if (!strncmp(Tcl_GetString(objv[nidx + 1]), "saveo", 5))
	     setfile(filename, NO_SUBCIRCUITS);
	 else
	     setfile(filename, CURRENT_PAGE);
	 if (savepage != pageno) newpage(savepage);
	 break;

      case LinksIdx:
	 if ((objc - nidx) < 2 && (objc - nidx) > 6) {
	    Tcl_WrongNumArgs(interp, 1, objv, "links");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2)
	    linktype = TOTAL_PAGES;
	 else {
	    if (Tcl_GetIndexFromObj(interp, objv[2 + nidx],
			(CONST84 char **)linkTypes,
			"link type", 0, &linktype) != TCL_OK)
	       return TCL_ERROR;
	 }
	 multi = 0;
         pagelist = pagetotals(pageno, (linktype >= PendingIdx) ?
			LINKED_PAGES : linktype);
	 TechReplaceSave();
	 switch (linktype) {

	    /* Load any pending links, that is, objects that have a	*/
	    /* "link" parameter containing a string indicating a file	*/
	    /* defining the schematic for that symbol.  Allow the use	*/
	    /* of the same "-replace" flag used by "page load".		*/

	    case LinkLoadIdx:
	       locidx = objc - 1;
	       argv = Tcl_GetString(objv[locidx]);
	       if (*argv != '-') argv = Tcl_GetString(objv[--locidx]);
	       if ((*argv == '-') && !strncmp(argv, "-repl", 5)) {
	          if (locidx < objc - 1) {
		     char *techstr = Tcl_GetString(objv[locidx + 1]);
		     if (!strcmp(techstr, "all")) TechReplaceAll();
		     else if (!strcmp(techstr, "none")) TechReplaceNone();
		     else {
			TechPtr nsptr = LookupTechnology(techstr);
			if (nsptr != NULL)
			   nsptr->flags |= TECH_REPLACE;
		     }
		     objc--;
	          }
	          else
		     TechReplaceAll();		/* replace ALL */
		  objc--;
	       }
	       if ((*argv == '-') && !strncmp(argv, "-targ", 5)) {
	          if (locidx < objc - 1) {
		     ParseLibArguments(interp, 2, &objv[locidx], NULL, &target);
		     objc--;
	          }
		  objc--;
	       }
	       /* drop through */

	    case PendingIdx:
	       key = ((objc - nidx) == 4) ? Tcl_GetString(objv[3 + nidx]) : "link";
	       for (i = 0; i < xobjs.pages; i++) {
		  if (pagelist[i] > 0) {
		     objinstptr tinst;
		     objectptr tpage = xobjs.pagelist[i]->pageinst->thisobject;
		     genericptr *tgen;

		     for (tgen = tpage->plist; tgen < tpage->plist
				+ tpage->parts; tgen++) {
		        if ((*tgen)->type == OBJINST) {
			   tinst = TOOBJINST(tgen);
			   /* Corrected 8/31/07:  Instance value of "link" has	*/
			   /* priority over any default value in the object!	*/
		           ops = find_param(tinst, key);
		           if ((ops != NULL) && (ops->type == XC_STRING)) {
 			      filename = textprint(ops->parameter.string, tinst);
			      if (strlen(filename) > 0) {
				 if ((result = loadlinkfile(tinst, filename, target,
						(linktype == LinkLoadIdx))) > 0) {
				    multi++;
				    setsymschem();	/* Update GUI */
				    result = TCL_OK;
				 }
				 else if (result < 0) {
				    Tcl_SetResult(interp, "Cannot load link", NULL);
				    result = TCL_ERROR;
				 }
				 else result = TCL_OK;
			      }
			      free(filename);
			   }
			}
		     }
		  }
	       }
	       break;
	    default:
	       for (i = 0; i < xobjs.pages; i++) {
	          if (pagelist[i] > 0) {
		     multi++;
		     if ((linktype == SheetIdx) && (i == pageno) && (pagelist[i] > 0))
			 break;
		  }
	       }
	       break;
	 }
	 TechReplaceRestore();
	 free((char *)pagelist);
	 if (result == TCL_ERROR) return result;
	 Tcl_SetObjResult(interp, Tcl_NewIntObj(multi));
	 break;

      case DirIdx:
	 startcatalog(NULL, PAGELIB, NULL);
	 break;

      case GoToIdx:
         newpage((short)pageno);
	 break;

      case UpdateIdx:
	 calcbbox(curpage->pageinst);
	 if (curpage->pmode & 2) autoscale(pageno);
	 break;

      case BBoxIdx:
         if (((objc - nidx) == 2) || ((objc - nidx) == 3)) {
	    Tcl_Obj *tuple;
	    BBox *bbox, *sbbox;
	    int value;

	    bbox = &curpage->pageinst->bbox;
	    if (bbox == NULL)
	       bbox = &curpage->pageinst->thisobject->bbox;
	    sbbox = bbox;

	    if ((objc - nidx) == 3) {
	       sbbox = curpage->pageinst->schembbox;
	       if (sbbox == NULL) sbbox = bbox;
	    }

	    objPtr = Tcl_NewListObj(0, NULL);

	    tuple = Tcl_NewListObj(0, NULL);
	    value = min(sbbox->lowerleft.x, bbox->lowerleft.x);
	    Tcl_ListObjAppendElement(interp, tuple, Tcl_NewIntObj(value));
	    value = min(sbbox->lowerleft.y, bbox->lowerleft.y);
	    Tcl_ListObjAppendElement(interp, tuple, Tcl_NewIntObj(value));
	    Tcl_ListObjAppendElement(interp, objPtr, tuple);

	    tuple = Tcl_NewListObj(0, NULL);
	    value = max(sbbox->lowerleft.x + sbbox->width,
			bbox->lowerleft.x + bbox->width);
	    Tcl_ListObjAppendElement(interp, tuple, Tcl_NewIntObj(value));
	    value = max(sbbox->lowerleft.y + sbbox->height,
			bbox->lowerleft.y + bbox->height);
	    Tcl_ListObjAppendElement(interp, tuple, Tcl_NewIntObj(value));
	    Tcl_ListObjAppendElement(interp, objPtr, tuple);

	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }
	 else {
            Tcl_WrongNumArgs(interp, 1, objv, "bbox [all]");
            return TCL_ERROR;
         }
	 break;

      case SizeIdx:
         if ((objc - nidx) != 2 && (objc - nidx) != 3) {
            Tcl_WrongNumArgs(interp, 1, objv, "size ?\"width x height\"?");
            return TCL_ERROR;
         }
	 if ((objc - nidx) == 2) {
	    float xsize, ysize, cfact;

	    objPtr = Tcl_NewListObj(0, NULL);

	    cfact = (curpage->coordstyle == CM) ? IN_CM_CONVERT
			: 72.0;
            xsize = (float)curpage->pagesize.x / cfact;
            ysize = (float)curpage->pagesize.y / cfact;

	    Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewDoubleObj((double)xsize));
	    Tcl_ListObjAppendElement(interp, objPtr,
	 	Tcl_NewStringObj("x", 1));
	    Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewDoubleObj((double)ysize));
	    Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewStringObj(((curpage->coordstyle == CM) ?
			"cm" : "in"), 2));
	    Tcl_SetObjResult(interp, objPtr);

	    return XcTagCallback(interp, objc, objv);
	 }

         strcpy(_STR2, Tcl_GetString(objv[2 + nidx]));
         setoutputpagesize(&curpage->pagesize);

         /* Only need to recompute values and refresh if autoscaling is enabled */
         if (curpage->pmode & 2) autoscale(pageno);
	 break;

      case MarginsIdx:
	 if ((objc - nidx) < 2 && (objc - nidx) > 4) {
	    Tcl_WrongNumArgs(interp, 1, objv, "margins ?x y?");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    newwidth = (double)curpage->margins.x / 72.0;
	    newheight = (double)curpage->margins.y / 72.0;
	    objPtr = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(interp, objPtr,
			Tcl_NewDoubleObj(newwidth));
	    Tcl_ListObjAppendElement(interp, objPtr,
			Tcl_NewDoubleObj(newheight));
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }
	 newwidth = (double)parseunits(Tcl_GetString(objv[2 + nidx]));
	 if ((objc - nidx) == 4)
	    newheight = (double)parseunits(Tcl_GetString(objv[3 + nidx]));
	 else
	    newheight = newwidth;

	 newheight *= 72.0;
	 newwidth *= 72.0;
	 curpage->margins.x = (int)newwidth;
	 curpage->margins.y = (int)newheight;
	 break;

      case HeightIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "height ?output_height?");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    newheight = toplevelheight(curpage->pageinst, NULL);
	    newheight *= getpsscale(curpage->outscale, pageno);
	    newheight /= (curpage->coordstyle == CM) ?  IN_CM_CONVERT : 72.0;
	    objPtr = Tcl_NewDoubleObj((double)newheight);
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }
	 newheight = (double)parseunits(Tcl_GetString(objv[2 + nidx]));
	 if (newheight <= 0 || topobject->bbox.height == 0) {
	    Tcl_SetResult(interp, "Illegal height value", NULL);
            return TCL_ERROR;
	 }
	 newheight = (newheight * ((curpage->coordstyle == CM) ?
		IN_CM_CONVERT : 72.0)) / topobject->bbox.height;
	 newheight /= getpsscale(1.0, pageno);
	 curpage->outscale = (float)newheight;

	 if (curpage->pmode & 2) autoscale(pageno);
	 break;

      case WidthIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "output_width");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    newwidth = toplevelwidth(curpage->pageinst, NULL);
	    newwidth *= getpsscale(curpage->outscale, pageno);
	    newwidth /= (curpage->coordstyle == CM) ?  IN_CM_CONVERT : 72.0;
	    objPtr = Tcl_NewDoubleObj((double)newwidth);
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }
	 newwidth = (double)parseunits(Tcl_GetString(objv[2 + nidx]));
	 if (newwidth <= 0 || topobject->bbox.width == 0) {
	    Tcl_SetResult(interp, "Illegal width value", NULL);
	    return TCL_ERROR;
	 }

	 newwidth = (newwidth * ((curpage->coordstyle == CM) ?
		IN_CM_CONVERT : 72.0)) / topobject->bbox.width;
	 newwidth /= getpsscale(1.0, pageno);
	 curpage->outscale = (float)newwidth;

	 if (curpage->pmode & 2) autoscale(pageno);
	 break;

      case ScaleIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "output_scale");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    objPtr = Tcl_NewDoubleObj((double)curpage->outscale);
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }
	 result = Tcl_GetDoubleFromObj(interp, objv[2 + nidx], &newscale);
	 if (result != TCL_OK) return result;

	 oldscale = curpage->outscale;

	 if (oldscale == (float)newscale) return TCL_OK;	/* nothing to do */
	 else curpage->outscale = (float)newscale;

	 if (curpage->pmode & 2) autoscale(pageno);
	 break;

      case OrientIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "orientation");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    objPtr = Tcl_NewIntObj((int)curpage->orient);
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }
	 result = Tcl_GetIntFromObj(interp, objv[2 + nidx], &newrot);
	 if (result != TCL_OK) return result;
	 curpage->orient = (short)newrot;

	 /* rescale after rotation if "auto-scale" is set */
	 if (curpage->pmode & 2) autoscale(pageno);
	 break;

      case EPSIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "encapsulation");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    newstr = psTypes[curpage->pmode & 1];
	    Tcl_SetResult(interp, newstr, NULL);
	    return XcTagCallback(interp, objc, objv);
	 }
	 newstr = Tcl_GetString(objv[2 + nidx]);
	 if (Tcl_GetIndexFromObj(interp, objv[2 + nidx],
		(CONST84 char **)psTypes,
		"encapsulation", 0, &newmode) != TCL_OK) {
	    return result;
	 }
	 curpage->pmode &= 0x2;			/* preserve auto-fit flag */
	 curpage->pmode |= (short)newmode;
	 break;

      case LabelIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "label ?name?");
	    return TCL_ERROR;
	 }
	 if ((objc - nidx) == 2) {
	    objPtr = Tcl_NewStringObj(pageobj->name, strlen(pageobj->name));
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
	 }

	 /* Whitespace and non-printing characters not allowed */

	 strcpy(_STR2, Tcl_GetString(objv[2 + nidx]));
	 for (i = 0; i < strlen(_STR2); i++) {
	    if ((!isprint(_STR2[i])) || (isspace(_STR2[i]))) {
	       _STR2[i] = '_';
	       Wprintf("Replaced illegal whitespace in name with underscore");
	    }
	 }

	 if (!strcmp(pageobj->name, _STR2)) return TCL_OK; /* no change in string */
	 if (strlen(_STR2) == 0)
	    sprintf(pageobj->name, "Page %d", areawin->page + 1);
	 else
	    sprintf(pageobj->name, "%.79s", _STR2);

	 /* For schematics, all pages with associations to symbols must have  */
	 /* unique names.                                                     */
	 if (pageobj->symschem != NULL) checkpagename(pageobj);

	 if (pageobj == topobject) printname(pageobj);
	 renamepage(pageno);
	 break;

      case FileIdx:

	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "filename ?name?");
	    return TCL_ERROR;
	 }

	 oldstr = curpage->filename;

	 if ((objc - nidx) == 2) {
	    if (oldstr)
	       objPtr = Tcl_NewStringObj(oldstr, strlen(oldstr));
	    else
	       objPtr = Tcl_NewListObj(0, NULL);	/* NULL list */
	    Tcl_SetObjResult(interp, objPtr);
	    return XcTagCallback(interp, objc, objv);
         }

	 newstr = Tcl_GetString(objv[2 + nidx]);
	 if (strlen(newstr) > 0) {
	    froot = strrchr(newstr, '/');
	    if (froot == NULL) froot = newstr;
	    if (strchr(froot, '.') == NULL) {
	       astr = malloc(strlen(newstr) + 4);
	       sprintf(astr, "%s.ps", newstr);
	       newstr = astr;
	    }
	 }

	 if (oldstr && (!strcmp(oldstr, newstr))) {	/* no change in string */
	    if (newstr == astr) free(astr);
	    return XcTagCallback(interp, objc, objv);
	 }

	 if (strlen(newstr) == 0) {		/* empty string */
	    Tcl_SetResult(interp, "Warning:  No filename!", NULL);
	    multi = 1;
	 }
	 else {
	    multi = pagelinks(pageno);	/* Are there multiple pages? */
	 }

	 /* Make the change to the current page */
	 curpage->filename = strdup(newstr);
	 if (newstr == astr) free(astr);

	 /* All existing filenames which match the old string should	*/
	 /* also be changed unless the filename has been set to the	*/
	 /* null string, which unlinks the page.			*/

	 if ((strlen(curpage->filename) > 0) && (multi > 1)) {
	    for (cpage = 0; cpage < xobjs.pages; cpage++) {
	       lpage = xobjs.pagelist[cpage];
	       if ((lpage->pageinst != NULL) && (cpage != pageno)) {
	          if (lpage->filename && (!filecmp(lpage->filename, oldstr))) {
	             free(lpage->filename);
	             lpage->filename = strdup(newstr);
	          }
	       }
	    }
	 }
	 free(oldstr);
	 autoscale(pageno);

         /* Run pagelinks again; this checks if a page has been attached */
	 /* to existing schematics by being renamed to match.		 */

	 if ((strlen(curpage->filename) > 0) && (multi <= 1)) {
	    for (cpage = 0; cpage < xobjs.pages; cpage++) {
	       lpage = xobjs.pagelist[cpage];
	       if ((lpage->pageinst != NULL) && (cpage != pageno)) {
	          if (lpage->filename && (!filecmp(lpage->filename,
				curpage->filename))) {
	             free(curpage->filename);
	             curpage->filename = strdup(lpage->filename);
		     break;
	          }
	       }
	    }
	 }
	 break;

      case FitIdx:
	 if ((objc - nidx) > 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "fit ?true|false?");
	    return TCL_ERROR;
	 }
	 else if ((objc - nidx) == 3) {
	    result = Tcl_GetBooleanFromObj(interp, objv[2 + nidx], &aval);
	    if (result != TCL_OK) return result;
	    if (aval)
	       curpage->pmode |= 2;
	    else
	       curpage->pmode &= 1;
	 }
	 else
	    Tcl_SetResult(interp, ((curpage->pmode & 2) > 0) ? "true" : "false", NULL);

	 /* Refresh values (does autoscale if specified) */
	 autoscale(pageno);
	 break;

      case ChangesIdx:
	 if ((objc - nidx) != 2 && (objc - nidx) != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "changes");
	    return TCL_ERROR;
	 }
	 /* Allow changes to be set, so that a page can be forced to be	*/
	 /* recognized as either modified or unmodified.		*/

	 if ((objc - nidx) == 3) {
	    int value;
	    Tcl_GetIntFromObj(interp, objv[2 + nidx], &value);
	    curpage->pageinst->thisobject->changes = (u_short)value;
	 }
	 changes = getchanges(curpage->pageinst->thisobject);
	 objPtr = Tcl_NewIntObj((double)changes);
	 Tcl_SetObjResult(interp, objPtr);
	 return XcTagCallback(interp, objc, objv);
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* The "technology" command deals with library *technologies*, where	*/
/* they	differ from files or pages (see the "library" command		*/
/* xctcl_library, below).  Specifically, "library load" loads a file	*/
/* (containing object defintions in a specific technology) onto a page,	*/
/* whereas "technology save" writes back the object definitions that	*/
/* came from the specified file.  Although one would typically have one	*/
/* library page per technology, this is not necessarily the case.	*/
/*									*/
/* Only one technology is defined by a library file, but the library	*/
/* may contain (copies of) dependent objects from another technology.	*/
/*----------------------------------------------------------------------*/

int xctcl_tech(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   char *technology, *filename, *libobjname;
   short *pagelist;
   int idx, ilib, j, pageno, nidx, result;
   TechPtr nsptr = NULL;
   Tcl_Obj *olist;
   objectptr libobj;
   Boolean usertech = FALSE;
   FILE *chklib;

   char *subCmds[] = {
      "save", "list", "objects", "filename", "changed", "used", "prefer",
      "writable", "writeable", NULL
   };
   enum SubIdx {
      SaveIdx, ListIdx, ObjectsIdx, FileNameIdx, ChangedIdx, UsedIdx,
	PreferIdx, WritableIdx, WriteableIdx
   };

   if (objc < 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }
   if (Tcl_GetIndexFromObj(interp, objv[1],
		(CONST84 char **)subCmds, "option", 0, &idx) != TCL_OK) {
      return TCL_ERROR;
   }

   /* All options except "list" and "used" expect a technology argument */
   if (idx != ListIdx && idx != UsedIdx) {
      if (objc > 2) {
         technology = Tcl_GetString(objv[2]);
	 nsptr = LookupTechnology(technology);
	 if (nsptr == NULL) {

	    /* If the command is "objects" and has one or more		*/
	    /* additional arguments, then a NULL nsptr is okay (new	*/
	    /* technology will be created and added to the list).	*/

	    if (idx != ObjectsIdx || objc <= 3) {

	       /* If nsptr is NULL, then the technology should be	*/
	       /* "none", "user", or "default".				*/

	       if ((strstr(technology, "none") == NULL) &&
			(strstr(technology, "user") == NULL) &&
			(strstr(technology, "default") == NULL)) {
	          Tcl_SetResult(interp, "Error:  Unknown technology name!", NULL);
	          return TCL_ERROR;
	       }
	       usertech = TRUE;
	    }
	 }

	 /* And if the user technology has been saved to a file, the technology	*/
	 /* will have a NULL string.  Also check for technology name "(user)",	*/
	 /* although that is not supposed to happen.				*/

	 else if (*nsptr->technology == '\0')
	    usertech = TRUE;

	 else if (!strcmp(nsptr->technology, "(user)"))
	    usertech = TRUE;
      }
      else {
         Tcl_WrongNumArgs(interp, 1, objv, "<option> technology ?args ...?");
         return TCL_ERROR;
      }
   }

   switch (idx) {
      case ListIdx:
	 /* List all of the known technologies */
	 olist = Tcl_NewListObj(0, NULL);
	 for (nsptr = xobjs.technologies; nsptr != NULL; nsptr = nsptr->next) {
	    Tcl_ListObjAppendElement(interp, olist,
			Tcl_NewStringObj(nsptr->technology,
			strlen(nsptr->technology)));
	 }
	 Tcl_SetObjResult(interp, olist);
	 break;

      case UsedIdx:
	 /* List all of the technologies used by the schematic of the	*/
	 /* indicated (or current) page.  That is, enumerate all	*/
	 /* in the hierarchy of the schematic, and list all unique	*/
	 /* technology prefixes.					*/

	 result = ParsePageArguments(interp, objc - 1, objv + 1, &nidx, &pageno);
	 if (result != TCL_OK) return result;
	 olist = Tcl_NewListObj(0, NULL);

         pagelist = pagetotals(pageno, TOTAL_PAGES);
	 for (j = 0; j < xobjs.pages; j++) {
	    if (pagelist[j] > 0) {
	       objinstptr tinst;
	       objectptr tpage = xobjs.pagelist[j]->pageinst->thisobject;
	       genericptr *tgen;

	       for (tgen = tpage->plist; tgen < tpage->plist + tpage->parts; tgen++) {
		  if ((*tgen)->type == OBJINST) {
		     tinst = TOOBJINST(tgen);
		     nsptr = GetObjectTechnology(tinst->thisobject);
		     if (nsptr != NULL) {
			if ((nsptr->technology == NULL) ||
				(strlen(nsptr->technology) == 0)) continue;
			if (!(nsptr->flags & TECH_USED)) {
			   Tcl_ListObjAppendElement(interp, olist,
				Tcl_NewStringObj(nsptr->technology,
					strlen(nsptr->technology)));
			   nsptr->flags |= TECH_USED;
			}
		     }
		  }
	       }
	    }
	 }
	 Tcl_SetObjResult(interp, olist);
	 for (nsptr = xobjs.technologies; nsptr != NULL; nsptr = nsptr->next)
	    nsptr->flags &= ~TECH_USED;
	 free((char *)pagelist);
	 break;

      case ObjectsIdx:

	 if (objc > 3) {
	    int numobjs, objnamelen, technamelen;
	    Tcl_Obj *tobj;
	    char *cptr;
	    TechPtr otech;

	    /* Check that 4th argument is a list of objects or that	*/
	    /* 4th and higher arguments are all names of objects, and	*/
	    /* that these objects are valid existing objects.		*/

	    if (objc == 4) {
	       result = Tcl_ListObjLength(interp, objv[3], &numobjs);
	       if (result != TCL_OK) return result;
	       for (j = 0; j < numobjs; j++) {
		  result = Tcl_ListObjIndex(interp, objv[3], j, &tobj);
	          if (result != TCL_OK) return result;
		  libobj = NameToObject(Tcl_GetString(tobj), NULL, FALSE);
		  if (libobj == NULL) {
		     Tcl_SetResult(interp, "No such object name", NULL);
		     return TCL_ERROR;
		  }
	       }
	    }
	    else {
	       for (j = 0; j < objc - 4; j++) {
		  libobj = NameToObject(Tcl_GetString(objv[3 + j]), NULL, FALSE);
		  if (libobj == NULL) {
		     Tcl_SetResult(interp, "No such object name", NULL);
		     return TCL_ERROR;
		  }
	       }
	    }

	    /* Create a new technology if needed */
	    technology = Tcl_GetString(objv[2]);
	    if ((nsptr == NULL) && !usertech)
		AddNewTechnology(technology, NULL);

	    nsptr = LookupTechnology(technology);
	    technamelen = (usertech) ? 0 : strlen(technology);


	    /* Change the technology prefix of all the objects listed */

	    if (objc == 4) {
	       result = Tcl_ListObjLength(interp, objv[3], &numobjs);
	       if (result != TCL_OK) return result;
	       for (j = 0; j < numobjs; j++) {
		  result = Tcl_ListObjIndex(interp, objv[3], j, &tobj);
	          if (result != TCL_OK) return result;
		  libobj = NameToObject(Tcl_GetString(tobj), NULL, FALSE);
		  cptr = strstr(libobj->name, "::");
		  if (cptr == NULL) {
		     objnamelen = strlen(libobj->name);
		     memmove(libobj->name + technamelen + 2,
				libobj->name, (size_t)strlen(libobj->name));
		  }
		  else {
		     otech = GetObjectTechnology(libobj);
		     otech->flags |= TECH_CHANGED;
		     objnamelen = strlen(cptr + 2);
		     memmove(libobj->name + technamelen + 2,
				cptr + 2, (size_t)strlen(cptr + 2));
		  }

		  if (!usertech) strcpy(libobj->name, technology);
		  *(libobj->name + technamelen) = ':';
		  *(libobj->name + technamelen + 1) = ':';
		  *(libobj->name + technamelen + 2 + objnamelen) = '\0';
	       }
	    }
	    else {
	       for (j = 0; j < objc - 4; j++) {
		  libobj = NameToObject(Tcl_GetString(objv[3 + j]), NULL, FALSE);
		  cptr = strstr(libobj->name, "::");
		  if (cptr == NULL) {
		     objnamelen = strlen(libobj->name);
		     memmove(libobj->name + technamelen + 2,
				libobj->name, (size_t)strlen(libobj->name));
		  }
		  else {
		     otech = GetObjectTechnology(libobj);
		     otech->flags |= TECH_CHANGED;
		     objnamelen = strlen(cptr + 2);
		     memmove(libobj->name + technamelen + 2,
				cptr + 2, (size_t)strlen(cptr + 2));
		  }

		  if (!usertech) strcpy(libobj->name, technology);
		  *(libobj->name + technamelen) = ':';
		  *(libobj->name + technamelen + 1) = ':';
		  *(libobj->name + technamelen + 2 + objnamelen) = '\0';
	       }
	    }
	    if (nsptr != NULL) nsptr->flags |= TECH_CHANGED;
	    break;
	 }

	 /* List all objects having this technology */

	 olist = Tcl_NewListObj(0, NULL);
	 for (ilib = 0; ilib < xobjs.numlibs; ilib++) {
            for (j = 0; j < xobjs.userlibs[ilib].number; j++) {
               libobj = *(xobjs.userlibs[ilib].library + j);
	       if (GetObjectTechnology(libobj) == nsptr) {
		  libobjname = strstr(libobj->name, "::");
		  if (libobjname == NULL)
		     libobjname = libobj->name;
		  else
		     libobjname += 2;
	          Tcl_ListObjAppendElement(interp, olist,
			Tcl_NewStringObj(libobjname, strlen(libobjname)));
	       }
	    }
	 }
	 Tcl_SetObjResult(interp, olist);
	 break;

      case FileNameIdx:
	 if (nsptr != NULL) {
	    if (objc == 3) {
	       if (nsptr->filename == NULL)
	          Tcl_SetResult(interp, "(no associated file)", NULL);
	       else
	          Tcl_SetResult(interp, nsptr->filename, NULL);
	    }
	    else {
	       if (nsptr->filename != NULL) free(nsptr->filename);
	       nsptr->filename = strdup(Tcl_GetString(objv[3]));
	    }
	 }
	 else {
	    Tcl_SetResult(interp, "Valid technology is required", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case ChangedIdx:
	 if (objc == 4) {
	     int bval;
	     if (Tcl_GetBooleanFromObj(interp, objv[3], &bval) != TCL_OK)
		return TCL_ERROR;
	     else if (bval == 1)
	        nsptr->flags |= TECH_CHANGED;
	     else
	        nsptr->flags &= ~TECH_CHANGED;
	 }
	 else {
	     tech_set_changes(nsptr); /* Ensure change flags are updated */
	     Tcl_SetObjResult(interp,
			Tcl_NewBooleanObj(((nsptr->flags & TECH_CHANGED)
			== 0) ? FALSE : TRUE));
	 }
	 break;

      case PreferIdx:
	 if (nsptr) {
	    if (objc == 3) {
	       Tcl_SetObjResult(interp,
			Tcl_NewBooleanObj(((nsptr->flags & TECH_PREFER) == 0)
			? TRUE : FALSE));
	    }
	    else if (objc == 4) {
	       int bval;

	       Tcl_GetBooleanFromObj(interp, objv[3], &bval);
	       if (bval == 0)
	          nsptr->flags |= TECH_PREFER;
	       else
	          nsptr->flags &= (~TECH_PREFER);
	    }
	 }
	 else {
	    Tcl_SetResult(interp, "Valid technology is required", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case WritableIdx:
      case WriteableIdx:
	 if (nsptr) {
	    if (objc == 3) {
	       Tcl_SetObjResult(interp,
			Tcl_NewBooleanObj(((nsptr->flags & TECH_READONLY) == 0)
			? TRUE : FALSE));
	    }
	    else if (objc == 4) {
	       int bval;

	       Tcl_GetBooleanFromObj(interp, objv[3], &bval);
	       if (bval == 0)
	          nsptr->flags |= TECH_READONLY;
	       else
	          nsptr->flags &= (~TECH_READONLY);
	    }
	 }
	 else {
	    Tcl_SetResult(interp, "Valid technology is required", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case SaveIdx:

	 /* technology save [filename] */
         if ((objc == 3) && ((nsptr == NULL) || (nsptr->filename == NULL))) {
	    Tcl_SetResult(interp, "Error:  Filename is required.", NULL);
	    return TCL_ERROR;
	 }
 	 else if ((nsptr != NULL) && (objc == 4)) {
	    /* Technology being saved under a different filename. */
	    filename = Tcl_GetString(objv[3]);

	    /* Re-check read-only status of the file */
	    nsptr->flags &= ~(TECH_READONLY);
	    chklib = fopen(filename, "a");
	    if (chklib == NULL)
	       nsptr->flags |= TECH_READONLY;
	    else
	       fclose(chklib);
	 }
	 else if (objc == 4) {
	    filename = Tcl_GetString(objv[3]);
	    if (!usertech) AddNewTechnology(technology, filename);
	 }
	 else
	    filename = nsptr->filename;

	 savetechnology((usertech) ? NULL : technology, filename);
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* The "library" command deals with library *pages*			*/
/*----------------------------------------------------------------------*/

int xctcl_library(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
  char *filename = NULL, *objname, *argv;
  int j = 0, libnum = -1;
   int idx, nidx, result, res;
   Tcl_Obj *olist;
   Tcl_Obj **newobjv;
   int newobjc, hidmode;
   objectptr libobj;
   liblistptr spec;
   char *subCmds[] = {
      "load", "make", "directory", "next", "goto", "override",
	"handle", "import", "list", "compose", NULL
   };
   enum SubIdx {
      LoadIdx, MakeIdx, DirIdx, NextIdx, GoToIdx, OverrideIdx,
	HandleIdx, ImportIdx, ListIdx, ComposeIdx
   };

   result = ParseLibArguments(interp, objc, objv, &nidx, &libnum);
   if ((result != TCL_OK) || (nidx < 0)) return result;
   else if ((objc - nidx) > 5) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }
   else if (objc <= (1 + nidx)) {  /* No subcommand */

      /* return index if name given; return name if index given. */
      /* return index if neither is given (current library)	 */

      if (objc > 1) {
	 int lnum;	/* unused; only checks if argument is integer */
	 char *lname;
	 result = Tcl_GetIntFromObj(interp, objv[1], &lnum);
	 if (result == TCL_OK) {
	    lname = xobjs.libtop[libnum + LIBRARY]->thisobject->name;
            Tcl_SetObjResult(interp, Tcl_NewStringObj(lname, strlen(lname)));
	 }
	 else {
            result = TCL_OK;
            Tcl_SetObjResult(interp, Tcl_NewIntObj(libnum + 1));
         }
      }
      else
         Tcl_SetObjResult(interp, Tcl_NewIntObj(libnum + 1));
      idx = -1;
   }
   else if (Tcl_GetIndexFromObj(interp, objv[1 + nidx],
		(CONST84 char **)subCmds, "option", 0, &idx) != TCL_OK) {

      /* Backwards compatibility: "library filename [number]" is */
      /* the same as "library [number] load filename"		 */

      Tcl_ResetResult(interp);
      newobjv = (Tcl_Obj **)(&objv[1]);
      newobjc = objc - 1;

      result = ParseLibArguments(interp, newobjc, newobjv, &nidx, &libnum);
      if (result != TCL_OK) return result;

      idx = LoadIdx;
      filename = Tcl_GetString(newobjv[0]);
   }

   /* libnum = -1 is equivalent to "USER LIBRARY" */
   if (libnum < 0) libnum = xobjs.numlibs - 1;

   switch (idx) {
      case LoadIdx:
	 TechReplaceSave();

	 /* library [<name>|<number>] load <filename> [-replace [library]] */
	 if (objc < (3 + nidx)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	    return TCL_ERROR;
	 }
	 if (filename == NULL) filename = Tcl_GetString(objv[2 + nidx]);

	 /* if loading of default libraries is not overridden, load them first */

	 if (!(flags & (LIBOVERRIDE | LIBLOADED))) {
	    result = defaultscript();
	    flags |= LIBLOADED;
	 }

	 /* If library number is out of range, create a new library	*/
	 /* libnum = -1 is equivalent to the user library page.		*/

	 if (libnum > (xobjs.numlibs - 1))
	    libnum = createlibrary(FALSE);
	 else if (libnum < 0)
	    libnum = USERLIB;
	 else
	    libnum += LIBRARY;

	 if (objc > (3 + nidx)) {
	    argv = Tcl_GetString(objv[3 + nidx]);
	    if ((*argv == '-') && !strncmp(argv, "-repl", 5)) {
	       if (objc > (4 + nidx)) {
		  char *techstr = Tcl_GetString(objv[3 + nidx]);
		  if (!strcmp(techstr, "all")) TechReplaceAll();
		  else if (!strcmp(techstr, "none")) TechReplaceNone();
		  else {
		     TechPtr nsptr = LookupTechnology(techstr);
		     if (nsptr != NULL)
			nsptr->flags |= TECH_REPLACE;
		  }
	       }
	       else
		  TechReplaceAll();		/* replace ALL */
	    }
	 }

	 strcpy(_STR, filename);
	 res = loadlibrary(libnum);
	 if (res == False) {
	    res = loadfile(2, libnum);
	    TechReplaceRestore();
	    if (res == False) {
	       Tcl_SetResult(interp, "Error loading library.\n", NULL);
	       return TCL_ERROR;
	    }
	 }
	 TechReplaceRestore();
	 break;

      case ImportIdx:
	 /* library [<name>|<number>] import <filename> <objectname> */
	 if (objc != (4 + nidx)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	    return TCL_ERROR;
	 }
	 if (filename == NULL) filename = Tcl_GetString(objv[2 + nidx]);

	 /* if loading of default libraries is not overridden, load them first */

	 if (!(flags & (LIBOVERRIDE | LIBLOADED))) {
	    defaultscript();
	    flags |= LIBLOADED;
	 }

	 if ((libnum >= xobjs.numlibs) || (libnum < 0))
	    libnum = createlibrary(FALSE);
	 else
	    libnum += LIBRARY;

	 objname = Tcl_GetString(objv[3 + nidx]);
	 importfromlibrary(libnum, filename, objname);
	 break;

      case ListIdx:

	 if (!strncmp(Tcl_GetString(objv[objc - 1]), "-vis", 4))
	    hidmode = 1;	/* list visible objects only */
	 else if (!strncmp(Tcl_GetString(objv[objc - 1]), "-hid", 4))
	    hidmode = 2;	/* list hidden objects only */
	 else
	    hidmode = 3;	/* list everything */

	 /* library [name|number] list [-visible|-hidden] */
	 olist = Tcl_NewListObj(0, NULL);
         for (j = 0; j < xobjs.userlibs[libnum].number; j++) {
            libobj = *(xobjs.userlibs[libnum].library + j);
	    if (((libobj->hidden) && (hidmode & 2)) ||
			((!libobj->hidden) && (hidmode & 1)))
	       Tcl_ListObjAppendElement(interp, olist,
			Tcl_NewStringObj(libobj->name, strlen(libobj->name)));
	 }
	 Tcl_SetObjResult(interp, olist);
	 break;

      case HandleIdx:

	 if (objc == (3 + nidx)) {
	    /* library [name|number] handle <object name> */

	    olist = Tcl_NewListObj(0, NULL);
	    for (spec = xobjs.userlibs[libnum].instlist; spec != NULL;
			spec = spec->next) {
	       libobj = spec->thisinst->thisobject;
	       if (!strcmp(libobj->name, Tcl_GetString(objv[objc - 1])))
		  Tcl_ListObjAppendElement(interp, olist,
				Tcl_NewHandleObj((genericptr)spec->thisinst));
	    }
	    Tcl_SetObjResult(interp, olist);
         }
	 else if (objc == (2 + nidx)) {
	    /* library [name|number] handle */

	    olist = Tcl_NewListObj(0, NULL);
	    for (spec = xobjs.userlibs[libnum].instlist; spec != NULL;
			spec = spec->next) {
	       Tcl_ListObjAppendElement(interp, olist,
			Tcl_NewHandleObj((genericptr)spec->thisinst));
	    }
	    Tcl_SetObjResult(interp, olist);
	 }
	 else {
	    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	    return TCL_ERROR;
	 }
	 break;

      case ComposeIdx:
	 composelib(libnum + LIBRARY);
	 centerview(xobjs.libtop[libnum + LIBRARY]);
	 break;

      case MakeIdx:
	 /* library make [name] */
	 if (nidx == 1) {
	    Tcl_SetResult(interp, "syntax is: library make [<name>]", NULL);
	    return TCL_ERROR;
	 }

	 /* If the (named or numbered) library exists, don't create it. */
	 /* ParseLibArguments() returns the library number for the User	*/
	 /* Library.  The User Library always exists and cannot be	*/
	 /* created or destroyed, so it's okay to use it as a check for	*/
	 /* "no library found".						*/

	 if (libnum == xobjs.numlibs - 1)
	    libnum = createlibrary(TRUE);

	 if (objc == 3) {
	    strcpy(xobjs.libtop[libnum]->thisobject->name, Tcl_GetString(objv[2]));
	    renamelib(libnum);
	    composelib(LIBLIB);
	 }
	 /* Don't go to the library page---use "library goto" instead */
	 /* startcatalog((Tk_Window)clientData, libnum, NULL); */
	 break;

      case DirIdx:
	 /* library directory */
	 if ((nidx == 0) && (objc == 2)) {
	    startcatalog(NULL, LIBLIB, NULL);
	 }
	 else if ((nidx == 0) && (objc == 3) &&
		!strcmp(Tcl_GetString(objv[2]), "list")) {
	    olist = Tcl_NewListObj(0, NULL);
            for (j = 0; j < xobjs.numlibs; j++) {
               libobj = xobjs.libtop[j + LIBRARY]->thisobject;
	       Tcl_ListObjAppendElement(interp, olist,
			Tcl_NewStringObj(libobj->name, strlen(libobj->name)));
	    }
	    Tcl_SetObjResult(interp, olist);
	 }
	 else {
	    Tcl_SetResult(interp, "syntax is: library directory [list]", NULL);
	    return TCL_ERROR;
	 }
	 break;

      case NextIdx:
	 libnum = is_library(topobject);
	 if (++libnum >= xobjs.numlibs) libnum = 0;	/* fall through */

      case GoToIdx:
	 /* library go */
	 startcatalog(NULL, LIBRARY + libnum, NULL);
	 break;
      case OverrideIdx:
	 flags |= LIBOVERRIDE;
	 return TCL_OK;			/* no tag callback */
	 break;
   }
   return (result == TCL_OK) ? XcTagCallback(interp, objc, objv) : result;
}

/*----------------------------------------------------------------------*/
/* "bindkey" command --- this is a direct implementation of the same	*/
/* key binding found in the "ad-hoc" and Python interfaces;  it is	*/
/* preferable to make use of the Tk "bind" command directly, and work	*/
/* from the event handler.						*/
/*----------------------------------------------------------------------*/

int xctcl_bind(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   Tk_Window window = (Tk_Window)NULL;
   XCWindowDataPtr searchwin;
   char *keyname, *commandname, *binding;
   int keywstate, func = -1, value = -1;
   int result;
   Boolean compat = FALSE;

   if (objc == 2) {
      keyname = Tcl_GetString(objv[1]);
      if (!strcmp(keyname, "override")) {
	 flags |= KEYOVERRIDE;
	 return TCL_OK;			/* no tag callback */
      }
   }

   if (!(flags & KEYOVERRIDE)) {
      default_keybindings();
      flags |= KEYOVERRIDE;
   }

   if (objc == 1) {
      Tcl_Obj *list;
      int i;

      list = Tcl_NewListObj(0, NULL);
      for (i = 0; i < NUM_FUNCTIONS; i++) {
         commandname = func_to_string(i);
	 Tcl_ListObjAppendElement(interp, list,
		Tcl_NewStringObj(commandname, strlen(commandname)));
      }
      Tcl_SetObjResult(interp, list);
      return TCL_OK;
   }
   else if (objc > 5) {
      Tcl_WrongNumArgs(interp, 1, objv,
		"[<key> [<window>] [<command> [<value>|forget]]]");
      return TCL_ERROR;
   }

   /* If 1st argument matches a window name, create a window-specific	*/
   /* binding.  Otherwise, create a binding for all windows.		*/

   if (objc > 1) {
      window = Tk_NameToWindow(interp, Tcl_GetString(objv[1]), Tk_MainWindow(interp));
      if (window == (Tk_Window)NULL)
	 Tcl_ResetResult(interp);
      else {
         for (searchwin = xobjs.windowlist; searchwin != NULL; searchwin =
			searchwin->next)
            if (searchwin->area == window)
	       break;
         if (searchwin != NULL) {
	    /* Shift arguments */
            objc--;
            objv++;
         }
         else
            window = (xcWidget)NULL;
      }
   }

   /* 1st argument can be option "-compatible" */
   if ((objc > 1) && !strncmp(Tcl_GetString(objv[1]), "-comp", 5)) {
      objc--;
      objv++;
      compat = TRUE;
   }

   keyname = Tcl_GetString(objv[1]);
   keywstate = string_to_key(keyname);

   /* 1st arg may be a function, not a key, if we want the binding returned */
   if ((objc == 3) && !strncmp(keyname, "-func", 5)) {
      keywstate = -1;
      func = string_to_func(Tcl_GetString(objv[2]), NULL);
      objc = 2;
      if (func == -1) {
	 Tcl_SetResult(interp, "Invalid function name\n", NULL);
	 return TCL_ERROR;
      }
   }
   else if ((objc == 2) && (keywstate == 0)) {
      keywstate = -1;
      func = string_to_func(keyname, NULL);
   }

   if ((keywstate == -1 || keywstate == 0) && func == -1) {
      Tcl_SetResult(interp, "Invalid key name ", NULL);
      Tcl_AppendElement(interp, keyname);
      return TCL_ERROR;
   }

   if (objc == 2) {
      if (keywstate == -1)
         binding = function_binding_to_string(window, func);
      else if (compat)
         binding = compat_key_to_string(window, keywstate);
      else
         binding = key_binding_to_string(window, keywstate);
      Tcl_SetResult(interp, binding, TCL_VOLATILE);
      free(binding);
      return TCL_OK;
   }

   if (objc < 3) {
      Tcl_SetResult(interp, "Usage: bindkey <key> [<function>]\n", NULL);
      return TCL_ERROR;
   }

   commandname = Tcl_GetString(objv[2]);
   if (strlen(commandname) == 0)
      func = -1;
   else
      func = string_to_func(commandname, NULL);

   if (objc == 4) {
      result = Tcl_GetIntFromObj(interp, objv[3], &value);
      if (result != TCL_OK)
      {
	 if (strcmp(Tcl_GetString(objv[3]), "forget"))
	    return (result);
	 else {
	    /*  Unbind command */
	    Tcl_ResetResult(interp);
	    result = remove_binding(window, keywstate, func);
	    if (result == 0)
		return TCL_OK;
	    else {
	       Tcl_SetResult(interp, "Key/Function pair not found "
			"in binding list.\n", NULL);
	       return TCL_ERROR;
	    }
	 }
      }
   }
   result = add_vbinding(window, keywstate, func, value);
   if (result == 1) {
      Tcl_SetResult(interp, "Key is already bound to a command.\n", NULL);
      return (result);
   }
   return (result == TCL_OK) ? XcTagCallback(interp, objc, objv) : result;
}

/*----------------------------------------------------------------------*/

int xctcl_font(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   char *fontname;
   int result;

   /* font name */
   if (objc != 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "fontname");
      return TCL_ERROR;
   }
   fontname = Tcl_GetString(objv[1]);

   /* Allow overrides of the default font loading mechanism */
   if (!strcmp(fontname, "override")) {
      flags |= FONTOVERRIDE;
      return TCL_OK;
   }

   /* If we need to load the default font "Helvetica" because no fonts	*/
   /* have been loaded yet, then we call this function twice, so that	*/
   /* the command tag callback gets applied both times.			*/

   if (!(flags & FONTOVERRIDE)) {
      flags |= FONTOVERRIDE;
      xctcl_font(clientData, interp, objc, objv);
      loadfontfile("Helvetica");
   }
   result = loadfontfile((char *)fontname);
   if (result >= 1) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(fonts[fontcount - 1].family,
		strlen(fonts[fontcount - 1].family)));
   }
   switch (result) {
      case 1:
	 return XcTagCallback(interp, objc, objv);
      case 0:
	 return TCL_OK;
      case -1:
         return TCL_ERROR;
   }
   return TCL_ERROR;  /* (jdk) */
}

/*----------------------------------------------------------------------*/
/* Set the X11 cursor to one of those defined in the XCircuit cursor	*/
/* set (cursors.h)							*/
/*----------------------------------------------------------------------*/

int xctcl_cursor(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   int idx, result;

   static char *cursNames[] = {
      "arrow", "cross", "scissors", "copy", "rotate", "edit",
      "text", "circle", "question", "wait", "hand", NULL
   };

   if (!areawin) return TCL_ERROR;

   /* cursor name */
   if (objc != 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "cursor name");
      return TCL_ERROR;
   }
   if ((result = Tcl_GetIndexFromObj(interp, objv[1],
	(CONST84 char **)cursNames,
	"cursor name", 0, &idx)) != TCL_OK)
      return result;

   XDefineCursor(dpy, areawin->window, appcursors[idx]);
   areawin->defaultcursor = &appcursors[idx];
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/

int xctcl_filerecover(ClientData clientData, Tcl_Interp *interp,
	int objc, Tcl_Obj *CONST objv[])
{
   if (objc != 1) {
      Tcl_WrongNumArgs(interp, 1, objv, "(no arguments)");
      return TCL_ERROR;
   }
   crashrecover();
   return XcTagCallback(interp, objc, objv);
}

/*----------------------------------------------------------------------*/
/* Replace the functions of the simple rcfile.c interpreter.    	*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Execute a single command from a script or from the command line      */
/*----------------------------------------------------------------------*/

short execcommand(short pflags, char *cmdptr)
{
   flags = pflags;
   Tcl_Eval(xcinterp, cmdptr);
   refresh(NULL, NULL, NULL);
   return flags;
}

/*----------------------------------------------------------------------*/
/* Load the default script (like execscript() but don't allow recursive */
/* loading of the startup script)                                       */
/*----------------------------------------------------------------------*/

int defaultscript()
{
   FILE *fd;
   char *tmp_s = getenv((const char *)"XCIRCUIT_SRC_DIR");
   int result;

   flags = LIBOVERRIDE | LIBLOADED | FONTOVERRIDE;

   if (!tmp_s) tmp_s = SCRIPTS_DIR;
   sprintf(_STR2, "%s/%s", tmp_s, STARTUP_FILE);

   if ((fd = fopen(_STR2, "r")) == NULL) {
      sprintf(_STR2, "%s/%s", SCRIPTS_DIR, STARTUP_FILE);
      if ((fd = fopen(_STR2, "r")) == NULL) {
         sprintf(_STR2, "%s/tcl/%s", SCRIPTS_DIR, STARTUP_FILE);
         if ((fd = fopen(_STR2, "r")) == NULL) {
            Wprintf("Failed to open startup script \"%s\"\n", STARTUP_FILE);
            return TCL_ERROR;
	 }
      }
   }
   fclose(fd);
   result = Tcl_EvalFile(xcinterp, _STR2);
   return result;
}

/*----------------------------------------------------------------------*/
/* Execute a script                                                     */
/*----------------------------------------------------------------------*/

void execscript()
{
   FILE *fd;

   flags = 0;

   xc_tilde_expand(_STR2, 249);
   if ((fd = fopen(_STR2, "r")) != NULL) {
      fclose(fd);
      Tcl_EvalFile(xcinterp, _STR2);
      refresh(NULL, NULL, NULL);
   }
   else {
      Wprintf("Failed to open script file \"%s\"\n", _STR2);
   }
}

/*----------------------------------------------------------------------*/
/* Evaluate an expression from a parameter and return the result as a 	*/
/* Tcl object.   The actual return value (TCL_OK, TCL_ERROR) is stored	*/
/* in pointer "eval_status", if it is non-NULL.				*/
/*----------------------------------------------------------------------*/

Tcl_Obj *evaluate_raw(objectptr thisobj, oparamptr ops, objinstptr pinst,
		int *eval_status)
{
   Tcl_SavedResult state;
   Tcl_Obj *robj;
   int status;
   char *exprptr, *pptr, *pkey, *pnext;

   /* Sanity check */
   if (ops->type != XC_EXPR) return NULL;
   exprptr = ops->parameter.expr;
   pnext = exprptr;
   if (pnext == NULL) return NULL;

   /* Check for "@<parameter>" notation and substitute parameter values */
   while ((pptr = strchr(pnext, '@')) != NULL)
   {
      oparam temps;
      oparamptr ips;
      char psave, *promoted, *newexpr;

      pptr++;
      for (pkey = pptr; *pkey && !isspace(*pkey); pkey++)
	 if (*pkey == '{' || *pkey == '}' || *pkey == '[' || *pkey == ']' ||
		*pkey == '(' || *pkey == ')' || *pkey == ',')
	    break;

      if (pkey > pptr) {
	 psave = *pkey;
	 *pkey = '\0';
	 if (pinst)
	    ips = find_param(pinst, pptr);
	 else
	    ips = match_param(thisobj, pptr);
	 if (ips == ops) {
	    /* Avoid infinite recursion by treating a reference	*/
	    /* to oneself as plain text.			*/
	    ips = NULL;
	 }
	 if ((ips == NULL) && !strncmp(pptr, "p_", 2)) {
	    ips = &temps;
	    if (!strcmp(pptr + 2, "rotation")) {
	       temps.type = XC_FLOAT;
	       temps.parameter.fvalue = pinst ? pinst->rotation : 0;
	    }
	    else if (!strcmp(pptr + 2, "xposition")) {
	       temps.type = XC_INT;
	       temps.parameter.ivalue = pinst ? pinst->position.x : 0;
	    }
	    else if (!strcmp(pptr + 2, "yposition")) {
	       temps.type = XC_INT;
	       temps.parameter.ivalue = pinst ? pinst->position.y : 0;
	    }
	    else if (!strcmp(pptr + 2, "scale")) {
	       temps.type = XC_FLOAT;
	       temps.parameter.fvalue = pinst ? pinst->scale : 1.0;
	    }
	    else if (!strcmp(pptr + 2, "color")) {
	       temps.type = XC_INT;
	       temps.parameter.ivalue = pinst ? pinst->color : DEFAULTCOLOR;
	    }
	    else if (!strcmp(pptr + 2, "top_xposition")) {
	       temps.type = XC_INT;
	       UTopDrawingOffset(&temps.parameter.ivalue, NULL);
	    }
	    else if (!strcmp(pptr + 2, "top_yposition")) {
	       temps.type = XC_INT;
	       UTopDrawingOffset(NULL, &temps.parameter.ivalue);
	    }
	    else if (!strcmp(pptr + 2, "top_rotation")) {
	       temps.type = XC_FLOAT;
	       temps.parameter.fvalue = UTopRotation();
	    }
	    else if (!strcmp(pptr + 2, "top_scale")) {
	       temps.type = XC_FLOAT;
	       temps.parameter.fvalue = UTopDrawingScale();
	    }
	    else
	       ips = NULL;
	 }
	 *pkey = psave;
	 if (ips != NULL) {
	    switch (ips->type) {
	       case XC_INT:
		  promoted = malloc(12);
		  snprintf(promoted, 12, "%d", ips->parameter.ivalue);
		  break;
	       case XC_FLOAT:
		  promoted = malloc(12);
		  snprintf(promoted, 12, "%g", ips->parameter.fvalue);
		  break;
	       case XC_STRING:
		  promoted = textprint(ips->parameter.string, pinst);
		  break;
	       case XC_EXPR:
		  /* We really ought to prevent infinite loops here. . .*/
		  promoted = evaluate_expr(thisobj, ips, pinst);
		  break;
	    }
	    if (promoted == NULL) break;
	    newexpr = (char *)malloc(1 + strlen(exprptr) +
			(max(strlen(promoted), strlen(pkey))));
	    *(pptr - 1) = '\0';
	    strcpy(newexpr, exprptr);
	    *(pptr - 1) = '@';
	    strcat(newexpr, promoted);
	    pnext = newexpr + strlen(newexpr);	/* For next search of '@' escape */
	    strcat(newexpr, pkey);
	    free(promoted);
	    if (exprptr != ops->parameter.expr) free(exprptr);
	    exprptr = newexpr;
	 }
	 else {
	    /* Ignore the keyword and move to the end */
	    pnext = pkey;
	 }
      }
   }

   /* Evaluate the expression in TCL */

   Tcl_SaveResult(xcinterp, &state);
   status = Tcl_Eval(xcinterp, exprptr);
   robj = Tcl_GetObjResult(xcinterp);
   Tcl_IncrRefCount(robj);
   Tcl_RestoreResult(xcinterp, &state);
   if (eval_status) *eval_status = status;
   if (exprptr != ops->parameter.expr) free(exprptr);
   return robj;
}

/*----------------------------------------------------------------------*/
/* Evaluate an expression from a parameter and return the result as an	*/
/* allocated string.							*/
/*----------------------------------------------------------------------*/

char *evaluate_expr(objectptr thisobj, oparamptr ops, objinstptr pinst)
{
   Tcl_Obj *robj;
   char *rexpr = NULL;
   int status, ip = 0;
   float fp = 0.0;
   stringpart *tmpptr, *promote = NULL;
   oparamptr ips = (pinst == NULL) ? NULL : match_instance_param(pinst, ops->key);

   robj = evaluate_raw(thisobj, ops, pinst, &status);
   if (robj != NULL) {
      rexpr = strdup(Tcl_GetString(robj));
      Tcl_DecrRefCount(robj);
   }

   if ((status == TCL_ERROR) && (ips != NULL)) {
      switch(ips->type) {
	 case XC_STRING:
            rexpr = textprint(ips->parameter.string, pinst);
	    break;
	 case XC_FLOAT:
	    fp = ips->parameter.fvalue;
	    break;
      }
   }

   /* If a TCL expression contains a three digit octal value \ooo  */
   /* then the string returned by TclEval() can contain a          */
   /* multi-byte UTF-8 character.                                  */
   /*                                                              */
   /* This multi-byte character needs to be converted back to a    */
   /* character that can be displayed.                             */
   /*                                                              */
   /* The following fix assumes that at most two bytes will        */
   /* represent any converted character.  In this case, the most   */
   /* significant digit (octal) of the first byte will be 3, and   */
   /* the most significant digit of the second byte will be 2.     */
   /*                                                              */
   /* See: https://en.wikipedia.org/wiki/UTF-8                     */

   if ((rexpr != NULL) && ((status == TCL_RETURN) || (status == TCL_OK))) {
      u_char *strptr1 = rexpr;
      u_char *strptr2 = rexpr;
      while (*strptr1 != '\0') {
         if (*strptr1 >= 0300 && *(strptr1 + 1) >= 0200) {
            *strptr2 = ((*strptr1 & ~0300) << 6) | (*(strptr1 + 1) & 0077);
            strptr1 += 2;
         } else {
            *strptr2 = *strptr1;
            strptr1++;
         }
         strptr2++;
      }
      if (*strptr1 == '\0')
         *strptr2 = *strptr1;
   }

   /* If an instance redefines an expression, don't preserve	*/
   /* the result.  It is necessary in this case that the	*/
   /* expression does not reference objects during redisplay,	*/
   /* or else the correct result will not be written to the	*/
   /* output.							*/

   if ((ips != NULL) && (ips->type == XC_EXPR))
      return rexpr;

   /* Preserve the result in the object instance; this will be	*/
   /* used when writing the output or when the result cannot	*/
   /* be evaluated (see above).					*/

   if ((rexpr != NULL) && (status == TCL_OK) && (pinst != NULL)) {
      switch (ops->which) {
	 case P_SUBSTRING: case P_EXPRESSION:
            if (ips == NULL) {
	       ips = make_new_parameter(ops->key);
	       ips->which = ops->which;
	       ips->type = XC_STRING;
	       ips->next = pinst->params;
	       pinst->params = ips;
            }
            else {
	       free(ips->parameter.string);
            }
            /* Promote the expression result to an XCircuit string type */
            tmpptr = makesegment(&promote, NULL);
            tmpptr->type = TEXT_STRING;
            tmpptr = makesegment(&promote, NULL);
            tmpptr->type = PARAM_END;
            promote->data.string = strdup(rexpr);
            ips->parameter.string = promote;
	    break;

	 case P_COLOR:	/* must be integer, exact to 32 bits */
            if (ips == NULL) {
	       ips = make_new_parameter(ops->key);
	       ips->which = ops->which;
	       ips->next = pinst->params;
	       pinst->params = ips;
            }
            /* Promote the expression result to type float */
	    if (rexpr != NULL) {
	       if (sscanf(rexpr, "%i", &ip) == 1)
		  ips->parameter.ivalue = ip;
	       else
		  ips->parameter.ivalue = 0;
	    }
	    else
	       ips->parameter.ivalue = ip;
	    ips->type = XC_INT;
	    break;

	 default:	/* all others convert to type float */
            if (ips == NULL) {
	       ips = make_new_parameter(ops->key);
	       ips->which = ops->which;
	       ips->next = pinst->params;
	       pinst->params = ips;
            }
            /* Promote the expression result to type float */
	    if (rexpr != NULL) {
	       if (sscanf(rexpr, "%g", &fp) == 1)
		  ips->parameter.fvalue = fp;
	       else
		  ips->parameter.fvalue = 0.0;
	    }
	    else
	       ips->parameter.fvalue = fp;
	    ips->type = XC_FLOAT;
	    break;
      }
   }
   return rexpr;
}

/*----------------------------------------------------------------------*/
/* Execute the .xcircuitrc startup script                               */
/*----------------------------------------------------------------------*/

int loadrcfile()
{
   char *userdir = getenv((const char *)"HOME");
   FILE *fd;
   short i;
   int result = TCL_OK, result1 = TCL_OK;

   /* Initialize flags */

   flags = 0;

   /* Try first in current directory, then look in user's home directory */
   /* First try looking for a file .xcircuitrc followed by a dash and	 */
   /* the program version; this allows backward compatibility of the rc	 */
   /* file in cases where a new version (e.g., 3 vs. 2) introduces 	 */
   /* incompatible syntax.  Thanks to Romano Giannetti for this		 */
   /* suggestion plus provided code.					 */

   /* (names USER_RC_FILE and PROG_VERSION imported from Makefile) */

   sprintf(_STR2, "%s-%s", USER_RC_FILE, PROG_VERSION);
   xc_tilde_expand(_STR2, 249);
   if ((fd = fopen(_STR2, "r")) == NULL) {
      /* Not found; check for the same in $HOME directory */
      if (userdir != NULL) {
         sprintf(_STR2, "%s/%s-%s", userdir, USER_RC_FILE, PROG_VERSION);
         if ((fd = fopen(_STR2, "r")) == NULL) {
	    /* Not found again; check for rc file w/o version # in CWD */
            sprintf(_STR2, "%s", USER_RC_FILE);
            xc_tilde_expand(_STR2, 249);
            if ((fd = fopen(_STR2, "r")) == NULL) {
               /* last try: plain USER_RC_FILE in $HOME */
               sprintf(_STR2, "%s/%s", userdir, USER_RC_FILE);
               fd = fopen(_STR2, "r");
            }
	 }
      }
   }
   if (fd != NULL) {
      fclose(fd);
      result = Tcl_EvalFile(xcinterp, _STR2);
      if (result != TCL_OK) {
         Fprintf(stderr, "Encountered error in startup file.");
         Fprintf(stderr, "%s\n", Tcl_GetStringResult(xcinterp));
         Fprintf(stderr, "Running default startup script instead.\n");
      }
   }

   /* Add the default font if not loaded already */

   if (!(flags & FONTOVERRIDE)) {
      loadfontfile("Helvetica");
      if (areawin->psfont == -1)
         for (i = 0; i < fontcount; i++)
            if (!strcmp(fonts[i].psname, "Helvetica")) {
               areawin->psfont = i;
               break;
            }
   }
   if (areawin->psfont == -1) areawin->psfont = 0;

   setdefaultfontmarks();

   /* arrange the loaded libraries */

   if ((result != TCL_OK) || !(flags & (LIBOVERRIDE | LIBLOADED))) {
      result1 = defaultscript();
   }

   /* Add the default colors */

   if (!(flags & COLOROVERRIDE)) {
      addnewcolorentry(xc_alloccolor("Gray40"));
      addnewcolorentry(xc_alloccolor("Gray60"));
      addnewcolorentry(xc_alloccolor("Gray80"));
      addnewcolorentry(xc_alloccolor("Gray90"));
      addnewcolorentry(xc_alloccolor("Red"));
      addnewcolorentry(xc_alloccolor("Blue"));
      addnewcolorentry(xc_alloccolor("Green2"));
      addnewcolorentry(xc_alloccolor("Yellow"));
      addnewcolorentry(xc_alloccolor("Purple"));
      addnewcolorentry(xc_alloccolor("SteelBlue2"));
      addnewcolorentry(xc_alloccolor("Red3"));
      addnewcolorentry(xc_alloccolor("Tan"));
      addnewcolorentry(xc_alloccolor("Brown"));
      addnewcolorentry(xc_alloccolor("#d20adc"));
      addnewcolorentry(xc_alloccolor("Pink"));
   }

   if ((result != TCL_OK) || !(flags & KEYOVERRIDE)) {
      default_keybindings();
   }
   return (result1 != TCL_OK) ? result1 : result;
}

/*----------------------------------------------------------------------*/
/* Alternative button handler for use with Tk "bind"			*/
/*----------------------------------------------------------------------*/

int xctcl_standardaction(ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
   int idx, result, knum, kstate;
   XKeyEvent kevent;
   static char *updown[] = {"up", "down", NULL};

   if ((objc != 3) && (objc != 4)) goto badargs;

   if ((result = Tcl_GetIntFromObj(interp, objv[1], &knum)) != TCL_OK)
      goto badargs;

   if ((result = Tcl_GetIndexFromObj(interp, objv[2],
		(CONST84 char **)updown, "direction", 0, &idx)) != TCL_OK)
      goto badargs;

   if (objc == 4) {
      if ((result = Tcl_GetIntFromObj(interp, objv[3], &kstate)) != TCL_OK)
	 goto badargs;
   }
   else
      kstate = 0;

   make_new_event(&kevent);
   kevent.state = kstate;
   kevent.keycode = 0;

   if (idx == 0)
      kevent.type = KeyRelease;
   else
      kevent.type = KeyPress;

   switch (knum) {
     case 1:
	 kevent.state |= Button1Mask;
	 break;
     case 2:
	 kevent.state |= Button2Mask;
	 break;
     case 3:
	 kevent.state |= Button3Mask;
	 break;
     case 4:
	 kevent.state |= Button4Mask;
	 break;
     case 5:
	 kevent.state |= Button5Mask;
	 break;
     default:
	 kevent.keycode = knum;
	 break;
   }
#ifdef _MSC_VER
   if (kevent.state & Mod1Mask) {
     kevent.state &= ~Mod1Mask;
   }
   if (kevent.state & (AnyModifier<<2)) {
     kevent.state &= ~(AnyModifier<<2);
     kevent.state |= Mod1Mask;
   }
#endif
   keyhandler((xcWidget)NULL, (caddr_t)NULL, &kevent);
   return TCL_OK;

badargs:
   Tcl_SetResult(interp, "Usage: standardaction <button_num> up|down [<keystate>]\n"
			"or standardaction <keycode> up|down [<keystate>]\n", NULL);
   return TCL_ERROR;
}

/*----------------------------------------------------------------------*/
/* Action handler for use with Tk "bind"				*/
/* This dispatches events based on specific named actions that xcircuit	*/
/* knows about, rather than by named key.  This bypasses xcircuit's	*/
/* key bindings.							*/
/*----------------------------------------------------------------------*/

int xctcl_action(ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
   short value = 0;
   int function, result, ival;
   XPoint newpos, wpoint;

   if (objc >= 2 && objc <= 4) {
      function = string_to_func(Tcl_GetString(objv[1]), &value);
      if (objc >= 3) {
	 result = (short)Tcl_GetIntFromObj(interp, objv[2], &ival);
	 if (result == TCL_ERROR) return TCL_ERROR;
	 value = (short)ival;
      }

      newpos = UGetCursorPos();
      user_to_window(newpos, &wpoint);

      result = compatible_function(function);
      if (result == -1)
	 Tcl_SetResult(interp, "Action not allowed\n", NULL);

      result = functiondispatch(function, value, wpoint.x, wpoint.y);
      if (result == -1)
	 Tcl_SetResult(interp, "Action not handled\n", NULL);
   }
   else {
      Tcl_SetResult(interp, "Usage: action <action_name> [<value>]\n", NULL);
      return TCL_ERROR;
   }
   return XcTagCallback(interp, objc, objv);
}


/*----------------------------------------------------------------------*/
/* Argument-converting wrappers from Tk callback to Xt callback format	*/
/*----------------------------------------------------------------------*/

void xctk_drawarea(ClientData clientData, XEvent *eventPtr)
{
   Tcl_ServiceAll();
   if (areawin->topinstance != NULL)
      drawarea(areawin->area, (caddr_t)clientData, (caddr_t)NULL);
}

/*----------------------------------------------------------------------*/

void xctk_resizearea(ClientData clientData, XEvent *eventPtr)
{
   resizearea(areawin->area, (caddr_t)clientData, (caddr_t)NULL);
   /* Callback to function "arrangetoolbar" */
   Tcl_Eval(xcinterp, "catch {xcircuit::arrangetoolbar $XCOps(focus)}");
}

/*----------------------------------------------------------------------*/
/* Because Tk doesn't filter MotionEvent events based on context, we	*/
/* have to filter the context here.					*/
/*----------------------------------------------------------------------*/

void xctk_panhbar(ClientData clientData, XEvent *eventPtr)
{
   XMotionEvent *mevent = (XMotionEvent *)eventPtr;
   u_int state = mevent->state;
   if (state & (Button1Mask | Button2Mask))
      panhbar(areawin->scrollbarh, (caddr_t)clientData, (XButtonEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_panvbar(ClientData clientData, XEvent *eventPtr)
{
   XMotionEvent *mevent = (XMotionEvent *)eventPtr;
   u_int state = mevent->state;
   if (state & (Button1Mask | Button2Mask))
      panvbar(areawin->scrollbarv, (caddr_t)clientData, (XButtonEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_drawhbar(ClientData clientData, XEvent *eventPtr)
{
   if (areawin->topinstance)
      drawhbar(areawin->scrollbarh, (caddr_t)clientData, (caddr_t)NULL);
}

/*----------------------------------------------------------------------*/

void xctk_drawvbar(ClientData clientData, XEvent *eventPtr)
{
   if (areawin->topinstance)
      drawvbar(areawin->scrollbarv, (caddr_t)clientData, (caddr_t)NULL);
}

/*----------------------------------------------------------------------*/

void xctk_endhbar(ClientData clientData, XEvent *eventPtr)
{
   if (areawin->topinstance)
      endhbar(areawin->scrollbarh, (caddr_t)clientData, (XButtonEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_endvbar(ClientData clientData, XEvent *eventPtr)
{
   if (areawin->topinstance)
      endvbar(areawin->scrollbarv, (caddr_t)clientData, (XButtonEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_zoomview(ClientData clientData, XEvent *eventPtr)
{
   zoomview((xcWidget)NULL, (caddr_t)clientData, (caddr_t)NULL);
}

/*----------------------------------------------------------------------*/

void xctk_swapschem(ClientData clientData, XEvent *eventPtr)
{
   swapschem((int)((pointertype)clientData), -1, NULL);
}

/*----------------------------------------------------------------------*/

void xctk_drag(ClientData clientData, XEvent *eventPtr)
{
   XButtonEvent *b_event = (XButtonEvent *)eventPtr;

   drag((int)b_event->x, (int)b_event->y);
   flusharea();
#ifdef HAVE_CAIRO
   if (areawin->redraw_needed)
     drawarea(NULL, NULL, NULL);
#endif /* HAVE_CAIRO */
}

/*----------------------------------------------------------------------*/
/* This really should be set up so that the "okay" button command tcl	*/
/* procedure does the job of lookdirectory().				*/
/*----------------------------------------------------------------------*/

void xctk_fileselect(ClientData clientData, XEvent *eventPtr)
{
   XButtonEvent *beventPtr = (XButtonEvent *)eventPtr;
   popupstruct *listp = (popupstruct *)clientData;
   char curentry[150];

   if (beventPtr->button == Button2) {
      Tcl_Eval(xcinterp, ".filelist.textent.txt get");
      sprintf(curentry, "%.149s", (char *)Tcl_GetStringResult(xcinterp));

      if (strlen(curentry) > 0) {
         if (lookdirectory(curentry, 149))
            newfilelist(listp->filew, listp);
	 else
	    Tcl_Eval(xcinterp, ".filelist.bbar.okay invoke");
      }
   }
   else if (beventPtr->button == Button4) {	/* scroll wheel binding */
      flstart--;
      showlscroll(listp->scroll, NULL, NULL);
      listfiles(listp->filew, listp, NULL);
   }
   else if (beventPtr->button == Button5) {	/* scroll wheel binding */
      flstart++;
      showlscroll(listp->scroll, NULL, NULL);
      listfiles(listp->filew, listp, NULL);
   }
   else
      fileselect(listp->filew, listp, beventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_listfiles(ClientData clientData, XEvent *eventPtr)
{
   popupstruct *listp = (popupstruct *)clientData;
   char *filter;

   Tcl_Eval(xcinterp, ".filelist.listwin.win cget -data");
   filter = (char *)Tcl_GetStringResult(xcinterp);

   if (filter != NULL) {
      if ((listp->filter == NULL) || (strcmp(filter, listp->filter))) {
         if (listp->filter != NULL)
	    free(listp->filter);
         listp->filter = strdup(filter);
         newfilelist(listp->filew, listp);
      }
      else
	 listfiles(listp->filew, listp, NULL);
   }
   else {
      if (listp->filter != NULL) {
	 free(listp->filter);
	 listp->filter = NULL;
      }
      listfiles(listp->filew, listp, NULL);
   }
}

/*----------------------------------------------------------------------*/

void xctk_startfiletrack(ClientData clientData, XEvent *eventPtr)
{
   startfiletrack((Tk_Window)clientData, NULL, (XCrossingEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_endfiletrack(ClientData clientData, XEvent *eventPtr)
{
   endfiletrack((Tk_Window)clientData, NULL, (XCrossingEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_dragfilebox(ClientData clientData, XEvent *eventPtr)
{
   dragfilebox((Tk_Window)clientData, NULL, (XMotionEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_draglscroll(ClientData clientData, XEvent *eventPtr)
{
   popupstruct *listp = (popupstruct *)clientData;
   XMotionEvent *mevent = (XMotionEvent *)eventPtr;
   u_int state = mevent->state;

   if (state & (Button1Mask | Button2Mask))
      draglscroll(listp->scroll, listp, (XButtonEvent *)eventPtr);
}

/*----------------------------------------------------------------------*/

void xctk_showlscroll(ClientData clientData, XEvent *eventPtr)
{
   showlscroll((Tk_Window)clientData, NULL, NULL);
}

/*----------------------------------------------------------------------*/
/* Build or rebuild the database of colors, fonts, and other settings	*/
/*  from the Tk option settings.					*/
/*----------------------------------------------------------------------*/

void build_app_database(Tk_Window tkwind)
{
   Tk_Uid	xcuid;

   /*--------------------------*/
   /* Build the color database */
   /*--------------------------*/

   if ((xcuid = Tk_GetOption(tkwind, "globalpincolor", "Color")) == NULL)
      xcuid = "Orange2";
   appdata.globalcolor = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "localpincolor", "Color")) == NULL)
      xcuid = "Red";
   appdata.localcolor = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "infolabelcolor", "Color")) == NULL)
      xcuid = "SeaGreen";
   appdata.infocolor = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "ratsnestcolor", "Color")) == NULL)
      xcuid = "tan4";
   appdata.ratsnestcolor = xc_alloccolor((char *)xcuid);

   if ((xcuid = Tk_GetOption(tkwind, "bboxcolor", "Color")) == NULL)
      xcuid = "greenyellow";
   appdata.bboxpix = xc_alloccolor((char *)xcuid);

   if ((xcuid = Tk_GetOption(tkwind, "fixedbboxcolor", "Color")) == NULL)
      xcuid = "pink";
   appdata.fixedbboxpix = xc_alloccolor((char *)xcuid);

   if ((xcuid = Tk_GetOption(tkwind, "clipcolor", "Color")) == NULL)
      xcuid = "powderblue";
   appdata.clipcolor = xc_alloccolor((char *)xcuid);

   if ((xcuid = Tk_GetOption(tkwind, "paramcolor", "Color")) == NULL)
      xcuid = "Plum3";
   appdata.parampix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "auxiliarycolor", "Color")) == NULL)
      xcuid = "Green3";
   appdata.auxpix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "axescolor", "Color")) == NULL)
      xcuid = "Antique White";
   appdata.axespix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "filtercolor", "Color")) == NULL)
      xcuid = "SteelBlue3";
   appdata.filterpix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "selectcolor", "Color")) == NULL)
      xcuid = "Gold3";
   appdata.selectpix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "snapcolor", "Color")) == NULL)
      xcuid = "Red";
   appdata.snappix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "gridcolor", "Color")) == NULL)
      xcuid = "Gray95";
   appdata.gridpix = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "pagebackground", "Color")) == NULL)
      xcuid = "White";
   appdata.bg = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "pageforeground", "Color")) == NULL)
      xcuid = "Black";
   appdata.fg = xc_alloccolor((char *)xcuid);

   if ((xcuid = Tk_GetOption(tkwind, "paramcolor2", "Color")) == NULL)
      xcuid = "Plum3";
   appdata.parampix2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "auxiliarycolor2", "Color")) == NULL)
      xcuid = "Green";
   appdata.auxpix2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "selectcolor2", "Color")) == NULL)
      xcuid = "Gold";
   appdata.selectpix2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "filtercolor2", "Color")) == NULL)
      xcuid = "SteelBlue1";
   appdata.gridpix2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "snapcolor2", "Color")) == NULL)
      xcuid = "Red";
   appdata.snappix2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "axescolor2", "Color")) == NULL)
      xcuid = "NavajoWhite4";
   appdata.axespix2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "background2", "Color")) == NULL)
      xcuid = "DarkSlateGray";
   appdata.bg2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "foreground2", "Color")) == NULL)
      xcuid = "White";
   appdata.fg2 = xc_alloccolor((char *)xcuid);
   if ((xcuid = Tk_GetOption(tkwind, "barcolor", "Color")) == NULL)
      xcuid = "Tan";
   appdata.barpix = xc_alloccolor((char *)xcuid);

   /* These are GUI colors---unused by Tcl */
   appdata.buttonpix = xc_alloccolor("Gray85");
   appdata.buttonpix2 = xc_alloccolor("Gray50");

   /* Get some default fonts (Should be using Tk calls here. . . ) */

   if ((xcuid = Tk_GetOption(tkwind, "filelistfont", "Font")) == NULL)
      xcuid = "-*-helvetica-medium-r-normal--14-*";
   appdata.filefont = XLoadQueryFont(dpy, (char *)xcuid);

   if (appdata.filefont == NULL)
   {
      appdata.filefont = XLoadQueryFont(dpy, "-*-*-medium-r-normal--14-*");
      if (appdata.filefont == NULL)
	 appdata.filefont = XLoadQueryFont(dpy, "-*-*-*-*-*--*-*");
	 if (appdata.filefont == NULL)
	    appdata.filefont = XLoadQueryFont(dpy, "*");
	    if (appdata.filefont == NULL) {
	       Fprintf(stderr, "Fatal error:  No X11 fonts found.\n");
	    }
   }

   /* Other defaults */

   if ((xcuid = Tk_GetOption(tkwind, "timeout", "TimeOut")) == NULL)
      xcuid = "10";
   appdata.timeout = atoi((char *)xcuid);
}

/*--------------------------------------------------------------*/
/* GUI Initialization under Tk					*/
/* First argument is the Tk path name of the drawing window.	*/
/* This function should be called for each new window created.	*/
/*--------------------------------------------------------------*/

XCWindowData *GUI_init(int objc, Tcl_Obj *CONST objv[])
{
   Tk_Window 	tkwind, tktop, tkdraw, tksb;
   Tk_Window	wsymb, wschema,	corner;
   int 		i, locobjc, done = 1;
   XGCValues	values;
   Window	win;
   popupstruct	*fileliststruct;
   char *xctopwin, *xcdrawwin;
   char winpath[512];
   XCWindowData *newwin;

   tktop = Tk_MainWindow(xcinterp);
   if (tktop == (Tk_Window)NULL) {
      Fprintf(stderr, "No Top-Level Tk window available. . .\n");

      /* No top level window, assuming batch mode.  To get	*/
      /* access to font information requires that cairo be set	*/
      /* up with a surface, even if it is not an xlib target.	*/

      newwin = create_new_window();
      newwin->area = NULL;
      newwin->scrollbarv = NULL;
      newwin->scrollbarh = NULL;
      newwin->width = 100;
      newwin->height = 100;

#ifdef HAVE_CAIRO
      newwin->surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
		newwin->width, newwin->height);
      newwin->cr = cairo_create(newwin->surface);
#endif /* !HAVE_CAIRO */

      number_colors = NUMBER_OF_COLORS;
      colorlist = (colorindex *)malloc(NUMBER_OF_COLORS * sizeof(colorindex));

      return newwin;
   }

   /* Check if any parameter is a Tk window name */

   locobjc = objc;
   while (locobjc > 0) {
      xctopwin = Tcl_GetString(objv[locobjc - 1]);
      tkwind = Tk_NameToWindow(xcinterp, xctopwin, tktop);
      if (tkwind != (Tk_Window)NULL)
	 break;
      locobjc--;
   }

   if (locobjc == 0) {
      /* Okay to have no GUI wrapper.  However, if this is the case,	*/
      /* then the variable "XCOps(window)" must be set to the Tk path	*/
      /* name of the drawing window.					*/

      xcdrawwin = (char *)Tcl_GetVar2(xcinterp, "XCOps", "window", 0);
      if (xcdrawwin == NULL) {
	  Fprintf(stderr, "The Tk window hierarchy must be rooted at"
		" .xcircuit, or XCOps(top)");
	  Fprintf(stderr, " must point to the hierarchy.  If XCOps(top)"
		" is NULL, then XCOps(window) must");
	  Fprintf(stderr, " point to the drawing window.\n");
	  return NULL;
      }
      tkwind = Tk_NameToWindow(xcinterp, xcdrawwin, tktop);
      if (tkwind == NULL) {
	 Fprintf(stderr, "Error:  XCOps(window) is set but does not point to"
		" a valid Tk window.\n");
	 return NULL;
      }

      /* Create new window data structure */
      newwin = create_new_window();
      newwin->area = tkwind;

      /* No GUI---GUI widget pointers need to be NULL'd */
      newwin->scrollbarv = NULL;
      newwin->scrollbarh = NULL;
   }
   else {

      /* Expect a top-level window name passed as the first argument.	*/
      /* Having a fixed hierarchy is a total kludge and needs to be	*/
      /* rewritten. . . 						*/

      if (tkwind == NULL) {
	 Fprintf(stderr, "Error:  config init given a bad window name!\n");
	 return NULL;
      }
      else {
	 /* Make sure that this window does not already exist */
	 XCWindowDataPtr searchwin;
         sprintf(winpath, "%s.mainframe.mainarea.drawing", xctopwin);
         tkdraw = Tk_NameToWindow(xcinterp, winpath, tktop);
	 for (searchwin = xobjs.windowlist; searchwin != NULL; searchwin =
			searchwin->next) {
	    if (searchwin->area == tkdraw) {
	       Fprintf(stderr, "Error:  window already exists!\n");
	       return NULL;
	    }
	 }
      }

      /* Create new window data structure and */
      /* fill in global variables from the Tk window values */

      newwin = create_new_window();
      sprintf(winpath, "%s.mainframe.mainarea.sbleft", xctopwin);
      newwin->scrollbarv = Tk_NameToWindow(xcinterp, winpath, tktop);
      sprintf(winpath, "%s.mainframe.mainarea.sbbottom", xctopwin);
      newwin->scrollbarh = Tk_NameToWindow(xcinterp, winpath, tktop);
      sprintf(winpath, "%s.mainframe.mainarea.drawing", xctopwin);
      newwin->area = Tk_NameToWindow(xcinterp, winpath, tktop);

      sprintf(winpath, "%s.mainframe.mainarea.corner", xctopwin);
      corner = Tk_NameToWindow(xcinterp, winpath, tktop);

      sprintf(winpath, "%s.infobar.symb", xctopwin);
      wsymb = Tk_NameToWindow(xcinterp, winpath, tktop);

      sprintf(winpath, "%s.infobar.schem", xctopwin);
      wschema = Tk_NameToWindow(xcinterp, winpath, tktop);

      Tk_CreateEventHandler(newwin->scrollbarh, ButtonMotionMask,
		(Tk_EventProc *)xctk_panhbar, NULL);
      Tk_CreateEventHandler(newwin->scrollbarv, ButtonMotionMask,
		(Tk_EventProc *)xctk_panvbar, NULL);
      Tk_CreateEventHandler(newwin->scrollbarh, StructureNotifyMask | ExposureMask,
		(Tk_EventProc *)xctk_drawhbar, NULL);
      Tk_CreateEventHandler(newwin->scrollbarv, StructureNotifyMask | ExposureMask,
		(Tk_EventProc *)xctk_drawvbar, NULL);
      Tk_CreateEventHandler(newwin->scrollbarh, ButtonReleaseMask,
		(Tk_EventProc *)xctk_endhbar, NULL);
      Tk_CreateEventHandler(newwin->scrollbarv, ButtonReleaseMask,
		(Tk_EventProc *)xctk_endvbar, NULL);

      Tk_CreateEventHandler(corner, ButtonPressMask,
		(Tk_EventProc *)xctk_zoomview, Number(1));
      Tk_CreateEventHandler(wsymb, ButtonPressMask,
		(Tk_EventProc *)xctk_swapschem, Number(0));
      Tk_CreateEventHandler(wschema, ButtonPressMask,
		(Tk_EventProc *)xctk_swapschem, Number(0));

      /* Setup event handlers for the drawing area and scrollbars		*/
      /* There are purposely no callback functions for these windows---they are	*/
      /* defined as type "simple" to keep down the cruft, as I will define my	*/
      /* own event handlers.							*/

      Tk_CreateEventHandler(newwin->area, StructureNotifyMask,
		(Tk_EventProc *)xctk_resizearea, NULL);
      Tk_CreateEventHandler(newwin->area, ExposureMask,
		(Tk_EventProc *)xctk_drawarea, NULL);
   }

   if ((locobjc > 0) || !Tk_IsMapped(newwin->area)) {

      /* This code copied from code for the "tkwait" command */

      Tk_CreateEventHandler(newwin->area,
		VisibilityChangeMask|StructureNotifyMask,
		WaitVisibilityProc, (ClientData) &done);
      done = 0;
   }

   /* Make sure the window is mapped */

   Tk_MapWindow(tkwind);
   win = Tk_WindowId(tkwind);
   Tk_MapWindow(newwin->area);

   if (!done) {
      while (!done) Tcl_DoOneEvent(0);
      Tk_DeleteEventHandler(newwin->area,
		VisibilityChangeMask|StructureNotifyMask,
		WaitVisibilityProc, (ClientData) &done);
   }

   newwin->window = Tk_WindowId(newwin->area);
   newwin->width = Tk_Width(newwin->area);
   newwin->height = Tk_Height(newwin->area);

   /* Things to set once only */

   if (dpy == NULL) {
      dpy = Tk_Display(tkwind);
      cmap = Tk_Colormap(tkwind);
      // (The following may be required on some systems where
      // Tk will not report a valid colormap after Tk_MapWindow())
      // cmap = DefaultColormap(dpy, DefaultScreen(dpy));

      /*------------------------------------------------------*/
      /* Handle different screen resolutions in a sane manner */
      /*------------------------------------------------------*/

      screenDPI = getscreenDPI();

      /*-------------------------*/
      /* Create stipple patterns */
      /*-------------------------*/

      for (i = 0; i < STIPPLES; i++)
         STIPPLE[i] = XCreateBitmapFromData(dpy, win, STIPDATA[i], 4, 4);

      /*----------------------------------------*/
      /* Allocate space for the basic color map */
      /*----------------------------------------*/

      number_colors = NUMBER_OF_COLORS;
      colorlist = (colorindex *)malloc(NUMBER_OF_COLORS * sizeof(colorindex));
      areawin = newwin;
      build_app_database(tkwind);
      areawin = NULL;

      /* Create the filelist window and its event handlers */

      tksb = Tk_NameToWindow(xcinterp, ".filelist.listwin.sb", tktop);
      tkdraw = Tk_NameToWindow(xcinterp, ".filelist.listwin.win", tktop);

      fileliststruct = (popupstruct *) malloc(sizeof(popupstruct));
      fileliststruct->popup = Tk_NameToWindow(xcinterp, ".filelist", tktop);
      fileliststruct->textw = Tk_NameToWindow(xcinterp, ".filelist.textent",
		fileliststruct->popup);
      fileliststruct->filew = tkdraw;
      fileliststruct->scroll = tksb;
      fileliststruct->setvalue = NULL;
      fileliststruct->filter = NULL;

      if (tksb != NULL) {
         Tk_CreateEventHandler(tksb, ButtonMotionMask,
		(Tk_EventProc *)xctk_draglscroll, (ClientData)fileliststruct);
         Tk_CreateEventHandler(tksb, ExposureMask,
		(Tk_EventProc *)xctk_showlscroll, (ClientData)tksb);
      }
      if (tkdraw != NULL) {
         Tk_CreateEventHandler(tkdraw, ButtonPressMask,
		(Tk_EventProc *)xctk_fileselect, (ClientData)fileliststruct);
         Tk_CreateEventHandler(tkdraw, ExposureMask,
		(Tk_EventProc *)xctk_listfiles, (ClientData)fileliststruct);
         Tk_CreateEventHandler(tkdraw, EnterWindowMask,
		(Tk_EventProc *)xctk_startfiletrack, (ClientData)tkdraw);
         Tk_CreateEventHandler(tkdraw, LeaveWindowMask,
		(Tk_EventProc *)xctk_endfiletrack, (ClientData)tkdraw);
      }
   }

   /*-------------------------------------------------------------------*/
   /* Generate the GC							*/
   /* Set "graphics_exposures" to False.  Every XCopyArea function	*/
   /* copies from virtual memory (dbuf pixmap), which can never be	*/
   /* obscured.  Otherwise, the server gets flooded with useless	*/
   /* NoExpose events.	  				 		*/
   /*-------------------------------------------------------------------*/

   values.foreground = BlackPixel(dpy, DefaultScreen(dpy));
   values.background = WhitePixel(dpy, DefaultScreen(dpy));
   values.graphics_exposures = False;
   newwin->gc = XCreateGC(dpy, win, GCForeground | GCBackground
		| GCGraphicsExposures, &values);

#ifdef HAVE_CAIRO
   newwin->surface = cairo_xlib_surface_create(dpy, newwin->window,
         DefaultVisual(dpy, 0), newwin->width, newwin->height);
   newwin->cr = cairo_create(newwin->surface);
#else /* HAVE_CAIRO */
   newwin->clipmask = XCreatePixmap(dpy, win, newwin->width,
                newwin->height, 1);

   values.foreground = 0;
   values.background = 0;
   newwin->cmgc = XCreateGC(dpy, newwin->clipmask, GCForeground
		| GCBackground, &values);
#endif /* HAVE_CAIRO */

   XDefineCursor (dpy, win, *newwin->defaultcursor);
   return newwin;
}

/*--------------------------------------*/
/* Inline the main wrapper prodedure	*/
/*--------------------------------------*/

int xctcl_start(ClientData clientData, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[])
{
   int result = TCL_OK;
   Boolean rcoverride = False;
   char *filearg = NULL;
   Tcl_Obj *cmdname = objv[0];

   Fprintf(stdout, "Starting xcircuit under Tcl interpreter\n");

   /* xcircuit initialization routines --- these assume that the */
   /* GUI has been created by the startup script;  otherwise bad */
   /* things will probably occur.				 */

   pre_initialize();
   areawin = GUI_init(--objc, ++objv);
   if (areawin == NULL) {
      /* Create new window data structure */
      areawin = create_new_window();
      areawin->area = NULL;
      areawin->scrollbarv = NULL;
      areawin->scrollbarh = NULL;

      Tcl_SetResult(interp, "Invalid or missing top-level windowname"
		" given to start command, assuming batch mode.\n", NULL);
   }
   post_initialize();

   ghostinit();

   /* The Tcl version accepts some command-line arguments.  Due	*/
   /* to the way ".wishrc" is processed, all arguments are	*/
   /* glommed into one Tcl (list) object, objv[1].		*/

   filearg = (char *)malloc(sizeof(char));
   *filearg = '\0';

   if (objc == 2) {
      char **argv;
      int argc;

      Tcl_SplitList(interp, Tcl_GetString(objv[1]), &argc,
		(CONST84 char ***)&argv);
      while (argc) {
         if (**argv == '-') {
	    if (!strncmp(*argv, "-exec", 5)) {
	       if (--argc > 0) {
		  argv++;
	          result = Tcl_EvalFile(interp, *argv);
	          if (result != TCL_OK) {
		     free(filearg);
		     return result;
		  }
	          else
		     rcoverride = True;
	       }
	       else {
	          Tcl_SetResult(interp, "No filename given to exec argument.", NULL);
		  free(filearg);
	          return TCL_ERROR;
	       }
	    }
	    else if (!strncmp(*argv, "-2", 2)) {
	       /* 2-button mouse bindings option */
	       pressmode = 1;
	    }
	 }
	 else if (strcmp(*argv, ".xcircuit")) {
	    filearg = (char *)realloc(filearg, sizeof(char) *
			(strlen(filearg) + strlen(*argv) + 2));
	    strcat(filearg, ",");
	    strcat(filearg, *argv);
	 }
	 argv++;
	 argc--;
      }
   }
   else {
      /* Except---this appears to be no longer true.  When did it change? */
      int argc = objc;
      char *argv;

      for (argc = 0; argc < objc; argc++) {
	 argv = Tcl_GetString(objv[argc]);
         if (*argv == '-') {
	    if (!strncmp(argv, "-exec", 5)) {
	       if (++argc < objc) {
		  argv = Tcl_GetString(objv[argc]);
	          result = Tcl_EvalFile(interp, argv);
	          if (result != TCL_OK) {
		     free(filearg);
		     return result;
		  }
	          else
		     rcoverride = True;
	       }
	       else {
	          Tcl_SetResult(interp, "No filename given to exec argument.", NULL);
		  free(filearg);
	          return TCL_ERROR;
	       }
	    }
	    else if (!strncmp(argv, "-2", 2)) {
	       /* 2-button mouse bindings option */
	       pressmode = 1;
	    }
	 }
	 else if (strcmp(argv, ".xcircuit")) {
	    filearg = (char *)realloc(filearg, sizeof(char) *
			(strlen(filearg) + strlen(argv) + 2));
	    strcat(filearg, ",");
	    strcat(filearg, argv);
	 }
      }
   }

   if (!rcoverride)
      result = loadrcfile();

   composelib(PAGELIB);	/* make sure we have a valid page list */
   composelib(LIBLIB);	/* and library directory */
   if ((objc >= 2) && (*filearg != '\0')) {
      char *libname;
      int target = -1;

      strcpy(_STR2, filearg);
      libname = (char *)Tcl_GetVar2(xcinterp, "XCOps", "library", 0);
      if (libname != NULL) {
	 target = NameToLibrary(libname);
      }
      startloadfile((target >= 0) ? target + LIBRARY : -1);
   }
   else {
      findcrashfiles();
   }
   pressmode = 0;	/* Done using this to track 2-button bindings */

   /* Note that because the setup has the windows generated and */
   /* mapped prior to calling the xcircuit routines, nothing	*/
   /* gets CreateNotify, MapNotify, or other definitive events.	*/
   /* So, we have to do all the drawing once.			*/

   xobjs.suspend = -1;		/* Release from suspend mode */
   if (areawin->scrollbarv)
      drawvbar(areawin->scrollbarv, NULL, NULL);
   if (areawin->scrollbarh)
      drawhbar(areawin->scrollbarh, NULL, NULL);
   drawarea(areawin->area, NULL, NULL);

   /* Return back to the interpreter; Tk is handling the GUI */
   free(filearg);
   return (result == TCL_OK) ? XcTagCallback(interp, 1, &cmdname) : result;
}

/*--------------------------------------------------------------*/
/* Message printing procedures for the Tcl version		*/
/*								*/
/* Evaluate the variable-length argument, and make a call to	*/
/* the routine xcircuit::print, which should be defined.	*/
/*--------------------------------------------------------------*/

void W0vprintf(char *window, const char *format, va_list args_in)
{
   char tstr[128], *bigstr = NULL, *strptr;
   int n, size;
   va_list args;

   if (window != NULL) {
      sprintf(tstr, "catch {xcircuit::print %s {", window);
      size = strlen(tstr);

      va_copy(args, args_in);
      n = vsnprintf(tstr + size, 128 - size, format, args);
      va_end(args);

      if (n <= -1 || n > 125 - size) {
         bigstr = malloc(n + size + 4);
	 strncpy(bigstr, tstr, size);
         va_copy(args, args_in);
         vsnprintf(bigstr + size, n + 1, format, args);
         va_end(args);
         strptr = bigstr;
	 strcat(bigstr, "}}");
      }
      else {
         strptr = tstr;
	 strcat(tstr, "}}");
      }
      Tcl_Eval(xcinterp, strptr);
      if (bigstr != NULL) free(bigstr);
   }
}

/* Prints to pagename window */

void W1printf(char *format, ...)
{
   va_list args;
   va_start(args, format);
   W0vprintf("coord", format, args);
   va_end(args);
}

/* Prints to coordinate window */

void W2printf(char *format, ...)
{
   va_list args;
   va_start(args, format);
   W0vprintf("page", format, args);
   va_end(args);
}

/* Prints to status window but does not tee output to the console. */

void W3printf(char *format, ...)
{
   va_list args;
   va_start(args, format);
   W0vprintf("stat", format, args);
   va_end(args);
}

/* Prints to status window and duplicates the output to stdout.	*/

void Wprintf(char *format, ...)
{
   va_list args;
   va_start(args, format);
   W0vprintf("stat", format, args);
   if (strlen(format) > 0) {
      if (strstr(format, "Error")) {
         tcl_vprintf(stderr, format, args);
         tcl_printf(stderr, "\n");
      }
      else {
         tcl_vprintf(stdout, format, args);
         tcl_printf(stdout, "\n");
      }
   }
   va_end(args);
}

/*------------------------------------------------------*/

#endif /* defined(TCL_WRAPPER) && !defined(HAVE_PYTHON) */

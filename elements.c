/*----------------------------------------------------------------------*/
/* elements.c --- xcircuit routines for creating basic elements		*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University            */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*      written by Tim Edwards, 8/13/93                                 */
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#ifdef TCL_WRAPPER 
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/Xw.h"
#endif
#endif

/*----------------------------------------------------------------------*/
/* Local includes                                                       */
/*----------------------------------------------------------------------*/

#include "xcircuit.h"
#include "colordefs.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations					*/
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Global Variable definitions                                          */
/*----------------------------------------------------------------------*/

extern Display  *dpy;    /* Works well to make this globally accessible */
extern Cursor   appcursors[NUM_CURSORS];
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern xcWidget   top;
extern fontinfo *fonts;
extern short fontcount;
extern char  _STR[150], _STR2[250];
extern int number_colors;
extern colorindex *colorlist;
#if !defined(HAVE_CAIRO)
extern Pixmap dbuf;
#endif

extern double atan2();

/*------------------------------------------------------------------------*/
/* Declarations of global variables                                       */
/*------------------------------------------------------------------------*/

char extchar[20];
double saveratio;
u_char texttype;

/*--------------------------------------*/
/* Element constructor functions	*/
/*--------------------------------------*/

/*--------------------------------------------------------------*/
/* Label constructor:  Create a new label element in the object	*/
/* whose instance is "destinst" and return a pointer to it.	*/
/*								*/
/*	"destinst" is the destination instance.  If NULL, the	*/
/*	   top-level instance (areawin->topinstance) is used.	*/
/*	"strptr" is a pointer to a stringpart string, and may	*/
/*	   be NULL.  If non-NULL, should NOT be free'd by the	*/
/*	   calling routine.					*/
/*	"pintype" is NORMAL, LOCAL, GLOBAL, or INFO		*/
/*	"x" and "y" are the label coordinates.			*/
/*	if "dochange" is FALSE, then this label is being drawn	*/
/*	   as part of a font or library and should not cause	*/
/*	   changes count to increment.				*/
/*								*/
/* Other properties must be set individually by the calling	*/
/* 	routine.						*/
/*								*/
/* Return value is a pointer to the newly created label.	*/
/*--------------------------------------------------------------*/

labelptr new_label(objinstptr destinst, stringpart *strptr, int pintype,
	int x, int y, u_char dochange)
{
   labelptr *newlab;
   objectptr destobject;
   objinstptr locdestinst;

   locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
   destobject = locdestinst->thisobject;

   NEW_LABEL(newlab, destobject);
   labeldefaults(*newlab, pintype, x, y);

   if (strptr->type == FONT_NAME) {
      free ((*newlab)->string);
      (*newlab)->string = strptr;
   }
   else
      (*newlab)->string->nextpart = strptr;

   calcbboxvalues(locdestinst, (genericptr *)newlab);
   updatepagebounds(destobject);
   if (dochange != (u_char)0) incr_changes(destobject);
   return *newlab;
}

/*--------------------------------------------------------------*/
/* Variant of the above; creates a label from a (char *) string	*/
/* instead of a stringpart pointer.  Like the stringpart 	*/
/* pointer above, "cstr" should NOT be free'd by the calling	*/
/* routine.							*/
/*--------------------------------------------------------------*/

labelptr new_simple_label(objinstptr destinst, char *cstr,
	int pintype, int x, int y)
{
   stringpart *strptr;

   strptr = (stringpart *)malloc(sizeof(stringpart));
   strptr->type = TEXT_STRING;
   strptr->nextpart = NULL;
   strptr->data.string = cstr;

   return new_label(destinst, strptr, pintype, x, y, (u_char)0);
}

/*--------------------------------------------------------------*/
/* Another variant of the above; creates a "temporary" label	*/
/* from a (char *) string.  As above, "cstr" should NOT be	*/
/* free'd by the calling routine.  The "temporary" label has no	*/
/* font information, and cannot be displayed nor saved/loaded	*/
/* from the PostScript output file.  Used to name networks or	*/
/* to mark port positions.  Pin type is always LOCAL.  Does not	*/
/* require updating bounding box info since it cannot be	*/
/* displayed.  Consequently, only requires passing the object	*/
/* to get the new label, not its instance.			*/
/*--------------------------------------------------------------*/

labelptr new_temporary_label(objectptr destobject, char *cstr,
	int x, int y)
{
   labelptr *newlab;

   NEW_LABEL(newlab, destobject);
   labeldefaults(*newlab, LOCAL, x, y);

   (*newlab)->string->type = TEXT_STRING; /* overwrites FONT record */
   (*newlab)->string->data.string = cstr;
   return *newlab;
}

/*--------------------------------------------------------------*/
/* Polygon constructor:  Create a new polygon element in the	*/
/* object whose instance is "destinst" and return a pointer to  */
/* it.								*/
/*								*/
/*	"destinst" is the destination instance.  If NULL, the	*/
/*	   top-level instance (areawin->topinstance) is used.	*/
/*	"points" is a list of XPoint pointers, should not be	*/
/*	   NULL.  It is transferred to the polygon verbatim,	*/
/*	   and should NOT be free'd by the calling routine.	*/
/*	"number" is the number of points in the list, or zero	*/
/*	   if "points" is NULL.					*/
/*								*/
/* Other properties must be set individually by the calling	*/
/* 	routine.						*/
/*								*/
/* Return value is a pointer to the newly created polygon.	*/
/*--------------------------------------------------------------*/

polyptr new_polygon(objinstptr destinst, pointlist *points, int number)
{
   polyptr *newpoly;
   objectptr destobject;
   objinstptr locdestinst;

   locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
   destobject = locdestinst->thisobject;

   NEW_POLY(newpoly, destobject);
   polydefaults(*newpoly, 0, 0, 0);
   (*newpoly)->number = number;
   (*newpoly)->points = *points;

   calcbboxvalues(locdestinst, (genericptr *)newpoly);
   updatepagebounds(destobject);
   incr_changes(destobject);
   return *newpoly;
}

/*--------------------------------------------------------------*/
/* Spline constructor:  Create a new spline element in the	*/
/* object whose instance is "destinst" and return a pointer to  */
/* it.								*/
/*								*/
/*	"destinst" is the destination instance.  If NULL, the	*/
/*	   top-level instance (areawin->topinstance) is used.	*/
/*	"points" is a array of 4 XPoints; should not be	NULL.	*/
/*								*/
/* Other properties must be set individually by the calling	*/
/* 	routine.						*/
/*								*/
/* Return value is a pointer to the newly created spline.	*/
/*--------------------------------------------------------------*/

splineptr new_spline(objinstptr destinst, pointlist points)
{
   splineptr *newspline;
   objectptr destobject;
   objinstptr locdestinst;
   int i;

   locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
   destobject = locdestinst->thisobject;

   NEW_SPLINE(newspline, destobject);
   splinedefaults(*newspline, 0, 0);

   for (i = 0; i < 4; i++)
      (*newspline)->ctrl[i] = points[i];

   calcspline(*newspline);
   calcbboxvalues(locdestinst, (genericptr *)newspline);
   updatepagebounds(destobject);
   incr_changes(destobject);
   return *newspline;
}

/*--------------------------------------------------------------*/
/* Arc constructor:  Create a new arc element in the object	*/
/* whose instance is "destinst" and return a pointer to it.	*/
/*								*/
/*	"destinst" is the destination instance.  If NULL, the	*/
/*	   top-level instance (areawin->topinstance) is used.	*/
/*	"radius" is the radius of the (circular) arc.		*/
/*	"x" and "y" represents the arc center position.		*/
/*								*/
/* Other properties must be set individually by the calling	*/
/* 	routine.						*/
/*								*/
/* Return value is a pointer to the newly created arc.		*/
/*--------------------------------------------------------------*/

arcptr new_arc(objinstptr destinst, int radius, int x, int y)
{
   arcptr *newarc;
   objectptr destobject;
   objinstptr locdestinst;

   locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
   destobject = locdestinst->thisobject;

   NEW_ARC(newarc, destobject);
   arcdefaults(*newarc, x, y);
   (*newarc)->radius = (*newarc)->yaxis = radius;

   calcarc(*newarc);
   calcbboxvalues(locdestinst, (genericptr *)newarc);
   updatepagebounds(destobject);
   incr_changes(destobject);
   return *newarc;
}

/*--------------------------------------------------------------*/
/* Instance constructor:  Create a new object instance element	*/
/* in the object whose instance is "destinst" and return a	*/
/* pointer to it.						*/
/*								*/
/*	"destinst" is the destination instance.  If NULL, the	*/
/*	   top-level instance (areawin->topinstance) is used.	*/
/*	"srcinst" is the source instance of which this is a	*/
/*	   copy.						*/
/*	"x" and "y" represents the instance position.		*/
/*								*/
/* Other properties must be set individually by the calling	*/
/* 	routine.						*/
/*								*/
/* Return value is a pointer to the newly created arc.		*/
/*--------------------------------------------------------------*/

objinstptr new_objinst(objinstptr destinst, objinstptr srcinst, int x, int y)
{
   objinstptr *newobjinst;
   objectptr destobject;
   objinstptr locdestinst;

   locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
   destobject = locdestinst->thisobject;

   NEW_OBJINST(newobjinst, destobject);
   instcopy(*newobjinst, srcinst);
   (*newobjinst)->position.x = x;
   (*newobjinst)->position.y = y;

   calcbboxvalues(locdestinst, (genericptr *)newobjinst);
   updatepagebounds(destobject);
   incr_changes(destobject);
   return *newobjinst;
}

/*--------------------------------------------------------------*/
/* Generic element destructor function				*/
/* (Note---this function is not being used anywhere. . .)	*/
/*--------------------------------------------------------------*/

void remove_element(objinstptr destinst, genericptr genelem)
{
   objectptr destobject;
   objinstptr locdestinst;

   locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
   destobject = locdestinst->thisobject;

   genelem->type &= REMOVE_TAG;
   delete_tagged(locdestinst);
   calcbboxvalues(locdestinst, (genericptr *)NULL);
   updatepagebounds(destobject);
}

/*-------------------------------------*/
/* Sane values for a new path instance */
/*-------------------------------------*/

void pathdefaults(pathptr newpath, int x, int y)
{
   UNUSED(x);
   UNUSED(y);

   newpath->style = NORMAL;
   newpath->width = areawin->linewidth;
   newpath->style = areawin->style;
   newpath->color = areawin->color;
   newpath->parts = 0;
   newpath->plist = (genericptr *)NULL;
   newpath->passed = NULL;
}

/*---------------------------------------*/
/* Sane values for a new object instance */
/*---------------------------------------*/

void instancedefaults(objinstptr newinst, objectptr thisobj, int x, int y)
{
   newinst->position.x = x;
   newinst->position.y = y;
   newinst->rotation = 0.0;
   newinst->scale = 1.0;
   newinst->style = LINE_INVARIANT;
   newinst->thisobject = thisobj;
   newinst->color = areawin->color;
   newinst->params = NULL;
   newinst->passed = NULL;

   newinst->bbox.lowerleft.x = thisobj->bbox.lowerleft.x;
   newinst->bbox.lowerleft.y = thisobj->bbox.lowerleft.y;
   newinst->bbox.width = thisobj->bbox.width;
   newinst->bbox.height = thisobj->bbox.height;

   newinst->schembbox = NULL;
}

/*--------------------------------------*/
/* Draw a dot at the current point.     */
/*--------------------------------------*/

void drawdot(int xpos, int ypos)
{
   arcptr *newarc;
   objinstptr *newdot;
   objectptr dotobj;
   
   /* Find the object "dot" in the builtin library, or else use an arc */
   
   if ((dotobj = finddot()) != (objectptr)NULL) {
      NEW_OBJINST(newdot, topobject);
      instancedefaults(*newdot, dotobj, xpos, ypos);
      register_for_undo(XCF_Dot, UNDO_DONE, areawin->topinstance, *newdot);
   }
   else {
      NEW_ARC(newarc, topobject);
      arcdefaults(*newarc, xpos, ypos);
      (*newarc)->radius = 6;
      (*newarc)->yaxis = 6;
      (*newarc)->width = 1.0;
      (*newarc)->style = FILLED | FILLSOLID | NOBORDER;
      (*newarc)->passed = NULL;
      (*newarc)->cycle = NULL;
      calcarc(*newarc);
      register_for_undo(XCF_Arc, UNDO_DONE, areawin->topinstance, *newarc);
   }
   incr_changes(topobject);
}

/*--------------------------------------*/
/* Sane default values for a label	*/
/*--------------------------------------*/

void labeldefaults(labelptr newlabel, u_char dopin, int x, int y)
{
   newlabel->rotation = 0.0;
   newlabel->color = areawin->color;
   newlabel->scale = areawin->textscale;
   newlabel->string = (stringpart *)malloc(sizeof(stringpart));
   newlabel->passed = NULL;
   newlabel->cycle = NULL;

   /* initialize string with font designator */
   newlabel->string->type = FONT_NAME;
   newlabel->string->data.font = areawin->psfont;
   newlabel->string->nextpart = NULL;

   newlabel->pin = dopin;
   if (dopin == LOCAL) newlabel->color = LOCALPINCOLOR;
   else if (dopin == GLOBAL) newlabel->color = GLOBALPINCOLOR;
   else if (dopin == INFO) newlabel->color = INFOLABELCOLOR;

   newlabel->anchor = areawin->anchor;
   newlabel->position.x = x;
   newlabel->position.y = y;
}

/*--------------------------------------*/
/* Button handler when creating a label */
/*--------------------------------------*/

void textbutton(u_char dopin, int x, int y)
{
   labelptr *newlabel;
   XPoint userpt;
   short tmpheight, *newselect;

   XDefineCursor(dpy, areawin->window, TEXTPTR);
   W3printf("Click to end or cancel.");

   if (fontcount == 0)
      Wprintf("Warning:  No fonts available!");

   unselect_all();
   NEW_LABEL(newlabel, topobject);
   newselect = allocselect();
   *newselect = topobject->parts - 1;
   snap(x, y, &userpt);
   labeldefaults(*newlabel, dopin, userpt.x, userpt.y);

   tmpheight = (short)(TEXTHEIGHT * (*newlabel)->scale);
   userpt.y -= ((*newlabel)->anchor & NOTBOTTOM) ?
	(((*newlabel)->anchor & TOP) ? tmpheight : tmpheight / 2) : 0;
   areawin->origin.x = userpt.x;
   areawin->origin.y = userpt.y;
   areawin->textpos = 1;  /* Text position is *after* the font declaration */

   text_mode_draw(xcDRAW_EDIT, *newlabel);
}

/*----------------------------------------------------------------------*/
/* Report on characters surrounding the current text position		*/
/*----------------------------------------------------------------------*/

#define MAXCHARS 10

void charreport(labelptr curlabel)
{
   int i, locpos, cleft = 149;
   stringpart *strptr;

   _STR2[0] = '\0';
   for (i = areawin->textpos - MAXCHARS; i <= areawin->textpos + MAXCHARS - 1; i++) {
      if (i < 0) continue; 
      strptr = findstringpart(i, &locpos, curlabel->string, areawin->topinstance);
      if (i == areawin->textpos) {
	 strncat(_STR2, "| ", cleft);
	 cleft -= 2;
      }
      if (strptr == NULL) break;
      if (strptr->type != RETURN || strptr->data.flags == 0) {
         charprint(_STR, strptr, locpos);
         cleft -= strlen(_STR);
         strncat(_STR2, _STR, cleft);
         strncat(_STR2, " ", --cleft);
         if (cleft <= 0) break;
      }
   }
   W3printf("%s", _STR2);
}

/*----------------------------------------------------------------------*/
/* See if a (pin) label has a copy (at least one) in this drawing.	*/
/*----------------------------------------------------------------------*/

labelptr findlabelcopy(labelptr curlabel, stringpart *curstring)
{
   genericptr *tgen;
   labelptr tlab;

   for (tgen = topobject->plist; tgen < topobject->plist + topobject->parts; tgen++) {
      if (IS_LABEL(*tgen)) {
         tlab = TOLABEL(tgen);
	 if (tlab->pin != LOCAL) continue;
	 else if (tlab == curlabel) continue;  /* Don't count self! */
         else if (!stringcomp(tlab->string, curstring)) return tlab;  
      }
   }
   return NULL;
}

/*--------------------------------------------------------------*/
/* Interpret string and add to current label.			*/
/* 	keypressed is a KeySym					*/
/*	clientdata can pass information for label controls	*/
/*								*/
/* Return TRUE if labeltext handled the character, FALSE if the	*/
/* character was not recognized.				*/
/*--------------------------------------------------------------*/

Boolean labeltext(int keypressed, char *clientdata)
{
   labelptr curlabel;
   stringpart *curpos, *labelbuf;
   int locpos;
   Boolean r = True, do_redraw = False;
   short cfont;
   TextExtents tmpext;
   TechPtr oldtech, newtech;

   curlabel = TOLABEL(EDITPART);

   if (curlabel == NULL || curlabel->type != LABEL) {
      Wprintf("Error:  Bad label string");
      text_mode_draw(xcDRAW_EMPTY, curlabel);
      eventmode = NORMAL_MODE;
      return FALSE;
   }

   /* find text segment of the current position */
   curpos = findstringpart(areawin->textpos, &locpos, curlabel->string,
		areawin->topinstance);

   if (clientdata != NULL && keypressed == TEXT_DELETE) {
      if (areawin->textpos > 1) {
         int curloc, strpos;
         stringpart *strptr;

         if (areawin->textend == 0) areawin->textend = areawin->textpos - 1;

         undrawtext(curlabel);
         for (strpos = areawin->textpos - 1; strpos >= areawin->textend; strpos--) {
	    strptr = findstringpart(strpos, &curloc, curlabel->string,
			 areawin->topinstance);
	    if (curloc >= 0) {
	       memmove(strptr->data.string + curloc,
			strptr->data.string + curloc + 1,
			strlen(strptr->data.string + curloc + 1) + 1);
	       if (strlen(strptr->data.string) == 0)
	          deletestring(strptr, &curlabel->string, areawin->topinstance);
	    }

            /* Don't delete any parameter boundaries---must use	*/
            /* "unparameterize" command for that.		*/

	    else if (strptr != NULL) {
	       if ((strptr->type != PARAM_START) && (strptr->type != PARAM_END))
	          deletestring(strptr, &curlabel->string, areawin->topinstance);
	       else
	          areawin->textpos++;
	    }
	    else
	       Fprintf(stdout, "Error:  Unexpected NULL string part\n");
            areawin->textpos--;
         }
         areawin->textend = 0;
         do_redraw = True;
      }
   }
   else if (clientdata != NULL && keypressed == TEXT_DEL_PARAM) {
      if (areawin->textpos > 1) {
         int curloc, strpos;
         stringpart *strptr;

         strpos = areawin->textpos - 1;
         strptr = findstringpart(strpos, &curloc, curlabel->string,
			 areawin->topinstance);
         if (curloc > 0) strpos -= curloc;	 /* move to beginning of string */
         if ((strptr != NULL) && (strptr->type != PARAM_START) && (strpos > 0)) {
            strpos--;
	    strptr = findstringpart(strpos--, &curloc, curlabel->string,
			areawin->topinstance);
	 }
         if ((strptr != NULL) && (strptr->type == PARAM_START)) {
            if (strpos >= 0) {
	        undrawtext(curlabel);
	        unmakeparam(curlabel, areawin->topinstance, strptr);
                do_redraw = True;
	    }
	 }
      }
   }
   else if (clientdata != NULL && keypressed == TEXT_RETURN) {
      Boolean hasstuff = False;   /* Check for null string */
      stringpart *tmppos;

      for (tmppos = curlabel->string; tmppos != NULL; tmppos = tmppos->nextpart) {
	 if (tmppos->type == PARAM_START) hasstuff = True;
	 else if (tmppos->type == TEXT_STRING) hasstuff = True;
      }
      XDefineCursor(dpy, areawin->window, DEFAULTCURSOR);

      W3printf("");

      if (hasstuff && (eventmode != ETEXT_MODE && eventmode != CATTEXT_MODE)) {
         register_for_undo(XCF_Text, UNDO_MORE, areawin->topinstance,
                    curlabel);

	 incr_changes(topobject);
	 invalidate_netlist(topobject);

      }
      else if (!hasstuff && (eventmode == ETEXT_MODE)) {
	 if (*(areawin->selectlist) < topobject->parts) {
	    /* Force the "delete" undo record to be a continuation of	*/
	    /* the undo series containing the edit.  That way, "undo"	*/
	    /* does not generate a label with null text.		*/

	    xobjs.undostack->idx = -xobjs.undostack->idx;
	    standard_element_delete(NORMAL);
	 }
	 else {
	    /* Label had just been created; just delete it w/o undo */
	    freelabel(curlabel->string);
	    free(curlabel);
	    topobject->parts--;
	    unselect_all();
	 }
      }

      if ((!hasstuff) && (eventmode == CATTEXT_MODE)) {  /* can't have null labels! */ 
	 undo_action();
	 redrawtext(curlabel);
         areawin->redraw_needed = False; /* ignore previous redraw requests */
   	 text_mode_draw(xcDRAW_FINAL, curlabel);
	 Wprintf("Object must have a name!");
	 eventmode = CATALOG_MODE;
      }
      else if (!hasstuff) {
         areawin->redraw_needed = False; /* ignore previous redraw requests */
   	 text_mode_draw(xcDRAW_FINAL, curlabel);
	 eventmode = NORMAL_MODE;
      }
      else if (eventmode == CATTEXT_MODE) {
	 objectptr libobj;
	 stringpart *oldname;
	 int page, libnum;

	 /* Get the library object whose name matches the original string */
	 oldname = get_original_string(curlabel);
	 if ((libobj = NameToObject(oldname->nextpart->data.string, NULL, FALSE))
			!= NULL) {

            /* Set name of object to new string.  Don't overwrite the	*/
	    /* object's technology *unless* the new string has a	*/
	    /* namespace, in which case the object's technology gets	*/
	    /* changed.							*/

	    char *techptr, *libobjname = libobj->name;
	    if ((techptr = strstr(libobjname, "::")) != NULL &&
			(strstr(curlabel->string->nextpart->data.string, "::")
			== NULL))
	       libobjname = techptr + 2;

	    /* Save a pointer to the old technology before we overwrite the name */
	    oldtech = GetObjectTechnology(libobj);

	    strcpy(libobjname, curlabel->string->nextpart->data.string);

	    /* If checkname() alters the name, it has to be copied back to */
	    /* the catalog label for the object.			   */

	    if (checkname(libobj)) {
	       undrawtext(curlabel);
	       curlabel->string->nextpart->data.string = (char *)realloc(
			curlabel->string->nextpart->data.string,
		  	(strlen(libobj->name) + 1) * sizeof(char));
	       strcpy(curlabel->string->nextpart->data.string, libobj->name);
	       redrawtext(curlabel);
	    }
	    AddObjectTechnology(libobj);

	    /* If the technology name has changed, then both the old	*/
	    /* technology and the new technology need to be marked as	*/
	    /* having been modified.					*/

	    newtech = GetObjectTechnology(libobj);

	    if (oldtech != newtech) {
	       if (oldtech) oldtech->flags |= (u_char)TECH_CHANGED;
	       if (newtech) newtech->flags |= (u_char)TECH_CHANGED;
	    }
	 }

	 /* Check if we altered a page name */
	 else if ((libobj = NameToPageObject(oldname->nextpart->data.string,
		NULL, &page)) != NULL) {
	    strcpy(libobj->name, curlabel->string->nextpart->data.string);
	    renamepage(page);
	 }

	 /* Check if we altered a library name */
	 else if ((libnum = NameToLibrary(oldname->nextpart->data.string)) != -1) {
	    libobj = xobjs.libtop[libnum + LIBRARY]->thisobject;
	    strcpy(libobj->name, curlabel->string->nextpart->data.string);
	 }
 	 else {
	    Wprintf("Error:  Cannot match name to any object, page, or library!");
	    refresh(NULL, NULL, NULL);
         }
         areawin->redraw_needed = False; /* ignore previous redraw requests */
   	 text_mode_draw(xcDRAW_FINAL, curlabel);

         eventmode = CATALOG_MODE;
      }
      else {	/* (hasstuff && eventmode != CATTEXT_MODE) */
         areawin->redraw_needed = False; /* ignore previous redraw requests */
   	 text_mode_draw(xcDRAW_FINAL, curlabel);
	 eventmode = NORMAL_MODE;
	 incr_changes(topobject);
	 if (curlabel->pin != False) invalidate_netlist(topobject);
      }

      setdefaultfontmarks();
      setcolormark(areawin->color);

      if (!hasstuff) {
	 /* Delete empty labels */
	 standard_element_delete(NORMAL);
      }
      else if ((labelbuf = get_original_string(curlabel)) != NULL) {

	 /* If the original label (before modification) is a pin in a	*/
	 /* schematic/symbol with a matching symbol/schematic, and the	*/
	 /* name is unique, change every pin in the matching symbol/	*/
	 /* schematic to match the new text.				*/

	 if ((curlabel->pin == LOCAL) && (topobject->symschem != NULL) &&
			(topobject->symschem->schemtype != PRIMARY)) {
	    if ((findlabelcopy(curlabel, labelbuf) == NULL)
			&& (findlabelcopy(curlabel, curlabel->string) == NULL)) {
	       if (changeotherpins(curlabel, labelbuf) > 0) {
	          if (topobject->schemtype == PRIMARY ||
				topobject->schemtype == SECONDARY)
	             Wprintf("Changed corresponding pin in associated symbol");
	          else
	             Wprintf("Changed corresponding pin in associated schematic");
	          incr_changes(topobject->symschem);
	          invalidate_netlist(topobject->symschem);
	       }
	    }
	 }
	
	 resolveparams(areawin->topinstance);
         updateinstparam(topobject);
	 setobjecttype(topobject);
      }
      else
         calcbbox(areawin->topinstance);

      unselect_all();
      return r;
   }
   else if (clientdata != NULL && keypressed == TEXT_RIGHT) {
      if (curpos != NULL) areawin->textpos++;
   }
   else if (clientdata != NULL && keypressed == TEXT_LEFT) {
      if (areawin->textpos > 1) areawin->textpos--;
   }
   else if (clientdata != NULL && keypressed == TEXT_DOWN) {
      while (curpos != NULL) {
	 areawin->textpos++;
	 curpos = findstringpart(areawin->textpos, &locpos, curlabel->string,
			areawin->topinstance);
	 if (curpos != NULL)
	    if (curpos->type == RETURN || curpos->type == MARGINSTOP)
	       break;
      }
   }
   else if (clientdata != NULL && keypressed == TEXT_UP) {
      while (areawin->textpos > 1) {
	 areawin->textpos--;
	 curpos = findstringpart(areawin->textpos, &locpos, curlabel->string,
	 		areawin->topinstance);
	 if (curpos->type == RETURN || curpos->type == MARGINSTOP) {
	    if (areawin->textpos > 1) areawin->textpos--;
	    break;
	 }
      }
   }
   else if (clientdata != NULL && keypressed == TEXT_HOME)
      areawin->textpos = 1;
   else if (clientdata != NULL && keypressed == TEXT_END)
      areawin->textpos = stringlength(curlabel->string, True, areawin->topinstance);
   else if (clientdata != NULL && keypressed == TEXT_SPLIT) {
      labelptr *newlabel;
      XPoint points[4], points1[4], points2[4];

      /* Everything after the cursor gets dumped into a new label */

      if ((areawin->textpos > 1) && (curpos != NULL)) {
	 labelbbox(curlabel, points, areawin->topinstance);
         undrawtext(curlabel);
	 NEW_LABEL(newlabel, topobject);
	 labeldefaults(*newlabel, curlabel->pin, curlabel->position.x,
		curlabel->position.y);
         if (locpos > 0)
            curpos = splitstring(areawin->textpos, &curlabel->string,
			areawin->topinstance);
	 /* move back one position to find end of top part of string */
         curpos = splitstring(areawin->textpos - 1, &curlabel->string,
			areawin->topinstance);
	 if (curpos->nextpart->type == FONT_NAME) {
	    freelabel((*newlabel)->string);
	    (*newlabel)->string = curpos->nextpart;
	 }
	 else {
	    (*newlabel)->string->data.font = curlabel->string->data.font;
	    (*newlabel)->string->nextpart = curpos->nextpart;
	 }
	 curpos->nextpart = NULL;

	 /* Adjust position of both labels to retain their original	*/
	 /* relative positions.						*/

	 labelbbox(curlabel, points1, areawin->topinstance);
	 labelbbox((*newlabel), points2, areawin->topinstance);
	 curlabel->position.x += (points[1].x - points1[1].x);
	 curlabel->position.y += (points[1].y - points1[1].y);
	 (*newlabel)->position.x += (points[3].x - points2[3].x);
	 (*newlabel)->position.y += (points[3].y - points2[3].y);
	
         redrawtext(*newlabel);
         do_redraw = True;
      }
   }

   /* Write a font designator or other control into the string */

   else if (clientdata != NULL) {
      oparamptr ops;
      stringpart *newpart;
      Boolean errcond = False;
      TextLinesInfo tlinfo;

      /* erase first before redrawing unless the string is empty */
      undrawtext(curlabel);

      /* Get text width first.  Don't back up over spaces;  this	*/
      /* allows the margin width to be padded out with spaces.		*/

      if (keypressed == MARGINSTOP) {
	 tlinfo.dostop = areawin->textpos;
	 tlinfo.tbreak = NULL;
	 tlinfo.padding = NULL;
	 tmpext = ULength(curlabel, areawin->topinstance, &tlinfo);
	 if (tlinfo.padding != NULL) free(tlinfo.padding);
      }

      if (locpos > 0) {
	 if (keypressed == MARGINSTOP) {
	    /* Move forward by any spaces; if we're at the text */
	    /* end, move to the next text part;  otherwise,	*/
	    /* split the string.				*/

	    while (*(curpos->data.string + locpos) == ' ') locpos++;
	    if (*(curpos->data.string + locpos) == '\0') locpos = 0;
	 }
	 if (locpos > 0)
            curpos = splitstring(areawin->textpos, &curlabel->string,
			areawin->topinstance);
	 curpos = curpos->nextpart;
      }
      newpart = makesegment(&curlabel->string, curpos);
      newpart->type = keypressed;
      switch (keypressed) {
	 case RETURN:
	    /* Identify this as an explicitly placed line break */
	    newpart->data.flags = 0;
	    break;
	 case FONT_SCALE:
	    newpart->data.scale = *((float *)clientdata);
	    break;
	 case KERN:
	    newpart->data.kern[0] = *((short *)clientdata);
	    newpart->data.kern[1] = *((short *)clientdata + 1);
	    break;
	 case FONT_COLOR:
	    newpart->data.color = *((int *)clientdata);
	    if (newpart->data.color >= number_colors) errcond = True;
	    break;
	 case FONT_NAME:
	    newpart->data.font = *((int *)clientdata);
	    if (newpart->data.font >= fontcount) errcond = True;
	    break;
	 case MARGINSTOP:
	    /* A margin of 1 or 0 is useless, so such a value	*/
	    /* indicates to take the margin from the current	*/
	    /* position.					*/

	    if (*((int *)clientdata) <= 1)
	       newpart->data.width = (int)tmpext.width;
	    else
	       newpart->data.width = *((int *)clientdata);
	    CheckMarginStop(curlabel, areawin->topinstance, FALSE);
	    break;
	 case PARAM_START:
	    newpart->data.string = (char *)malloc(1 + strlen(clientdata));
	    strcpy(newpart->data.string, clientdata);
	    ops = match_param(topobject, clientdata);
	    if (ops == NULL) errcond = True;
	    else {
	       /* Forward edit cursor position to the end of the parameter */
	       do {
	          areawin->textpos++;
	          curpos = findstringpart(areawin->textpos, &locpos, curlabel->string,
	 		areawin->topinstance);
	       } while (curpos && (curpos->type != PARAM_END));
	    }
	    break;
      }
      if (errcond == True) {
	 Wprintf("Error in insertion.  Ignoring.");
	 deletestring(newpart, &curlabel->string, areawin->topinstance);
	 r = FALSE;
      }
      else {
         areawin->textpos++;
      }
      do_redraw = True;
   }

   /* Append the character to the string.  If the current label segment is	*/
   /* not text, then create a text segment for it.				*/

   else if (keypressed > 0 && keypressed < 256) {
      stringpart *lastpos;

      /* erase first. */
      undrawtext(curlabel);

      /* Current position is not in a text string */
      if (locpos < 0) {

         /* Find part of string which is immediately in front of areawin->textpos */
         lastpos = findstringpart(areawin->textpos - 1, &locpos, curlabel->string,
		areawin->topinstance);

	 /* No text on either side to attach to: make a new text segment */
	 if (locpos < 0) {
	    curpos = makesegment(&curlabel->string, curpos);
	    curpos->type = TEXT_STRING;
	    curpos->data.string = (u_char *) malloc(2);
	    curpos->data.string[0] = keypressed;
	    curpos->data.string[1] = '\0';
	 }
	 else {		/* append to end of lastpos text string */
	    int slen = strlen(lastpos->data.string);
	    lastpos->data.string = (u_char *) realloc(lastpos->data.string,
		2 +  slen);
	    *(lastpos->data.string + slen) = keypressed;
	    *(lastpos->data.string + slen + 1) = '\0';
	 }
      }
      else {	/* prepend to end of curpos text string */
         curpos->data.string = (u_char *) realloc(curpos->data.string, 
	     2 + strlen(curpos->data.string));
	 memmove(curpos->data.string + locpos + 1, curpos->data.string + locpos,
		strlen(curpos->data.string + locpos) + 1);
         *(curpos->data.string + locpos) = keypressed;
      }
      areawin->textpos++;	/* move forward to next text position */
      do_redraw = True;
      r = TRUE;
   }

   /* Redraw the label */

   if (do_redraw) {
      /* Generate automatic line breaks if there is a MARGINSTOP directive */
      CheckMarginStop(curlabel, areawin->topinstance, TRUE);

      redrawtext(curlabel);
   }

   areawin->redraw_needed = False; /* ignore previous full redraw requests */
   text_mode_draw(xcDRAW_EDIT, curlabel);

   if (r || do_redraw) {

      /* Report on characters at the cursor position in the message window */

      charreport(curlabel);

      /* find current font and adjust menubuttons as necessary */

      cfont = findcurfont(areawin->textpos, curlabel->string, areawin->topinstance);
      if (cfont < 0) {
         Wprintf("Error:  Illegal label string");
         return r;
      }
      else
         setfontmarks(cfont, -1);

      areawin->textend = 0;
   }
   return r;
}

/*-------------------------------------*/
/* Initiate return from text edit mode */
/*-------------------------------------*/

void textreturn()
{
   labeltext(TEXT_RETURN, (char *)1);
}

/*--------------------------------------*/
/* Change the anchoring of a label	*/
/*--------------------------------------*/

void reanchor(short mode)
{
   labelptr curlabel = NULL;
   short    *tsel;
   short jsave;
   Boolean preselected = False, changed = False;
   static short transanchor[] = {15, 13, 12, 7, 5, 4, 3, 1, 0};

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      curlabel = TOLABEL(EDITPART);
      UDrawTLine(curlabel);
      undrawtext(curlabel);
      jsave = curlabel->anchor;
      curlabel->anchor = transanchor[mode] |
		(curlabel->anchor & NONANCHORFIELD);
      if (jsave != curlabel->anchor) {
	 register_for_undo(XCF_Anchor, UNDO_MORE, areawin->topinstance,
			(genericptr)curlabel, (int)jsave);
	 changed = True;
      }
      redrawtext(curlabel);
      UDrawTLine(curlabel);

      setfontmarks(-1, curlabel->anchor);
   }
   else {
      if (areawin->selects == 0) {
	 if (!checkselect(LABEL))
	    return;
      }
      else preselected = TRUE;

      for (tsel = areawin->selectlist; tsel < areawin->selectlist +
	      areawin->selects; tsel++) {
	 if (SELECTTYPE(tsel) == LABEL) {
	    curlabel = SELTOLABEL(tsel);
            jsave = curlabel->anchor;
	    undrawtext(curlabel);
      	    curlabel->anchor = transanchor[mode] |
		(curlabel->anchor & NONANCHORFIELD);
            if (jsave != curlabel->anchor) {
	       register_for_undo(XCF_Anchor, UNDO_MORE, areawin->topinstance,
			(genericptr)curlabel, (int)jsave);
	       changed = True;
	    }
	 }
      }
      if (preselected == FALSE && eventmode != MOVE_MODE && eventmode != COPY_MODE)
	 unselect_all();
      else
	 draw_all_selected();
   }
   if (curlabel == NULL)
      Wprintf("No labels chosen to reanchor");
   else if (changed) {
      pwriteback(areawin->topinstance);
      calcbbox(areawin->topinstance);
      incr_changes(topobject);
   }
}

/*----------------------------------*/
/* Sane default values for a spline */
/*----------------------------------*/

void splinedefaults(splineptr newspline, int x, int y)
{
   short j;

   for (j = 0; j < 4; j++) {
      newspline->ctrl[j].x = x;
      newspline->ctrl[j].y = y;
   }
   newspline->ctrl[1].x += (int)(xobjs.pagelist[areawin->page]->gridspace / 2);
   newspline->ctrl[2].x -= (int)(xobjs.pagelist[areawin->page]->gridspace / 2);
   newspline->width = areawin->linewidth;
   newspline->style = areawin->style;
   newspline->color = areawin->color;
   newspline->passed = NULL;
   newspline->cycle = NULL;
   calcspline(newspline);
}

/*-------------------------*/
/* Start drawing a spline. */
/*-------------------------*/

void splinebutton(int x, int y)
{
   splineptr *newspline;
   XPoint userpt;
   short *newselect;

   unselect_all();
   NEW_SPLINE(newspline, topobject);
   newselect = allocselect();
   *newselect = topobject->parts - 1;

   snap(x, y, &userpt);
   splinedefaults(*newspline, userpt.x, userpt.y);
   addcycle((genericptr *)newspline, 3, 0);
   makerefcycle((*newspline)->cycle, 3);

   spline_mode_draw(xcDRAW_EDIT, *newspline);

   xcAddEventHandler(areawin->area, PointerMotionMask, False,
        (xcEventHandler)trackelement, NULL);

   eventmode = SPLINE_MODE;
}

/*----------------------------------------------------------------------*/
/* Generate cycles on a path where endpoints meet, so that the path	*/
/* remains connected during an edit.  If the last point on any part	*/
/* of the path is a cycle, then the first point on the next part of	*/
/* the path should also be a cycle, with the same flags.		*/
/*									*/
/* If the global setting "tangents" is set, then the control points of	*/
/* connecting splines set the corresponding control point to "ANTIXY"	*/
/* so that the control points track angle and distance from the		*/
/* endpoint.								*/
/*----------------------------------------------------------------------*/

void updatepath(pathptr thepath)
{
   genericptr *ggen, *ngen;
   short locparts, cycle;
   pointselect *cptr;
   polyptr thispoly;
   splineptr thisspline;

   for (ggen = thepath->plist; ggen < thepath->plist + thepath->parts; ggen++) {
      switch (ELEMENTTYPE(*ggen)) {
	 case POLYGON:
	    findconstrained(TOPOLY(ggen));
	    break;
      }
   }

   locparts = (thepath->style & UNCLOSED) ? thepath->parts - 1 : thepath->parts;
   for (ggen = thepath->plist; ggen < thepath->plist + locparts; ggen++) {
      ngen = (ggen == thepath->plist + thepath->parts - 1) ? thepath->plist : ggen + 1;
    
      switch (ELEMENTTYPE(*ggen)) {
	 case POLYGON:
	    thispoly = TOPOLY(ggen);
	    if (thispoly->cycle == NULL) continue;
	    cycle = thispoly->number - 1;
	    for (cptr = thispoly->cycle;; cptr++) {
	       if (cptr->number == cycle) break;
	       if (cptr->flags & LASTENTRY) break;
	    }
	    if (cptr->number != cycle) continue;
	    break;
	 case SPLINE:
	    thisspline = TOSPLINE(ggen);
	    if (thisspline->cycle == NULL) continue;
	    cycle = 3;
	    for (cptr = thisspline->cycle;; cptr++) {
	       if (cptr->number == cycle) break;
	       if (cptr->flags & LASTENTRY) break;
	    }
	    if (cptr->number != cycle) continue;
	    break;
      }
      addcycle(ngen, 0, cptr->flags & (EDITX | EDITY));
      switch (ELEMENTTYPE(*ngen)) {
	 case POLYGON:
	    findconstrained(TOPOLY(ngen));
	    break;
      }
   }

   /* Do the same thing in the other direction */
   locparts = (thepath->style & UNCLOSED) ? 1 : 0;
   for (ggen = thepath->plist + thepath->parts - 1; ggen >= thepath->plist + locparts;
		ggen--) {
      ngen = (ggen == thepath->plist) ? thepath->plist + thepath->parts - 1 : ggen - 1;
    
      switch (ELEMENTTYPE(*ggen)) {
	 case POLYGON:
	    thispoly = TOPOLY(ggen);
	    if (thispoly->cycle == NULL) continue;
	    cycle = 0;
	    for (cptr = thispoly->cycle;; cptr++) {
	       if (cptr->number == cycle) break;
	       if (cptr->flags & LASTENTRY) break;
	    }
	    if (cptr->number != cycle) continue;
	    break;
	 case SPLINE:
	    thisspline = TOSPLINE(ggen);
	    if (thisspline->cycle == NULL) continue;
	    cycle = 0;
	    for (cptr = thisspline->cycle;; cptr++) {
	       if (cptr->number == cycle) break;
	       if (cptr->flags & LASTENTRY) break;
	    }
	    if (cptr->number != cycle) continue;
	    break;
      }
      switch (ELEMENTTYPE(*ngen)) {
	 case POLYGON:
            addcycle(ngen, TOPOLY(ngen)->number - 1, cptr->flags & (EDITX | EDITY));
	    break;
	 case SPLINE:
            addcycle(ngen, 3, cptr->flags & (EDITX | EDITY));
	    break;
      }
   }
}

/*--------------------------------------*/
/* Set default values for an arc	*/
/*--------------------------------------*/

void arcdefaults(arcptr newarc, int x, int y)
{
   newarc->style = areawin->style;
   newarc->color = areawin->color;
   newarc->position.x = x;
   newarc->position.y = y;
   newarc->width = areawin->linewidth;
   newarc->radius = 0;
   newarc->yaxis = 0;
   newarc->angle1 = 0;
   newarc->angle2 = 360;
   newarc->passed = NULL;
   newarc->cycle = NULL;
   calcarc(newarc);
}

/*-------------------------------------*/
/* Button handler when creating an arc */
/*-------------------------------------*/

void arcbutton(int x, int y)
{
   arcptr *newarc;
   XPoint userpt;
   short *newselect;

   unselect_all();
   NEW_ARC(newarc, topobject);
   newselect = allocselect();
   *newselect = topobject->parts - 1;
   snap(x, y, &userpt);
   saveratio = 1.0;
   arcdefaults(*newarc, userpt.x, userpt.y);
   addcycle((genericptr *)newarc, 0, 0);

   arc_mode_draw(xcDRAW_EDIT, *newarc);

   xcAddEventHandler(areawin->area, PointerMotionMask, False,
        (xcEventHandler)trackarc, NULL);

   eventmode = ARC_MODE;
}

/*----------------------------------*/
/* Track an arc during mouse motion */
/*----------------------------------*/

void trackarc(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   XPoint newpos;
   arcptr newarc;
   double adjrat;
   short  cycle;
   UNUSED(w); UNUSED(clientdata); UNUSED(calldata);

   newarc = TOARC(EDITPART);

   newpos = UGetCursorPos();
   u2u_snap(&newpos);
   if (areawin->save.x == newpos.x && areawin->save.y == newpos.y) return;

   cycle = (newarc->cycle == NULL) ? -1 : newarc->cycle->number;
   if (cycle == 1 || cycle == 2) {
      float *angleptr, tmpang;

      adjrat = (newarc->yaxis == 0) ? 1 :
		(double)(abs(newarc->radius)) / (double)newarc->yaxis;
      angleptr = (cycle == 1) ? &newarc->angle1 : &newarc->angle2;
      tmpang = (float)(atan2((double)(newpos.y - newarc->position.y) * adjrat,
	   (double)(newpos.x - newarc->position.x)) / RADFAC);
      if (cycle == 1) {
	 if (tmpang > newarc->angle2) tmpang -= 360;
	 else if (newarc->angle2 - tmpang > 360) newarc->angle2 -= 360;
      }
      else {
         if (tmpang < newarc->angle1) tmpang += 360;
	 else if (tmpang - newarc->angle1 > 360) newarc->angle1 += 360;
      }
      *angleptr = tmpang;

      if (newarc->angle2 <= 0) {
	 newarc->angle2 += 360;
	 newarc->angle1 += 360;
      }
      if (newarc->angle2 <= newarc->angle1)
	 newarc->angle1 -= 360;
   }
   else if (cycle == 0) {
      short direc = (newarc->radius < 0);
      newarc->radius = wirelength(&newpos, &(newarc->position));
      newarc->yaxis = (short)((double)newarc->radius * saveratio);
      if (direc) newarc->radius = -newarc->radius;
   }
   else {
      newarc->yaxis = wirelength(&newpos, &(newarc->position));
      saveratio = (double)newarc->yaxis / (double)newarc->radius;
   }

   calcarc(newarc);

   areawin->save.x = newpos.x;
   areawin->save.y = newpos.y;

   arc_mode_draw(xcDRAW_EDIT, newarc);
   printpos(newpos.x, newpos.y);

   flusharea();
}

/*--------------------------------------*/
/* Sane default values for a polygon	*/
/*--------------------------------------*/

void polydefaults(polyptr newpoly, int number, int x, int y)
{
   pointlist pointptr;

   newpoly->style = areawin->style & ~UNCLOSED;
   newpoly->color = areawin->color;
   newpoly->width = areawin->linewidth;
   newpoly->number = number;
   newpoly->passed = NULL;
   newpoly->cycle = NULL;
   if (number == 0)
      newpoly->points = NULL;
   else {
      newpoly->points = (pointlist) malloc(number * sizeof(XPoint));

      for (pointptr = newpoly->points; pointptr < newpoly->points + number;
		pointptr++) {
         pointptr->x = x;
         pointptr->y = y;
      }
   }
}

/*------------------------------------*/
/* Button handler when creating a box */
/*------------------------------------*/

void boxbutton(int x, int y)
{
   polyptr *newbox;
   XPoint userpt;
   short *newselect;

   unselect_all();
   NEW_POLY(newbox, topobject);
   newselect = allocselect();
   *newselect = topobject->parts - 1;
   snap(x, y, &userpt);
   polydefaults(*newbox, 4, userpt.x, userpt.y);

   poly_mode_draw(xcDRAW_EDIT, *newbox);

   xcAddEventHandler(areawin->area, PointerMotionMask, False,
        (xcEventHandler)trackbox, NULL);

   eventmode = BOX_MODE;
}

/*---------------------------------*/
/* Track a box during mouse motion */
/*---------------------------------*/

void trackbox(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   XPoint 	newpos;
   polyptr      newbox;
   pointlist	pointptr;
   UNUSED(w); UNUSED(clientdata); UNUSED(calldata);

   newbox = TOPOLY(EDITPART);
   newpos = UGetCursorPos();
   u2u_snap(&newpos);

   if (areawin->save.x == newpos.x && areawin->save.y == newpos.y) return;

   pointptr = newbox->points + 1; pointptr->y = newpos.y;
   pointptr++; pointptr->y = newpos.y; pointptr->x = newpos.x;
   pointptr++; pointptr->x = newpos.x;
   
   poly_mode_draw(xcDRAW_EDIT, newbox);
   printpos(newpos.x, newpos.y);

   areawin->save.x = newpos.x;
   areawin->save.y = newpos.y;

   flusharea();
}

/*----------------------------------------------------------------------*/
/* Track a wire during mouse motion					*/
/* Note:  The manhattanize algorithm will change the effective cursor	*/
/* position to keep the wire manhattan if the wire is only 1 segment.	*/
/* It will change the previous point's position if the wire has more	*/
/* than one segment.  They are called at different times to ensure the	*/
/* wire redraw is correct.						*/
/*----------------------------------------------------------------------*/

void trackwire(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   XPoint newpos, upos, *tpoint;
   polyptr	newwire;
   UNUSED(w); UNUSED(clientdata); UNUSED(calldata);

   newwire = TOPOLY(EDITPART);

   if (areawin->attachto >= 0) {
      upos = UGetCursorPos();
      findattach(&newpos, NULL, &upos); 
   }
   else {
      newpos = UGetCursorPos();
      u2u_snap(&newpos);
      if (areawin->manhatn && (newwire->number == 2))
	 manhattanize(&newpos, newwire, -1, TRUE);
   }

   if (areawin->save.x != newpos.x || areawin->save.y != newpos.y) {
      tpoint = newwire->points + newwire->number - 1;
      if (areawin->manhatn && (newwire->number > 2))
	 manhattanize(&newpos, newwire, -1, TRUE);
      tpoint->x = newpos.x;
      tpoint->y = newpos.y;
      XcTopSetForeground(newwire->color);
      poly_mode_draw(xcDRAW_EDIT, newwire);
      areawin->save.x = newpos.x;
      areawin->save.y = newpos.y;
      printpos(newpos.x, newpos.y);
   }

   flusharea();
}

/*--------------------------*/
/* Start drawing a polygon. */
/*--------------------------*/

void startwire(XPoint *userpt)
{
   polyptr *newwire;
   pointlist pointptr;
   short *newselect;

   unselect_all();
   NEW_POLY(newwire, topobject);
   newselect = allocselect();
   *newselect = topobject->parts - 1;

   /* always start unfilled, unclosed; can fix on next button-push. */

   (*newwire)->style = UNCLOSED | (areawin->style & (DASHED | DOTTED));
   (*newwire)->color = areawin->color;
   (*newwire)->number = 2;
   (*newwire)->width = areawin->linewidth;
   (*newwire)->points = (pointlist) malloc(2 * sizeof(XPoint));
   (*newwire)->passed = NULL;
   (*newwire)->cycle = NULL;
   pointptr = (*newwire)->points;
   pointptr->x = (pointptr + 1)->x = areawin->save.x = userpt->x;
   pointptr->y = (pointptr + 1)->y = areawin->save.y = userpt->y;

   poly_mode_draw(xcDRAW_EDIT, *newwire);

   xcAddEventHandler(areawin->area, PointerMotionMask, False,
          (xcEventHandler)trackwire, NULL);
}

/*--------------------------------------------------------------*/
/* Find which points should track along with the edit point in	*/
/* in polygon RHOMBOID or MANHATTAN edit modes.		    	*/
/* (point number is stored in lastpoly->cycle)	    	    	*/
/*								*/
/* NOTE:  This routine assumes that either the points have just	*/
/* been selected, or that advancecycle() has been called to	*/
/* remove all previously recorded tracking points.		*/
/*--------------------------------------------------------------*/

void findconstrained(polyptr lastpoly)
{
   XPoint *savept, *npt, *lpt;
   short cycle;
   short lflags, nflags;
   short lcyc, ncyc;
   pointselect *cptr, *nptr;

   if (areawin->boxedit == NORMAL) return;

   if (lastpoly->cycle == NULL) return;

   /* Set "process" flags on all original points */
   for (cptr = lastpoly->cycle;; cptr++) {
      cptr->flags |= PROCESS;
      if (cptr->flags & LASTENTRY) break;
   }

   cptr = lastpoly->cycle;
   while (1) {
      if (cptr->flags & PROCESS) {
	 cptr->flags &= ~PROCESS;
         cycle = cptr->number;
         savept = lastpoly->points + cycle;

         /* find points before and after the edit point */      

         lcyc = (cycle == 0) ? ((lastpoly->style & UNCLOSED) ?
		-1 : lastpoly->number - 1) : cycle - 1;
         ncyc = (cycle == lastpoly->number - 1) ?
		((lastpoly->style & UNCLOSED) ? -1 : 0) : cycle + 1;
    
         lpt = (lcyc == -1) ? NULL : lastpoly->points + lcyc;
         npt = (ncyc == -1) ? NULL : lastpoly->points + ncyc;

	 lflags = nflags = NONE;

         /* two-point polygons (lines) are a degenerate case in RHOMBOID edit mode */

         if (areawin->boxedit != MANHATTAN && lastpoly->number <= 2) return;

         /* This is complicated but logical:  in MANHATTAN mode, boxes maintain */
         /* box shape.  In RHOMBOID modes, parallelagrams maintain shape.  The  */
         /* "savedir" variable determines which coordinate(s) of which point(s) */
         /* should track along with the edit point.			     */

         if (areawin->boxedit != RHOMBOIDY) {
            if (lpt != NULL) {
               if (lpt->y == savept->y) {
	          lflags |= EDITY;
	          if (areawin->boxedit == RHOMBOIDX && lpt->x != savept->x)
		     lflags |= EDITX;
	          else if (areawin->boxedit == RHOMBOIDA && npt != NULL) {
	             if (npt->y != savept->y) nflags |= EDITX;
		  }
	       }
	    }
            if (npt != NULL) {
               if (npt->y == savept->y) {
	          nflags |= EDITY;
	          if (areawin->boxedit == RHOMBOIDX && npt->x != savept->x)
		     nflags |= EDITX;
	          else if (areawin->boxedit == RHOMBOIDA && lpt != NULL) {
	             if (lpt->y != savept->y) lflags |= EDITX;
	          }
	       }
	    }
         }
         if (areawin->boxedit != RHOMBOIDX) {
            if (lpt != NULL) {
               if (lpt->x == savept->x) {
	          lflags |= EDITX;
	          if (areawin->boxedit == RHOMBOIDY && lpt->y != savept->y)
		     lflags |= EDITY;
	          else if (areawin->boxedit == RHOMBOIDA && npt != NULL) {
	             if (npt->x != savept->x) nflags |= EDITY;
	          }
	       } 
	    }
            if (npt != NULL) {
               if (npt->x == savept->x) {
	          nflags |= EDITX;
	          if (areawin->boxedit == RHOMBOIDY && npt->y != savept->y)
		     nflags |= EDITY;
	          else if (areawin->boxedit == RHOMBOIDA && lpt != NULL) {
	             if (lpt->x != savept->x) lflags |= EDITY;
	          }
	       }
	    }
         }
	 nptr = cptr + 1;
         if (lpt != NULL && lflags != 0) {
	    addcycle((genericptr *)(&lastpoly), lcyc, lflags);
	    cptr = nptr = lastpoly->cycle;
	 }
         if (npt != NULL && nflags != 0) {
	    addcycle((genericptr *)(&lastpoly), ncyc, nflags);
	    cptr = nptr = lastpoly->cycle;
	 }
      }
      else
	 nptr = cptr + 1;
      if (cptr->flags & LASTENTRY) break;
      cptr = nptr;
   }
}

/*------------------------------------------------------*/
/* Track movement of arc, spline, or polygon segments	*/
/* during edit mode					*/
/*------------------------------------------------------*/

void trackelement(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   XPoint newpos, *curpt;
   short	*selobj;
   pointselect	*cptr;
   int		deltax, deltay;
   UNUSED(w); UNUSED(clientdata); UNUSED(calldata);

   newpos = UGetCursorPos();
   u2u_snap(&newpos);

   /* force attachment if required */
   if (areawin->attachto >= 0) {
      XPoint apos;
      findattach(&apos, NULL, &newpos); 
      newpos = apos;
   }

   if (areawin->save.x == newpos.x && areawin->save.y == newpos.y) return;

   /* Find the reference point */

   cptr = getrefpoint(TOGENERIC(EDITPART), &curpt);
   switch(ELEMENTTYPE(TOGENERIC(EDITPART))) {
      case POLYGON:
         if (cptr == NULL)
	    curpt = TOPOLY(EDITPART)->points;
	 break;
      case SPLINE:
         if (cptr == NULL)
	    curpt = &(TOSPLINE(EDITPART)->ctrl[0]);
	 break;
      case ARC:
	 curpt = &(TOARC(EDITPART)->position);
	 break;
      case OBJINST:
	 curpt = &(TOOBJINST(EDITPART)->position);
	 break;
      case GRAPHIC:
	 curpt = &(TOGRAPHIC(EDITPART)->position);
	 break;
   }
   deltax = newpos.x - curpt->x;
   deltay = newpos.y - curpt->y;

   /* Now adjust all edited elements relative to the reference point */
   for (selobj = areawin->selectlist; selobj < areawin->selectlist +
	 areawin->selects; selobj++) {
      if (eventmode == ARC_MODE || eventmode == EARC_MODE)
         editpoints(SELTOGENERICPTR(selobj), deltax, deltay);
      else if (eventmode == SPLINE_MODE || eventmode == ESPLINE_MODE)
         editpoints(SELTOGENERICPTR(selobj), deltax, deltay);
      else if (eventmode == BOX_MODE || eventmode == EPOLY_MODE
   	    || eventmode == WIRE_MODE)
         editpoints(SELTOGENERICPTR(selobj), deltax, deltay);
      else if (eventmode == EPATH_MODE)
         editpoints(SELTOGENERICPTR(selobj), deltax, deltay);
   } 

   if (eventmode == ARC_MODE || eventmode == EARC_MODE)
      arc_mode_draw(xcDRAW_EDIT, TOARC(EDITPART));
   else if (eventmode == SPLINE_MODE || eventmode == ESPLINE_MODE)
      spline_mode_draw(xcDRAW_EDIT, TOSPLINE(EDITPART));
   else if (eventmode == BOX_MODE || eventmode == EPOLY_MODE
	 || eventmode == WIRE_MODE)
      poly_mode_draw(xcDRAW_EDIT, TOPOLY(EDITPART));
   else if (eventmode == EPATH_MODE)
      path_mode_draw(xcDRAW_EDIT, TOPATH(EDITPART));

   printpos(newpos.x, newpos.y);
   areawin->save.x = newpos.x;
   areawin->save.y = newpos.y;

   flusharea();
}

/*-------------------------------------------------*/
/* Determine values of endpoints of an element	   */
/*-------------------------------------------------*/

void setendpoint(short *scnt, short direc, XPoint **endpoint, XPoint *arcpoint)
{
   genericptr *sptr = topobject->plist + (*scnt);

   switch(ELEMENTTYPE(*sptr)) {
      case POLYGON:
	 if (direc)
	    *endpoint = TOPOLY(sptr)->points + TOPOLY(sptr)->number - 1;
	 else
	    *endpoint = TOPOLY(sptr)->points;
	 break;
      case SPLINE:
	 if (direc)
	    *endpoint = &(TOSPLINE(sptr)->ctrl[3]);
	 else
	    *endpoint = &(TOSPLINE(sptr)->ctrl[0]);
	 break;
      case ARC:
	 if (direc) {
	    arcpoint->x = (short)(TOARC(sptr)->points[TOARC(sptr)->number - 1].x
		+ 0.5);
	    arcpoint->y = (short)(TOARC(sptr)->points[TOARC(sptr)->number - 1].y
		+ 0.5);
	 }
	 else {
	    arcpoint->x = (short)(TOARC(sptr)->points[0].x + 0.5);
	    arcpoint->y = (short)(TOARC(sptr)->points[0].y + 0.5);
	 }
	 *endpoint = arcpoint;
	 break;
   }
}

/*------------------------------------------------------------*/
/* Reverse points in a point list			      */
/*------------------------------------------------------------*/

void reversepoints(XPoint *plist, short number)
{
   XPoint hold, *ppt;
   XPoint *pend = plist + number - 1;
   short hnum = number >> 1;

   for (ppt = plist; ppt < plist + hnum; ppt++, pend--) {
      hold.x = ppt->x;
      hold.y = ppt->y;
      ppt->x = pend->x;
      ppt->y = pend->y;
      pend->x = hold.x;
      pend->y = hold.y;
   }
}

/*------------------------------------------------------------*/
/* Same as above for floating-point positions		      */
/*------------------------------------------------------------*/

void reversefpoints(XfPoint *plist, short number)
{
   XfPoint hold, *ppt;
   XfPoint *pend = plist + number - 1;
   short hnum = number >> 1;

   for (ppt = plist; ppt < plist + hnum; ppt++, pend--) {
      hold.x = ppt->x;
      hold.y = ppt->y;
      ppt->x = pend->x;
      ppt->y = pend->y;
      pend->x = hold.x;
      pend->y = hold.y; 
   }
}

/*--------------------------------------------------------------*/
/* Permanently remove an element from the topobject plist	*/
/*	add = 1 if plist has (parts + 1) elements		*/
/*--------------------------------------------------------------*/

void freepathparts(short *selectobj, short add)
{
   genericptr *oldelem = topobject->plist + (*selectobj);
   switch(ELEMENTTYPE(*oldelem)) {
      case POLYGON:
	 free((TOPOLY(oldelem))->points);
	 break;
   }
   free(*oldelem);
   removep(selectobj, add);
}

/*--------------------------------------------------------------*/
/* Remove a part from an object					*/
/* 	add = 1 if plist has (parts + 1) elements		*/
/*--------------------------------------------------------------*/

void removep(short *selectobj, short add)
{
   genericptr *oldelem = topobject->plist + (*selectobj);

   for (++oldelem; oldelem < topobject->plist + topobject->parts + add; oldelem++)
	    *(oldelem - 1) = *oldelem;

   topobject->parts--;
}

/*-------------------------------------------------*/
/* Break a path into its constituent components	   */
/*-------------------------------------------------*/

void unjoin()
{
   short *selectobj;
   genericptr *genp, *newg;
   pathptr oldpath;
   polyptr oldpoly, *newpoly;
   Boolean preselected;
   short i, cycle;

   if (areawin->selects == 0) {
      select_element(PATH | POLYGON);
      preselected = FALSE;
   }
   else preselected = TRUE;

   if (areawin->selects == 0) {
      Wprintf("No objects selected.");
      return;
   }

   /* for each selected path or polygon */

   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
        + areawin->selects; selectobj++) {
      SetForeground(dpy, areawin->gc, BACKGROUND);
      if (SELECTTYPE(selectobj) == PATH) {
         oldpath = SELTOPATH(selectobj);

         /* undraw the path */
      
         UDrawPath(oldpath, xobjs.pagelist[areawin->page]->wirewidth);
      
         /* move components to the top level */

	 topobject->plist = (genericptr *)realloc(topobject->plist,
		(topobject->parts + oldpath->parts) * sizeof(genericptr));
	 newg = topobject->plist + topobject->parts;
	 for (genp = oldpath->plist; genp < oldpath->plist +
		oldpath->parts; genp++, newg++) {
	    *newg = *genp;
	 }
	 topobject->parts += oldpath->parts;

         /* remove the path object and revise the selectlist */

         freepathparts(selectobj, 0);
         reviseselect(areawin->selectlist, areawin->selects, selectobj);
      }
      else if (SELECTTYPE(selectobj) == POLYGON) {
	 /* Method to break a polygon, in lieu of the edit-mode	*/
	 /* polygon break that was removed.			*/
         oldpoly = SELTOPOLY(selectobj);
         UDrawPolygon(oldpoly, xobjs.pagelist[areawin->page]->wirewidth);

	 /* Get the point nearest the cursor, and break at that point */
         cycle = closepoint(oldpoly, &areawin->save);
	 if (cycle > 0 && cycle < (oldpoly->number - 1)) {
            NEW_POLY(newpoly, topobject);
	    polycopy(*newpoly, oldpoly);
	    for (i = cycle; i < oldpoly->number; i++)
	       (*newpoly)->points[i - cycle] = (*newpoly)->points[i];
	    oldpoly->number = cycle + 1;
	    (*newpoly)->number = (*newpoly)->number - cycle;
	 }
      }
   }
   if (!preselected) clearselects();
   drawarea(NULL, NULL, NULL);
}

/*-------------------------------------------------*/
/* Test if two points are near each other	   */
/*-------------------------------------------------*/

Boolean neartest(XPoint *point1, XPoint *point2)
{
   short diff[2];

   diff[0] = point1->x - point2->x;
   diff[1] = point1->y - point2->y;
   diff[0] = abs(diff[0]);
   diff[1] = abs(diff[1]);

   if (diff[0] <= 2 && diff[1] <= 2) return True;
   else return False;
}


/*-------------------------------------------------*/
/* Join stuff together 			   	   */
/*-------------------------------------------------*/

void join()
{
   short     *selectobj;
   polyptr   *newpoly, nextwire;
   pathptr   *newpath;
   genericptr *pgen;
   short     *scount, *sptr, *sptr2, *direc, *order;
   short     ordered, startpt = 0;
   short     numpolys, numlabels, numpoints, polytype;
   int	     polycolor;
   float     polywidth;
   XPoint    *testpoint, *testpoint2, *begpoint, *endpoint, arcpoint[4];
   XPoint    *begpoint2, *endpoint2;
   Boolean   allpolys = True;
   objectptr delobj;

   numpolys = numlabels = 0;
   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
	+ areawin->selects; selectobj++) {
      if (SELECTTYPE(selectobj) == POLYGON) {
	 /* arbitrary:  keep style of last polygon in selectlist */
	 polytype = SELTOPOLY(selectobj)->style;
	 polywidth = SELTOPOLY(selectobj)->width;
	 polycolor = SELTOPOLY(selectobj)->color;
	 numpolys++;
      }
      else if (SELECTTYPE(selectobj) == SPLINE) {
	 polytype = SELTOSPLINE(selectobj)->style;
	 polywidth = SELTOSPLINE(selectobj)->width;
	 polycolor = SELTOSPLINE(selectobj)->color;
	 numpolys++;
	 allpolys = False;
      }
      else if (SELECTTYPE(selectobj) == ARC) {
	 polytype = SELTOARC(selectobj)->style;
	 polywidth = SELTOARC(selectobj)->width;
	 polycolor = SELTOARC(selectobj)->color;
	 numpolys++;
	 allpolys = False;
      }
      else if (SELECTTYPE(selectobj) == LABEL)
	 numlabels++;
   }
   if ((numpolys == 0) && (numlabels == 0)) {
      Wprintf("No elements selected for joining.");
      return;
   }
   else if ((numpolys == 1) || (numlabels == 1)) {
      Wprintf("Only one element: nothing to join to.");
      return;
   }
   else if ((numpolys > 1) && (numlabels > 1)) {
      Wprintf("Selection mixes labels and line segments.  Ignoring.");
      return;
   }
   else if (numlabels > 0) {
      joinlabels();
      return;
   }

   /* scount is a table of element numbers 				*/
   /* order is an ordered table of end-to-end elements 			*/
   /* direc is an ordered table of path directions (0=same as element,	*/
   /* 	1=reverse from element definition)				*/

   scount = (short *) malloc(numpolys * sizeof(short));
   order  = (short *) malloc(numpolys * sizeof(short));
   direc  = (short *) malloc(numpolys * sizeof(short));
   sptr = scount;
   numpoints = 1;

   /* make a record of the element instances involved */

   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
	+ areawin->selects; selectobj++) {
      if (SELECTTYPE(selectobj) == POLYGON) {
	  numpoints += SELTOPOLY(selectobj)->number - 1;
	  *(sptr++) = *selectobj;
      }
      else if (SELECTTYPE(selectobj) == SPLINE || SELECTTYPE(selectobj) == ARC)
	  *(sptr++) = *selectobj;
   }

   /* Sort the elements by sorting the scount array: 				*/
   /* Loop through each point as starting point in case of strangely connected 	*/
   /* structures. . . for normal structures it should break out on the first   	*/
   /* loop (startpt = 0).							*/

   for (startpt = 0; startpt < numpolys; startpt++) {

      /* set first in ordered list */

      direc[0] = 0;
      order[0] = *(scount + startpt);

      for (ordered = 0; ordered < numpolys - 1; ordered++) {

         setendpoint(order + ordered, (1 ^ direc[ordered]), &endpoint2, &arcpoint[0]);
         setendpoint(order, (0 ^ direc[0]), &begpoint2, &arcpoint[1]);

         for (sptr = scount; sptr < scount + numpolys; sptr++) {

	    /* don't compare with things already in the list */
	    for (sptr2 = order; sptr2 <= order + ordered; sptr2++)
	       if (*sptr == *sptr2) break;
	    if (sptr2 != order + ordered + 1) continue;

            setendpoint(sptr, 0, &begpoint, &arcpoint[2]);
            setendpoint(sptr, 1, &endpoint, &arcpoint[3]);

	    /* four cases of matching endpoint of one element to another */

	    if (neartest(begpoint, endpoint2)) {
	       order[ordered + 1] = *sptr;
	       direc[ordered + 1] = 0;
	       break;
	    }
	    else if (neartest(endpoint, endpoint2)) {
	       order[ordered + 1] = *sptr;
	       direc[ordered + 1] = 1;
	       break;
	    }
	    else if (neartest(begpoint, begpoint2)) {
	       for (sptr2 = order + ordered + 1; sptr2 > order; sptr2--)
	          *sptr2 = *(sptr2 - 1);
	       for (sptr2 = direc + ordered + 1; sptr2 > direc; sptr2--)
	          *sptr2 = *(sptr2 - 1);
	       order[0] = *sptr;
	       direc[0] = 1;
	       break;
	    }
	    else if (neartest(endpoint, begpoint2)) {
	       for (sptr2 = order + ordered + 1; sptr2 > order; sptr2--) 
	          *sptr2 = *(sptr2 - 1);
	       for (sptr2 = direc + ordered + 1; sptr2 > direc; sptr2--)
	          *sptr2 = *(sptr2 - 1);
	       order[0] = *sptr;
	       direc[0] = 0;
	       break;
	    }
         }
	 if (sptr == scount + numpolys) break;
      }
      if (ordered == numpolys - 1) break;
   }

   if (startpt == numpolys) {
      Wprintf("Cannot join: Too many free endpoints");
      free(order);
      free(direc);
      free(scount);
      return;
   }

   /* create the new polygon or path */

   if (allpolys) {
      NEW_POLY(newpoly, topobject);

      (*newpoly)->number = numpoints;
      (*newpoly)->points = (pointlist) malloc(numpoints * sizeof(XPoint));
      (*newpoly)->width  = polywidth;
      (*newpoly)->style  = polytype;
      (*newpoly)->color  = polycolor;
      (*newpoly)->passed = NULL;
      (*newpoly)->cycle = NULL;

      /* insert the points into the new polygon */

      testpoint2 = (*newpoly)->points;
      for (sptr = order; sptr < order + numpolys; sptr++) {
         nextwire = SELTOPOLY(sptr);
	 if (*(direc + (short)(sptr - order)) == 0) {
            for (testpoint = nextwire->points; testpoint < nextwire->points + 
		   nextwire->number - 1; testpoint++) {
	       testpoint2->x = testpoint->x;
	       testpoint2->y = testpoint->y; 
	       testpoint2++;
	    }
         }
         else {
            for (testpoint = nextwire->points + nextwire->number - 1; testpoint
		   > nextwire->points; testpoint--) {
	       testpoint2->x = testpoint->x;
	       testpoint2->y = testpoint->y; 
	       testpoint2++;
	    }
	 }
      }
      /* pick up the last point */
      testpoint2->x = testpoint->x;
      testpoint2->y = testpoint->y;

      /* delete the old elements from the list */

      register_for_undo(XCF_Wire, UNDO_MORE, areawin->topinstance, *newpoly);

      delobj = delete_element(areawin->topinstance, areawin->selectlist,
		areawin->selects, NORMAL);
      register_for_undo(XCF_Delete, UNDO_DONE, areawin->topinstance,
                delobj, NORMAL);

   }
   else {	/* create a path */

      NEW_PATH(newpath, topobject);
      (*newpath)->style = polytype;
      (*newpath)->color = polycolor;
      (*newpath)->width = polywidth;
      (*newpath)->parts = 0;
      (*newpath)->plist = (genericptr *) malloc(sizeof(genericptr));
      (*newpath)->passed = NULL;

      /* copy the elements from the top level into the path structure */

      for (sptr = order; sptr < order + numpolys; sptr++) {
	 genericptr *oldelem = topobject->plist + *sptr;
	 genericptr *newelem;

	 switch (ELEMENTTYPE(*oldelem)) {
	    case POLYGON: {
	       polyptr copypoly = TOPOLY(oldelem);
	       polyptr *newpoly;
	       NEW_POLY(newpoly, (*newpath));
	       polycopy(*newpoly, copypoly);
	    } break;
	    case ARC: {
	       arcptr copyarc = TOARC(oldelem);
	       arcptr *newarc;
	       NEW_ARC(newarc, (*newpath));
	       arccopy(*newarc, copyarc);
	    } break;
	    case SPLINE: {
	       splineptr copyspline = TOSPLINE(oldelem);
	       splineptr *newspline;
	       NEW_SPLINE(newspline, (*newpath));
	       splinecopy(*newspline, copyspline);
	    } break;
	 }
	 newelem = (*newpath)->plist + (*newpath)->parts - 1;

	 /* reverse point order if necessary */

         if (*(direc + (short)(sptr - order)) == 1) {
	    switch (ELEMENTTYPE(*newelem)) {
	       case POLYGON:
		  reversepoints(TOPOLY(newelem)->points, TOPOLY(newelem)->number);
	          break;
	       case ARC:
		  TOARC(newelem)->radius = -TOARC(newelem)->radius;
	          break;
	       case SPLINE:
		  reversepoints(TOSPLINE(newelem)->ctrl, 4);
		  calcspline(TOSPLINE(newelem));
	          break;
	    }
	 }

	 /* decompose arcs into bezier curves */
	 if (ELEMENTTYPE(*newelem) == ARC)
	    decomposearc(*newpath);
      }

      /* delete the old elements from the list */

      register_for_undo(XCF_Join, UNDO_MORE, areawin->topinstance, *newpath);

      delobj = delete_element(areawin->topinstance, scount, numpolys, NORMAL);

      register_for_undo(XCF_Delete, UNDO_DONE, areawin->topinstance,
                delobj, NORMAL);

      /* Remove the path parts from the selection list and add the path */
      clearselects();
      selectobj = allocselect();
      for (pgen = topobject->plist; pgen < topobject->plist + topobject->parts;
		pgen++) {
	 if ((TOPATH(pgen)) == (*newpath)) {
	    *selectobj = (short)(pgen - topobject->plist);
	    break;
	 }
      }
   }

   /* clean up */

   incr_changes(topobject);
   /* Do not clear the selection, to be consistent with all the	*/
   /* other actions that clear only when something has not been	*/
   /* preselected before the action.  Elements must be selected	*/
   /* prior to the "join" action, by necessity.			*/
   free(scount);
   free(order);
   free(direc);
}

/*----------------------------------------------*/
/* Add a new point to a polygon			*/
/*----------------------------------------------*/

void poly_add_point(polyptr thispoly, XPoint *newpoint) {
   XPoint *tpoint;

   thispoly->number++;
   thispoly->points = (XPoint *)realloc(thispoly->points,
			thispoly->number * sizeof(XPoint));
   tpoint = thispoly->points + thispoly->number - 1;
   tpoint->x = newpoint->x;
   tpoint->y = newpoint->y;
}

/*-------------------------------------------------*/
/* ButtonPress handler while a wire is being drawn */
/*-------------------------------------------------*/

void wire_op(int op, int x, int y)
{
   XPoint userpt, *tpoint;
   polyptr newwire;

   snap(x, y, &userpt);

   newwire = TOPOLY(EDITPART);

   if (areawin->attachto >= 0) {
      XPoint apos;
      findattach(&apos, NULL, &userpt);
      userpt = apos;
      areawin->attachto = -1;
   }
   else {
      if (areawin->manhatn) manhattanize(&userpt, newwire, -1, TRUE);
   }
 
   tpoint = newwire->points + newwire->number - 1;
   tpoint->x = userpt.x;
   tpoint->y = userpt.y;

   /* cancel wire operation completely */
   if (op == XCF_Cancel) {
      free(newwire->points);
      free(newwire);
      newwire = NULL;
      eventmode = NORMAL_MODE;
      topobject->parts--;
   }

   /* back up one point; prevent length zero wires */
   else if ((op == XCF_Cancel_Last) || ((tpoint - 1)->x == userpt.x &&
	   (tpoint - 1)->y == userpt.y)) {
      if (newwire->number <= 2) {
	 free(newwire->points);
	 free(newwire);
	 newwire = NULL;
         eventmode = NORMAL_MODE;
         topobject->parts--;
      }
      else {
         if (--newwire->number == 2) newwire->style = UNCLOSED |
   		(areawin->style & (DASHED | DOTTED));
      }
   }

   if (newwire && (op == XCF_Wire || op == XCF_Continue_Element)) {
      if (newwire->number == 2)
	 newwire->style = areawin->style;
      poly_add_point(newwire, &userpt);
   }
   else if ((newwire == NULL) || op == XCF_Finish_Element || op == XCF_Cancel) {
      xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
         (xcEventHandler)trackwire, NULL);
   }

   if (newwire) {
      if (op == XCF_Finish_Element) {

	 /* If the last points are the same, remove all redundant ones.	*/
	 /* This avoids the problem of extra points when people do left	*/
	 /* click followed by middle click to finish (the redundant way	*/
	 /* a lot of drawing programs work).				*/

	 XPoint *t2pt;
	 while (newwire->number > 2) {
	    tpoint = newwire->points + newwire->number - 1;
	    t2pt = newwire->points + newwire->number - 2;
	    if (tpoint->x != t2pt->x || tpoint->y != t2pt->y)
	       break;
	    newwire->number--;
	 }

	 incr_changes(topobject);
	 if (!nonnetwork(newwire)) invalidate_netlist(topobject);
	 register_for_undo(XCF_Wire, UNDO_MORE, areawin->topinstance, newwire);
   	 poly_mode_draw(xcDRAW_FINAL, newwire);
      }
      else
   	 poly_mode_draw(xcDRAW_EDIT, newwire);
      if (op == XCF_Cancel_Last)
	 checkwarp(newwire->points + newwire->number - 1);
   }

   if (op == XCF_Finish_Element) {
      eventmode = NORMAL_MODE;
      singlebbox(EDITPART);
   }
}

/*-------------------------------------------------------------------------*/
/* Helper functions for the xxx_mode_draw functions			   */
/* Functions to be used around the drawing of the edited element	   */
/* begin_event_mode_drawing starts double buffering and copies the	   */
/* fixed pixmap. end_event_mode stops the double buffering and displays	   */
/* everything on screen.						   */
/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
static Window old_win;
#endif /* !HAVE_CAIRO */

static void begin_event_mode_drawing(void)
{
   /* Start double buffering */
#ifdef HAVE_CAIRO
   cairo_identity_matrix(areawin->cr);
   cairo_translate(areawin->cr, areawin->panx, areawin->pany);
   cairo_push_group(areawin->cr);
#else /* HAVE_CAIRO */
   old_win = areawin->window;
   areawin->window = (Window) dbuf;
#endif /* HAVE_CAIRO */

   /* Copy background pixmap with the element(s) not currently being edited */
#ifdef HAVE_CAIRO
   if (areawin->panx || areawin->pany) {
      SetForeground(dpy, areawin->gc, BACKGROUND);
      cairo_paint(areawin->cr);
   }
   cairo_set_source(areawin->cr, areawin->fixed_pixmap);
   cairo_paint(areawin->cr);
#else /* HAVE_CAIRO */
   if (areawin->panx || areawin->pany) {
      int x = max(0, -areawin->panx);
      int y = max(0, -areawin->pany);
      unsigned int w = areawin->width - x;
      unsigned int h = areawin->height - y;
      SetForeground(dpy, areawin->gc, BACKGROUND);
      XSetFillStyle(dpy, areawin->gc, FillSolid);
      XFillRectangle(dpy, areawin->window, areawin->gc, 0, 0, areawin->width,
	    areawin->height);
      XCopyArea(dpy, areawin->fixed_pixmap, areawin->window, areawin->gc,
	 x, y, w, h, max(0, areawin->panx), max(0, areawin->pany));
   }
   else
      XCopyArea(dpy, areawin->fixed_pixmap, areawin->window, areawin->gc, 0, 0,
	    areawin->width, areawin->height, 0, 0);
#endif /* HAVE_CAIRO */

   areawin->redraw_ongoing = True;
   newmatrix();
}

static void end_event_mode_drawing(void)
{
   /* End double buffering */
#ifdef HAVE_CAIRO
   cairo_pop_group_to_source(areawin->cr);
   cairo_paint(areawin->cr);
#else /* HAVE_CAIRO */
   areawin->window = old_win;
   XCopyArea(dpy, dbuf, areawin->window, areawin->gc, 0, 0, areawin->width,
	 areawin->height, 0, 0);
#endif /* HAVE_CAIRO */

   areawin->redraw_ongoing = False;
}

/*-------------------------------------------------------------------------*/
/* Helper functions for the xxx_mode_draw functions			   */
/* Functions to be used when finishing the element. The final state is	   */
/* drawn into the fixed pixmap, which is show when the			   */
/* end_event_mode_drawing_final is called				   */
/* (Sorry about the name :-) )						   */
/*-------------------------------------------------------------------------*/

static void begin_event_mode_drawing_final(void)
{
#ifdef HAVE_CAIRO
   cairo_identity_matrix(areawin->cr);
   cairo_push_group(areawin->cr);
   cairo_set_source(areawin->cr, areawin->fixed_pixmap);
   cairo_paint(areawin->cr);
#else /* HAVE_CAIRO */
   old_win = areawin->window;
   areawin->window = (Window) areawin->fixed_pixmap;
#endif /* HAVE_CAIRO */

   areawin->redraw_ongoing = True;
   newmatrix();
}

static void end_event_mode_drawing_final(void)
{
#ifdef HAVE_CAIRO
   cairo_pattern_destroy(areawin->fixed_pixmap);
   areawin->fixed_pixmap = cairo_pop_group(areawin->cr);
#else /* HAVE_CAIRO */
   areawin->window = old_win;
#endif /* HAVE_CAIRO */

   /* Show fixed pixmap */
#ifdef HAVE_CAIRO
   cairo_identity_matrix(areawin->cr);
   cairo_set_source(areawin->cr, areawin->fixed_pixmap);
   cairo_paint(areawin->cr);
#else /* HAVE_CAIRO */
   XCopyArea(dpy, areawin->fixed_pixmap, areawin->window, areawin->gc, 0, 0,
	 areawin->width, areawin->height, 0, 0);
#endif /* HAVE_CAIRO */

   areawin->redraw_ongoing = False;
}

/*-------------------------------------------------------------------------*/
/* Helper function for the xxx_mode_draw functions			   */
/* Hide all the selected elements and draw all the element not currently   */
/* being edited to fixed_pixmap						   */
/*-------------------------------------------------------------------------*/

static void draw_fixed_without_selection(void)
{
   int idx;
   for (idx = 0; idx < areawin->selects; idx++)
      SELTOGENERIC(&areawin->selectlist[idx])->type |= DRAW_HIDE;
   draw_fixed();
   for (idx = 0; idx < areawin->selects; idx++)
      SELTOGENERIC(&areawin->selectlist[idx])->type &= ~DRAW_HIDE;
}

/*-------------------------------------------------------------------------*/
/* generic xxx_mode_draw function, that handles most of the thing needed   */
/* for arcs, splines and paths.						   */
/*-------------------------------------------------------------------------*/

static void generic_mode_draw(xcDrawType type, generic *newgen,
      void (*decorations)(generic *newgen))
{
   int idx;

   switch (type) {
      case xcDRAW_INIT:
      case xcREDRAW_FORCED:
	 draw_fixed_without_selection();
	 /* fallthrough */

      case xcDRAW_EDIT:
	 begin_event_mode_drawing();
   	 for (idx = 0; idx < areawin->selects; idx++) {
	    int scolor = SELTOCOLOR(&areawin->selectlist[idx]);
	    XcTopSetForeground(scolor);
      	    easydraw(areawin->selectlist[idx], scolor);
	 }
	 if (decorations)
	    (*decorations)(newgen);
	 end_event_mode_drawing();
	 break;

      case xcDRAW_FINAL:
	 begin_event_mode_drawing_final();
	 for (idx = 0; idx < areawin->selects; idx++) {
	    int scolor = SELTOCOLOR(&areawin->selectlist[idx]);
	    XcTopSetForeground(scolor);
	    easydraw(areawin->selectlist[idx], scolor);
	 }
	 end_event_mode_drawing_final();
	 if (areawin->selects > 1) /* FIXME: Temp. fix for multiple selects */
	    areawin->redraw_needed = True; /* Will be removed later on */
	 break;

      case xcDRAW_EMPTY:
	 /* Do not remove the empty begin/end. For cairo, this renders the */
	 /* background with the fixed elements */
	 begin_event_mode_drawing_final();
	 end_event_mode_drawing_final();
	 break;
   }
}

/*-------------------------------------------------------------------------*/
/* Drawing function for ARC_MODE and EARC_MODE				   */
/*-------------------------------------------------------------------------*/

static void arc_mode_decorations(generic *newgen)
{
   UDrawXLine(((arc*) newgen)->position, areawin->save);
}

void arc_mode_draw(xcDrawType type, arc *newarc)
{
   generic_mode_draw(type, (generic*) newarc, arc_mode_decorations);
}

/*-------------------------------------------------------------------------*/
/* Drawing function for SPLINE_MODE and ESPLINE_MODE			   */
/*-------------------------------------------------------------------------*/

static void spline_mode_decorations(generic *newgen)
{
   spline *newspline = (spline*) newgen;
   UDrawXLine(newspline->ctrl[0], newspline->ctrl[1]);
   UDrawXLine(newspline->ctrl[3], newspline->ctrl[2]);
}

void spline_mode_draw(xcDrawType type, spline *newspline)
{
   generic_mode_draw(type, (generic*) newspline, spline_mode_decorations);
}

/*-------------------------------------------------------------------------*/
/* Drawing function for WIRE_MODE, BOX_MODE and EPOLY_MODE		   */
/*-------------------------------------------------------------------------*/

void poly_mode_draw(xcDrawType type, polygon *newpoly)
{
   generic_mode_draw(type, (generic*) newpoly, NULL);
}

/*-------------------------------------------------------------------------*/
/* Drawing function for EPATH_MODE					   */
/*-------------------------------------------------------------------------*/

static void path_mode_decorations(generic *newgen)
{
   genericptr *ggen;
   path *newpath = (path*) newgen;
   for (ggen = newpath->plist; ggen < newpath->plist + newpath->parts; ggen++) {
      if (ELEMENTTYPE(*ggen) == SPLINE) {
	 spline *lastspline = TOSPLINE(ggen);
	 UDrawXLine(lastspline->ctrl[0], lastspline->ctrl[1]);
	 UDrawXLine(lastspline->ctrl[3], lastspline->ctrl[2]);
      }
   }
}

void path_mode_draw(xcDrawType type, path *newpath)
{
   generic_mode_draw(type, (generic*) newpath, path_mode_decorations);
}

/*-------------------------------------------------------------------------*/
/* Drawing function for TEXT_MODE, CATTEXT_MODE and ETEXT_MODE		   */
/*-------------------------------------------------------------------------*/

static void text_mode_decorations(generic *newgen)
{
   UDrawTLine((label*) newgen);
}

void text_mode_draw(xcDrawType type, label *newlabel)
{
   generic_mode_draw(type, (generic*) newlabel, text_mode_decorations);
}

/*-------------------------------------------------------------------------*/
/* Drawing function for SELAREA_MODE					   */
/*-------------------------------------------------------------------------*/

void selarea_mode_draw(xcDrawType type, void *unused)
{
   UNUSED(unused);

   switch (type) {
      case xcREDRAW_FORCED:
	 draw_fixed();
	 /* fallthrough */

      case xcDRAW_INIT:
      case xcDRAW_EDIT:
	 begin_event_mode_drawing();
	 draw_all_selected();
	 UDrawBox(areawin->origin, areawin->save);
	 end_event_mode_drawing();
	 break;

      case xcDRAW_FINAL:
      case xcDRAW_EMPTY:
	 /* No need for rendering the background, since it will be */
	 /* overwritten by the select_area() function anyway */
	 break;
   }
}

/*-------------------------------------------------------------------------*/
/* Drawing function for RESCALE_MODE					   */
/*-------------------------------------------------------------------------*/

void rescale_mode_draw(xcDrawType type, void *unused)
{
   UNUSED(unused);

   switch (type) {
      case xcREDRAW_FORCED:
	 draw_fixed();
	 /* fallthrough */

      case xcDRAW_INIT:
      case xcDRAW_EDIT:
	 begin_event_mode_drawing();
	 UDrawRescaleBox(&areawin->save);
	 end_event_mode_drawing();
	 break;

      case xcDRAW_FINAL:
      case xcDRAW_EMPTY:
	 /* No need for rendering the background, since it will be */
	 /* overwritten by the select_area() function anyway */
	 break;
   }
}

/*-------------------------------------------------------------------------*/
/* Drawing function for CATMOVE_MODE, MOVE_MODE and COPY_MODE		   */
/*-------------------------------------------------------------------------*/

void move_mode_draw(xcDrawType type, void *unused)
{
   float wirewidth = xobjs.pagelist[areawin->page]->wirewidth;
   short *selectobj;
   genericptr *pgen;
   int idx;
   UNUSED(unused);

   switch (type) {
      case xcREDRAW_FORCED:
      case xcDRAW_INIT:
	 draw_fixed_without_selection();
	 /* fallthrough */

      case xcDRAW_EDIT:
	 begin_event_mode_drawing();
	 XTopSetForeground(SELECTCOLOR);
   	 for (idx = 0; idx < areawin->selects; idx++)
	    easydraw(areawin->selectlist[idx], DOFORALL);
	 for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
                + areawin->selects; selectobj++) {
	    if (SELECTTYPE(selectobj) == LABEL) {
	       label *labelobj = SELTOLABEL(selectobj);
	       if (labelobj->pin == False)
		  UDrawX(labelobj);
	    }
	 }
	 if (areawin->pinattach) {
	    for (pgen = topobject->plist; pgen < topobject->plist +
		  topobject->parts; pgen++) {
	       if (ELEMENTTYPE(*pgen) == POLYGON) {
		  polygon *cpoly = TOPOLY(pgen);
		  if (cpoly->cycle != NULL)
		     UDrawPolygon(cpoly, wirewidth);
	       }
	    }	
	 }
	 end_event_mode_drawing();
	 break;

      case xcDRAW_FINAL:
	 begin_event_mode_drawing_final();
	 for (selectobj = areawin->selectlist; selectobj
	       < areawin->selectlist + areawin->selects; selectobj++) {
	    XTopSetForeground(SELTOCOLOR(selectobj));
	    easydraw(*selectobj, DOFORALL);
	 }
	 end_event_mode_drawing_final();
	 break;

      case xcDRAW_EMPTY:
	 /* Do not remove the empty begin/end. For cairo, this renders the */
	 /* background with the fixed elements */
	 begin_event_mode_drawing_final();
	 end_event_mode_drawing_final();
	 break;
   }
}

/*-------------------------------------------------------------------------*/
/* Drawing function for ASSOC_MODE, EINST_MODE, (E)FONTCAT_MODE, PAN_MODE, */
/* NORMAL_MODE, UNDO_MODE and CATALOG_MODE				   */
/*-------------------------------------------------------------------------*/

void normal_mode_draw(xcDrawType type, void *unused)
{
   UNUSED(unused);

   switch (type) {
      case xcDRAW_INIT:
      case xcREDRAW_FORCED:
	 draw_fixed_without_selection();
	 /* fallthrough */

      case xcDRAW_EDIT:
	 begin_event_mode_drawing();

         /* draw the highlighted netlist, if any */
         if (checkvalid(topobject) != -1)
            if (topobject->highlight.netlist != NULL)
               highlightnetlist(topobject, areawin->topinstance, 1);

         if ((areawin->selects == 1) && SELECTTYPE(areawin->selectlist) == LABEL
               && areawin->textend > 0 && areawin->textpos > areawin->textend) {
            labelptr drawlabel = SELTOLABEL(areawin->selectlist);
            UDrawString(drawlabel, DOSUBSTRING, areawin->topinstance);
         }
         else if (eventmode == NORMAL_MODE || eventmode == CATALOG_MODE)
            draw_all_selected();
	 end_event_mode_drawing();
	 break;

      case xcDRAW_FINAL:
	 break;

      case xcDRAW_EMPTY:
	 break;
   }
}



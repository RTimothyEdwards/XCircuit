/*-------------------------------------------------------------------------*/
/* events.c --- xcircuit routines handling Xevents and Callbacks	   */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*      written by Tim Edwards, 8/13/93    				   */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#define  XK_MISCELLANY
#define  XK_LATIN1
#include <X11/keysymdef.h>
#else
#ifdef TCL_WRAPPER
#define  XK_MISCELLANY
#define  XK_LATIN1
#include <X11/keysymdef.h>
#endif
#endif

#ifdef HAVE_CAIRO
#include <cairo/cairo-xlib.h>
#endif

/*-------------------------------------------------------------------------*/
/* Local includes							   */
/*-------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif

#include "xcircuit.h"
#include "colordefs.h"

#define HOLD_MASK	(Mod4Mask << 16)

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*-------------------------------------------------------------------------*/
/* Global Variable definitions						   */
/*-------------------------------------------------------------------------*/

extern XtAppContext app;
extern Display	*dpy;
extern Cursor	appcursors[NUM_CURSORS];
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern ApplicationData appdata;
extern colorindex *colorlist;
extern short popups;
extern int pressmode;
extern xcWidget message2, top;
extern char  _STR[150], _STR2[250];
extern short beeper;
extern double saveratio;
extern u_char texttype;
extern aliasptr aliastop;

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#else
extern short help_up;
#endif

/* double buffer */
#if !defined(HAVE_CAIRO)
Pixmap dbuf = (Pixmap)NULL;
#endif

Boolean was_preselected;

/*----------------------------------------------------------------------------*/
/* Edit Object pushing and popping.					      */
/*----------------------------------------------------------------------------*/

Boolean recursefind(objectptr parent, objectptr suspect)
{
   genericptr *shell;

   if (parent == suspect) return True;

   for (shell = parent->plist; shell < parent->plist + parent->parts; shell++)
      if (IS_OBJINST(*shell))
         if (recursefind(TOOBJINST(shell)->thisobject, suspect)) return True;

   return False;
}

/*--------------------------------------------------------------*/
/* Transfer objects in the select list to the current object	*/
/* (but disallow infinitely recursive loops!)			*/
/*--------------------------------------------------------------*/
/* IMPORTANT:  delete_for_xfer() MUST be executed prior to	*/
/* calling transferselects(), so that the deleted elements are	*/
/* in an object saved in areawin->editstack.			*/
/*--------------------------------------------------------------*/

void transferselects()
{
   short locselects;
   objinstptr tobj;
   XPoint newpos;

   if (areawin->editstack->parts == 0) return;

   if (eventmode == MOVE_MODE || eventmode == COPY_MODE || 
		eventmode == UNDO_MODE || eventmode == CATMOVE_MODE) {
      short ps = topobject->parts;

      freeselects();

      locselects = areawin->editstack->parts;
      areawin->selectlist = xc_undelete(areawin->topinstance,
		areawin->editstack, (short)NORMAL, (short *)NULL);
      areawin->selects = locselects;

      /* Move all selected items to the cursor position	*/
      newpos = UGetCursor();
      drag((int)newpos.x, (int)newpos.y);

      /* check to make sure this object is not the current object	*/
      /* or one of its direct ancestors, else an infinite loop results. */

      for (ps = 0; ps < topobject->parts; ps++) {
	 if (IS_OBJINST(*(topobject->plist + ps))) {
	    tobj = TOOBJINST(topobject->plist + ps);
	    if (recursefind(tobj->thisobject, topobject)) {
	       Wprintf("Attempt to place object inside of itself");
	       delete_noundo(NORMAL);
	       break;
	    }
	 }
      }
   }
}

/*-------------------------------------------------------------------*/
/* Make a new matrix corresponding to the current position and scale */
/*-------------------------------------------------------------------*/

void newmatrix()
{
   if (DCTM == NULL) {
      DCTM = (Matrixptr)malloc(sizeof(Matrix));
      DCTM->nextmatrix = NULL;
   }
   UResetCTM(DCTM);
   UMakeWCTM(DCTM);
}

/*-------------------------------------------------------*/
/* set the viewscale variable to the proper address	 */
/*-------------------------------------------------------*/

void setpage(Boolean killselects)
{
   areawin->vscale = topobject->viewscale;
   areawin->pcorner = topobject->pcorner;
   newmatrix();

   if (killselects) clearselects();

#ifdef TCL_WRAPPER
   if (xobjs.suspend < 0)
      XcInternalTagCall(xcinterp, 2, "page", "goto");
#endif
}

/*-------------------------------------------------------*/
/* switch to a new page					 */
/*-------------------------------------------------------*/

int changepage(short pagenumber)
{
   short npage;
   objectptr pageobj;
   u_char undo_type;

   /* to add to existing number of top level pages. . . */

   if (pagenumber == 255) {
      if (xobjs.pages == 255) {
	 Wprintf("Out of available pages!");
	 return -1;
      }
      else pagenumber = xobjs.pages;
   }

   if (pagenumber >= xobjs.pages) {
      
      xobjs.pagelist = (Pagedata **)realloc(xobjs.pagelist, (pagenumber + 1)
		* sizeof(Pagedata *));
      xobjs.pagelist[pagenumber] = (Pagedata *)malloc(sizeof(Pagedata));
      xobjs.pagelist[pagenumber]->filename = NULL;
      xobjs.pagelist[pagenumber]->background.name = NULL;
      xobjs.pagelist[pagenumber]->pageinst = NULL;

      /* If we skipped ahead to pagenumber, fill in the pages in between */
      for (npage = xobjs.pages; npage < pagenumber; npage++) {
         xobjs.pagelist[npage] = (Pagedata *)malloc(sizeof(Pagedata));
         xobjs.pagelist[npage]->pageinst = NULL;
      }

      xobjs.pages = pagenumber + 1;
      makepagebutton();
   }

   if (eventmode == MOVE_MODE || eventmode == COPY_MODE || eventmode == UNDO_MODE) {
      delete_for_xfer(NORMAL, areawin->selectlist, areawin->selects);
      undo_type = UNDO_MORE;
   }
   else {
      clearselects();
      undo_type = UNDO_DONE;
   }
   if (areawin->page != pagenumber)
      register_for_undo(XCF_Page, undo_type, areawin->topinstance,
	  areawin->page, pagenumber);

   if (eventmode != ASSOC_MODE) {
      areawin->page = pagenumber;
      free_stack(&areawin->stack);
   }
   if (xobjs.pagelist[pagenumber]->pageinst == NULL) {

      /* initialize a new page */

      pageobj = (objectptr) malloc (sizeof(object));
      initmem(pageobj);
      sprintf(pageobj->name, "Page %d", pagenumber + 1);

      xobjs.pagelist[pagenumber]->pageinst = newpageinst(pageobj);
      xobjs.pagelist[pagenumber]->filename = NULL;
      xobjs.pagelist[pagenumber]->background.name = NULL;

      pagereset(pagenumber);
   }

   /* Write back the current view parameters */
   if (areawin->topinstance != NULL) {
      topobject->viewscale = areawin->vscale;
      topobject->pcorner = areawin->pcorner;
   }

   areawin->topinstance = xobjs.pagelist[pagenumber]->pageinst;

   setpage(TRUE);

   return 0;
}

/*-------------------------------------------------------*/
/* switch to a new page and redisplay			 */
/*-------------------------------------------------------*/

void newpage(short pagenumber)
{
   switch (eventmode) {
      case CATALOG_MODE:
         eventmode = NORMAL_MODE;
	 catreturn();
	 break;

      case NORMAL_MODE: case COPY_MODE: case MOVE_MODE: case UNDO_MODE:
	 if (changepage(pagenumber) >= 0) {
	    transferselects();
	    renderbackground();
	    refresh(NULL, NULL, NULL);

	    togglegrid((u_short)xobjs.pagelist[areawin->page]->coordstyle);
	    setsymschem();
	 }
	 break;

      default:
         Wprintf("Cannot switch pages from this mode");
	 break;
   }
}

/*---------------------------------------*/
/* Stack structure push and pop routines */
/*---------------------------------------*/

void push_stack(pushlistptr *stackroot, objinstptr thisinst, char *clientdata)
{
   pushlistptr newpush;

   newpush = (pushlistptr)malloc(sizeof(pushlist));
   newpush->next = *stackroot;
   newpush->clientdata = clientdata;
   newpush->thisinst = thisinst;
   *stackroot = newpush;
}

/*----------------------------------------------------------*/

void pop_stack(pushlistptr *stackroot)
{
   pushlistptr lastpush;

   if (!(*stackroot)) {
      Fprintf(stderr, "pop_genstack() Error: NULL instance stack!\n");
      return;
   }

   lastpush = (*stackroot)->next;
   free(*stackroot);
   *stackroot = lastpush;
}

/*----------------------------------------------------------*/

void free_stack(pushlistptr *stackroot)
{
   while ((*stackroot) != NULL)
      pop_stack(stackroot);
}

/*------------------------------------------*/
/* Push object onto hierarchy stack to edit */
/*------------------------------------------*/

void pushobject(objinstptr thisinst)
{
  short *selectobj, *savelist;
   int saves;
   u_char undo_type = UNDO_DONE;
   objinstptr pushinst = thisinst;

   savelist = NULL;
   saves = 0;
   if (eventmode == MOVE_MODE || eventmode == COPY_MODE) {
      savelist = areawin->selectlist;
      saves = areawin->selects;
      areawin->selectlist = NULL;
      areawin->selects = 0;
      undo_type = UNDO_MORE;
   }

   if (pushinst == NULL) {
      selectobj = areawin->selectlist;
      if (areawin->selects == 0) {
         disable_selects(topobject, savelist, saves);
	 selectobj = select_element(OBJINST);
         enable_selects(topobject, savelist, saves);
      }
      if (areawin->selects == 0) {
         Wprintf("No objects selected.");
         return;
      }
      else if (areawin->selects > 1) {
         Wprintf("Choose only one object.");
         return;
      }
      else if (SELECTTYPE(selectobj) != OBJINST) {
         Wprintf("Element to push must be an object.");
         return;
      }
      else pushinst = SELTOOBJINST(selectobj);
   }

   if (savelist != NULL) {
      delete_for_xfer(NORMAL, savelist, saves);
      free(savelist);
   }

   register_for_undo(XCF_Push, undo_type, areawin->topinstance, pushinst);

   /* save the address of the current object to the push stack */

   push_stack(&areawin->stack, areawin->topinstance, NULL);

   topobject->viewscale = areawin->vscale;
   topobject->pcorner = areawin->pcorner;
   areawin->topinstance = pushinst;

   /* move selected items to the new object */

   setpage(TRUE);
   transferselects();
   refresh(NULL, NULL, NULL);
   setsymschem();
}

/*--------------------------*/
/* Pop edit hierarchy stack */
/*--------------------------*/

void popobject(xcWidget w, pointertype no_undo, caddr_t calldata)
{
   u_char undo_type = UNDO_DONE;
   UNUSED(w); UNUSED(calldata);

   if (areawin->stack == NULL || (eventmode != NORMAL_MODE && eventmode != MOVE_MODE
	&& eventmode != COPY_MODE && eventmode != FONTCAT_MODE &&
	eventmode != ASSOC_MODE && eventmode != UNDO_MODE &&
	eventmode != EFONTCAT_MODE)) return;

   if ((eventmode == MOVE_MODE || eventmode == COPY_MODE || eventmode == UNDO_MODE)
	&& ((areawin->stack->thisinst == xobjs.libtop[LIBRARY]) ||
       (areawin->stack->thisinst == xobjs.libtop[USERLIB]))) return;

   /* remove any selected items from the current object */

   if (eventmode == MOVE_MODE || eventmode == COPY_MODE || eventmode == UNDO_MODE) {
      undo_type = UNDO_MORE;
      delete_for_xfer(NORMAL, areawin->selectlist, areawin->selects);
   }
   else if (eventmode != FONTCAT_MODE && eventmode != EFONTCAT_MODE)
      unselect_all();

   /* If coming from the library, don't register an undo action, because */
   /* it has already been registered as type XCF_Library_Pop.		 */

   if (no_undo == (pointertype)0)
      register_for_undo(XCF_Pop, undo_type, areawin->topinstance);

   topobject->viewscale = areawin->vscale;
   topobject->pcorner = areawin->pcorner;
   areawin->topinstance = areawin->stack->thisinst;
   pop_stack(&areawin->stack);

   /* if new object is a library or PAGELIB, put back into CATALOG_MODE */

   if (is_library(topobject) >= 0) eventmode = CATALOG_MODE;

   /* move selected items to the new object */

   if (eventmode == FONTCAT_MODE || eventmode == EFONTCAT_MODE)
      setpage(False);
   else {
      setpage(True);
      setsymschem();
      if (eventmode != ASSOC_MODE)
         transferselects();
   }
   refresh(NULL, NULL, NULL);
}

/*-------------------------------------------------------------------------*/
/* Destructive reset of entire object		 			   */
/*-------------------------------------------------------------------------*/

void resetbutton(xcWidget button, pointertype pageno, caddr_t calldata)
{
   short page;
   objectptr pageobj;
   objinstptr pageinst;
   UNUSED(button); UNUSED(calldata);

   if (eventmode != NORMAL_MODE) return;
 
   page = (pageno == (pointertype)0) ? areawin->page : (short)(pageno - 1);

   pageinst = xobjs.pagelist[page]->pageinst;

   if (pageinst == NULL) return; /* page already cleared */

   pageobj = pageinst->thisobject;

   /* Make sure this is a real top-level page */

   if (is_page(topobject) < 0) {
      if (pageno == (pointertype)0) {
	 Wprintf("Can only clear top-level pages!");
	 return;
      }
      else {
	 /* Make sure that we're not in the hierarchy of the page being deleted */
	 pushlistptr slist;
	 for (slist = areawin->stack; slist != NULL; slist = slist->next)
	    if (slist->thisinst->thisobject == pageobj) {
	       Wprintf("Can't delete the page while you're in its hierarchy!");
	       return;
	    }
      }
   }

   /* Watch for pages which are linked by schematic/symbol. */
   
   if (pageobj->symschem != NULL) {
      Wprintf("Schematic association to object %s", pageobj->symschem->name);
      return;
   }

   sprintf(pageobj->name, "Page %d", page + 1);
   xobjs.pagelist[page]->filename = (char *)realloc(xobjs.pagelist[page]->filename,
		(strlen(pageobj->name) + 1) * sizeof(char));
   strcpy(xobjs.pagelist[page]->filename, pageobj->name);
   reset(pageobj, NORMAL);
   flush_undo_stack();

   if (page == areawin->page) {
      areawin->redraw_needed = True;
      drawarea(areawin->area, NULL, NULL);
      printname(pageobj);
      renamepage(page);
      Wprintf("Page cleared.");
   }
}

/*------------------------------------------------------*/
/* Redraw the horizontal scrollbar			*/
/*------------------------------------------------------*/

void drawhbar(xcWidget bar, caddr_t clientdata, caddr_t calldata)
{
   Window bwin;
   float frac;
   long rleft, rright, rmid;
   int sbarsize;
   char *scale;
   UNUSED(clientdata); UNUSED(calldata);

   if (!xcIsRealized(bar)) return;
   if (xobjs.suspend >= 0) return;

#ifdef TCL_WRAPPER
   scale = (char *)Tcl_GetVar2(xcinterp, "XCOps", "scale", TCL_GLOBAL_ONLY);
   sbarsize = SBARSIZE * atoi(scale);
#else
   sbarsize = SBARSIZE;
#endif

   bwin = xcWindow(bar);

   if (topobject->bbox.width > 0) {
      frac = (float) areawin->width / (float) topobject->bbox.width;
      rleft = (long)(frac * (float)(areawin->pcorner.x
		- topobject->bbox.lowerleft.x));
      rright = rleft + (long)(frac * (float)areawin->width / areawin->vscale);
   }
   else {
      rleft = 0L;
      rright = (long)areawin->width;
   }
   rmid = (rright + rleft) >> 1;

   if (rleft < 0) rleft = 0;
   if (rright > areawin->width) rright = areawin->width;

   XSetFunction(dpy, areawin->gc, GXcopy);
   XSetForeground(dpy, areawin->gc, colorlist[BARCOLOR].color.pixel);
   if (rmid > 0 && rleft > 0)
      XClearArea(dpy, bwin, 0, 0, (int)rleft, sbarsize, FALSE);
   XFillRectangle(dpy, bwin, areawin->gc, (int)rleft + 1, 1,
	  (int)(rright - rleft), sbarsize - 1);
   if (rright > rmid)
      XClearArea(dpy, bwin, (int)rright + 1, 0, areawin->width
	  - (int)rright, sbarsize, FALSE);
   XClearArea(dpy, bwin, (int)rmid - 1, 1, 3, sbarsize, FALSE);

   XSetForeground(dpy, areawin->gc, colorlist[areawin->gccolor].color.pixel);
}

/*------------------------------------------------------*/
/* Redraw the vertical scrollbar			*/
/*------------------------------------------------------*/

void drawvbar(xcWidget bar, caddr_t clientdata, caddr_t calldata)
{
   Window bwin = xcWindow(bar);
   float frac;
   char *scale;
   int sbarsize;
   long rtop, rbot, rmid;
   UNUSED(clientdata); UNUSED(calldata);

#ifdef TCL_WRAPPER
   scale = (char *)Tcl_GetVar2(xcinterp, "XCOps", "scale", TCL_GLOBAL_ONLY);
   sbarsize = SBARSIZE * atoi(scale);
#else
   sbarsize = SBARSIZE;
#endif

   if (!xcIsRealized(bar)) return;
   if (xobjs.suspend >= 0) return;

   if (topobject->bbox.height > 0) {
      frac = (float)areawin->height / (float)topobject->bbox.height;
      rbot = (long)(frac * (float)(topobject->bbox.lowerleft.y
		- areawin->pcorner.y + topobject->bbox.height));
      rtop = rbot - (long)(frac * (float)areawin->height / areawin->vscale);
   }
   else {
      rbot = areawin->height;
      rtop = 0;
   }
   rmid = (rtop + rbot) >> 1;

   if (rtop < 0) rtop = 0;
   if (rbot > areawin->height) rbot = areawin->height;

   XSetFunction(dpy, areawin->gc, GXcopy);
   XSetForeground(dpy, areawin->gc, colorlist[BARCOLOR].color.pixel);
   if (rmid > 0 && rtop > 0)
      XClearArea(dpy, bwin, 0, 0, sbarsize, (int)rtop, FALSE);
   XFillRectangle(dpy, bwin, areawin->gc, 0, (int)rtop + 2, sbarsize,
	     (int)(rbot - rtop));
   if (rbot > rmid)
      XClearArea(dpy, bwin, 0, (int)rbot + 1, sbarsize, areawin->height
		- (int)rbot, FALSE);
   XClearArea(dpy, bwin, 0, (int)rmid - 1, sbarsize, 3, FALSE);

   XSetForeground(dpy, areawin->gc, colorlist[areawin->gccolor].color.pixel);
}

/*------------------------------------------------------*/
/* Simultaneously scroll the screen and horizontal	*/
/* bar when dragging the mouse in the scrollbar area 	*/
/*------------------------------------------------------*/

void panhbar(xcWidget bar, caddr_t clientdata, XButtonEvent *event)
{
   long  newx, newpx;
   short savex = areawin->pcorner.x;
   UNUSED(clientdata);

   if (eventmode == SELAREA_MODE) return;

   newx = (long)(event->x * ((float)topobject->bbox.width /
	areawin->width) + topobject->bbox.lowerleft.x - 0.5 * 
	((float)areawin->width / areawin->vscale));
   areawin->pcorner.x = (short)newx;
   drawhbar(bar, NULL, NULL);
   areawin->pcorner.x = savex;
   
   if ((newpx = (long)(newx - savex) * areawin->vscale) == 0)
      return;

   areawin->panx = -newpx;
   drawarea(NULL, NULL, NULL);
}

/*------------------------------------------------------*/
/* End the horizontal scroll and refresh entire screen	*/
/*------------------------------------------------------*/

void endhbar(xcWidget bar, caddr_t clientdata, XButtonEvent *event)
{
   long  newx;
   short savex = areawin->pcorner.x;
   UNUSED(clientdata);

   areawin->panx = 0;

   newx = (long)(event->x * ((float)topobject->bbox.width /
	areawin->width) + topobject->bbox.lowerleft.x - 0.5 * 
	((float)areawin->width / areawin->vscale));

   areawin->pcorner.x = (short)newx;

   if ((newx << 1) != (long)((short)(newx << 1)) || checkbounds() == -1) {
      areawin->pcorner.x = savex;
      Wprintf("Reached boundary:  cannot pan further");
   }
   else
      W3printf(" ");

   areawin->redraw_needed = True;
   areawin->lastbackground = NULL;
   renderbackground();
   drawhbar(bar, NULL, NULL);
   drawarea(bar, NULL, NULL);
}

/*------------------------------------------------------*/
/* Simultaneously scroll the screen and vertical	*/
/* bar when dragging the mouse in the scrollbar area 	*/
/*------------------------------------------------------*/

void panvbar(xcWidget bar, caddr_t clientdata, XButtonEvent *event)
{
   long  newy, newpy;
   short savey = areawin->pcorner.y;
   UNUSED(clientdata);

   if (eventmode == SELAREA_MODE) return;

   newy = (int)((areawin->height - event->y) *
	((float)topobject->bbox.height / areawin->height) +
	topobject->bbox.lowerleft.y - 0.5 * ((float)areawin->height /
	     areawin->vscale));
   areawin->pcorner.y = (short)newy;
   drawvbar(bar, NULL, NULL);
   areawin->pcorner.y = savey;
   
   if ((newpy = (long)(newy - savey) * areawin->vscale) == 0)
      return;
   
   areawin->pany = newpy;
   drawarea(NULL, NULL, NULL);
}

/*------------------------------------------------------*/
/* Pan the screen to follow the cursor position 	*/
/*------------------------------------------------------*/

void trackpan(int x, int y)
{
   XPoint newpos;
   short savey = areawin->pcorner.y;
   short savex = areawin->pcorner.x;

   newpos.x = areawin->origin.x - x;
   newpos.y = y - areawin->origin.y;

   areawin->pcorner.x += newpos.x / areawin->vscale;
   areawin->pcorner.y += newpos.y / areawin->vscale;

   drawhbar(areawin->scrollbarh, NULL, NULL);
   drawvbar(areawin->scrollbarv, NULL, NULL);

   areawin->panx = -newpos.x;
   areawin->pany = newpos.y;

   drawarea(NULL, NULL, NULL);

   areawin->pcorner.x = savex;
   areawin->pcorner.y = savey;

}

/*------------------------------------------------------*/
/* End the vertical scroll and refresh entire screen	*/
/*------------------------------------------------------*/

void endvbar(xcWidget bar, caddr_t clientdata, XButtonEvent *event)
{
   long  newy;
   short savey = areawin->pcorner.y;
   UNUSED(clientdata);

   areawin->pany = 0;

   newy = (int)((areawin->height - event->y) *
	((float)topobject->bbox.height / areawin->height) +
	topobject->bbox.lowerleft.y - 0.5 * ((float)areawin->height /
	     areawin->vscale));

   areawin->pcorner.y = (short)newy;

   if ((newy << 1) != (long)((short)(newy << 1)) || checkbounds() == -1) {
      areawin->pcorner.y = savey;
      Wprintf("Reached boundary:  cannot pan further");
   }
   else
      W3printf(" ");

   areawin->redraw_needed = True;
   areawin->lastbackground = NULL;
   renderbackground();
   drawvbar(bar, NULL, NULL);
   drawarea(bar, NULL, NULL);
}

/*--------------------------------------------------------------------*/
/* Zoom functions-- zoom box, zoom in, zoom out, and pan.	      */
/*--------------------------------------------------------------------*/

void postzoom()
{
   W3printf(" ");
   areawin->lastbackground = NULL;
   renderbackground();
   newmatrix();
}

/*--------------------------------------------------------------------*/

void zoominbox(int x, int y)
{
   float savescale;
   float delxscale, delyscale;
   XPoint savell; /* ucenter, ncenter, (jdk)*/
   UNUSED(x); UNUSED(y);

   savescale = areawin->vscale;
   savell.x = areawin->pcorner.x;
   savell.y = areawin->pcorner.y;

   /* zoom-box function: corners are in areawin->save and areawin->origin */
   /* select box has lower-left corner in .origin, upper-right in .save */
   /* ignore if zoom box is size zero */

   if (areawin->save.x == areawin->origin.x || areawin->save.y == areawin->origin.y) {
      Wprintf("Zoom box of size zero: Ignoring.");
      eventmode = NORMAL_MODE;
      return;
   }

   /* determine whether x or y is limiting factor in zoom */
   delxscale = (areawin->width / areawin->vscale) /
	   abs(areawin->save.x - areawin->origin.x);
   delyscale = (areawin->height / areawin->vscale) /
	   abs(areawin->save.y - areawin->origin.y);
   areawin->vscale *= min(delxscale, delyscale);

   areawin->pcorner.x = min(areawin->origin.x, areawin->save.x) -
	 (areawin->width / areawin->vscale - 
	 abs(areawin->save.x - areawin->origin.x)) / 2;
   areawin->pcorner.y = min(areawin->origin.y, areawin->save.y) -
	 (areawin->height / areawin->vscale - 
	 abs(areawin->save.y - areawin->origin.y)) / 2;
   eventmode = NORMAL_MODE;

   /* check for minimum scale */

   if (checkbounds() == -1) {
      areawin->pcorner.x = savell.x;
      areawin->pcorner.y = savell.y;
      areawin->vscale = savescale;
      Wprintf("At minimum scale: cannot scale further");

      /* this is a rare case where an object gets out-of-bounds */

      if (checkbounds() == -1) {
	 if (beeper) XBell(dpy, 100);
	 Wprintf("Unable to scale: Delete out-of-bounds object!");
      }
      return;
   }
   postzoom();
}

/*--------------------------------------------------------------------*/

void zoomin(int x, int y)
{
   float savescale;
   XPoint ucenter, ncenter, savell;

   savescale = areawin->vscale;
   savell.x = areawin->pcorner.x;
   savell.y = areawin->pcorner.y;

   window_to_user(areawin->width / 2, areawin->height / 2, &ucenter);
   areawin->vscale *= areawin->zoomfactor;
   window_to_user(areawin->width / 2, areawin->height / 2, &ncenter);
   areawin->pcorner.x += (ucenter.x - ncenter.x);
   areawin->pcorner.y += (ucenter.y - ncenter.y);

   /* check for minimum scale */

   if (checkbounds() == -1) {
      areawin->pcorner.x = savell.x;
      areawin->pcorner.y = savell.y;
      areawin->vscale = savescale;
      Wprintf("At minimum scale: cannot scale further");

      /* this is a rare case where an object gets out-of-bounds */

      if (checkbounds() == -1) {
	 if (beeper) XBell(dpy, 100);
	 Wprintf("Unable to scale: Delete out-of-bounds object!");
      }
      return;
   }
   else if (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == CATMOVE_MODE)
      drag(x, y);

   postzoom();
}

/*--------------------------------------------------------------------*/

void zoominrefresh(int x, int y)
{
   if (eventmode == SELAREA_MODE)
      zoominbox(x, y);
   else
      zoomin(x, y);
   refresh(NULL, NULL, NULL);
}

/*--------------------------------------------------------------------*/

void zoomoutbox(int x, int y)
{
   float savescale;
   float delxscale, delyscale, scalefac;
   XPoint savell; /* ucenter, ncenter, (jdk)*/
   XlPoint newll;
   UNUSED(x); UNUSED(y);

   savescale = areawin->vscale;
   savell.x = areawin->pcorner.x;
   savell.y = areawin->pcorner.y;

   /* zoom-box function, analogous to that for zoom-in */
   /* ignore if zoom box is size zero */

   if (areawin->save.x == areawin->origin.x || areawin->save.y == areawin->origin.y) {
      Wprintf("Zoom box of size zero: Ignoring.");
      eventmode = NORMAL_MODE;
      return;
   }

   /* determine whether x or y is limiting factor in zoom */
   delxscale = abs(areawin->save.x - areawin->origin.x) /
	   (areawin->width / areawin->vscale);
   delyscale = abs(areawin->save.y - areawin->origin.y) /
	   (areawin->height / areawin->vscale);
   scalefac = min(delxscale, delyscale);
   areawin->vscale *= scalefac;

   /* compute lower-left corner of (reshaped) select box */
   if (delxscale < delyscale) {
      newll.y = min(areawin->save.y, areawin->origin.y);
      newll.x = (areawin->save.x + areawin->origin.x
		- (abs(areawin->save.y - areawin->origin.y) *
		areawin->width / areawin->height)) / 2;
   }
   else {
      newll.x = min(areawin->save.x, areawin->origin.x);
      newll.y = (areawin->save.y + areawin->origin.y
		- (abs(areawin->save.x - areawin->origin.x) *
		areawin->height / areawin->width)) / 2;
   }

   /* extrapolate to find new lower-left corner of screen */
   newll.x = areawin->pcorner.x - (int)((float)(newll.x -
		areawin->pcorner.x) / scalefac);
   newll.y = areawin->pcorner.y - (int)((float)(newll.y -
		areawin->pcorner.y) / scalefac);

   eventmode = NORMAL_MODE;
   areawin->pcorner.x = (short)newll.x;
   areawin->pcorner.y = (short)newll.y;

   if ((newll.x << 1) != (long)(areawin->pcorner.x << 1) || (newll.y << 1)
	 != (long)(areawin->pcorner.y << 1) || checkbounds() == -1) {
      areawin->vscale = savescale; 
      areawin->pcorner.x = savell.x;
      areawin->pcorner.y = savell.y;
      Wprintf("At maximum scale: cannot scale further.");
      return;
   }
   postzoom();
}

/*--------------------------------------------------------------------*/

void zoomout(int x, int y)
{
   float savescale;
   XPoint ucenter, ncenter, savell;
   XlPoint newll;

   savescale = areawin->vscale;
   savell.x = areawin->pcorner.x;
   savell.y = areawin->pcorner.y;

   window_to_user(areawin->width / 2, areawin->height / 2, &ucenter); 
   areawin->vscale /= areawin->zoomfactor;
   window_to_user(areawin->width / 2, areawin->height / 2, &ncenter); 
   newll.x = (long)areawin->pcorner.x + (long)(ucenter.x - ncenter.x);
   newll.y = (long)areawin->pcorner.y + (long)(ucenter.y - ncenter.y);
   areawin->pcorner.x = (short)newll.x;
   areawin->pcorner.y = (short)newll.y;

   if ((newll.x << 1) != (long)(areawin->pcorner.x << 1) || (newll.y << 1)
	 != (long)(areawin->pcorner.y << 1) || checkbounds() == -1) {
      areawin->vscale = savescale; 
      areawin->pcorner.x = savell.x;
      areawin->pcorner.y = savell.y;
      Wprintf("At maximum scale: cannot scale further.");
      return;
   }
   else if (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == CATMOVE_MODE)
      drag(x, y);

   postzoom();
}

/*--------------------------------------------------------------------*/

void zoomoutrefresh(int x, int y)
{
   if (eventmode == SELAREA_MODE)
      zoomoutbox(x, y);
   else
      zoomout(x, y);
   refresh(NULL, NULL, NULL);
}

/*--------------------------------------*/
/* Call to XWarpPointer			*/
/*--------------------------------------*/

void warppointer(int x, int y)
{
   XWarpPointer(dpy, None, areawin->window, 0, 0, 0, 0, x, y);
}

/*--------------------------------------------------------------*/
/* ButtonPress handler during center pan 			*/
/* x and y are cursor coordinates.				*/
/* If ptype is 1-4 (directional), then "value" is a fraction of	*/
/* 	the screen to scroll.					*/
/*--------------------------------------------------------------*/

void panbutton(u_int ptype, int x, int y, float value)
{
  /* Window pwin; (jdk) */
   int  xpos, ypos, newllx, newlly;
   XPoint savell; /* , newpos; (jdk)*/
   Dimension hwidth = areawin->width >> 1, hheight = areawin->height >> 1;

   savell.x = areawin->pcorner.x;
   savell.y = areawin->pcorner.y;

   switch(ptype) {
      case 1:
         xpos = hwidth - (hwidth * 2 * value);
         ypos = hheight;
	 break;
      case 2:
         xpos = hwidth + (hwidth * 2 * value);
         ypos = hheight;
	 break;
      case 3:
         xpos = hwidth;
         ypos = hheight - (hheight * 2 * value);
	 break;
      case 4:
         xpos = hwidth;
         ypos = hheight + (hheight * 2 * value);
	 break;
      case 5:
         xpos = x;
         ypos = y;
	 break;
      case 6:	/* "pan follow" */
	 if (eventmode == PAN_MODE)
	    finish_op(XCF_Finish, x, y);
	 else if (eventmode == NORMAL_MODE) {
	    eventmode = PAN_MODE;
	    areawin->save.x = x;
	    areawin->save.y = y;
	    u2u_snap(&areawin->save);
	    areawin->origin = areawin->save;
#ifdef TCL_WRAPPER
	    Tk_CreateEventHandler(areawin->area, PointerMotionMask |
			ButtonMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
	    xcAddEventHandler(areawin->area, PointerMotionMask |
			ButtonMotionMask , False, (xcEventHandler)xlib_drag,
			NULL);
#endif
	 }
	 return;
	 break;
      default:	/* "pan here" */
	 xpos = x;
	 ypos = y;
         warppointer(hwidth, hheight);
	 break;
   }

   xpos -= hwidth;
   ypos = hheight - ypos;

   newllx = (int)areawin->pcorner.x + (int)((float)xpos / areawin->vscale);
   newlly = (int)areawin->pcorner.y + (int)((float)ypos / areawin->vscale);

   areawin->pcorner.x = (short) newllx;
   areawin->pcorner.y = (short) newlly;

   if ((newllx << 1) != (long)(areawin->pcorner.x << 1) || (newlly << 1)
	   != (long)(areawin->pcorner.y << 1) || checkbounds() == -1) {
      areawin->pcorner.x = savell.x;
      areawin->pcorner.x = savell.y;
      Wprintf("Reached bounds:  cannot pan further.");
      return;
   }
   else if (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == CATMOVE_MODE)
      drag(x, y);

   postzoom();
}

/*--------------------------------------------------------------*/

void panrefresh(u_int ptype, int x, int y, float value)
{
   panbutton(ptype, x, y, value);
   refresh(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Check for out-of-bounds before warping pointer, and pan window */
/* if necessary.                                                  */
/*----------------------------------------------------------------*/

void checkwarp(XPoint *userpt)
{
  XPoint wpoint;

  user_to_window(*userpt, &wpoint);

  if (wpoint.x < 0 || wpoint.y < 0 || wpoint.x > areawin->width ||
        wpoint.y > areawin->height) {
     panrefresh(5, wpoint.x, wpoint.y, 0); 
     wpoint.x = areawin->width >> 1;
     wpoint.y = areawin->height >> 1;
     /* snap(wpoint.x, wpoint.y, userpt); */
  }
  warppointer(wpoint.x, wpoint.y);
}

/*--------------------------------------------------------------*/
/* Return a pointer to the element containing a reference point	*/
/*--------------------------------------------------------------*/

genericptr getsubpart(pathptr editpath, int *idx)
{
   pointselect *tmpptr = NULL;
   genericptr *pgen;

   if (idx) *idx = 0;

   for (pgen = editpath->plist; pgen < editpath->plist + editpath->parts; pgen++) {
      switch (ELEMENTTYPE(*pgen)) {
         case POLYGON:
	    if (TOPOLY(pgen)->cycle != NULL) {
	       for (tmpptr = TOPOLY(pgen)->cycle;; tmpptr++) {
	          if (tmpptr->flags & REFERENCE) break;
	          if (tmpptr->flags & LASTENTRY) break;
	       }
	       if (tmpptr->flags & REFERENCE) return *pgen;
	    }
	    break;
         case SPLINE:
	    if (TOSPLINE(pgen)->cycle != NULL) {
	       for (tmpptr = TOSPLINE(pgen)->cycle;; tmpptr++) {
	          if (tmpptr->flags & REFERENCE) break;
	          if (tmpptr->flags & LASTENTRY) break;
	       }
	       if (tmpptr->flags & REFERENCE) return *pgen;
	    }
	    break;
      }
      if (idx) (*idx)++;
   }
   return NULL;
}

/*--------------------------------------------------------------*/
/* Return a pointer to the current reference point of an	*/
/* edited element (polygon, spline, or path)			*/
/*--------------------------------------------------------------*/

pointselect *getrefpoint(genericptr genptr, XPoint **refpt)
{
   pointselect *tmpptr = NULL;
   genericptr *pgen;

   if (refpt) *refpt = NULL;
   switch (genptr->type) {
      case POLYGON:
	 if (((polyptr)genptr)->cycle != NULL) {
	    for (tmpptr = ((polyptr)genptr)->cycle;; tmpptr++) {
	       if (tmpptr->flags & REFERENCE) break;
	       if (tmpptr->flags & LASTENTRY) break;
	    }
	    if (!(tmpptr->flags & REFERENCE)) tmpptr = NULL;
	    else if (refpt) *refpt = ((polyptr)genptr)->points + tmpptr->number;
	 }
	 break;
      case SPLINE:
	 if (((splineptr)genptr)->cycle != NULL) {
	    for (tmpptr = ((splineptr)genptr)->cycle;; tmpptr++) {
	       if (tmpptr->flags & REFERENCE) break;
	       if (tmpptr->flags & LASTENTRY) break;
	    }
	    if (!(tmpptr->flags & REFERENCE)) tmpptr = NULL;
	    else if (refpt) *refpt = &((splineptr)genptr)->ctrl[tmpptr->number];
	 }
	 break;
      case PATH:
	 for (pgen = ((pathptr)genptr)->plist; pgen < ((pathptr)genptr)->plist +
		((pathptr)genptr)->parts; pgen++) {
	    if ((tmpptr = getrefpoint(*pgen, refpt)) != NULL)
	       return tmpptr;
	 }
	 break;
      default:
	 tmpptr = NULL;
	 break;
   }
   return tmpptr;
}

/*--------------------------------------------------------------*/
/* Return next edit point on a polygon, arc, or spline.  Do not	*/
/* update the cycle of the element.				*/
/*--------------------------------------------------------------*/

int checkcycle(genericptr genptr, short dir)
{
   pointselect *tmpptr;
   short tmppt, points;
   genericptr *pgen;

   switch (genptr->type) {
      case POLYGON:
	 if (((polyptr)genptr)->cycle == NULL)
	    tmpptr = NULL;
	 else {
	    for (tmpptr = ((polyptr)genptr)->cycle;; tmpptr++) {
	       if (tmpptr->flags & REFERENCE) break;
	       if (tmpptr->flags & LASTENTRY) break;
	    }
	    if (!(tmpptr->flags & REFERENCE)) tmpptr = ((polyptr)genptr)->cycle;
	 }
	 tmppt = (tmpptr == NULL) ? -1 : tmpptr->number;
	 points = ((polyptr)genptr)->number;
	 break;
      case SPLINE:
	 if (((splineptr)genptr)->cycle == NULL)
	    tmpptr = NULL;
	 else {
	    for (tmpptr = ((splineptr)genptr)->cycle;; tmpptr++) {
	       if (tmpptr->flags & REFERENCE) break;
	       if (tmpptr->flags & LASTENTRY) break;
	    }
	    if (!(tmpptr->flags & REFERENCE)) tmpptr = ((splineptr)genptr)->cycle;
	 }
	 tmppt = (tmpptr == NULL) ? -1 : tmpptr->number;
	 points = 4;
	 break;
      case ARC:
	 tmpptr = ((arcptr)genptr)->cycle;
	 tmppt = (tmpptr == NULL) ? -1 : tmpptr->number;
	 points = 4;
	 break;
      case PATH:
	 for (pgen = ((pathptr)genptr)->plist; pgen < ((pathptr)genptr)->plist +
		((pathptr)genptr)->parts; pgen++) {
	    if ((tmppt = checkcycle(*pgen, dir)) >= 0)
	       return tmppt;
	 }
	 break;
      default:
	 tmppt = -1;
	 break;
   }
   if (tmppt >= 0) {	/* Ignore nonexistent cycles */
      tmppt += dir;
      if (tmppt < 0) tmppt += points;
      tmppt %= points;
   }
   return tmppt;
}

/*--------------------------------------------------------------*/
/* Change to the next part of a path for editing		*/
/* For now, |dir| is treated as 1 regardless of its value.	*/
/*--------------------------------------------------------------*/

void nextpathcycle(pathptr nextpath, short dir)
{
   genericptr ppart = getsubpart(nextpath, NULL);
   genericptr *ggen;
   XPoint *curpt;
   polyptr thispoly;
   splineptr thisspline;
   pointselect *cptr;
   short cycle, newcycle;

   /* Simple cases---don't need to switch elements */

   switch (ELEMENTTYPE(ppart)) {
      case POLYGON:
	 thispoly = (polyptr)ppart;
	 cptr = thispoly->cycle;
	 if (cptr == NULL) return;
	 curpt = thispoly->points + cptr->number;
         newcycle = checkcycle(ppart, dir);
         advancecycle(&ppart, newcycle);
	 if (cptr->number < thispoly->number && cptr->number > 0) {
	    checkwarp(thispoly->points + cptr->number);
	    removeothercycles(nextpath, ppart);
	    updatepath(nextpath);
	    return;
	 }
	 break;
      case SPLINE:
	 thisspline = (splineptr)ppart;
	 cptr = ((splineptr)ppart)->cycle;
	 if (cptr == NULL) return;
	 curpt = &thisspline->ctrl[cptr->number];
         newcycle = checkcycle(ppart, dir);
         advancecycle(&ppart, newcycle);
	 if (cptr->number < 4 && cptr->number > 0) {
	    checkwarp(&thisspline->ctrl[cptr->number]);
	    removeothercycles(nextpath, ppart);
	    updatepath(nextpath);
	    if (newcycle == 1 || newcycle == 2)
	       addanticycle(nextpath, thisspline, newcycle);
	    return;
	 }
	 break;
   }

   /* Moving on to the next element. . . */

   /* If dir < 0, go to the penultimate cycle of the last part	*/
   /* If dir > 0, go to the second cycle of the next part 	*/

   for (ggen = nextpath->plist; (*ggen != ppart) &&
		(ggen < nextpath->plist + nextpath->parts); ggen++);

   if (ggen == nextpath->plist + nextpath->parts) return;  /* shouldn't happen! */

   if (dir > 0)
      ggen++;
   else
      ggen--;

   if (ggen < nextpath->plist)
      ggen = nextpath->plist + nextpath->parts - 1;
   else if (ggen == nextpath->plist + nextpath->parts)
      ggen = nextpath->plist;

   removecycle((genericptr *)(&nextpath));

   /* The next point to edit is the first point in the next segment	*/
   /* that is not at the same position as the one we were last editing.	*/

   switch (ELEMENTTYPE(*ggen)) {
      case POLYGON:
	 thispoly = TOPOLY(ggen);
	 cycle = (dir > 0) ? 0 : thispoly->number - 1;
	 addcycle(ggen, cycle, 0);
	 makerefcycle(thispoly->cycle, cycle);
	 if ((thispoly->points + cycle)->x == curpt->x &&
		(thispoly->points + cycle)->y == curpt->y) {
            newcycle = checkcycle((genericptr)thispoly, 1);
	    advancecycle(ggen, newcycle);
	    cycle = newcycle;
	 }
         checkwarp(thispoly->points + cycle);
	 break;
      case SPLINE:
	 thisspline = TOSPLINE(ggen);
	 cycle = (dir > 0) ? 0 : 3;
	 addcycle(ggen, cycle, 0);
	 makerefcycle(thisspline->cycle, cycle);
	 if (thisspline->ctrl[cycle].x == curpt->x &&
		thisspline->ctrl[cycle].y == curpt->y) {
            newcycle = checkcycle((genericptr)thisspline, 1);
	    advancecycle(ggen, newcycle);
	    cycle = newcycle;
	    if (cycle == 1 || cycle == 2)
	       addanticycle(nextpath, thisspline, cycle);
	 }
	 checkwarp(&(thisspline->ctrl[cycle]));
	 break;
   }
   updatepath(nextpath);
}

/*--------------------------------------------------------------*/
/* Change to next edit point on a polygon			*/
/*--------------------------------------------------------------*/

void nextpolycycle(polyptr *nextpoly, short dir)
{
   short newcycle;

   newcycle = checkcycle((genericptr)(*nextpoly), dir);
   advancecycle((genericptr *)nextpoly, newcycle);
   findconstrained(*nextpoly);
   printeditbindings();

   newcycle = (*nextpoly)->cycle->number;
   checkwarp((*nextpoly)->points + newcycle);
}

/*--------------------------------------------------------------*/
/* Change to next edit cycle on a spline			*/
/*--------------------------------------------------------------*/

void nextsplinecycle(splineptr *nextspline, short dir)
{
   short newcycle;
   newcycle = checkcycle((genericptr)(*nextspline), dir);
   advancecycle((genericptr *)nextspline, newcycle);

   if (newcycle == 1 || newcycle == 2)
      Wprintf("Adjust control point");
   else
      Wprintf("Adjust endpoint position");

   checkwarp(&(*nextspline)->ctrl[newcycle]);
}

/*--------------------------------------------------------------*/
/* Warp pointer to the edit point on an arc.			*/
/*--------------------------------------------------------------*/

void warparccycle(arcptr nextarc, short cycle)
{
   XPoint curang;
   double rad;

   switch(cycle) {
      case 0:
	 curang.x = nextarc->position.x + abs(nextarc->radius);
	 curang.y = nextarc->position.y;
	 if (abs(nextarc->radius) != nextarc->yaxis)
	    Wprintf("Adjust ellipse size");
	 else
	    Wprintf("Adjust arc radius");
	 break;
      case 1:
         rad = (double)(nextarc->angle1 * RADFAC);
         curang.x = nextarc->position.x + abs(nextarc->radius) * cos(rad);
         curang.y = nextarc->position.y + nextarc->yaxis * sin(rad);
	 Wprintf("Adjust arc endpoint");
  	 break;
      case 2:
         rad = (double)(nextarc->angle2 * RADFAC);
         curang.x = nextarc->position.x + abs(nextarc->radius) * cos(rad);
         curang.y = nextarc->position.y + nextarc->yaxis * sin(rad);
	 Wprintf("Adjust arc endpoint");
	 break;
      case 3:
	 curang.x = nextarc->position.x;
	 curang.y = nextarc->position.y + nextarc->yaxis;
	 Wprintf("Adjust ellipse minor axis");
	 break;
   }
   checkwarp(&curang);
}

/*--------------------------------------------------------------*/
/* Change to next edit cycle on an arc				*/
/*--------------------------------------------------------------*/

void nextarccycle(arcptr *nextarc, short dir)
{
   short newcycle;

   newcycle = checkcycle((genericptr)(*nextarc), dir);
   advancecycle((genericptr *)nextarc, newcycle);
   warparccycle(*nextarc, newcycle);
}

/*------------------------------------------------------*/
/* Get a numerical response from the keyboard (0-9)     */
/*------------------------------------------------------*/

#ifndef TCL_WRAPPER

short getkeynum()
{
   XEvent event;
   XKeyEvent *keyevent = (XKeyEvent *)(&event);
   KeySym keypressed;

   for (;;) {
      XNextEvent(dpy, &event);
      if (event.type == KeyPress) break;
      else xcDispatchEvent(&event);
   }
   XLookupString(keyevent, _STR, 150, &keypressed, NULL);
   if (keypressed > XK_0 && keypressed <= XK_9)
      return (short)(keypressed - XK_1);
   else
      return -1;
}

#endif

/*--------------------------*/
/* Register a "press" event */
/*--------------------------*/

#ifdef TCL_WRAPPER
void makepress(ClientData clientdata)
#else
void makepress(XtPointer clientdata, xcIntervalId *id) 
#endif
{
   int keywstate = (int)((pointertype)clientdata);
#ifndef TCL_WRAPPER
   UNUSED(id);
#endif

   /* Button/Key was pressed long enough to make a "press", not a "tap" */

   areawin->time_id = 0;
   pressmode = keywstate;
   eventdispatch(keywstate | HOLD_MASK, areawin->save.x, areawin->save.y);
}

/*------------------------------------------------------*/
/* Handle button events as if they were keyboard events */
/*------------------------------------------------------*/

void buttonhandler(xcWidget w, caddr_t clientdata, XButtonEvent *event)
{
   XKeyEvent *kevent = (XKeyEvent *)event;

   if (event->type == ButtonPress)
      kevent->type = KeyPress;
   else
      kevent->type = KeyRelease;

   switch (event->button) {
      case Button1:
	 kevent->state |= Button1Mask;
         break;
      case Button2:
	 kevent->state |= Button2Mask;
         break;
      case Button3:
	 kevent->state |= Button3Mask;
         break;
      case Button4:
	 kevent->state |= Button4Mask;
         break;
      case Button5:
	 kevent->state |= Button5Mask;
         break;
   }
   keyhandler(w, clientdata, kevent);
}

/*--------------------------------------------------------------*/
/* Edit operations specific to polygons (point manipulation)	*/
/*--------------------------------------------------------------*/

void poly_edit_op(int op)
{
   genericptr keygen = *(EDITPART);
   polyptr lwire;
   XPoint *lpoint;
   short cycle;

   if (IS_PATH(keygen))
      keygen = getsubpart((pathptr)keygen, NULL);

   switch(ELEMENTTYPE(keygen)) {
      case POLYGON: {
   	 lwire = (polyptr)keygen;

         /* Remove a point from the polygon */
	 if (op == XCF_Edit_Delete) {
	    if (lwire->number < 3) return;
	    if (lwire->number == 3 && !(lwire->style & UNCLOSED)) 
	       lwire->style |= UNCLOSED;
	    cycle = checkcycle((genericptr)lwire, 0);
	    lwire->number--;
	    for (lpoint = lwire->points + cycle; lpoint <
			lwire->points + lwire->number; lpoint++)
	       *lpoint = *(lpoint + 1);
	    if (eventmode == EPOLY_MODE)
	       poly_mode_draw(xcDRAW_EDIT, TOPOLY(EDITPART));
	    else
	       path_mode_draw(xcDRAW_EDIT, TOPATH(EDITPART));
	    nextpolycycle(&lwire, -1);
         }

         /* Add a point to the polygon */
	 else if (op == XCF_Edit_Insert || op == XCF_Edit_Append) {
	    lwire->number++;
	    lwire->points = (XPoint *)realloc(lwire->points, lwire->number
		      * sizeof(XPoint));
	    cycle = checkcycle((genericptr)lwire, 0);
	    for (lpoint = lwire->points + lwire->number - 1; lpoint > lwire->
		         points + cycle; lpoint--)
	       *lpoint = *(lpoint - 1);
	    if (eventmode == EPOLY_MODE)
	       poly_mode_draw(xcDRAW_EDIT, TOPOLY(EDITPART));
	    else
	       path_mode_draw(xcDRAW_EDIT, TOPATH(EDITPART));
	    if (op == XCF_Edit_Append)
	       nextpolycycle(&lwire, 1);
         }

	 /* Parameterize the position of a polygon point */
	 else if (op == XCF_Edit_Param) {
	    cycle = checkcycle((genericptr)lwire, 0);
	    makenumericalp(&keygen, P_POSITION_X, NULL, cycle);
	    makenumericalp(&keygen, P_POSITION_Y, NULL, cycle);
	 }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Handle attachment of edited elements to nearby elements		*/
/*----------------------------------------------------------------------*/

void attach_to()
{
   /* Conditions: One element is selected, key "A" is pressed.	*/
   /* Then there must exist a spline, polygon, arc, or label	*/
   /* to attach to.						*/

   if (areawin->selects <= 1) {
      short *refsel;

      if (areawin->attachto >= 0) {
	 areawin->attachto = -1;	/* default value---no attachments */
	 Wprintf("Unconstrained moving");
      }
      else {
	 int select_prev;

	 select_prev = areawin->selects;
	 refsel = select_add_element(SPLINE|ARC|POLYGON|LABEL|OBJINST);
	 if ((refsel != NULL) && (areawin->selects > select_prev)) {

	    /* transfer refsel over to attachto */

	    areawin->attachto = *(refsel + areawin->selects - 1);
	    areawin->selects--;
	    if (areawin->selects == 0) freeselects();
	    XTopSetForeground(SELTOCOLOR(refsel));
	    easydraw(areawin->attachto, DEFAULTCOLOR);

	    /* restore graphics state */
	    SetForeground(dpy, areawin->gc, areawin->gccolor);

	    Wprintf("Constrained attach");

	    /* Starting a new wire? */
	    if (eventmode == NORMAL_MODE) {
	       XPoint newpos, userpt;
	       userpt = UGetCursorPos();
	       findattach(&newpos, NULL, &userpt);
	       startwire(&newpos);
	       eventmode = WIRE_MODE;
	       areawin->attachto = -1;
	    }
	 }
	 else {
	    Wprintf("Nothing found to attach to");
	 }
      }
   }
}

/*--------------------------------------------------------------*/
/* This function returns TRUE if the indicated function is	*/
/* compatible with the current eventmode;  that is, whether	*/
/* the function could ever be called from eventdispatch()	*/
/* given the existing eventmode.				*/
/*								*/
/* Note that this function has to be carefully written or the	*/
/* function dispatch mechanism can put xcircuit into a bad	*/
/* state.							*/
/*--------------------------------------------------------------*/

Boolean compatible_function(int function)
{
   int r = FALSE;
   char *funcname;

   switch(function) {
      case XCF_Text_Left: case XCF_Text_Right:
      case XCF_Text_Home: case XCF_Text_End:
      case XCF_Text_Return: case XCF_Text_Delete:
      case XCF_Text_Delete_Param:
	 r = (eventmode == CATTEXT_MODE || eventmode == TEXT_MODE ||
		eventmode == ETEXT_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Linebreak: case XCF_Halfspace:
      case XCF_Quarterspace: case XCF_TabStop:
      case XCF_TabForward: case XCF_TabBackward:
      case XCF_Superscript: case XCF_Subscript:
      case XCF_Normalscript: case XCF_Underline:
      case XCF_Overline: case XCF_Font:
      case XCF_Boldfont: case XCF_Italicfont:
      case XCF_Normalfont: case XCF_ISO_Encoding:
      case XCF_Special: case XCF_Text_Split:
      case XCF_Text_Up: case XCF_Text_Down:
      case XCF_Parameter:
	 r = (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Anchor:
	 r = (eventmode == TEXT_MODE || eventmode == ETEXT_MODE ||
		eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == NORMAL_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Edit_Delete: case XCF_Edit_Insert: case XCF_Edit_Append:
      case XCF_Edit_Param:
	 r = (eventmode == EPOLY_MODE || eventmode == EPATH_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Edit_Next:
	 r = (eventmode == EPOLY_MODE || eventmode == EPATH_MODE ||
		eventmode == EINST_MODE || eventmode == EARC_MODE ||
		eventmode == ESPLINE_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Attach:
	 r = (eventmode == EPOLY_MODE || eventmode == EPATH_MODE ||
		eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == WIRE_MODE || eventmode == NORMAL_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Rotate: case XCF_Flip_X:
      case XCF_Flip_Y:
	 r = (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == NORMAL_MODE || eventmode == CATALOG_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Snap: case XCF_Swap:
	 r = (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == NORMAL_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Double_Snap: case XCF_Halve_Snap:
      case XCF_SnapTo:
	 r = (eventmode == CATALOG_MODE || eventmode == CATTEXT_MODE ||
		eventmode == ASSOC_MODE || eventmode == CATMOVE_MODE) ?
		FALSE : TRUE;
	 break;

      case XCF_Library_Pop:
	 r = (eventmode == CATALOG_MODE || eventmode == ASSOC_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Library_Edit: case XCF_Library_Delete:
      case XCF_Library_Duplicate: case XCF_Library_Hide:
      case XCF_Library_Virtual: case XCF_Library_Move:
      case XCF_Library_Copy:
	 r = (eventmode == CATALOG_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Library_Directory:
	 r = (eventmode == CATALOG_MODE || eventmode == NORMAL_MODE ||
		eventmode == ASSOC_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Next_Library:
	 r = (eventmode == CATALOG_MODE || eventmode == NORMAL_MODE ||
		eventmode == ASSOC_MODE || eventmode == CATMOVE_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Select: case XCF_Exit:
	 r = (eventmode == CATALOG_MODE || eventmode == NORMAL_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Pop:
	 r = (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == CATALOG_MODE || eventmode == NORMAL_MODE ||
		eventmode == ASSOC_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Push:
	 r = (eventmode == MOVE_MODE || eventmode == COPY_MODE ||
		eventmode == CATALOG_MODE || eventmode == NORMAL_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_SelectBox: case XCF_Wire:
      case XCF_Delete: case XCF_Rescale:
      case XCF_Pin_Label: case XCF_Pin_Global:
      case XCF_Info_Label: case XCF_Connectivity:
      case XCF_Box: case XCF_Arc:
      case XCF_Text: case XCF_Exchange:
      case XCF_Copy: case XCF_Virtual:
      case XCF_Page_Directory: case XCF_Join:
      case XCF_Unjoin: case XCF_Spline:
      case XCF_Edit: case XCF_Undo:
      case XCF_Redo: case XCF_Select_Save:
      case XCF_Unselect: case XCF_Dashed:
      case XCF_Dotted: case XCF_Solid:
      case XCF_Dot: case XCF_Write:
      case XCF_Netlist: case XCF_Sim:
      case XCF_SPICE: case XCF_SPICEflat:
      case XCF_PCB: case XCF_Move:
	 r = (eventmode == NORMAL_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Nothing: case XCF_View:
      case XCF_Redraw: case XCF_Zoom_In:
      case XCF_Zoom_Out: case XCF_Pan:
      case XCF_Page: case XCF_Help:
      case XCF_Cancel: case XCF_Prompt:
	 r = TRUE;
	 break;

      case XCF_Continue_Copy:
      case XCF_Finish_Copy:
	 r = (eventmode == COPY_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Continue_Element:
      case XCF_Finish_Element:
	 r = (eventmode == WIRE_MODE || eventmode == BOX_MODE ||
		eventmode == ARC_MODE || eventmode == SPLINE_MODE ||
		eventmode == EPATH_MODE || eventmode == EPOLY_MODE ||
		eventmode == EARC_MODE || eventmode == ESPLINE_MODE ||
		eventmode == MOVE_MODE || eventmode == CATMOVE_MODE ||
		eventmode == EINST_MODE || eventmode == RESCALE_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Cancel_Last:
	 r = (eventmode == WIRE_MODE || eventmode == ARC_MODE ||
		eventmode == SPLINE_MODE || eventmode == EPATH_MODE || 
		eventmode == EPOLY_MODE || eventmode == EARC_MODE ||
		eventmode == EINST_MODE || eventmode == ESPLINE_MODE) ?
		TRUE : FALSE;
	 break;

      case XCF_Finish:
	 r = (eventmode == FONTCAT_MODE || eventmode == EFONTCAT_MODE ||
		eventmode == ASSOC_MODE || eventmode == CATALOG_MODE ||
		eventmode == CATTEXT_MODE || eventmode == MOVE_MODE ||
		eventmode == RESCALE_MODE || eventmode == SELAREA_MODE ||
		eventmode == PAN_MODE || eventmode == NORMAL_MODE ||
		eventmode == CATMOVE_MODE) ?
		TRUE : FALSE;
	 break;

      default:	/* Function type was not handled. */
	 funcname = func_to_string(function);
	 if (funcname == NULL)
	    Wprintf("Error:  \"%s\" is not a known function!");
	 else
	    Wprintf("Error:  Function type \"%s\" (%d) not handled by "
		"compatible_function()", func_to_string(function),
		function);
	 break;
   }
   return r;
}

/*----------------------------------------------------------------------*/
/* Main event dispatch routine.  Call one of the known routines based	*/
/* on the key binding.  Some handling is done by secondary dispatch	*/
/* routines in other files;  when this is done, the key binding is	*/
/* determined here and the bound operation type passed to the secondary	*/
/* dispatch routine.							*/
/*									*/
/* Return value:  0 if event was handled, -1 if not.			*/
/*----------------------------------------------------------------------*/

int eventdispatch(int keywstate, int x, int y)
{
   short value;		/* For return values from boundfunction() */
   int function;	/* What function should be invoked	  */
   int handled = -1;    /* event was handled */

   /* Invalid key state returned from getkeysignature(); usually this	*/
   /* means a modifier key pressed by itself.				*/

   if (keywstate == -1) return -1;
   function = boundfunction(areawin->area, keywstate, &value);

   /* Check for ASCII or ISO-Latin1-9 characters in keywstate while in	*/
   /* a text-entry state.  Only the function XCF_Special is allowed in	*/
   /* text-entry mode, because XCF_Special can be used to enter the	*/
   /* character bound to the XCF_Special function.			*/

   if (keywstate >= 32 && keywstate < 256) {
      if (eventmode == CATTEXT_MODE || eventmode == TEXT_MODE ||
		eventmode == ETEXT_MODE) {
	 if (function != XCF_Special)
	    handled = labeltext(keywstate, NULL);
	 else if (eventmode != CATTEXT_MODE) {
	    labelptr elabel = TOLABEL(EDITPART);
	    if (elabel->anchor & LATEXLABEL)
	       handled = labeltext(keywstate, NULL);
	 }
      }
   }

   if (handled == -1) {
      if (function > -1)
         handled = functiondispatch(function, value, x, y);
      else {
         char *keystring = key_to_string(keywstate);
#ifdef HAVE_PYTHON
         if (python_key_command(keywstate) < 0)
#endif
         Wprintf("Key \'%s\' is not bound to a macro", keystring);
         free(keystring);
      }
   }

   if (areawin->redraw_needed)
      drawarea(NULL, NULL, NULL);

   return handled;
}

/*----------------------------------------------------------------------*/
/* Dispatch actions by function number.  Note that the structure of	*/
/* this function is closely tied to the routine compatible_function().	*/
/*----------------------------------------------------------------------*/

int functiondispatch(int function, short value, int x, int y)
{
   int result = 0;

   switch (eventmode) {
      case MOVE_MODE:
      case COPY_MODE:
         snap(x, y, &areawin->save);
	 break;
      case NORMAL_MODE:
	 window_to_user(x, y, &areawin->save);
	 break;
      default:
	 break;
   }

   switch(function) {
      case XCF_Page:
	 if (value < 0 || value > xobjs.pages)
	    Wprintf("Page %d out of range.", (int)value);
	 else
	    newpage(value - 1);
	 break;
      case XCF_Anchor:
	 reanchor(value);
	 break;
      case XCF_Superscript:
	 labeltext(SUPERSCRIPT, (char *)1);
	 break;
      case XCF_Subscript:
	 labeltext(SUBSCRIPT, (char *)1);
	 break;
      case XCF_Normalscript:
	 labeltext(NORMALSCRIPT, (char *)1);
	 break;
      case XCF_Font:
	 setfont(NULL, 1000, NULL);
	 break;
      case XCF_Boldfont:
	 fontstyle(NULL, 1, NULL);
	 break;
      case XCF_Italicfont:
	 fontstyle(NULL, 2, NULL);
	 break;
      case XCF_Normalfont:
	 fontstyle(NULL, 0, NULL);
	 break;
      case XCF_Underline:
	 labeltext(UNDERLINE, (char *)1);
	 break;
      case XCF_Overline:
	 labeltext(OVERLINE, (char *)1);
	 break;
      case XCF_ISO_Encoding:
	 fontencoding(NULL, 2, NULL);
	 break;
      case XCF_Halfspace:
	 labeltext(HALFSPACE, (char *)1);
	 break;
      case XCF_Quarterspace:
	 labeltext(QTRSPACE, (char *)1);
	 break;
      case XCF_Special:
	 result = dospecial();
	 break;
#ifndef TCL_WRAPPER
      case XCF_Parameter:
	 insertparam();
	 break;
#endif
      case XCF_TabStop:
	 labeltext(TABSTOP, (char *)1);
	 break;
      case XCF_TabForward:
	 labeltext(TABFORWARD, (char *)1);
	 break;
      case XCF_TabBackward:
	 labeltext(TABBACKWARD, (char *)1);
	 break;
      case XCF_Text_Return:
	 labeltext(TEXT_RETURN, (char *)1);
	 break;
      case XCF_Text_Delete:
	 labeltext(TEXT_DELETE, (char *)1);
	 break;
      case XCF_Text_Delete_Param:
	 labeltext(TEXT_DEL_PARAM, (char *)1);
	 break;
      case XCF_Text_Right:
	 labeltext(TEXT_RIGHT, (char *)1);
	 break;
      case XCF_Text_Left:
	 labeltext(TEXT_LEFT, (char *)1);
	 break;
      case XCF_Text_Up:
	 labeltext(TEXT_UP, (char *)1);
	 break;
      case XCF_Text_Down:
	 labeltext(TEXT_DOWN, (char *)1);
	 break;
      case XCF_Text_Split:
	 labeltext(TEXT_SPLIT, (char *)1);
	 break;
      case XCF_Text_Home:
	 labeltext(TEXT_HOME, (char *)1);
	 break;
      case XCF_Text_End:
	 labeltext(TEXT_END, (char *)1);
	 break;
      case XCF_Linebreak:
	 labeltext(RETURN, (char *)1);
	 break;
      case XCF_Edit_Param:
      case XCF_Edit_Delete:
      case XCF_Edit_Insert:
      case XCF_Edit_Append:
	 poly_edit_op(function);
	 break;
      case XCF_Edit_Next:
	 path_op(*(EDITPART), XCF_Continue_Element, x, y);
	 break;
      case XCF_Attach:
	 attach_to();
	 break;
      case XCF_Next_Library:
	 changecat();
	 break;
      case XCF_Library_Directory:
	 startcatalog(NULL, LIBLIB, NULL);
	 break;
      case XCF_Library_Edit:
	 window_to_user(x, y, &areawin->save);
	 unselect_all();
	 select_element(LABEL);
	 if (areawin->selects == 1)
	    edit(x, y);
	 break;
      case XCF_Library_Delete:
	 catalog_op(XCF_Select, x, y);
	 catdelete();
	 break;
      case XCF_Library_Duplicate:
	 catalog_op(XCF_Select, x, y);
	 copycat();
	 break;
      case XCF_Library_Hide:
	 catalog_op(XCF_Select, x, y);
	 cathide();
	 break;
      case XCF_Library_Virtual:
	 catalog_op(XCF_Select, x, y);
	 catvirtualcopy();
	 break;
      case XCF_Page_Directory:
	 startcatalog(NULL, PAGELIB, NULL);
	 break;
      case XCF_Library_Copy:
      case XCF_Library_Pop:
	 catalog_op(function, x, y);
	 break;
      case XCF_Virtual:
	 copyvirtual();
	 break;
      case XCF_Help:
	 starthelp(NULL, NULL, NULL);
	 break;
      case XCF_Redraw:
   	 areawin->redraw_needed = True;
	 break;
      case XCF_View:
	 zoomview(NULL, NULL, NULL);
	 break;
      case XCF_Zoom_In:
	 zoominrefresh(x, y);
	 break;
      case XCF_Zoom_Out:
	 zoomoutrefresh(x, y);
	 break;
      case XCF_Pan:
	 panrefresh(value, x, y, 0.3);
	 break;
      case XCF_Double_Snap:
	 setsnap(1);
	 break;
      case XCF_Halve_Snap:
	 setsnap(-1);
	 break;
      case XCF_Write:
#ifdef TCL_WRAPPER
	 Tcl_Eval(xcinterp, "xcircuit::promptsavepage");
#else
	 outputpopup(NULL, NULL, NULL);
#endif
	 break;
      case XCF_Rotate:
	 elementrotate(value, &areawin->save);
	 break;
      case XCF_Flip_X:
	 elementflip(&areawin->save);
	 break;
      case XCF_Flip_Y:
	 elementvflip(&areawin->save);
	 break;
      case XCF_Snap:
	 snapelement();
	 break;
      case XCF_SnapTo:
	 if (areawin->snapto) {
	    areawin->snapto = False;
	    Wprintf("Snap-to off");
	 }
	 else {
	    areawin->snapto = True;
	    Wprintf("Snap-to on");
	 }
	 break;
      case XCF_Pop:
	 if (eventmode == CATALOG_MODE || eventmode == ASSOC_MODE) {
	    eventmode = NORMAL_MODE;
	    catreturn();
	 }
	 else
	    popobject(NULL, 0, NULL);
	 break;
      case XCF_Push:
	 if (eventmode == CATALOG_MODE) {
	    /* Don't allow push from library directory */
	    if ((areawin->topinstance != xobjs.libtop[LIBLIB])
                	&& (areawin->topinstance != xobjs.libtop[PAGELIB])) {
	       window_to_user(x, y, &areawin->save);
	       eventmode = NORMAL_MODE;
	       pushobject(NULL);
	    }
	 }
	 else
	    pushobject(NULL);
	 break;
      case XCF_Delete:
	 deletebutton(x, y);
	 break;
      case XCF_Select:
	 if (eventmode == CATALOG_MODE)
	    catalog_op(function, x, y);
	 else
	    select_add_element(ALL_TYPES);
	 break;
      case XCF_Box:
	 boxbutton(x, y);
	 break;
      case XCF_Arc:
	 arcbutton(x, y);
	 break;
      case XCF_Text:
	 eventmode = TEXT_MODE;
	 textbutton(NORMAL, x, y);
	 break;
      case XCF_Exchange:
	 exchange();
	 break;
      case XCF_Library_Move:
	 /* Don't allow from library directory.  Then fall through to XCF_Move */
	 if (areawin->topinstance == xobjs.libtop[LIBLIB]) break;
      case XCF_Move:
	 if (areawin->selects == 0) {
	    was_preselected = FALSE;
	    if (eventmode == CATALOG_MODE)
	       catalog_op(XCF_Select, x, y);
	    else
	       select_element(ALL_TYPES);
	 }
	 else was_preselected = TRUE;

	 if (areawin->selects > 0) {
	    eventmode = (eventmode == CATALOG_MODE) ? CATMOVE_MODE : MOVE_MODE;
	    u2u_snap(&areawin->save);
	    areawin->origin = areawin->save;
	    reset_cycles();
	    select_connected_pins();
	    XDefineCursor(dpy, areawin->window, ARROW);
	    move_mode_draw(xcDRAW_INIT, NULL);
#ifdef TCL_WRAPPER
	    Tk_CreateEventHandler(areawin->area, ButtonMotionMask |
			PointerMotionMask, (Tk_EventProc *)xctk_drag,
			NULL);
#endif
	 }
	 break;
      case XCF_Join:
	 join();
	 break;
      case XCF_Unjoin:
	 unjoin();
	 break;
      case XCF_Spline:
	 splinebutton(x, y);
	 break;
      case XCF_Edit:
	 edit(x, y);
	 break;
      case XCF_Undo:
	 undo_action();
	 break;
      case XCF_Redo:
	 redo_action();
	 break;
      case XCF_Select_Save:
#ifdef TCL_WRAPPER
	 Tcl_Eval(xcinterp, "xcircuit::promptmakeobject");
#else
	 selectsave(NULL, NULL, NULL);
#endif
	 break;
      case XCF_Unselect:
	 select_add_element(-ALL_TYPES);
	 break;
      case XCF_Dashed:
	 setelementstyle(NULL, DASHED, NOBORDER | DOTTED | DASHED);
	 break;
      case XCF_Dotted:
	 setelementstyle(NULL, DOTTED, NOBORDER | DOTTED | DASHED);
	 break;
      case XCF_Solid:
	 setelementstyle(NULL, NORMAL, NOBORDER | DOTTED | DASHED);
	 break;
      case XCF_Prompt:
	 docommand();
	 break;
      case XCF_Dot:
	 snap(x, y, &areawin->save);
	 drawdot(areawin->save.x, areawin->save.y);
	 areawin->redraw_needed = True;
	 drawarea(NULL, NULL, NULL);
	 break;
      case XCF_Wire:
	 u2u_snap(&areawin->save);
	 startwire(&areawin->save);
	 eventmode = WIRE_MODE;
	 break;
      case XCF_Nothing:
	 DoNothing(NULL, NULL, NULL);
	 break;
      case XCF_Exit:
	 quitcheck(areawin->area, NULL, NULL);
	 break;
      case XCF_Netlist:
	 callwritenet(NULL, 0, NULL);
	 break;
      case XCF_Swap:
	 swapschem(0, -1, NULL);
	 break;
      case XCF_Pin_Label:
	 eventmode = TEXT_MODE;
	 textbutton(LOCAL, x, y);
	 break;
      case XCF_Pin_Global:
	 eventmode = TEXT_MODE;
	 textbutton(GLOBAL, x, y);
	 break;
      case XCF_Info_Label:
	 eventmode = TEXT_MODE;
	 textbutton(INFO, x, y);
	 break;
      case XCF_Rescale:
	 if (checkselect(LABEL | OBJINST | GRAPHIC) == TRUE) {
	    eventmode = RESCALE_MODE;
	    rescale_mode_draw(xcDRAW_INIT, NULL);
#ifdef TCL_WRAPPER
	    Tk_CreateEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
	    xcAddEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, False, (xcEventHandler)xlib_drag,
		NULL);
#endif
	 }
	 break;
      case XCF_SelectBox:
	 startselect();
	 break;
      case XCF_Connectivity:
	 connectivity(NULL, NULL, NULL);
	 break;
      case XCF_Copy:
      case XCF_Continue_Copy:
      case XCF_Finish_Copy:
	 copy_op(function, x, y);
	 break;
      case XCF_Continue_Element:
	 if (eventmode == CATMOVE_MODE || eventmode == MOVE_MODE ||
		eventmode == RESCALE_MODE)
	    finish_op(XCF_Finish, x, y);
	 else
	    continue_op(function, x, y);
	 break;
      case XCF_Finish_Element:
      case XCF_Cancel_Last:
      case XCF_Cancel:
	 finish_op(function, x, y);
	 break;
      case XCF_Finish:
	 if (eventmode == CATALOG_MODE || eventmode == ASSOC_MODE)
	    catalog_op(XCF_Library_Pop, x, y);
	 else
	    finish_op(function, x, y);
	 break;
      case XCF_Sim:
	 writenet(topobject, "flatsim", "sim");
	 break;
      case XCF_SPICE:
	 writenet(topobject, "spice", "spc");
	 break;
      case XCF_PCB:
	 writenet(topobject, "pcb", "pcbnet");
	 break;
      case XCF_SPICEflat:
	 writenet(topobject, "flatspice", "fspc");
	 break;
      case XCF_Graphic:
	 Wprintf("Action not handled");
	 result = -1;
	 break;
      case XCF_ChangeStyle:
	 Wprintf("Action not handled");
	 result = -1;
	 break;
   }

   /* Ensure that we do not get stuck in suspend mode	*/
   /* by removing the suspend state whenever a key is	*/
   /* pressed.						*/

   if (xobjs.suspend == 1) {
      xobjs.suspend = -1;
      refresh(NULL, NULL, NULL);
   }
   else if (xobjs.suspend != 2)
      xobjs.suspend = -1;

   return result;
}

/* forward declaration */
extern int utf8_reverse_lookup(char *);

/*------------------------------------------------------*/
/* Get a canonical signature for a button/key event	*/
/*------------------------------------------------------*/

int getkeysignature(XKeyEvent *event)
{
   KeySym keypressed;
   int keywstate;	/* KeySym with prepended state information	*/

   int utf8enc;		/* 8-bit value bound to UTF-8 encoding */
   char buffer[16];
   KeySym keysym;
   Status status;
   static XIM xim = NULL;
   static XIC xic = NULL;

#ifdef _MSC_VER
   if (event->keycode == 0 && event->state == 0)
	   return -1;
#endif
   XLookupString(event, _STR, 150, &keypressed, NULL);

   /* Ignore Shift, Control, Caps Lock, and Meta (Alt) keys 	*/
   /* when pressed alone.					*/

   if (keypressed == XK_Control_L || keypressed == XK_Control_R ||
        keypressed == XK_Alt_L || keypressed == XK_Alt_R ||
        keypressed == XK_Caps_Lock || keypressed == XK_Shift_L ||
	keypressed == XK_Shift_R)
      return -1;

   /* Only keep key state information pertaining to Shift, Caps Lock,	*/
   /* Control, and Alt (Meta)						*/

   keywstate = (keypressed & 0xffff);

   /* Convert codes outside the character (0 - 255) range but within	*/
   /* the ISO-Latin1,...,9 encoding scheme (256-5120).  X11 has unique	*/
   /* keysyms for each character, but the ISO-Latin encodings define	*/
   /* mappings to the 8-bit (256) character set.			*/

   if (keywstate >= 256 && keywstate < 5120)
      keywstate = XKeysymToKeycode(dpy, (KeySym)keywstate);

   if (event->keycode != 0) {		/* Only for actual key events */

	/* Get keyboard input method */
	if (xim == NULL) {
	    xim = XOpenIM(dpy, 0, 0, 0);
	    xic = XCreateIC(xim,
			XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
			XNClientWindow, areawin->window,
			XNFocusWindow,  areawin->window,
			NULL);
	    XSetICFocus(xic);
	}

	/* Do a UTF-8 code lookup */
	Xutf8LookupString(xic, event, buffer, 15, &keysym, &status);
	/* Convert a UTF-8 code to a known encoding */
	utf8enc = utf8_reverse_lookup(buffer);
	if ((utf8enc != -1) && (utf8enc != (keywstate & 0xff))) keywstate = utf8enc;
   }

   /* ASCII values already come upper/lowercase; we only want to register  */
   /* a Shift key if it's a non-ASCII key or another modifier is in effect */

   keywstate |= (((LockMask | ControlMask | Mod1Mask) & event->state) << 16);
   if (keywstate > 255) keywstate |= ((ShiftMask & event->state) << 16);

   /* Treat button events and key events in the same way by setting	*/
   /* a key state for buttons						*/

   if (keypressed == 0)
      keywstate |= (((Button1Mask | Button2Mask | Button3Mask | Button4Mask |
		Button5Mask | ShiftMask)
		& event->state) << 16);

   return keywstate;
}

/*------------------------*/
/* Handle keyboard inputs */
/*------------------------*/

void keyhandler(xcWidget w, caddr_t clientdata, XKeyEvent *event)
{
   int keywstate;	/* KeySym with prepended state information	*/
   int func;
   UNUSED(w); UNUSED(clientdata);

#ifdef TCL_WRAPPER
   if (popups > 0) return;
#else
   if (popups > 0 && help_up == 0) return;
#endif

   if ((event->type == KeyRelease) || (event->type == ButtonRelease)) {

      /* Register a "tap" event if a key or button was released	*/
      /* while a timeout event is pending.			*/

      if (areawin->time_id != 0) {
         xcRemoveTimeOut(areawin->time_id);
         areawin->time_id = 0;
         keywstate = getkeysignature(event);
         eventdispatch(keywstate, areawin->save.x, areawin->save.y);
      }
      else {
         keywstate = getkeysignature(event);
	 if ((pressmode != 0) && (keywstate == pressmode)) {
            /* Events that require hold & drag (namely, MOVE_MODE)	*/
	    /* must be resolved here.  Call finish_op() to ensure	*/
	    /* that we restore xcircuit to	a state of sanity.	*/

	    finish_op(XCF_Finish, event->x, event->y);
	    pressmode = 0;
   	    if (areawin->redraw_needed)
               drawarea(NULL, NULL, NULL);
         }
	 return;	/* Ignore all other release events */
      }
   }

   /* Check if any bindings match key/button "hold".  If so, then start	*/
   /* the timer and wait for key release or timeout.			*/

   else {
      keywstate = getkeysignature(event);
      if ((keywstate != -1) && (xobjs.hold == TRUE)) {

	 /* Establish whether a HOLD modifier binding would apply in	*/
	 /* the current eventmode.  If so, set the HOLD timer.		*/

	 func = boundfunction(areawin->area, keywstate | HOLD_MASK, NULL);
	 if (func != -1) {
            areawin->save.x = event->x;
            areawin->save.y = event->y;
            areawin->time_id = xcAddTimeOut(app, PRESSTIME, 
			makepress, (ClientData)((pointertype)keywstate));
            return;
	 }

      }
      eventdispatch(keywstate, event->x, event->y);
   }
}

/*--------------------------------*/
/* Set snap spacing from keyboard */
/*--------------------------------*/

void setsnap(short direction)
{
   float oldsnap = xobjs.pagelist[areawin->page]->snapspace;
   char buffer[50];

   if (direction > 0) xobjs.pagelist[areawin->page]->snapspace *= 2;
   else {
      if (oldsnap >= 2.0)
         xobjs.pagelist[areawin->page]->snapspace /= 2;
      else {
	 measurestr(xobjs.pagelist[areawin->page]->snapspace, buffer);
	 Wprintf("Snap space at minimum value of %s", buffer);
      }
   }
   if (xobjs.pagelist[areawin->page]->snapspace != oldsnap) {
      measurestr(xobjs.pagelist[areawin->page]->snapspace, buffer);
      Wprintf("Snap spacing set to %s", buffer);
      areawin->redraw_needed = True;
      drawarea(NULL, NULL, NULL);
   }
}

/*-----------------------------------------*/
/* Reposition an object onto the snap grid */
/*-----------------------------------------*/

void snapelement()
{
   short *selectobj;
   Boolean preselected;

   preselected = (areawin->selects > 0) ? TRUE : FALSE;
   if (!checkselect(ALL_TYPES)) return;
   SetForeground(dpy, areawin->gc, BACKGROUND);
   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
	+ areawin->selects; selectobj++) {
      easydraw(*selectobj, DOFORALL);
      switch(SELECTTYPE(selectobj)) {
         case OBJINST: {
	    objinstptr snapobj = SELTOOBJINST(selectobj);

            u2u_snap(&snapobj->position);
	    } break;
         case GRAPHIC: {
	    graphicptr snapg = SELTOGRAPHIC(selectobj);

            u2u_snap(&snapg->position);
	    } break;
         case LABEL: {
	    labelptr snaplabel = SELTOLABEL(selectobj);

	    u2u_snap(&snaplabel->position);
	    } break;
         case POLYGON: {
	    polyptr snappoly = SELTOPOLY(selectobj);
	    pointlist snappoint;

	    for (snappoint = snappoly->points; snappoint < snappoly->points +
	         snappoly->number; snappoint++)
	       u2u_snap(snappoint);
	    } break;
         case ARC: {
	    arcptr snaparc = SELTOARC(selectobj);

	    u2u_snap(&snaparc->position);
	    if (areawin->snapto) {
	       snaparc->radius = (snaparc->radius /
		    xobjs.pagelist[areawin->page]->snapspace) *
		    xobjs.pagelist[areawin->page]->snapspace;
	       snaparc->yaxis = (snaparc->yaxis /
		    xobjs.pagelist[areawin->page]->snapspace) *
	            xobjs.pagelist[areawin->page]->snapspace;
	    }
	    calcarc(snaparc);
	    } break;
	 case SPLINE: {
	    splineptr snapspline = SELTOSPLINE(selectobj);
	    short i;

	    for (i = 0; i < 4; i++)
	       u2u_snap(&snapspline->ctrl[i]);
	    calcspline(snapspline);
	    } break;
      }
      if (preselected || (eventmode != NORMAL_MODE)) {
         SetForeground(dpy, areawin->gc, SELECTCOLOR);
	 easydraw(*selectobj, DOFORALL);
      }
   }
   select_invalidate_netlist();
   if (eventmode == NORMAL_MODE)
      if (!preselected)
         unselect_all();
}

/*----------------------------------------------*/
/* Routines to print the cursor position	*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/* fast integer power-of-10 routine 		*/
/*----------------------------------------------*/

int ipow10(int a)
{
   int i;
   char istr[12];
   
   switch (a) {
      case 0: return 1;     break;
      case 1: return 10;    break;
      case 2: return 100;   break;
      case 3: return 1000;  break;
      default:
         istr[0] = '1';
	 for (i = 1; i < a + 1; i++) istr[i] = '0';
	 istr[i] = '\0';
	 return atoi(istr);
	 break;
   }
}

/*--------------------------------------------------*/
/* find greatest common factor between two integers */
/*--------------------------------------------------*/

int calcgcf(int a, int b)
{
   register int mod;

   if ((mod = a % b) == 0) return (b);
   else return (calcgcf(b, mod));
}

/*--------------------------------------------------------------*/
/* generate a fraction from a float, if possible 		*/
/* fraction returned as a string (must be allocated beforehand)	*/
/*--------------------------------------------------------------*/

void fraccalc(float xyval, char *fstr)
{
   short i, t, rept;
   int ip, mant, divisor, denom, numer, rpart;
   double fp;
   char num[10], *nptr = &num[2], *sptr;
   
   ip = (int)xyval;
   fp = fabs(xyval - ip);

   /* write fractional part and grab mantissa as integer */

   sprintf(num, "%1.7f", fp);
   num[8] = '\0';		/* no rounding up! */
   sscanf(nptr, "%d", &mant);

   if (mant != 0) {    /* search for repeating substrings */
      for (i = 1; i <= 3; i++) {
         rept = 1;
	 nptr = &num[8] - i;
	 while ((sptr = nptr - rept * i) >= &num[2]) {
            for (t = 0; t < i; t++)
	       if (*(sptr + t) != *(nptr + t)) break;
	    if (t != i) break;
	    else rept++;
	 }
	 if (rept > 1) break;
      }
      nptr = &num[8] - i;
      sscanf(nptr, "%d", &rpart);  /* rpart is repeating part of mantissa */
      if (i > 3 || rpart == 0) { /* no repeat */ 
	 divisor = calcgcf(1000000, mant);
	 denom = 1000000 / divisor;
      }
      else { /* repeat */
	 int z, p, fd;

	 *nptr = '\0';
	 sscanf(&num[2], "%d", &z);
	 p = ipow10(i) - 1;
	 mant = z * p + rpart;
	 fd = ipow10(nptr - &num[2]) * p;

	 divisor = calcgcf(fd, mant);
	 denom = fd / divisor;
      }
      numer = mant / divisor;
      if (denom > 1024)
	 sprintf(fstr, "%5.3f", xyval); 
      else if (ip == 0)
         sprintf(fstr, "%hd/%hd", (xyval > 0) ? numer : -numer, denom);
      else
         sprintf(fstr, "%hd %hd/%hd", ip, numer, denom);
   }
   else sprintf(fstr, "%hd", ip);
}

/*------------------------------------------------------------------------------*/
/* Print the position of the cursor in the upper right-hand message window	*/
/*------------------------------------------------------------------------------*/

void printpos(short xval, short yval)
{
   float f1, f2;
   float oscale, iscale = (float)xobjs.pagelist[areawin->page]->drawingscale.y /
	(float)xobjs.pagelist[areawin->page]->drawingscale.x;
   int llen, lwid;
   u_char wlflag = 0;
   XPoint *tpoint, *npoint;
   char *sptr;
   short cycle;

   /* For polygons, print the length (last line of a wire or polygon) or  */
   /* length and width (box only)					  */

   if (eventmode == BOX_MODE || eventmode == EPOLY_MODE || eventmode == WIRE_MODE) {
      polyptr lwire = (eventmode == BOX_MODE) ?  TOPOLY(ENDPART) : TOPOLY(EDITPART);
      if ((eventmode == EPOLY_MODE) && (lwire->number > 2)) {
	 /* sanity check on edit cycle */
	 cycle = (lwire->cycle) ? lwire->cycle->number : -1;
	 if (cycle < 0 || cycle >= lwire->number) {
	    advancecycle((genericptr *)(&lwire), 0);
	    cycle = 0;
	 }
	 tpoint = lwire->points + cycle;
	 npoint = lwire->points + checkcycle((genericptr)lwire, 1);
         llen = wirelength(tpoint, npoint);
	 npoint = lwire->points + checkcycle((genericptr)lwire, -1);
         lwid = wirelength(tpoint, npoint);
         wlflag = 3;
	 if (lwire->style & UNCLOSED) {   /* unclosed polys */
	    if (cycle == 0)
	       wlflag = 1;
	    else if (cycle == lwire->number - 1) {
	       wlflag = 1;
	       llen = lwid;
	    }
	 }
	 if ((npoint->y - tpoint->y) == 0) {	/* swap width and length */
	    int tmp = lwid;
	    lwid = llen;
	    llen = tmp;
	 }
      }
      else if (eventmode == BOX_MODE) {
         tpoint = lwire->points;
         npoint = lwire->points + 1;
         llen = wirelength(tpoint, npoint);
         npoint = lwire->points + 3;
         lwid = wirelength(tpoint, npoint);
	 if ((npoint->y - tpoint->y) == 0) {	/* swap width and length */
	    int tmp = lwid;
	    lwid = llen;
	    llen = tmp;
	 }
	 wlflag = 3;
      }
      else {
         tpoint = lwire->points + lwire->number - 1;
         llen = wirelength(tpoint - 1, tpoint);
         wlflag = 1;
      }
   }
   else if (eventmode == ARC_MODE || eventmode == EARC_MODE) {
      arcptr larc = (eventmode == ARC_MODE) ?  TOARC(ENDPART) : TOARC(EDITPART);
      llen = larc->radius;
      if (abs(larc->radius) != larc->yaxis) {
	 lwid = larc->yaxis;
	 wlflag = 3;
      }
      else
         wlflag = 1;
   }

   switch (xobjs.pagelist[areawin->page]->coordstyle) {
      case INTERNAL:
	 sprintf(_STR, "%g, %g", xval * iscale, yval * iscale);
	 sptr = _STR + strlen(_STR);
	 if (wlflag) {
	    if (wlflag & 2)
	       sprintf(sptr, " (%g x %g)", llen * iscale, lwid * iscale);
	    else
	       sprintf(sptr, " (length %g)", llen * iscale);
	 }
	 break;
      case DEC_INCH:
         oscale = xobjs.pagelist[areawin->page]->outscale * INCHSCALE;
         f1 = ((float)(xval) * iscale * oscale) / 72.0;
         f2 = ((float)(yval) * iscale * oscale) / 72.0;
   	 sprintf(_STR, "%5.3f, %5.3f in", f1, f2);
	 sptr = _STR + strlen(_STR);
	 if (wlflag) {
            f1 = ((float)(llen) * iscale * oscale) / 72.0;
	    if (wlflag & 2) {
               f2 = ((float)(lwid) * iscale * oscale) / 72.0;
	       sprintf(sptr, " (%5.3f x %5.3f in)", f1, f2);
	    }
	    else
	       sprintf(sptr, " (length %5.3f in)", f1);
	 }
	 break;
      case FRAC_INCH: {
	 char fstr1[30], fstr2[30];
	 
         oscale = xobjs.pagelist[areawin->page]->outscale * INCHSCALE;
	 fraccalc((((float)(xval) * iscale * oscale) / 72.0), fstr1); 
         fraccalc((((float)(yval) * iscale * oscale) / 72.0), fstr2);
   	 sprintf(_STR, "%s, %s in", fstr1, fstr2);
	 sptr = _STR + strlen(_STR);
	 if (wlflag) {
	    fraccalc((((float)(llen) * iscale * oscale) / 72.0), fstr1);
	    if (wlflag & 2) {
	       fraccalc((((float)(lwid) * iscale * oscale) / 72.0), fstr2);
	       sprintf(sptr, " (%s x %s in)", fstr1, fstr2);
	    }
	    else
	       sprintf(sptr, " (length %s in)", fstr1);
	 }
	 } break;
      case CM:
         oscale = xobjs.pagelist[areawin->page]->outscale * CMSCALE;
         f1 = ((float)(xval) * iscale * oscale) / IN_CM_CONVERT;
         f2 = ((float)(yval) * iscale * oscale) / IN_CM_CONVERT;
   	 sprintf(_STR, "%5.3f, %5.3f cm", f1, f2);
	 sptr = _STR + strlen(_STR);
	 if (wlflag) {
            f1 = ((float)(llen) * iscale * oscale) / IN_CM_CONVERT;
	    if (wlflag & 2) {
               f2 = ((float)(lwid) * iscale * oscale) / IN_CM_CONVERT;
	       sprintf(sptr, " (%5.3f x %5.3f cm)", f1, f2);
	    }
   	    else
   	       sprintf(sptr, " (length %5.3f cm)", f1);
	 }
	 break;
   }
   W1printf(_STR);
}

/*---------------------------------------------------*/
/* Find nearest point of intersection of the cursor  */
/* position to a wire and move there.		     */
/*---------------------------------------------------*/

void findwirex(XPoint *endpt1, XPoint *endpt2, XPoint *userpt,
		XPoint *newpos, float *rot)
{
   long xsq, ysq, zsq;
   float frac;

   xsq = sqwirelen(endpt1, endpt2);
   ysq = sqwirelen(endpt1, userpt);
   zsq = sqwirelen(endpt2, userpt);
   frac = 0.5 + (float)(ysq - zsq) / (float)(xsq << 1);
   if (frac > 1) frac = 1;
   else if (frac < 0) frac = 0;
   newpos->x = endpt1->x + (int)((endpt2->x - endpt1->x) * frac);
   newpos->y = endpt1->y + (int)((endpt2->y - endpt1->y) * frac);

   *rot = 180.0 + INVRFAC * atan2((double)(endpt1->x -
	  endpt2->x), (double)(endpt1->y - endpt2->y));
}

/*----------------------------------------------------------------*/
/* Find the closest point of attachment from the pointer position */
/* to the "attachto" element.					  */
/*----------------------------------------------------------------*/

void findattach(XPoint *newpos, float *rot, XPoint *userpt)
{
   XPoint *endpt1, *endpt2;
   float frac;
   double tmpang;
   float locrot = 0.0;

   if (rot) locrot = *rot;

   /* find point of intersection and slope */

   if (SELECTTYPE(&areawin->attachto) == ARC) {
      arcptr aarc = SELTOARC(&areawin->attachto);
      float tmpdeg;
      tmpang = atan2((double)(userpt->y - aarc->position.y) * (double)
		(abs(aarc->radius)), (double)(userpt->x - aarc->position.x) *
		(double)aarc->yaxis);

      /* don't follow the arc beyond its endpoints */

      tmpdeg = (float)(tmpang * INVRFAC);
      if (tmpdeg < 0) tmpdeg += 360;
      if (((aarc->angle2 > 360) && (tmpdeg > aarc->angle2 - 360) &&
		(tmpdeg < aarc->angle1)) ||
	  	((aarc->angle1 < 0) && (tmpdeg > aarc->angle2) &&
		(tmpdeg < aarc->angle1 + 360)) ||
	  	((aarc->angle1 >= 0) && (aarc->angle2 <= 360) && ((tmpdeg
		> aarc->angle2) || (tmpdeg < aarc->angle1)))) {
	 float testd1 = aarc->angle1 - tmpdeg;
	 float testd2 = tmpdeg - aarc->angle2;
	 if (testd1 < 0) testd1 += 360;
	 if (testd2 < 0) testd2 += 360;

	 /* if arc is closed, attach to the line between the endpoints */

	 if (!(aarc->style & UNCLOSED)) {
	    XPoint end1, end2;
	    tmpang = (double) aarc->angle1 / INVRFAC;
	    end1.x = aarc->position.x + abs(aarc->radius) * cos(tmpang);
	    end1.y = aarc->position.y + aarc->yaxis  * sin(tmpang);
	    tmpang = (double) aarc->angle2 / INVRFAC;
	    end2.x = aarc->position.x + abs(aarc->radius) * cos(tmpang);
	    end2.y = aarc->position.y + aarc->yaxis  * sin(tmpang);
            findwirex(&end1, &end2, userpt, newpos, &locrot);
	    if (rot) *rot = locrot;
	    return;
	 }
	 else
	    tmpang = (double)((testd1 < testd2) ? aarc->angle1 : aarc->angle2)
			/ INVRFAC;
      }

      /* get position in user coordinates nearest to the intersect pt */

      newpos->x = aarc->position.x + abs(aarc->radius) * cos(tmpang);
      newpos->y = aarc->position.y + aarc->yaxis  * sin(tmpang);

      /* rotation of object is normal to the curve of the ellipse */

      if (rot) {
         *rot = 90.0 - INVRFAC * tmpang;
         if (*rot < 0.0) *rot += 360.0;
      }
   }
   else if (SELECTTYPE(&areawin->attachto) == SPLINE) {
       splineptr aspline = SELTOSPLINE(&areawin->attachto);
       frac = findsplinemin(aspline, userpt);
       findsplinepos(aspline, frac, newpos, &locrot);
       if (rot) *rot = locrot;
   }
   else if (SELECTTYPE(&areawin->attachto) == OBJINST) {

      objinstptr ainst = SELTOOBJINST(&areawin->attachto);
      objectptr aobj = ainst->thisobject;
      genericptr *ggen;
      long testdist, mindist = 1e8;
      XPoint mdpoint;

      /* In case instance has no pin labels, we will attach to	*/
      /* the instance's own origin.				*/

      mdpoint.x = mdpoint.y = 0;
      ReferencePosition(ainst, &mdpoint, newpos);

      /* Find the nearest pin label in the object instance and attach to it */
      for (ggen = aobj->plist; ggen < aobj->plist + aobj->parts; ggen++) {
	 if (ELEMENTTYPE(*ggen) == LABEL) {
	    labelptr alab = TOLABEL(ggen);
	    if (alab->pin == LOCAL || alab->pin == GLOBAL) {
	       ReferencePosition(ainst, &alab->position, &mdpoint);
	       testdist = sqwirelen(&mdpoint, userpt);
	       if (testdist < mindist) {
		  mindist = testdist;
		  *newpos = mdpoint;
	       }
	    }
	 }
      }
   }
   else if (SELECTTYPE(&areawin->attachto) == LABEL) {
      /* Only one choice:  Attach to the label position */
      labelptr alabel = SELTOLABEL(&areawin->attachto);
      newpos->x = alabel->position.x;
      newpos->y = alabel->position.y;
   }
   else if (SELECTTYPE(&areawin->attachto) == POLYGON) {
       polyptr apoly = SELTOPOLY(&areawin->attachto);
       XPoint *testpt, *minpt, *nxtpt;
       long mindist = 1e8, testdist;
       minpt = nxtpt = apoly->points; 	/* so variables aren't uninitialized */
       for (testpt = apoly->points; testpt < apoly->points +
		apoly->number - 1; testpt++) {
	 testdist =  finddist(testpt, testpt + 1, userpt);
	 if (testdist < mindist) {
	    mindist = testdist;
	    minpt = testpt;
	    nxtpt = testpt + 1;
	 }
      }
      if (!(apoly->style & UNCLOSED)) {
	 testdist = finddist(testpt, apoly->points, userpt);
	 if (testdist < mindist) {
	    mindist = testdist;
	    minpt = testpt;
	    nxtpt = apoly->points;
	 }
      }
      endpt1 = minpt;
      endpt2 = nxtpt;
      findwirex(endpt1, endpt2, userpt, newpos, &locrot);
      if (rot) *rot = locrot;
   }
}

/*--------------------------------------------*/
/* Find closest point in a path to the cursor */
/*--------------------------------------------*/

XPoint *pathclosepoint(pathptr dragpath, XPoint *newpos)
{
   XPoint *rpoint;
   genericptr *cpoint;
   short mpoint;
   int mdist = 1000000, tdist;

   for (cpoint = dragpath->plist; cpoint < dragpath->plist + dragpath->parts;
	   cpoint++) {
      switch(ELEMENTTYPE(*cpoint)) {
	 case ARC:
	   tdist = wirelength(&(TOARC(cpoint)->position), newpos);
	    if (tdist < mdist) {
	       mdist = tdist;
	       rpoint = &(TOARC(cpoint)->position);
	    }
	    break;
	 case POLYGON:
	    mpoint = closepoint(TOPOLY(cpoint), newpos);
	    tdist = wirelength(TOPOLY(cpoint)->points + mpoint, newpos);
	    if (tdist < mdist) {
	       mdist = tdist;
	       rpoint = TOPOLY(cpoint)->points + mpoint;
	    }
	    break;
	 case SPLINE:
	    tdist = wirelength(&(TOSPLINE(cpoint)->ctrl[0]), newpos);
	    if (tdist < mdist) {
	       mdist = tdist;
	       rpoint = &(TOSPLINE(cpoint)->ctrl[0]);
	    }
	    tdist = wirelength(&(TOSPLINE(cpoint)->ctrl[3]), newpos);
	    if (tdist < mdist) {
	       mdist = tdist;
	       rpoint = &(TOSPLINE(cpoint)->ctrl[3]);
	    }
	    break;
      }
   }
   return rpoint;
}

/*-------------------------------------------*/
/* Drag a selected element around the screen */
/*-------------------------------------------*/

void drag(int x, int y)
{
   XEvent again;
   Boolean eventcheck = False;
   XPoint userpt;
   short deltax, deltay;
   int locx, locy;

   locx = x;
   locy = y;

   /* flush out multiple pointermotion events from the event queue */
   /* use only the last motion event */
   while (XCheckWindowEvent(dpy, areawin->window, PointerMotionMask |
	Button1MotionMask, &again) == True) eventcheck = True;
   if (eventcheck) {
      XButtonEvent *event = (XButtonEvent *)(&again);
      locx = (int)event->x;
      locy = (int)event->y;
   }

   /* Determine if this event is supposed to be handled by 	*/
   /* trackselarea(), or whether we should not be here at all	*/
   /* (button press and mouse movement in an unsupported mode)	*/

   if (eventmode == SELAREA_MODE) {
      trackselarea();
      return;
   }
   else if (eventmode == RESCALE_MODE) {
      trackrescale();
      return;
   }
   else if (eventmode == PAN_MODE) {
      trackpan(locx, locy);
      return;
   }
   else if (eventmode != CATMOVE_MODE && eventmode != MOVE_MODE
		&& eventmode != COPY_MODE)
      return;

   snap(locx, locy, &userpt);
   deltax = userpt.x - areawin->save.x;
   deltay = userpt.y - areawin->save.y;
   if (deltax == 0 && deltay == 0) return;

   areawin->save.x = userpt.x;
   areawin->save.y = userpt.y;

   /* set up the graphics state for moving a selected object */

   XTopSetForeground(SELECTCOLOR);

   placeselects(deltax, deltay, &userpt);

   /* restore graphics state */

   SetForeground(dpy, areawin->gc, areawin->gccolor);

   /* print the position and other useful measurements */

   printpos(userpt.x, userpt.y);
}

/*------------------------------------------------------*/
/* Wrapper for drag() for xlib callback compatibility.	*/
/*------------------------------------------------------*/

void xlib_drag(xcWidget w, caddr_t clientdata, XEvent *event)
{
   XButtonEvent *bevent = (XButtonEvent *)event;
   UNUSED(w); UNUSED(clientdata);

   drag(bevent->x, bevent->y);
   if (areawin->redraw_needed)
     drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------*/
/* Rotate an element of a path			*/
/*----------------------------------------------*/

void elemrotate(genericptr *genobj, float direction, XPoint *position)
{
   XPoint negpt, *newpts = (XPoint *)NULL;

   negpt.x = -position->x;
   negpt.y = -position->y;

   switch(ELEMENTTYPE(*genobj)) {
      case(ARC):{
	 arcptr rotatearc = TOARC(genobj);
	 rotatearc->angle1 -= direction;
	 rotatearc->angle2 -= direction;
         if (rotatearc->angle1 >= 360) {
            rotatearc->angle1 -= 360;
            rotatearc->angle2 -= 360;
         }
         else if (rotatearc->angle2 <= 0) {
            rotatearc->angle1 += 360;
            rotatearc->angle2 += 360;
         } 
	 newpts = (XPoint *)malloc(sizeof(XPoint));
	 UTransformPoints(&rotatearc->position, newpts, 1, negpt, 1.0, 0);
	 UTransformPoints(newpts, &rotatearc->position, 1, *position,
			1.0, direction);
	 calcarc(rotatearc);
	 }break;

      case(SPLINE):{
	 splineptr rotatespline = TOSPLINE(genobj);
	 newpts = (XPoint *)malloc(4 * sizeof(XPoint));
	 UTransformPoints(rotatespline->ctrl, newpts, 4, negpt, 1.0, 0);
	 UTransformPoints(newpts, rotatespline->ctrl, 4, *position,
			1.0, direction);
	 calcspline(rotatespline);
	 }break;

      case(POLYGON):{
	 polyptr rotatepoly = TOPOLY(genobj);
	 newpts = (XPoint *)malloc(rotatepoly->number * sizeof(XPoint));
	 UTransformPoints(rotatepoly->points, newpts, rotatepoly->number,
		   negpt, 1.0, 0);
	 UTransformPoints(newpts, rotatepoly->points, rotatepoly->number,
		   *position, 1.0, direction);
	 }break;
   }
   if (newpts) free(newpts);
}

/*------------------------------------------------------*/
/* Rotate an element or group of elements		*/
/* Objects and labels, if selected singly, rotate	*/
/* about their position point.  All other elements,	*/
/* and groups, rotate about the cursor point.		*/
/*------------------------------------------------------*/

void elementrotate(float direction, XPoint *position)
{
  short    *selectobj; /* , ld; (jdk) */
   Boolean  single = False;
   Boolean  need_refresh = False;
   Boolean  preselected;
   XPoint   newpt, negpt;

   preselected = (areawin->selects > 0) ? TRUE : FALSE;
   if (!checkselect(ALL_TYPES)) return;
   if (areawin->selects == 1) single = True;

   negpt.x = -position->x;
   negpt.y = -position->y;

   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
	+ areawin->selects; selectobj++) { 

      /* erase the element */
      if (!need_refresh) {
         SetForeground(dpy, areawin->gc, BACKGROUND);
         easydraw(*selectobj, DOFORALL);
      }

      switch(SELECTTYPE(selectobj)) {

	 case(OBJINST):{
            objinstptr rotateobj = SELTOOBJINST(selectobj);

	    if (is_library(topobject) >= 0 && !is_virtual(rotateobj)) break;
            rotateobj->rotation += direction;
	    while (rotateobj->rotation >= 360) rotateobj->rotation -= 360;
	    while (rotateobj->rotation <= 0) rotateobj->rotation += 360;
	    if (!single) {
	       UTransformPoints(&rotateobj->position, &newpt, 1, negpt, 1.0, 0);
	       UTransformPoints(&newpt, &rotateobj->position, 1, *position,
			1.0, direction);
	    }
	    }break;

	 case(LABEL):{
            labelptr rotatetext = SELTOLABEL(selectobj);

            rotatetext->rotation += direction;
	    while (rotatetext->rotation >= 360) rotatetext->rotation -= 360;
	    while (rotatetext->rotation <= 0) rotatetext->rotation += 360;
	    if (!single) {
	       UTransformPoints(&rotatetext->position, &newpt, 1, negpt, 1.0, 0);
	       UTransformPoints(&newpt, &rotatetext->position, 1, *position,
			1.0, direction);
	    }
	    }break;

	 case(GRAPHIC):{
            graphicptr rotateg = SELTOGRAPHIC(selectobj);

            rotateg->rotation += direction;
	    while (rotateg->rotation >= 360) rotateg->rotation -= 360;
	    while (rotateg->rotation <= 0) rotateg->rotation += 360;
#ifndef HAVE_CAIRO
	    rotateg->valid = FALSE;
#endif /* !HAVE_CAIRO */
	    if (!single) {
	       UTransformPoints(&rotateg->position, &newpt, 1, negpt, 1.0, 0);
	       UTransformPoints(&newpt, &rotateg->position, 1, *position,
			1.0, direction);
	    }
	    need_refresh = True;
	    }break;

	 case POLYGON: case ARC: case SPLINE:{
	    genericptr *genpart = topobject->plist + *selectobj;
	    register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			*genpart);
	    elemrotate(genpart, direction, position);
	    }break;

	 case PATH:{
	    genericptr *genpart;
	    pathptr rotatepath = SELTOPATH(selectobj);

	    register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			rotatepath);
	    for (genpart = rotatepath->plist; genpart < rotatepath->plist
		  + rotatepath->parts; genpart++)
	       elemrotate(genpart, direction, position);
	    }break;
      }

      /* redisplay the element */
      if (preselected || ((eventmode != NORMAL_MODE) && !need_refresh)) {
	 SetForeground(dpy, areawin->gc, SELECTCOLOR);
	 easydraw(*selectobj, DOFORALL);
      }
   }

   /* This takes care of all selected instances and labels in one go,	*/
   /* because we only need to know the origin and amount of rotation.	*/

   if (eventmode != COPY_MODE)
      register_for_undo(XCF_Rotate, UNDO_MORE, areawin->topinstance,
		(eventmode == MOVE_MODE) ? &areawin->origin : position,
		(double)direction);

   /* New rule (6/15/07) to be applied generally:  If objects were	*/
   /* selected prior to calling elementrotate() and similar functions,	*/
   /* leave them selected upon exit.  Otherwise, deselect them.		*/

   if (eventmode == NORMAL_MODE || eventmode == CATALOG_MODE)
      if (!preselected)
         unselect_all();

   if (eventmode == CATALOG_MODE) {
      int libnum;
      if ((libnum = is_library(topobject)) >= 0) {
	 composelib(libnum + LIBRARY);
	 need_refresh = TRUE;
      }
   }
   else {
      pwriteback(areawin->topinstance);
      calcbbox(areawin->topinstance);
   }

   if (need_refresh) drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------*/
/* Rescale the current edit element to the	*/
/* dimensions of the rescale box.		*/
/*----------------------------------------------*/

void elementrescale(float newscale)
{
   short *selectobj;
   labelptr sclab;
   objinstptr scinst;
   graphicptr scgraph;
   float oldsize;

   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
		+ areawin->selects; selectobj++) {
      switch (SELECTTYPE(selectobj)) {
	 case LABEL:
	    sclab = SELTOLABEL(selectobj);
	    oldsize = sclab->scale;
	    sclab->scale = newscale;
	    break;
	 case OBJINST:
	    scinst = SELTOOBJINST(selectobj);
	    oldsize = scinst->scale;
	    scinst->scale = newscale;
	    break;
	 case GRAPHIC:
	    scgraph = SELTOGRAPHIC(selectobj);
	    oldsize = scgraph->scale;
	    scgraph->scale = newscale;
	    break;
      }
      register_for_undo(XCF_Rescale, UNDO_MORE, areawin->topinstance,
             SELTOGENERIC(selectobj), (double)oldsize);
   }
   calcbbox(areawin->topinstance);
}

/*-------------------------------------------------*/
/* Edit an element in an element-dependent fashion */
/*-------------------------------------------------*/

void edit(int x, int y)
{
   short *selectobj;

   if (areawin->selects == 0) {
      Boolean saveredraw = areawin->redraw_needed;
      selectobj = select_element(ALL_TYPES);
      areawin->redraw_needed = saveredraw;
   }
   else
      selectobj = areawin->selectlist;
   if (areawin->selects == 0)
      return;
   else if (areawin->selects != 1) {	/* Multiple object edit */
      int selnum;
      short *selectlist, selrefno;
      Boolean save_redraw = areawin->redraw_needed;

      /* Find the closest part to use as a reference */
      selnum = areawin->selects;
      selectlist = areawin->selectlist;
      areawin->selects = 0;
      areawin->selectlist = NULL;
      selectobj = select_element(ALL_TYPES);
      if (selectobj != NULL)
         selrefno = *selectobj;
      else
	 selrefno = -1;
      freeselects();
      areawin->selects = selnum;
      areawin->selectlist = selectlist;
      areawin->redraw_needed = save_redraw;
      for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
		+ areawin->selects; selectobj++) {
	 if (*selectobj == selrefno) break;
      }
      if (selectobj == areawin->selectlist + areawin->selects) {
         Wprintf("Put cursor close to the reference element.");
	 return;
      }

      /* Shuffle the reference element to the beginning of the select list */
      *selectobj = *(areawin->selectlist);
      *(areawin->selectlist) = selrefno;
      selectobj = areawin->selectlist;
   }

   switch(SELECTTYPE(selectobj)) {
       case LABEL: {
	 labelptr *lastlabel = (labelptr *)EDITPART;
	 short curfont;
	 XPoint tmppt;
	 TextExtents tmpext;
	
	 /* save the old string, including parameters */
	 register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			*lastlabel);

	 /* fill any NULL instance parameters with the default value */
	 copyparams(areawin->topinstance, areawin->topinstance);

	 /* place text cursor line at point nearest the cursor */
	 /* unless textend is set. . .			       */
 
	 if (areawin->textend == 0) {
	    TextLinesInfo tlinfo;
	    tlinfo.dostop = 0;
	    tlinfo.tbreak = NULL;
	    tlinfo.padding = NULL;

	    window_to_user(x, y, &areawin->save);
	    InvTransformPoints(&areawin->save, &tmppt, 1, (*lastlabel)->position,
		(*lastlabel)->scale, (*lastlabel)->rotation);
            tmpext = ULength(*lastlabel, areawin->topinstance, &tlinfo);
	    tmppt.x += ((*lastlabel)->anchor & NOTLEFT ?
		((*lastlabel)->anchor & RIGHT ? tmpext.maxwidth
		: tmpext.maxwidth >> 1) : 0);
	    tmppt.y += ((*lastlabel)->anchor & NOTBOTTOM ?
		((*lastlabel)->anchor & TOP ? tmpext.ascent :
		(tmpext.ascent + tmpext.base) >> 1) : tmpext.base);
	    if ((*lastlabel)->pin)
	       pinadjust((*lastlabel)->anchor, &tmppt.x, NULL, -1);

	    /* Where tbreak is passed to ULength, the character position */
	    /* is returned in the TextLinesInfo dostop field.		 */
	    tlinfo.tbreak = &tmppt;
            tmpext = ULength(*lastlabel, areawin->topinstance, &tlinfo);
	    areawin->textpos = tlinfo.dostop;

	    if (tlinfo.padding != NULL) free(tlinfo.padding);
	 }

	 /* find current font */

	 curfont = findcurfont(areawin->textpos, (*lastlabel)->string,
			areawin->topinstance);

	 /* change menu buttons accordingly */

	 setfontmarks(curfont, (*lastlabel)->anchor);

	 if (eventmode == CATALOG_MODE) {
	    /* CATTEXT_MODE may show an otherwise hidden library namespace */
	    undrawtext(*lastlabel);
	    eventmode = CATTEXT_MODE;
	    redrawtext(*lastlabel);
            areawin->redraw_needed = False; /* ignore prev. redraw requests */
	    text_mode_draw(xcDRAW_INIT, *lastlabel);
	 }
	 else {
	    eventmode = ETEXT_MODE;
	    text_mode_draw(xcDRAW_INIT, *lastlabel);
	 }

         XDefineCursor(dpy, areawin->window, TEXTPTR);

	 /* write the text at the bottom */

	 charreport(*lastlabel);

      } break;

      case POLYGON: case ARC: case SPLINE: case PATH:
	 window_to_user(x, y, &areawin->save);
	 pathedit(*(EDITPART));
	 break;

      case OBJINST: case GRAPHIC:
	 if (areawin->selects == 1)
	    unselect_all();
	 return;
   }
   XDefineCursor (dpy, areawin->window, EDCURSOR);
}

/*----------------------------------------------------------------------*/
/* edit() routine for path-type elements (polygons, splines, arcs, and	*/
/* paths)								*/
/*----------------------------------------------------------------------*/

void pathedit(genericptr editpart)
{
   splineptr lastspline = NULL;
   pathptr lastpath;
   polyptr lastpoly = NULL;
   arcptr lastarc;
   XPoint *savept;
   short *eselect;
   int cycle;
   Boolean havecycle;

   /* Find and set constrained edit points on all elements.  Register	*/
   /* each element with the undo mechanism.				*/

   for (eselect = areawin->selectlist; eselect < areawin->selectlist +
			areawin->selects; eselect++) {
      switch (SELECTTYPE(eselect)) {
	 case POLYGON:
	    findconstrained(SELTOPOLY(eselect));
	    /* fall through */
	 default:
            register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			SELTOGENERIC(eselect));
	    break;
      }
   }

   switch(ELEMENTTYPE(editpart)) {
      case PATH: {
	 genericptr *ggen, *savegen = NULL;
	 int mincycle, dist, mindist = 1e6;

         lastpath = (pathptr)editpart;
         havecycle = (checkcycle(editpart, 0) >= 0) ? True : False;

	 /* determine which point of the path is closest to the cursor */
	 for (ggen = lastpath->plist; ggen < lastpath->plist + lastpath->parts;
		ggen++) {
	    switch (ELEMENTTYPE(*ggen)) {
	       case POLYGON:
		  lastpoly = TOPOLY(ggen);
		  cycle = closepoint(lastpoly, &areawin->save);
		  dist = wirelength(lastpoly->points + cycle, &areawin->save);
		  break;
	       case SPLINE:
		  lastspline = TOSPLINE(ggen);
		  cycle =  (wirelength(&lastspline->ctrl[0],
			&areawin->save) < wirelength(&lastspline->ctrl[3],
			&areawin->save)) ? 0 : 3;
		  dist = wirelength(&lastspline->ctrl[cycle], &areawin->save);
		  break;
	    }
	    if (dist < mindist) {
	       mindist = dist;
	       mincycle = cycle;
	       savegen = ggen;
	    }
	 }
	 if (savegen == NULL) return;	/* something went terribly wrong */
	 switch (ELEMENTTYPE(*savegen)) {
	    case POLYGON:
	       lastpoly = TOPOLY(savegen);
	       addcycle(savegen, mincycle, 0);
	       savept = lastpoly->points + mincycle;
	       makerefcycle(lastpoly->cycle, mincycle);
	       findconstrained(lastpoly);
	       break;
	    case SPLINE:
	       lastspline = TOSPLINE(savegen);
	       addcycle(savegen, mincycle, 0);
	       savept = &lastspline->ctrl[mincycle];
	       makerefcycle(lastspline->cycle, mincycle);
	       break;
	 }
	 updatepath(lastpath);
	 if (!havecycle)
            register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			(genericptr)lastpath);
	 patheditpush(lastpath);
	 areawin->origin = areawin->save;
	 checkwarp(savept);

	 path_mode_draw(xcDRAW_INIT, lastpath);
	 
	 xcAddEventHandler(areawin->area, PointerMotionMask, False,
	    (xcEventHandler)trackelement, NULL);
	 eventmode = EPATH_MODE;
         printpos(savept->x, savept->y);

      } break;
      case POLYGON: {

	 lastpoly = (polyptr)editpart;

	 /* Determine which point of polygon is closest to cursor */
	 cycle = closepoint(lastpoly, &areawin->save);
	 havecycle = (lastpoly->cycle == NULL) ? False : True;
	 addcycle(&editpart, cycle, 0);
         savept = lastpoly->points + cycle;
	 makerefcycle(lastpoly->cycle, cycle);
	 if (!havecycle) {
	    findconstrained(lastpoly);
            register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			(genericptr)lastpoly);
 	 }

	 /* Push onto the editstack */
	 polyeditpush(lastpoly);

	 /* remember our postion for pointer restore */
	 areawin->origin = areawin->save;

	 checkwarp(savept);

	 poly_mode_draw(xcDRAW_INIT, lastpoly);

	 xcAddEventHandler(areawin->area, PointerMotionMask, False,
	    (xcEventHandler)trackelement, NULL);
	 eventmode = EPOLY_MODE;
	 printeditbindings();
         printpos(savept->x, savept->y);
      } break;
      case SPLINE: {
	 XPoint *curpt;

	 lastspline = (splineptr)editpart;

	 /* find which point is closest to the cursor */

         cycle =  (wirelength(&lastspline->ctrl[0],
	      &areawin->save) < wirelength(&lastspline->ctrl[3],
	      &areawin->save)) ? 0 : 3;
	 havecycle = (lastspline->cycle == NULL) ? False : True;
	 addcycle(&editpart, cycle, 0);
	 makerefcycle(lastspline->cycle, cycle);
	 curpt = &lastspline->ctrl[cycle];
	 if (!havecycle)
            register_for_undo(XCF_Edit, UNDO_MORE, areawin->topinstance,
			(genericptr)lastspline);

	 /* Push onto the editstack */
	 splineeditpush(lastspline);

	 /* remember our postion for pointer restore */
	 areawin->origin = areawin->save;

         checkwarp(curpt);

	 spline_mode_draw(xcDRAW_INIT, lastspline);
	 xcAddEventHandler(areawin->area, PointerMotionMask, False,
               (xcEventHandler)trackelement, NULL);
	 eventmode = ESPLINE_MODE;
      } break;
      case ARC: {
	 XPoint curpt;
	 float tmpratio, tlen;

	 lastarc = (arcptr)editpart;

	 /* find a part of the arc close to the pointer */

	 tlen = (float)wirelength(&areawin->save, &(lastarc->position));
	 tmpratio = (float)(abs(lastarc->radius)) / tlen;
	 curpt.x = lastarc->position.x + tmpratio * (areawin->save.x
	     - lastarc->position.x);
	 tmpratio = (float)lastarc->yaxis / tlen;
	 curpt.y = lastarc->position.y + tmpratio * (areawin->save.y
	     - lastarc->position.y);
	 addcycle(&editpart, 0, 0);
	 saveratio = (double)(lastarc->yaxis) / (double)(abs(lastarc->radius));
	 
	 /* Push onto the editstack */
	 arceditpush(lastarc);
	 areawin->origin = areawin->save;

	 checkwarp(&curpt);

	 areawin->save.x = curpt.x;	/* for redrawing dotted edit line */
	 areawin->save.y = curpt.y;
	 arc_mode_draw(xcDRAW_INIT, lastarc);
	 xcAddEventHandler(areawin->area, PointerMotionMask, False,
	    (xcEventHandler)trackarc, NULL);
	 eventmode = EARC_MODE;
         printpos(curpt.x, curpt.y);
      } break;
   }
}

/*------------------------------------------------------*/
/* Raise an element to the top of the list	   	*/
/*------------------------------------------------------*/

void xc_top(short *selectno, short *orderlist)
{
   short i;
   genericptr *raiseobj, *genobj, temp;

   raiseobj = topobject->plist + *selectno;
   temp = *raiseobj;
   i = *selectno;
   for (genobj = topobject->plist + *selectno; genobj <
		topobject->plist + topobject->parts - 1; genobj++) {
      *genobj = *(genobj + 1);
      *(orderlist + i) = *(orderlist + i + 1);
      i++;
   }
   *(topobject->plist + topobject->parts - 1) = temp;
   *(orderlist + topobject->parts - 1) = *selectno;
   *selectno = topobject->parts - 1;
}

/*------------------------------------------------------*/
/* Lower an element to the bottom of the list	   	*/
/*------------------------------------------------------*/

void xc_bottom(short *selectno, short *orderlist)
{
   short i;
   genericptr *lowerobj, *genobj, temp;

   lowerobj = topobject->plist + *selectno;
   temp = *lowerobj;
   i = *selectno;
   for (genobj = topobject->plist + *selectno;
		genobj > topobject->plist; genobj--) {
      *genobj = *(genobj - 1);
      *(orderlist + i) = *(orderlist + i - 1);
      i--;
   }
   *genobj = temp;
   *orderlist = *selectno;
   *selectno = 0;
}

/*--------------------------------------------------------------*/
/* Raise all selected elements by one position in the list	*/
/*--------------------------------------------------------------*/

void xc_raise()
{
   short *sel, topsel, maxsel, *topidx, limit, *orderlist, i;
   genericptr *raiseobj, temp;

   orderlist = (short *)malloc(topobject->parts * sizeof(short));
   for (i = 0; i < topobject->parts; i++) *(orderlist + i) = i;

   /* Find topmost element in the select list */
   maxsel = -1;
   for (sel = areawin->selectlist; sel < areawin->selectlist + areawin->selects;
		sel++) {
      if (*sel > maxsel) {
	 maxsel = *sel;
	 topidx = sel;
      }
   }
   if (maxsel == -1) return;	/* Error condition */

   topsel = maxsel;
   limit = topobject->parts - 1;
   while (1) {

      /* Exchange the topmost element with the one above it */
      if (topsel < limit) {
         raiseobj = topobject->plist + topsel;
         temp = *raiseobj;
         *raiseobj = *(raiseobj + 1);
         *(raiseobj + 1) = temp;
	 (*topidx)++;
	 i = *(orderlist + topsel);
	 *(orderlist + topsel) = *(orderlist + topsel + 1);
	 *(orderlist + topsel + 1) = i;
      }
      else
	 limit = topsel - 1;

      /* Find next topmost element */
      topsel = -1;
      for (sel = areawin->selectlist; sel < areawin->selectlist + areawin->selects;
		sel++) {
	 if (*sel < maxsel) {
	    if (*sel > topsel) {
	       topsel = *sel;
	       topidx = sel;
	    }
	 }
      }
      if (topsel == -1) break;	/* No more elements to raise */
      maxsel = topsel;
   }
   register_for_undo(XCF_Reorder, UNDO_MORE, areawin->topinstance, orderlist,
		topobject->parts);
}

/*--------------------------------------------------------------*/
/* Lower all selected elements by one position in the list	*/
/*--------------------------------------------------------------*/

void xc_lower()
{
   short *sel, botsel, minsel, *botidx, limit, *orderlist, i;
   genericptr *lowerobj, temp;

   orderlist = (short *)malloc(topobject->parts * sizeof(short));
   for (i = 0; i < topobject->parts; i++) *(orderlist + i) = i;

   /* Find bottommost element in the select list */
   minsel = topobject->parts;
   for (sel = areawin->selectlist; sel < areawin->selectlist + areawin->selects;
		sel++) {
      if (*sel < minsel) {
	 minsel = *sel;
	 botidx = sel;
      }
   }
   if (minsel == topobject->parts) return;	/* Error condition */

   botsel = minsel;
   limit = 0;
   while (1) {

      /* Exchange the topmost element with the one below it */
      if (botsel > limit) {
         lowerobj = topobject->plist + botsel;
         temp = *lowerobj;
         *lowerobj = *(lowerobj - 1);
         *(lowerobj - 1) = temp;
	 (*botidx)--;
	 i = *(orderlist + botsel);
	 *(orderlist + botsel) = *(orderlist + botsel - 1);
	 *(orderlist + botsel - 1) = i;
      }
      else
	 limit = botsel + 1;

      /* Find next topmost element */
      botsel = topobject->parts;
      for (sel = areawin->selectlist; sel < areawin->selectlist + areawin->selects;
		sel++) {
	 if (*sel > minsel) {
	    if (*sel < botsel) {
	       botsel = *sel;
	       botidx = sel;
	    }
	 }
      }
      if (botsel == topobject->parts) break;	/* No more elements to raise */
      minsel = botsel;
   }
   register_for_undo(XCF_Reorder, UNDO_MORE, areawin->topinstance, orderlist,
		topobject->parts);
}

/*------------------------------------------------------*/
/* Generate a virtual copy of an object instance in the	*/ 
/* user library.  This is like the library virtual copy	*/
/* except that it allows the user to generate a library	*/
/* copy of an existing instance, without having to make	*/
/* a copy of the master library instance and edit it.	*/
/* copyvirtual() also allows the library virtual	*/
/* instance to take on a specific rotation or flip	*/
/* value, which cannot be done with the library virtual	*/
/* copy function.					*/
/*------------------------------------------------------*/

void copyvirtual()
{
   short *selectno, created = 0;
   objinstptr vcpobj, libinst;

   for (selectno = areawin->selectlist; selectno < areawin->selectlist +
		areawin->selects; selectno++) {
      if (SELECTTYPE(selectno) == OBJINST) {
	 vcpobj = SELTOOBJINST(selectno);
	 libinst = addtoinstlist(USERLIB - LIBRARY, vcpobj->thisobject, TRUE);
	 instcopy(libinst, vcpobj);
	 created++;
      }
   }
   if (created == 0) {
      Wprintf("No object instances selected for virtual copy!");
   }
   else {
      unselect_all();
      composelib(USERLIB);
   }
}

/*------------------------------------------------------*/
/* Exchange the list position (drawing order) of two	*/
/* elements, or move the position of one element to the	*/
/* top or bottom.					*/
/*------------------------------------------------------*/

void exchange()
{
   short *selectno, *orderlist, i;
   genericptr *exchobj, *exchobj2, temp;
   Boolean preselected;

   preselected = (areawin->selects > 0) ? TRUE : FALSE;
   if (!checkselect(ALL_TYPES)) {
      Wprintf("Select 1 or 2 objects");
      return;
   }

   selectno = areawin->selectlist;
   orderlist = (short *)malloc(topobject->parts * sizeof(short));
   for (i = 0; i < topobject->parts; i++) *(orderlist + i) = i;

   if (areawin->selects == 1) {  /* lower if on top; raise otherwise */
      if (*selectno == topobject->parts - 1)
	 xc_bottom(selectno, orderlist);
      else
	 xc_top(selectno, orderlist);
   }
   else {  /* exchange the two objects */
      exchobj = topobject->plist + *selectno;
      exchobj2 = topobject->plist + *(selectno + 1);

      temp = *exchobj;
      *exchobj = *exchobj2;
      *exchobj2 = temp;

      i = *(orderlist + *selectno);
      *(orderlist + *selectno) = *(orderlist + *(selectno + 1));
      *(orderlist + *(selectno + 1)) = i;
   }
   register_for_undo(XCF_Reorder, UNDO_MORE, areawin->topinstance,
		orderlist, topobject->parts);

   incr_changes(topobject);
   if (!preselected)
      clearselects();
   drawarea(NULL, NULL, NULL);
}

/*--------------------------------------------------------*/
/* Flip an element horizontally (POLYGON, ARC, or SPLINE) */
/*--------------------------------------------------------*/

void elhflip(genericptr *genobj, short x)
{
   switch(ELEMENTTYPE(*genobj)) {
      case POLYGON:{
	 polyptr flippoly = TOPOLY(genobj);
	 pointlist ppoint;
	 for (ppoint = flippoly->points; ppoint < flippoly->points +
	       flippoly->number; ppoint++)
	    ppoint->x = (x << 1) - ppoint->x;	
	 }break;

      case ARC:{
	 arcptr fliparc = TOARC(genobj);
	 float tmpang = 180 - fliparc->angle1;
	 fliparc->angle1 = 180 - fliparc->angle2;
	 fliparc->angle2 = tmpang;
	 if (fliparc->angle2 < 0) {
	    fliparc->angle1 += 360;
	    fliparc->angle2 += 360;
	 }
	 fliparc->radius = -fliparc->radius;
	 fliparc->position.x = (x << 1) - fliparc->position.x;
	 calcarc(fliparc);
	 }break;

      case SPLINE:{
	 splineptr flipspline = TOSPLINE(genobj);
	 int i;
	 for (i = 0; i < 4; i++)
	    flipspline->ctrl[i].x = (x << 1) - flipspline->ctrl[i].x;
	 calcspline(flipspline);
	 }break;
   }
}

/*--------------------------------------------------------*/
/* Flip an element vertically (POLYGON, ARC, or SPLINE)   */
/*--------------------------------------------------------*/

void elvflip(genericptr *genobj, short y)
{
   switch(ELEMENTTYPE(*genobj)) {

      case POLYGON:{
	 polyptr flippoly = TOPOLY(genobj);
	 pointlist ppoint;

	 for (ppoint = flippoly->points; ppoint < flippoly->points +
	    	   flippoly->number; ppoint++)
	    ppoint->y = (y << 1) - ppoint->y;	
	 }break;

      case ARC:{
	 arcptr fliparc = TOARC(genobj);
	 float tmpang = 360 - fliparc->angle1;
	 fliparc->angle1 = 360 - fliparc->angle2;
	 fliparc->angle2 = tmpang;
	 if (fliparc->angle1 >= 360) {
	    fliparc->angle1 -= 360;
	    fliparc->angle2 -= 360;
	 }
	 fliparc->radius = -fliparc->radius;
	 fliparc->position.y = (y << 1) - fliparc->position.y;
	 calcarc(fliparc);
	 }break;

      case SPLINE:{
	 splineptr flipspline = TOSPLINE(genobj);
	 int i;
	 for (i = 0; i < 4; i++)
	    flipspline->ctrl[i].y = (y << 1) - flipspline->ctrl[i].y;
	 calcspline(flipspline);
	 }break;
   }
}

/*------------------------------------------------------*/
/* Horizontally flip an element				*/
/*------------------------------------------------------*/

void elementflip(XPoint *position)
{
   short *selectobj;
   Boolean single = False;
   Boolean preselected;

   preselected = (areawin->selects > 0) ? TRUE : FALSE;
   if (!checkselect(ALL_TYPES)) return;
   if (areawin->selects == 1) single = True;

   if (eventmode != COPY_MODE)
      register_for_undo(XCF_Flip_X, UNDO_MORE, areawin->topinstance,
		(eventmode == MOVE_MODE) ? &areawin->origin : position);

   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
	+ areawin->selects; selectobj++) {

      /* erase the object */
      SetForeground(dpy, areawin->gc, BACKGROUND);
      easydraw(*selectobj, DOFORALL);

      switch(SELECTTYPE(selectobj)) {
	 case LABEL:{
	    labelptr fliplab = SELTOLABEL(selectobj);
	    if ((fliplab->anchor & (RIGHT | NOTLEFT)) != NOTLEFT)
	       fliplab->anchor ^= (RIGHT | NOTLEFT);
	    if (!single)
	       fliplab->position.x = (position->x << 1) - fliplab->position.x;
	    }break;
	 case GRAPHIC:{
	    graphicptr flipg = SELTOGRAPHIC(selectobj);
	    flipg->scale = -flipg->scale;
#ifndef HAVE_CAIRO
	    flipg->valid = FALSE;
#endif /* !HAVE_CAIRO */
	    if (!single)
	       flipg->position.x = (position->x << 1) - flipg->position.x;
	    }break;
	 case OBJINST:{
            objinstptr flipobj = SELTOOBJINST(selectobj);
	    if (is_library(topobject) >= 0 && !is_virtual(flipobj)) break;
   	    flipobj->scale = -flipobj->scale;
	    if (!single)
	       flipobj->position.x = (position->x << 1) - flipobj->position.x;
	    }break;
	 case POLYGON: case ARC: case SPLINE:
	    elhflip(topobject->plist + *selectobj, position->x);
	    break;
	 case PATH:{
	    genericptr *genpart;
	    pathptr flippath = SELTOPATH(selectobj);

	    for (genpart = flippath->plist; genpart < flippath->plist
		  + flippath->parts; genpart++)
	       elhflip(genpart, position->x);
	    }break;
      }

      if (preselected || (eventmode != NORMAL_MODE)) {
         SetForeground(dpy, areawin->gc, SELECTCOLOR);
	 easydraw(*selectobj, DOFORALL);
      }
   }
   select_invalidate_netlist();
   if (eventmode == NORMAL_MODE || eventmode == CATALOG_MODE)
      if (!preselected)
         unselect_all();

   if (eventmode == NORMAL_MODE)
      incr_changes(topobject);
   if (eventmode == CATALOG_MODE) {
      int libnum;
      if ((libnum = is_library(topobject)) >= 0) {
	 composelib(libnum + LIBRARY);
	 drawarea(NULL, NULL, NULL);
      }
   }
   else {
      pwriteback(areawin->topinstance);
      calcbbox(areawin->topinstance);
   }
}

/*----------------------------------------------*/
/* Vertically flip an element			*/
/*----------------------------------------------*/

void elementvflip(XPoint *position)
{
   short *selectobj;
   /*short fsign;  (jdk) */
   Boolean preselected;
   Boolean single = False;

   preselected = (areawin->selects > 0) ? TRUE : FALSE;
   if (!checkselect(ALL_TYPES)) return;
   if (areawin->selects == 1) single = True;

   if (eventmode != COPY_MODE)
      register_for_undo(XCF_Flip_Y, UNDO_MORE, areawin->topinstance,
		(eventmode == MOVE_MODE) ? &areawin->origin : position);

   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
	+ areawin->selects; selectobj++) {

      /* erase the object */
      SetForeground(dpy, areawin->gc, BACKGROUND);
      easydraw(*selectobj, DOFORALL);

      switch(SELECTTYPE(selectobj)) {
	 case(LABEL):{
	    labelptr fliplab = SELTOLABEL(selectobj);
	    if ((fliplab->anchor & (TOP | NOTBOTTOM)) != NOTBOTTOM)
	       fliplab->anchor ^= (TOP | NOTBOTTOM);
	    if (!single)
	       fliplab->position.y = (position->y << 1) - fliplab->position.y;
	    } break;
	 case(OBJINST):{
            objinstptr flipobj = SELTOOBJINST(selectobj);

	    if (is_library(topobject) >= 0 && !is_virtual(flipobj)) break;
	    flipobj->scale = -(flipobj->scale);
	    flipobj->rotation += 180;
	    while (flipobj->rotation >= 360) flipobj->rotation -= 360;
	    if (!single)
	       flipobj->position.y = (position->y << 1) - flipobj->position.y;
	    }break;
	 case(GRAPHIC):{
            graphicptr flipg = SELTOGRAPHIC(selectobj);

	    flipg->scale = -(flipg->scale);
	    flipg->rotation += 180;
	    while (flipg->rotation >= 360) flipg->rotation -= 360;
	    if (!single)
	       flipg->position.y = (position->y << 1) - flipg->position.y;
	    }break;
	 case POLYGON: case ARC: case SPLINE:
	    elvflip(topobject->plist + *selectobj, position->y);
	    break;
	 case PATH:{
	    genericptr *genpart;
	    pathptr flippath = SELTOPATH(selectobj);

	    for (genpart = flippath->plist; genpart < flippath->plist
		  + flippath->parts; genpart++)
	       elvflip(genpart, position->y);
	    }break;
      }
      if (preselected || (eventmode != NORMAL_MODE)) {
         SetForeground(dpy, areawin->gc, SELECTCOLOR);
	 easydraw(*selectobj, DOFORALL);
      }
   }
   select_invalidate_netlist();
   if (eventmode == NORMAL_MODE || eventmode == CATALOG_MODE)
      if (!preselected)
         unselect_all();
   if (eventmode == NORMAL_MODE) {
      incr_changes(topobject);
   }
   if (eventmode == CATALOG_MODE) {
      int libnum;
      if ((libnum = is_library(topobject)) >= 0) {
	 composelib(libnum + LIBRARY);
	 drawarea(NULL, NULL, NULL);
      }
   }
   else {
      pwriteback(areawin->topinstance);
      calcbbox(areawin->topinstance);
   }
}

/*----------------------------------------*/
/* ButtonPress handler during delete mode */
/*----------------------------------------*/

void deletebutton(int x, int y)
{
   UNUSED(x); UNUSED(y);

   if (checkselect(ALL_TYPES)) {
      standard_element_delete(ERASE);
      calcbbox(areawin->topinstance);
   }
   setoptionmenu();	/* Return GUI check/radio boxes to default */
}

/*----------------------------------------------------------------------*/
/* Process of element deletion.  Remove one element from the indicated	*/
/* object.								*/
/*----------------------------------------------------------------------*/

void delete_one_element(objinstptr thisinstance, genericptr thiselement)
{
   objectptr thisobject;
   genericptr *genobj;
   Boolean pinchange = False;

   thisobject = thisinstance->thisobject;

   /* The netlist contains pointers to elements which no longer		*/
   /* exist on the page, so we should remove them from the netlist.	*/

   if (RemoveFromNetlist(thisobject, thiselement)) pinchange = True;
   for (genobj = thisobject->plist; genobj < thisobject->plist
		+ thisobject->parts; genobj++)
      if (*genobj == thiselement)
	 break;

   if (genobj == thisobject->plist + thisobject->parts) return;

   for (++genobj; genobj < thisobject->plist + thisobject->parts; genobj++)
      *(genobj - 1) = *genobj;
   thisobject->parts--;

   if (pinchange) setobjecttype(thisobject);
   incr_changes(thisobject);
   calcbbox(thisinstance);
   invalidate_netlist(thisobject);
   /* freenetlist(thisobject); */
}
  
/*----------------------------------------------------------------------*/
/* Process of element deletion.  Remove everything in the selection	*/
/* list from the indicated object, and return a new object containing	*/
/* only the deleted elements.						*/
/*									*/
/* if drawmode is DRAW, we erase the objects as we remove them.		*/
/*									*/
/* Note that if "slist" is areawin->selectlist, it is freed by this	*/
/* routine (calls freeselects()), but not otherwise.			*/
/*----------------------------------------------------------------------*/

objectptr delete_element(objinstptr thisinstance, short *slist, int selects,
		short drawmode)
{
   short *selectobj;
   objectptr delobj, thisobject;
   genericptr *genobj;
   Boolean pinchange = False;

   if (slist == NULL || selects == 0) return NULL;

   thisobject = thisinstance->thisobject;

   delobj = (objectptr) malloc(sizeof(object));
   initmem(delobj);

   if (drawmode) {
      SetForeground(dpy, areawin->gc, BACKGROUND);
   }

   for (selectobj = slist; selectobj < slist + selects; selectobj++) {
      genobj = thisobject->plist + *selectobj;
      if (drawmode) easydraw(*selectobj, DOFORALL);
      PLIST_INCR(delobj);	
      *(delobj->plist + delobj->parts) = *genobj;
      delobj->parts++;

       /* The netlist contains pointers to elements which no longer	*/
       /* exist on the page, so we should remove them from the netlist.	*/

      if (RemoveFromNetlist(thisobject, *genobj)) pinchange = True;
      for (++genobj; genobj < thisobject->plist + thisobject->parts; genobj++)
	 *(genobj - 1) = *genobj;
      thisobject->parts--;
      reviseselect(slist, selects, selectobj);
   }
   if (pinchange) setobjecttype(thisobject);

   if (slist == areawin->selectlist)
      freeselects();

   calcbbox(thisinstance);
   /* freenetlist(thisobject); */

   if (drawmode) {
      SetForeground(dpy, areawin->gc, FOREGROUND);
      drawarea(NULL, NULL, NULL);
   }
   return delobj;
}
  
/*----------------------------------------------------------------------*/
/* Wrapper for delete_element().  Remember this deletion for the undo	*/
/* function.								*/
/*----------------------------------------------------------------------*/

void standard_element_delete(short drawmode)
{
   objectptr delobj;

/* register_for_undo(XCF_Select, UNDO_MORE, areawin->topinstance, */
/*		areawin->selectlist, areawin->selects); */
   select_invalidate_netlist();
   delobj = delete_element(areawin->topinstance, areawin->selectlist,
	areawin->selects, drawmode);
   register_for_undo(XCF_Delete, UNDO_DONE, areawin->topinstance,
		delobj, (int)drawmode);
   incr_changes(topobject);  /* Treat as one change */
}

/*----------------------------------------------------------------------*/
/* Another wrapper for delete_element(), in which we do not save the	*/
/* deletion as an undo event.  However, the returned object is saved	*/
/* on areawin->editstack, so that the objects can be grabbed.  This	*/
/* allows objects to be carried across pages and through the hierarchy.	*/
/*----------------------------------------------------------------------*/

void delete_for_xfer(short drawmode, short *slist, int selects)
{
   if (selects > 0) {
      reset(areawin->editstack, DESTROY);
      areawin->editstack = delete_element(areawin->topinstance,
		slist, selects, drawmode);
   }
}

/*----------------------------------------------------------------------*/
/* Yet another wrapper for delete_element(), in which we destroy the	*/
/* object returned and free all associated memory.			*/
/*----------------------------------------------------------------------*/

void delete_noundo(short drawmode)
{
   objectptr delobj;

   select_invalidate_netlist();
   delobj = delete_element(areawin->topinstance, areawin->selectlist,
	areawin->selects, drawmode);

   if (delobj != NULL) reset(delobj, DESTROY);
}

/*----------------------------------------------------------------------*/
/* Undelete last deleted elements and return a selectlist of the	*/
/* elements.  If "olist" is non-NULL, then the undeleted elements are	*/
/* placed into the object of thisinstance in the order given by olist,	*/
/* and a copy of olist is returned.  If "olist" is NULL, then the	*/
/* undeleted elements are placed at the end of thisinstance->thisobject	*/
/* ->plist, and a new selection list is returned.  If "olist" is non-	*/
/* NULL, then the size of olist had better match the number of objects	*/
/* in delobj!  It is up to the calling routine to check this.		*/
/*----------------------------------------------------------------------*/

short *xc_undelete(objinstptr thisinstance, objectptr delobj, short mode,
	short *olist)
{
   objectptr  thisobject;
   genericptr *regen;
   short      *slist, count, i; /* position; (jdk) */

   thisobject = thisinstance->thisobject;
   slist = (short *)malloc(delobj->parts * sizeof(short));
   count = 0;

   for (regen = delobj->plist; regen < delobj->plist + delobj->parts; regen++) {
      PLIST_INCR(thisobject);
      if (olist == NULL) {
         *(slist + count) = thisobject->parts;
         *(topobject->plist + topobject->parts) = *regen;
      }
      else {
         *(slist + count) = *(olist + count);
	 for (i = thisobject->parts; i > *(olist + count); i--)
	    *(thisobject->plist + i) = *(thisobject->plist + i - 1);
	 *(thisobject->plist + i) = *regen;
      }
      thisobject->parts++;
      if (mode) {
         XTopSetForeground((*regen)->color);
	 easydraw(*(slist + count), DEFAULTCOLOR);
      }
      count++;

      /* If the element has passed parameters (eparam), then we have to */
      /* check if the key exists in the new parent object.  If not,	*/
      /* delete the parameter.						*/

      if ((*regen)->passed) {
	 eparamptr nextepp, epp = (*regen)->passed;
	 while (epp != NULL) {
	    nextepp = epp->next;
	    if (!match_param(thisobject, epp->key)) {
	       if (epp == (*regen)->passed) (*regen)->passed = nextepp;
	       free_element_param(*regen, epp);
	    }
	    epp = nextepp;
	 }
      }

      /* Likewise, string parameters must be checked in labels because	*/
      /* they act like element parameters.				*/

      if (IS_LABEL(*regen)) {
	 labelptr glab = TOLABEL(regen);
	 stringpart *gstr, *lastpart = NULL;
	 for (gstr = glab->string; gstr != NULL; gstr = lastpart->nextpart) {
	    if (gstr->type == PARAM_START) {
	       if (!match_param(thisobject, gstr->data.string)) {
		  free(gstr->data.string);
		  if (lastpart)
		     lastpart->nextpart = gstr->nextpart;
		  else
		     glab->string = gstr->nextpart;
		  free(gstr);
		  gstr = (lastpart) ? lastpart : glab->string;
	       }
	    }
	    lastpart = gstr;
	 }
      }
   }
   incr_changes(thisobject);	/* treat as one change */
   calcbbox(thisinstance);

   /* flush the delete buffer but don't delete the elements */
   reset(delobj, SAVE);

   if (delobj != areawin->editstack) free(delobj);

   return slist;
}

/*----------------------------*/
/* select save object handler */
/*----------------------------*/

void printname(objectptr curobject)
{
   char editstr[10], pagestr[10];
   short ispage;

#ifndef TCL_WRAPPER
   Arg	wargs[1];
   Dimension swidth, swidth2, sarea;
   char tmpname[256];
   char *sptr = tmpname;
#endif
   
   /* print full string to make message widget proper size */

   strcpy(editstr, ((ispage = is_page(curobject)) >= 0) ? "Editing: " : "");
   strcpy(editstr, (is_library(curobject) >= 0) ? "Library: " : "");
   if (strstr(curobject->name, "Page") == NULL && (ispage >= 0))
      sprintf(pagestr, " (p. %d)", areawin->page + 1);
   else
      pagestr[0] = '\0';
   W2printf("%s%s%s", editstr, curobject->name, pagestr); 

   /* Tcl doesn't update width changes immediately. . . what to do? */
   /* (i.e., Tk_Width(message2) gives the original width) */
#ifndef TCL_WRAPPER

   XtSetArg(wargs[0], XtNwidth, &sarea);
   XtGetValues(message2, wargs, 1); 
   
   /* in the remote case that the string is longer than message widget,    */
   /* truncate the string and denote the truncation with an ellipsis (...) */

   strcpy(tmpname, curobject->name);
   swidth2 = XTextWidth(appdata.xcfont, editstr, strlen(editstr));
   swidth = XTextWidth(appdata.xcfont, tmpname, strlen(tmpname));

   if ((swidth + swidth2) > sarea) {
      char *ip;
      while ((swidth + swidth2) > sarea) {
         sptr++;
         swidth = XTextWidth(appdata.xcfont, sptr, strlen(sptr));
      }
      for(ip = sptr; ip < sptr + 3 && *ip != '\0'; ip++) *ip = '.';

      W2printf("Editing: %s", sptr); 
   }
#endif
}

/*--------------------------------------------------------------*/
/* Make sure that a string does not conflict with postscript	*/
/* names (commands and definitions found in xcircps2.pro).	*/
/*								*/
/* Return value is NULL if no change was made.  Otherwise, the	*/
/* return value is an allocated string.				*/
/*--------------------------------------------------------------*/

char *checkvalidname(char *teststring, objectptr newobj)
{
   int i, j;
   short dupl;  /* flag a duplicate string */
   objectptr *libobj;
   char *sptr, *pptr, *cptr;
   aliasptr aref;
   slistptr sref;

   /* Try not to allocate memory unless necessary */

   sptr = teststring;
   pptr = sptr;

   do {
      dupl = 0;
      if (newobj != NULL) {
         for (i = 0; i < xobjs.numlibs; i++) {
	    for (j = 0; j < xobjs.userlibs[i].number; j++) {
	       libobj = xobjs.userlibs[i].library + j;

	       if (*libobj == newobj) continue;
               if (!strcmp(pptr, (*libobj)->name)) { 

		  /* Prepend an underscore to the object name.	If the	*/
		  /* object has no technology, create a null technology	*/
		  /* name.  Otherwise, the technology remains the same	*/
		  /* but the object name gets the prepended underscore.	*/

		  if ((cptr = strstr(pptr, "::")) == NULL) {
                     pptr = (char *)malloc(strlen((*libobj)->name) + 4);
		     sprintf(pptr, "::_%s", (*libobj)->name);
		  }
		  else {
		     int offset = cptr - pptr + 2;
		     if (pptr == sptr)
                        pptr = (char *)malloc(strlen((*libobj)->name) + 2);
		     else
                        pptr = (char *)realloc(pptr, strlen((*libobj)->name) + 2);
	             sprintf(pptr, "%s", (*libobj)->name);
	             sprintf(pptr + offset, "_%s", (*libobj)->name + offset);
		  }
	          dupl = 1;
	       }
	    }
	 }

         /* If we're in the middle of a file load, the name cannot be	*/
         /* the same as an alias, either.				*/
	
         if (aliastop != NULL) {
	    for (aref = aliastop; aref != NULL; aref = aref->next) {
	       for (sref = aref->aliases; sref != NULL; sref = sref->next) {
	          if (!strcmp(pptr, sref->alias)) {
		     if (pptr == sptr)
                        pptr = (char *)malloc(strlen(sref->alias) + 2);
		     else
                        pptr = (char *)realloc(pptr, strlen(sref->alias) + 2);
	             sprintf(pptr, "_%s", sref->alias);
	             dupl = 1;
		  }
	       }
	    }
         }
      }

   } while (dupl == 1);

   return (pptr == sptr) ? NULL : pptr;
}

/*--------------------------------------------------------------*/
/* Make sure that name for new object does not conflict with	*/
/* existing object definitions					*/
/*								*/
/* Return:  True if name required change, False otherwise	*/
/*--------------------------------------------------------------*/

Boolean checkname(objectptr newobj)
{
   char *pptr;

   /* Check for empty string */
   if (strlen(newobj->name) == 0) {
      Wprintf("Blank object name changed to default");
      sprintf(newobj->name, "user_object");
   }

   pptr = checkvalidname(newobj->name, newobj);

   /* Change name if necessary to avoid naming conflicts */
   if (pptr == NULL) {
      Wprintf("Created new object %s", newobj->name);
      return False;
   }
   else {
      Wprintf("Changed name from %s to %s to avoid conflict with "
			"existing object", newobj->name, pptr);
      
      strncpy(newobj->name, pptr, 79);
      free(pptr);
   }
   return True;
}

/*------------------------------------------------------------*/
/* Find the object "dot" in the builtin library, if it exists */
/*------------------------------------------------------------*/

objectptr finddot()
{
   objectptr dotobj;
   short i, j;
   char *name, *pptr;

   for (i = 0; i < xobjs.numlibs; i++) {
      for (j = 0; j < xobjs.userlibs[i].number; j++) {
	 dotobj = *(xobjs.userlibs[i].library + j);
	 name = dotobj->name;
         if ((pptr = strstr(name, "::")) != NULL) name = pptr + 2;
         if (!strcmp(name, "dot")) {
            return dotobj;
         }
      }
   }
   return (objectptr)NULL;
}

/*--------------------------------------*/
/* Add value origin to all points	*/
/*--------------------------------------*/

void movepoints(genericptr *ssgen, short deltax, short deltay)
{
   switch(ELEMENTTYPE(*ssgen)) {
	 case ARC:{
            fpointlist sspoints;
            TOARC(ssgen)->position.x += deltax;
            TOARC(ssgen)->position.y += deltay;
            for (sspoints = TOARC(ssgen)->points; sspoints < TOARC(ssgen)->points +
	          TOARC(ssgen)->number; sspoints++) {
	       sspoints->x += deltax;
	       sspoints->y += deltay;
            }
	    }break;

	 case POLYGON:{
            pointlist sspoints;
            for (sspoints = TOPOLY(ssgen)->points; sspoints < TOPOLY(ssgen)->points +
	          TOPOLY(ssgen)->number; sspoints++) {
	       sspoints->x += deltax;
	       sspoints->y += deltay;
	    }
	    }break;

	 case SPLINE:{
            fpointlist sspoints;
            short j;
            for (sspoints = TOSPLINE(ssgen)->points; sspoints <
		  TOSPLINE(ssgen)->points + INTSEGS; sspoints++) {
	       sspoints->x += deltax;
	       sspoints->y += deltay;
            }
            for (j = 0; j < 4; j++) {
               TOSPLINE(ssgen)->ctrl[j].x += deltax;
               TOSPLINE(ssgen)->ctrl[j].y += deltay;
            }
            }break;
	 case OBJINST:
	    TOOBJINST(ssgen)->position.x += deltax;
	    TOOBJINST(ssgen)->position.y += deltay;
	    break;
	 case GRAPHIC:
	    TOGRAPHIC(ssgen)->position.x += deltax;
	    TOGRAPHIC(ssgen)->position.y += deltay;
	    break;
	 case LABEL:
	    TOLABEL(ssgen)->position.x += deltax;
	    TOLABEL(ssgen)->position.y += deltay;
	    break;
   }
}

/*----------------------------------------------------------------------*/
/* Add value origin to all edited points, according to edit flags	*/
/*----------------------------------------------------------------------*/

void editpoints(genericptr *ssgen, short deltax, short deltay)
{
   pathptr editpath;
   polyptr editpoly;
   splineptr editspline;
   short cycle, cpoint;
   pointselect *cptr;
   XPoint *curpt;
   genericptr *ggen;

   switch(ELEMENTTYPE(*ssgen)) {
      case POLYGON:
	 editpoly = TOPOLY(ssgen);
	 if (editpoly->cycle == NULL)
	    movepoints(ssgen, deltax, deltay);
	 else {
	    for (cptr = editpoly->cycle;; cptr++) {
	       cycle = cptr->number;
	       curpt = editpoly->points + cycle;
	       if (cptr->flags & EDITX) curpt->x += deltax;
	       if (cptr->flags & EDITY) curpt->y += deltay;
	       if (cptr->flags & LASTENTRY) break;
	    }
	 }
	 exprsub(*ssgen);
	 break;

      case SPLINE:
	 editspline = TOSPLINE(ssgen);
	 if (editspline->cycle == NULL)
	    movepoints(ssgen, deltax, deltay);
	 else {
	    for (cptr = editspline->cycle;; cptr++) {
	       cycle = cptr->number;
	       if (cycle == 0 || cycle == 3) {
		  cpoint = (cycle == 0) ? 1 : 2;
	          if (cptr->flags & EDITX) editspline->ctrl[cpoint].x += deltax;
	          if (cptr->flags & EDITY) editspline->ctrl[cpoint].y += deltay;
	       }
	       if (cptr->flags & EDITX) editspline->ctrl[cycle].x += deltax;
	       if (cptr->flags & EDITY) editspline->ctrl[cycle].y += deltay;
	       if (cptr->flags & ANTIXY) {
		  editspline->ctrl[cycle].x -= deltax;
		  editspline->ctrl[cycle].y -= deltay;
	       }
	       if (cptr->flags & LASTENTRY) break;
	    }
	 }
	 exprsub(*ssgen);
	 calcspline(editspline);
         break;

      case PATH:
	 editpath = TOPATH(ssgen);
	 if (checkcycle(*ssgen, 0) < 0) {
	    for (ggen = editpath->plist;  ggen < editpath->plist + editpath->parts;
			ggen++)
	       movepoints(ggen, deltax, deltay);
	 }
	 else {
	    for (ggen = editpath->plist;  ggen < editpath->plist + editpath->parts;
			ggen++) {
	       if (checkcycle(*ggen, 0) >= 0)
	          editpoints(ggen, deltax, deltay);
	    }
         }
         break;

      default:
	 movepoints(ssgen, deltax, deltay);
	 exprsub(*ssgen);
	 break;
   }
}

#ifndef TCL_WRAPPER

void xlib_makeobject(xcWidget w, caddr_t nulldata)
{
   UNUSED(w); UNUSED(nulldata);

   domakeobject(-1, (char *)_STR2, FALSE);
}

#endif /* !TCL_WRAPPER */

/*--------------------------------------------------------------*/
/* Set the name for a new user-defined object and make the	*/
/* object.  If "forceempty" is true, we allow creation of a new	*/
/* object with no elements (normally would be used only from a	*/
/* script, where an object is being constructed automatically).	*/
/*--------------------------------------------------------------*/

objinstptr domakeobject(int libnum, char *name, Boolean forceempty)
{
   objectptr *newobj;
   objinstptr *newinst;
   genericptr *ssgen;
   oparamptr ops, newop;
   eparamptr epp, newepp;
   stringpart *sptr;
   XPoint origin;
   short loclibnum = libnum;

   if (libnum == -1) loclibnum = USERLIB - LIBRARY;

   /* make room for new entry in library list */

   xobjs.userlibs[loclibnum].library = (objectptr *)
	realloc(xobjs.userlibs[loclibnum].library,
	(xobjs.userlibs[loclibnum].number + 1) * sizeof(objectptr));

   newobj = xobjs.userlibs[loclibnum].library + xobjs.userlibs[loclibnum].number;

   *newobj = delete_element(areawin->topinstance, areawin->selectlist,
			areawin->selects, NORMAL);

   if (*newobj == NULL) {
      objectptr initobj;

      if (!forceempty) return NULL;

      /* Create a new (empty) object */

      initobj = (objectptr) malloc(sizeof(object));
      initmem(initobj);
      *newobj = initobj;
   }

   invalidate_netlist(topobject);
   xobjs.userlibs[loclibnum].number++;

   /* Create the instance of this object so we can compute a bounding box */

   NEW_OBJINST(newinst, topobject);
   instancedefaults(*newinst, *newobj, 0, 0);
   calcbbox(*newinst);

   /* find closest snap point to bbox center and make this the obj. center */

   if (areawin->center) {
      origin.x = (*newobj)->bbox.lowerleft.x + (*newobj)->bbox.width / 2;
      origin.y = (*newobj)->bbox.lowerleft.y + (*newobj)->bbox.height / 2;
   }
   else {
      origin.x = origin.y = 0;
   }
   u2u_snap(&origin);
   instancedefaults(*newinst, *newobj, origin.x, origin.y);

   for (ssgen = (*newobj)->plist; ssgen < (*newobj)->plist + (*newobj)->parts;
	  ssgen++) {
      switch(ELEMENTTYPE(*ssgen)) {

	 case(OBJINST):
            TOOBJINST(ssgen)->position.x -= origin.x;
            TOOBJINST(ssgen)->position.y -= origin.y;
	    break;

	 case(GRAPHIC):
            TOGRAPHIC(ssgen)->position.x -= origin.x;
            TOGRAPHIC(ssgen)->position.y -= origin.y;
	    break;

	 case(LABEL):
            TOLABEL(ssgen)->position.x -= origin.x;
            TOLABEL(ssgen)->position.y -= origin.y;
	    break;

	 case(PATH):{
	    genericptr *pathlist;
	    for (pathlist = TOPATH(ssgen)->plist;  pathlist < TOPATH(ssgen)->plist
		  + TOPATH(ssgen)->parts; pathlist++) {
	       movepoints(pathlist, -origin.x, -origin.y);
	    }
	    }break;

	 default:
	    movepoints(ssgen, -origin.x, -origin.y);
	    break;
      }
   }

   for (ssgen = (*newobj)->plist; ssgen < (*newobj)->plist + (*newobj)->parts;
	  ssgen++) {
      for (epp = (*ssgen)->passed; epp != NULL; epp = epp->next) {
	 ops = match_param(topobject, epp->key);
	 newop = copyparameter(ops);
	 newop->next = (*newobj)->params;
	 (*newobj)->params = newop;

	 /* Generate an indirect parameter reference from child to parent */
	 newepp = make_new_eparam(epp->key);
	 newepp->flags |= P_INDIRECT;
	 newepp->pdata.refkey = strdup(epp->key);
	 newepp->next = (*newinst)->passed;
	 (*newinst)->passed = newepp;
      }
      if (IS_LABEL(*ssgen)) {
	 /* Also need to check for substring parameters in labels */
	 for (sptr = TOLABEL(ssgen)->string; sptr != NULL; sptr = sptr->nextpart) {
	    if (sptr->type == PARAM_START) {
	       ops = match_param(topobject, sptr->data.string);
	       if (ops) {
	          newop = copyparameter(ops);
	          newop->next = (*newobj)->params;
	          (*newobj)->params = newop;
	       }

	       /* Generate an indirect parameter reference from child to parent */
	       newepp = make_new_eparam(sptr->data.string);
	       newepp->flags |= P_INDIRECT;
	       newepp->pdata.refkey = strdup(sptr->data.string);
	       newepp->next = (*newinst)->passed;
	       (*newinst)->passed = newepp;
	    }
	 }
      }
   }

   /* any parameters in the top-level object that used by the selected	*/
   /* elements must be copied into the new object.			*/

   /* put new object back into place */

   (*newobj)->hidden = False;
   (*newobj)->schemtype = SYMBOL;

   calcbbox(*newinst);
   incr_changes(*newobj);

   /* (netlist invalidation was taken care of by delete_element() */
   /* Also, incr_changes(topobject) was done by delete_element())	*/

   XTopSetForeground((*newinst)->color);
   UDrawObject(*newinst, SINGLE, (*newinst)->color,
		xobjs.pagelist[areawin->page]->wirewidth, NULL);


   /* Copy name into object and check for conflicts */

   strcpy((*newobj)->name, name);
   checkname(*newobj);

   /* register the technology and mark the technology as not saved */
   AddObjectTechnology(*newobj);

   /* generate library instance for this object (bounding box	*/
   /* should be default, so don't do calcbbox() on it)		*/

   addtoinstlist(loclibnum, *newobj, FALSE);

   /* recompile the user catalog and reset view bounds */

   composelib(loclibnum + LIBRARY);
   centerview(xobjs.libtop[loclibnum + LIBRARY]);

   return *newinst;
}

#ifndef TCL_WRAPPER

/*-------------------------------------------*/
/* Make a user object from selected elements */
/*-------------------------------------------*/

void selectsave(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   buttonsave *popdata = (buttonsave *)malloc(sizeof(buttonsave));
   UNUSED(clientdata); UNUSED(calldata);

   if (areawin->selects == 0) return;  /* nothing was selected */

   /* Get a name for the new object */

   eventmode = NORMAL_MODE;
   popdata->dataptr = NULL;
   popdata->button = NULL; /* indicates that no button is assc'd w/ the popup */
   popupprompt(w, "Enter name for new object:", "\0", xlib_makeobject, popdata, NULL);
}

#endif

/*-----------------------------*/
/* Edit-stack support routines */
/*-----------------------------*/

void arceditpush(arcptr lastarc)
{
   arcptr *newarc;

   NEW_ARC(newarc, areawin->editstack);
   arccopy(*newarc, lastarc);
   copycycles(&((*newarc)->cycle), &(lastarc->cycle));
}

/*--------------------------------------*/

void splineeditpush(splineptr lastspline)
{
   splineptr *newspline;

   NEW_SPLINE(newspline, areawin->editstack);
   splinecopy(*newspline, lastspline);
}

/*--------------------------------------*/

void polyeditpush(polyptr lastpoly)
{
   polyptr *newpoly;

   NEW_POLY(newpoly, areawin->editstack);
   polycopy(*newpoly, lastpoly);
}

/*--------------------------------------*/

void patheditpush(pathptr lastpath)
{
   pathptr *newpath;

   NEW_PATH(newpath, areawin->editstack);
   pathcopy(*newpath, lastpath);
}

/*-----------------------------*/
/* Copying support routines    */
/*-----------------------------*/

pointlist copypoints(pointlist points, int number)
{
   pointlist rpoints, cpoints, newpoints;

   rpoints = (pointlist) malloc(number * sizeof(XPoint));
   for (newpoints = rpoints, cpoints = points;
		newpoints < rpoints + number;
		newpoints++, cpoints++) {
      newpoints->x = cpoints->x;
      newpoints->y = cpoints->y;
   }
   return rpoints;
}

/*------------------------------------------*/

void graphiccopy(graphicptr newg, graphicptr copyg)
{
   Imagedata *iptr;
   int i;

   newg->source = copyg->source;
   newg->position.x = copyg->position.x;
   newg->position.y = copyg->position.y;
   newg->rotation = copyg->rotation;
   newg->scale = copyg->scale;
   newg->color = copyg->color;
   newg->passed = NULL;
   copyalleparams((genericptr)newg, (genericptr)copyg);
#ifndef HAVE_CAIRO
   newg->valid = FALSE;
   newg->target = NULL;
   newg->clipmask = (Pixmap)NULL;
#endif /* HAVE_CAIRO */

   /* Update the refcount of the source image */
   for (i = 0; i < xobjs.images; i++) {
      iptr = xobjs.imagelist + i;
      if (iptr->image == newg->source) {
	 iptr->refcount++;
	 break;
      }
   }
}

/*------------------------------------------*/

void labelcopy(labelptr newtext, labelptr copytext)
{
   newtext->string = stringcopy(copytext->string);
   newtext->position.x = copytext->position.x;
   newtext->position.y = copytext->position.y;
   newtext->rotation = copytext->rotation;
   newtext->scale = copytext->scale;
   newtext->anchor = copytext->anchor;
   newtext->color = copytext->color;
   newtext->passed = NULL;
   newtext->cycle = NULL;
   copyalleparams((genericptr)newtext, (genericptr)copytext);
   newtext->pin = copytext->pin;
}

/*------------------------------------------*/

void arccopy(arcptr newarc, arcptr copyarc)
{
   newarc->style = copyarc->style;
   newarc->color = copyarc->color;
   newarc->position.x = copyarc->position.x;
   newarc->position.y = copyarc->position.y;
   newarc->radius = copyarc->radius;
   newarc->yaxis = copyarc->yaxis;
   newarc->angle1 = copyarc->angle1;
   newarc->angle2 = copyarc->angle2;
   newarc->width = copyarc->width;
   newarc->passed = NULL;
   newarc->cycle = NULL;
   copyalleparams((genericptr)newarc, (genericptr)copyarc);
   calcarc(newarc);
}

/*------------------------------------------*/

void polycopy(polyptr newpoly, polyptr copypoly)
{
   newpoly->style = copypoly->style;
   newpoly->color = copypoly->color;
   newpoly->width = copypoly->width;
   newpoly->number = copypoly->number;
   copycycles(&(newpoly->cycle), &(copypoly->cycle));
   newpoly->points = copypoints(copypoly->points, copypoly->number);

   newpoly->passed = NULL;
   copyalleparams((genericptr)newpoly, (genericptr)copypoly);
}

/*------------------------------------------*/

void splinecopy(splineptr newspline, splineptr copyspline)
{
   short i;

   newspline->style = copyspline->style;
   newspline->color = copyspline->color;
   newspline->width = copyspline->width;
   copycycles(&(newspline->cycle), &(copyspline->cycle));
   for (i = 0; i < 4; i++) {
     newspline->ctrl[i].x = copyspline->ctrl[i].x;
     newspline->ctrl[i].y = copyspline->ctrl[i].y;
   }
   for (i = 0; i < INTSEGS; i++) {
      newspline->points[i].x = copyspline->points[i].x;
      newspline->points[i].y = copyspline->points[i].y;
   }
   newspline->passed = NULL;
   copyalleparams((genericptr)newspline, (genericptr)copyspline);
}

/*----------------------------------------------*/
/* Copy a path 					*/
/*----------------------------------------------*/

void pathcopy(pathptr newpath, pathptr copypath)
{
   genericptr *ggen;
   splineptr *newspline, copyspline;
   polyptr *newpoly, copypoly;

   newpath->style = copypath->style;
   newpath->color = copypath->color;
   newpath->width = copypath->width;
   newpath->parts = 0;
   newpath->passed = NULL;
   copyalleparams((genericptr)newpath, (genericptr)copypath);
   newpath->plist = (genericptr *)malloc(copypath->parts * sizeof(genericptr));

   for (ggen = copypath->plist; ggen < copypath->plist + copypath->parts; ggen++) {
      switch (ELEMENTTYPE(*ggen)) {
	 case POLYGON:
	    copypoly = TOPOLY(ggen);
  	    NEW_POLY(newpoly, newpath);
	    polycopy(*newpoly, copypoly);
	    break;
	 case SPLINE:
	    copyspline = TOSPLINE(ggen);
  	    NEW_SPLINE(newspline, newpath);
	    splinecopy(*newspline, copyspline);
	    break;
      }
   }
}

/*--------------------------------------------------------------*/
/* Copy an object instance					*/
/*--------------------------------------------------------------*/

void instcopy(objinstptr newobj, objinstptr copyobj)
{
   newobj->position.x = copyobj->position.x;
   newobj->position.y = copyobj->position.y;
   newobj->rotation = copyobj->rotation;
   newobj->scale = copyobj->scale;
   newobj->style = copyobj->style;
   newobj->thisobject = copyobj->thisobject;
   newobj->color = copyobj->color;
   newobj->bbox.lowerleft.x = copyobj->bbox.lowerleft.x;
   newobj->bbox.lowerleft.y = copyobj->bbox.lowerleft.y;
   newobj->bbox.width = copyobj->bbox.width;
   newobj->bbox.height = copyobj->bbox.height;

   newobj->passed = NULL;
   copyalleparams((genericptr)newobj, (genericptr)copyobj);

   newobj->params = NULL;
   copyparams(newobj, copyobj);

   /* If the parameters are the same, the bounding box should be, too. */
   if (copyobj->schembbox != NULL) {
      newobj->schembbox = (BBox *)malloc(sizeof(BBox));
      newobj->schembbox->lowerleft.x = copyobj->schembbox->lowerleft.x;
      newobj->schembbox->lowerleft.y = copyobj->schembbox->lowerleft.y;
      newobj->schembbox->width = copyobj->schembbox->width;
      newobj->schembbox->height = copyobj->schembbox->height;
   }
   else
      newobj->schembbox = NULL;
}

/*--------------------------------------------------------------*/
/* The method for removing objects from a list is to add the	*/
/* value REMOVE_TAG to the type of each object needing to be	*/
/* removed, and then calling this routine.			*/
/*--------------------------------------------------------------*/

void delete_tagged(objinstptr thisinst) {
   Boolean tagged = True;
   objectptr thisobject, delobj;
   genericptr *pgen;
   short *sobj, stmp;
 
   thisobject = thisinst->thisobject;

   while (tagged) {
      tagged = False;
      for (stmp = 0; stmp < thisobject->parts; stmp++) {
	 pgen = thisobject->plist + stmp;
         if ((*pgen)->type & REMOVE_TAG) {
	    (*pgen)->type &= (~REMOVE_TAG);
	    tagged = True;

  	    delobj = delete_element(thisinst, &stmp, 1, 0);
	    register_for_undo(XCF_Delete, UNDO_MORE, thisinst, delobj, 0);

	    /* If we destroy elements in the current window, we need to	   */
	    /* make sure that the selection list is updated appropriately. */

	    if ((thisobject == topobject) && (areawin->selects > 0)) {
	       for (sobj = areawin->selectlist; sobj < areawin->selectlist +
			areawin->selects; sobj++)
	          if (*sobj > stmp) (*sobj)--;
	    }

	    /* Also ensure that this element is not referenced in any	*/
	    /* netlist.   If it is, remove it and mark the netlist as	*/
	    /* invalid.							*/

	    remove_netlist_element(thisobject, *pgen);
	 }
      }
   }
   undo_finish_series();
}

/*-----------------------------------------------------------------*/
/* For copying:  Check if an object is about to be placed directly */
/* on top of the same object.  If so, delete the one underneath.   */
/*-----------------------------------------------------------------*/

void checkoverlap()
{
   short *sobj, *cobj;
   genericptr *sgen, *pgen;
   Boolean tagged = False;

   /* Work through the select list */

   for (sobj = areawin->selectlist; sobj < areawin->selectlist +
	areawin->selects; sobj++) {
      sgen = topobject->plist + (*sobj);

      /* For each object being copied, compare it against every object	*/
      /* on the current page (except self).  Flag if it's the same.	*/

      for (pgen = topobject->plist; pgen < topobject->plist + topobject->parts;
		pgen++) {
	 if (pgen == sgen) continue;
	 if (compare_single(sgen, pgen)) {
	    /* Make sure that this object is not part of the selection, */
	    /* else chaos will reign.					*/
	    for (cobj = areawin->selectlist; cobj < areawin->selectlist +
			areawin->selects; cobj++) {
	       if (pgen == topobject->plist + (*cobj)) break;
	    }
	    /* Tag it for future deletion and prevent further compares */
	    if (cobj == areawin->selectlist + areawin->selects) {
	       tagged = True;
	       (*pgen)->type |= REMOVE_TAG;
	   }
	 }	 
      }
   }
   if (tagged) {
      /* Delete the tagged elements */
      Wprintf("Duplicate object deleted");
      delete_tagged(areawin->topinstance);
      incr_changes(topobject);
   }
}

/*--------------------------------------------------------------*/
/* Direct placement of elements.  Assumes that the selectlist	*/
/* contains all the elements to be positioned.  "deltax" and	*/
/* "deltay" are relative x and y positions to move the		*/
/* elements.							*/
/*--------------------------------------------------------------*/

void placeselects(short deltax, short deltay, XPoint *userpt)
{
   short *dragselect;
   XPoint newpos, *ppt;
   float rot;
   short closest;
   Boolean doattach;
   genericptr *pgen;
   polyptr cpoly;

   doattach = ((userpt == NULL) || (areawin->attachto < 0)) ? FALSE : TRUE;

   /* under attachto condition, keep element attached to */
   /* the attachto element.				 */

   if (doattach) findattach(&newpos, &rot, userpt);

#ifdef HAVE_CAIRO
#else
   areawin->clipped = -1;	/* Prevent clipping */
#endif /* HAVE_CAIRO */

   for (dragselect = areawin->selectlist; dragselect < areawin->selectlist
      + areawin->selects; dragselect++) {

      switch(SELECTTYPE(dragselect)) {
         case OBJINST: {
	    objinstptr draginst = SELTOOBJINST(dragselect);

	    if (doattach) {
	       draginst->position.x = newpos.x;
	       draginst->position.y = newpos.y;
	       while (rot >= 360.0) rot -= 360.0;
	       while (rot < 0.0) rot += 360.0;
	       draginst->rotation = rot;
	    }
	    else {
	       draginst->position.x += deltax;
	       draginst->position.y += deltay;
	    }

	 } break;
         case GRAPHIC: {
	    graphicptr dragg = SELTOGRAPHIC(dragselect);
	    dragg->position.x += deltax;
	    dragg->position.y += deltay;
	 } break;
	 case LABEL: {
	    labelptr draglabel = SELTOLABEL(dragselect);
	    if (doattach) {
	       draglabel->position.x = newpos.x;
	       draglabel->position.y = newpos.y;
	       draglabel->rotation = rot;
	    }
	    else {
	       draglabel->position.x += deltax;
	       draglabel->position.y += deltay;
	    }
	 } break;
	 case PATH: {
	    pathptr dragpath = SELTOPATH(dragselect);
	    genericptr *pathlist;
	    
	    if (doattach) {
	       XPoint *pdelta = pathclosepoint(dragpath, &newpos);
	       deltax = newpos.x - pdelta->x;
	       deltay = newpos.y - pdelta->y;
	    }
	    for (pathlist = dragpath->plist;  pathlist < dragpath->plist
		  + dragpath->parts; pathlist++) {
	       movepoints(pathlist, deltax, deltay);
	    }
	 } break;
	 case POLYGON: {
	    polyptr dragpoly = SELTOPOLY(dragselect);
	    pointlist dragpoints;

	    /* if (dragpoly->cycle != NULL) continue; */
	    if (doattach) {
	       closest = closepoint(dragpoly, &newpos);
	       deltax = newpos.x - dragpoly->points[closest].x;
	       deltay = newpos.y - dragpoly->points[closest].y;
	    }
	    for (dragpoints = dragpoly->points; dragpoints < dragpoly->points
	           + dragpoly->number; dragpoints++) {
	       dragpoints->x += deltax;
	       dragpoints->y += deltay;
	    }
	 } break;   
	 case SPLINE: {
	    splineptr dragspline = SELTOSPLINE(dragselect);
	    short j;
	    fpointlist dragpoints;

	    /* if (dragspline->cycle != NULL) continue; */
	    if (doattach) {
	       closest = (wirelength(&dragspline->ctrl[0], &newpos)
		  > wirelength(&dragspline->ctrl[3], &newpos)) ? 3 : 0;
	       deltax = newpos.x - dragspline->ctrl[closest].x;
	       deltay = newpos.y - dragspline->ctrl[closest].y;
	    }
	    for (dragpoints = dragspline->points; dragpoints < dragspline->
		   points + INTSEGS; dragpoints++) {
	       dragpoints->x += deltax;
	       dragpoints->y += deltay;
	    }
	    for (j = 0; j < 4; j++) {
	       dragspline->ctrl[j].x += deltax;
	       dragspline->ctrl[j].y += deltay;
	    }
	 } break;
	 case ARC: {
	    arcptr dragarc = SELTOARC(dragselect);
	    fpointlist dragpoints;

	    if (doattach) {
	       deltax = newpos.x - dragarc->position.x;
	       deltay = newpos.y - dragarc->position.y;
	    }
	    dragarc->position.x += deltax;
	    dragarc->position.y += deltay;
	    for (dragpoints = dragarc->points; dragpoints < dragarc->
		 points + dragarc->number; dragpoints++) {
	       dragpoints->x += deltax;
	       dragpoints->y += deltay;
	    }
         } break;
      }
   }

   if (areawin->pinattach) {
      for (pgen = topobject->plist; pgen < topobject->plist +
		topobject->parts; pgen++) {
         if (ELEMENTTYPE(*pgen) == POLYGON) {
	    cpoly = TOPOLY(pgen);
	    if (cpoly->cycle != NULL) {
	       ppt = cpoly->points + cpoly->cycle->number;
	       newpos.x = ppt->x + deltax;
	       newpos.y = ppt->y + deltay;
	       if (areawin->manhatn)
		  manhattanize(&newpos, cpoly, cpoly->cycle->number, FALSE);
	       ppt->x = newpos.x;
	       ppt->y = newpos.y;
	    }
	 }
      }	
   }
   
   move_mode_draw(xcDRAW_EDIT, NULL);
#ifdef HAVE_CAIRO
#else
   areawin->clipped = 0;
#endif /* HAVE_CAIRO */
}

/*----------------------------------------------------------------------*/
/* Copy handler.  Assumes that the selectlist contains the elements	*/
/* to be copied, and that the initial position of the copy is held	*/
/* in areawin->save.							*/
/*----------------------------------------------------------------------*/

void createcopies()
{
   short *selectobj;

   if (!checkselect_draw(ALL_TYPES, True)) return;
   u2u_snap(&areawin->save);
   for (selectobj = areawin->selectlist; selectobj < areawin->selectlist
		+ areawin->selects; selectobj++) {

      /* Cycles will not be used for copy mode:  remove them */
      removecycle(topobject->plist + (*selectobj));

      switch(SELECTTYPE(selectobj)) {
         case LABEL: { /* copy label */
	    labelptr copytext = SELTOLABEL(selectobj);
	    labelptr *newtext;
	
	    NEW_LABEL(newtext, topobject);
	    labelcopy(*newtext, copytext);
         } break;
         case OBJINST: { /* copy object instance */
	    objinstptr copyobj = SELTOOBJINST(selectobj);
	    objinstptr *newobj;
	    NEW_OBJINST(newobj, topobject);
	    instcopy(*newobj, copyobj);
         } break;
         case GRAPHIC: { /* copy graphic instance */
	    graphicptr copyg = SELTOGRAPHIC(selectobj);
	    graphicptr *newg;
	    NEW_GRAPHIC(newg, topobject);
	    graphiccopy(*newg, copyg);
         } break;
	 case PATH: { /* copy path */
	    pathptr copypath = SELTOPATH(selectobj);
	    pathptr *newpath;
	    NEW_PATH(newpath, topobject);
	    pathcopy(*newpath, copypath);
	 } break;	
	 case ARC: { /* copy arc */
	    arcptr copyarc = SELTOARC(selectobj);
	    arcptr *newarc;
   	    NEW_ARC(newarc, topobject);
	    arccopy(*newarc, copyarc);
         } break;
         case POLYGON: { /* copy polygons */
            polyptr copypoly = SELTOPOLY(selectobj);
            polyptr *newpoly;
   	    NEW_POLY(newpoly, topobject);
	    polycopy(*newpoly, copypoly);
         } break;
	 case SPLINE: { /* copy spline */
	    splineptr copyspline = SELTOSPLINE(selectobj);
	    splineptr *newspline;
   	    NEW_SPLINE(newspline, topobject);
	    splinecopy(*newspline, copyspline);
	 } break;
      }

      /* change selection from the old to the new object */

      *selectobj = topobject->parts - 1;
   }
}

/*--------------------------------------------------------------*/
/* Function which initiates interactive placement of copied	*/
/* elements.							*/
/*--------------------------------------------------------------*/

void copydrag()
{
   if (areawin->selects > 0) {
      move_mode_draw(xcDRAW_INIT, NULL);
      if (eventmode == NORMAL_MODE) {
	 XDefineCursor(dpy, areawin->window, COPYCURSOR);
	 eventmode = COPY_MODE;
#ifdef TCL_WRAPPER
         Tk_CreateEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
         XtAddEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, False, (XtEventHandler)xlib_drag,
		NULL);
#endif
      }
      select_invalidate_netlist();
   }
}

/*-----------------------------------------------------------*/
/* Copy handler for copying from a button push or key event. */
/*-----------------------------------------------------------*/

void copy_op(int op, int x, int y)
{
   if (op == XCF_Copy) {
      window_to_user(x, y, &areawin->save);
      createcopies();	/* This function does all the hard work */
      copydrag();		/* Start interactive placement */
   }
   else {
      eventmode = NORMAL_MODE;
      areawin->attachto = -1;
      W3printf("");
#ifdef TCL_WRAPPER
      xcRemoveEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, False, (xcEventHandler)xctk_drag,
		NULL);
#else
      xcRemoveEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, False, (xcEventHandler)xlib_drag,
		NULL);
#endif
      XDefineCursor(dpy, areawin->window, DEFAULTCURSOR);
      u2u_snap(&areawin->save);
      if (op == XCF_Cancel) {
	 move_mode_draw(xcDRAW_EMPTY, NULL);
	 delete_noundo(NORMAL);
      }
      else if (op == XCF_Finish_Copy) {
	 move_mode_draw(xcDRAW_FINAL, NULL);
	 /* If selected objects are the only ones on the page, */
	 /* then do a full bbox calculation.			  */
	 if (topobject->parts == areawin->selects)
	    calcbbox(areawin->topinstance);
	 else	
	    calcbboxselect();
	 checkoverlap();
	 register_for_undo(XCF_Copy, UNDO_MORE, areawin->topinstance,
			areawin->selectlist, areawin->selects);
	 unselect_all();
         incr_changes(topobject);
      }
      else {	 /* XCF_Continue_Copy */
	 move_mode_draw(xcDRAW_FINAL, NULL);
	 if (topobject->parts == areawin->selects)
	    calcbbox(areawin->topinstance);
	 else
	    calcbboxselect();
	 checkoverlap();
	 register_for_undo(XCF_Copy, UNDO_DONE, areawin->topinstance,
			areawin->selectlist, areawin->selects);
         createcopies();
         copydrag();		/* Start interactive placement again */
         incr_changes(topobject);
      }
   }
}

/*----------------------------------------------*/
/* Check for more than one button being pressed	*/
/*----------------------------------------------*/

Boolean checkmultiple(XButtonEvent *event)
{
   int state = Button1Mask | Button2Mask | Button3Mask |
		Button4Mask | Button5Mask;
   state &= event->state;
   /* ((x - 1) & x) is always non-zero if x has more than one bit set */
   return (((state - 1) & state) == 0) ? False : True;
}

/*----------------------------------------------------------------------*/
/* Operation continuation---dependent upon the ongoing operation.	*/
/* This operation only pertains to a few event modes for which		*/
/* continuation	of action makes sense---drawing wires (polygons), and	*/
/* editing polygons, arcs, splines, and paths.				*/
/*----------------------------------------------------------------------*/

void continue_op(int op, int x, int y)
{
   XPoint ppos;

   if (eventmode != EARC_MODE && eventmode != ARC_MODE) {
      window_to_user(x, y, &areawin->save);
   }
   snap(x, y, &ppos);
   printpos(ppos.x, ppos.y);

   switch(eventmode) {
      case(COPY_MODE):
	 copy_op(op, x, y);
	 break;
      case(WIRE_MODE):
	 wire_op(op, x, y);
	 break;
      case(EPATH_MODE): case(EPOLY_MODE): case (ARC_MODE):
      case(EARC_MODE): case(SPLINE_MODE): case(ESPLINE_MODE):
	 path_op(*(EDITPART), op, x, y);
	 break;
      case(EINST_MODE):
	 inst_op(*(EDITPART), op, x, y);
	 break;
      case(BOX_MODE):
	 finish_op(XCF_Finish_Element, x, y);
	 break;
      default:
	 break;
   }
}

/*--------------------------------------------------------------*/
/* Finish or cancel an operation.  This forces a return to	*/
/* "normal" mode, with whatever other side effects are caused	*/
/* by the operation.						*/
/*--------------------------------------------------------------*/

void finish_op(int op, int x, int y)
{
   labelptr curlabel;
   int libnum;
   XPoint snappt, boxpts[4];
   float fscale;

   if (eventmode != EARC_MODE && eventmode != ARC_MODE) {
      window_to_user(x, y, &areawin->save);
   }
   switch(eventmode) {
      case(EPATH_MODE): case(BOX_MODE): case(EPOLY_MODE): case (ARC_MODE):
	     case(EARC_MODE): case(SPLINE_MODE): case(ESPLINE_MODE):
	 path_op(*(EDITPART), op, x, y);
	 break;

      case(EINST_MODE):
	 inst_op(*(EDITPART), op, x, y);
	 break;

      case (FONTCAT_MODE):
      case (EFONTCAT_MODE):
	 fontcat_op(op, x, y);
	 eventmode = (eventmode == FONTCAT_MODE) ? TEXT_MODE : ETEXT_MODE;
	 text_mode_draw(xcDRAW_INIT, TOLABEL(EDITPART));
	 XDefineCursor (dpy, areawin->window, TEXTPTR);
	 break;

      case(ASSOC_MODE):
      case(CATALOG_MODE):
	 catalog_op(op, x, y);
	 break;

      case(WIRE_MODE):
	 wire_op(op, x, y);
	 break;

      case(COPY_MODE):
	 copy_op(op, x, y);
	 break;

      case(TEXT_MODE):
	 curlabel = TOLABEL(EDITPART);
	 if (op == XCF_Cancel) {
	     redrawtext(curlabel);
   	     areawin->redraw_needed = False; /* ignore previous requests */
	     text_mode_draw(xcDRAW_EMPTY, curlabel);
	     freelabel(curlabel->string); 
	     free(curlabel);
	     topobject->parts--;
	     curlabel = NULL;
	 }
	 else {
	    singlebbox(EDITPART);
	    incr_changes(topobject);
            select_invalidate_netlist();
	    text_mode_draw(xcDRAW_FINAL, curlabel);
	 }
	 setdefaultfontmarks();
	 eventmode = NORMAL_MODE;
         break;

      case(ETEXT_MODE): case(CATTEXT_MODE):
	 curlabel = TOLABEL(EDITPART);
	 if (op == XCF_Cancel) {
	    /* restore the original text */
	    undrawtext(curlabel);
	    undo_finish_series();
	    undo_action();
	    redrawtext(curlabel);
   	    areawin->redraw_needed = False; /* ignore previous requests */
	    text_mode_draw(xcDRAW_EMPTY, curlabel);
	    if (eventmode == CATTEXT_MODE) eventmode = CATALOG_MODE;
	    W3printf("");
	    setdefaultfontmarks();
	 }
	 else textreturn();  /* Generate "return" key character */
	 areawin->textend = 0;
	 break;

      case(CATMOVE_MODE):
	 u2u_snap(&areawin->save);
#ifdef TCL_WRAPPER
	 Tk_DeleteEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
	 xcRemoveEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, FALSE, (xcEventHandler)xlib_drag,
		NULL);
#endif
	 if (op == XCF_Cancel) {
	    /* Just regenerate the library where we started */
	    if (areawin->selects >= 1) {
	       objinstptr selinst = SELTOOBJINST(areawin->selectlist);
	       libnum = libfindobject(selinst->thisobject, NULL);
	       if (libnum >= 0)
		  composelib(libnum + LIBRARY);
	    }
	 }
	 else {
	    catmove(x, y);
	 }
	 clearselects();
	 eventmode = CATALOG_MODE;
	 XDefineCursor(dpy, areawin->window, DEFAULTCURSOR);
	 break;

      case(MOVE_MODE):
	 u2u_snap(&areawin->save);
#ifdef TCL_WRAPPER
	 Tk_DeleteEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
	 xcRemoveEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, FALSE, (xcEventHandler)xlib_drag,
		NULL);
#endif
	 if (op == XCF_Cancel) {
	    /* If we came from the library with an object instance, in	*/
	    /* MOVE_MODE, then "cancel" should delete the element.	*/
	    /* Otherwise, put the position of the element back to what	*/
	    /* it was before we started the move.  The difference is	*/
	    /* indicated by the value of areawin->editpart.		*/

	    if ((areawin->selects > 0) && (*areawin->selectlist == topobject->parts))
	       delete_noundo(NORMAL);
	    else 
	       placeselects(areawin->origin.x - areawin->save.x,
			areawin->origin.y - areawin->save.y, NULL);
	    clearselects();
	    drawarea(NULL, NULL, NULL);
	 }
	 else {
	    if (areawin->selects > 0) {
	       register_for_undo(XCF_Move, 
			/* (was_preselected) ? UNDO_DONE : UNDO_MORE, */
			UNDO_MORE,
			areawin->topinstance,
			(int)(areawin->save.x - areawin->origin.x),
			(int)(areawin->save.y - areawin->origin.y));
	       pwriteback(areawin->topinstance);
	       incr_changes(topobject);
	       select_invalidate_netlist();
	       unselect_all();		/* The way it used to be. . . */
	    }
	    W3printf("");
	    /* full calc needed: move may shrink bbox */
	    calcbbox(areawin->topinstance);
	    checkoverlap();
	    /* if (!was_preselected) clearselects(); */
	 }
	 areawin->attachto = -1;
	 break;

      case(RESCALE_MODE):

#ifdef TCL_WRAPPER
	 Tk_DeleteEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
	 xcRemoveEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, FALSE, (xcEventHandler)xlib_drag,
		NULL);
#endif
	 rescale_mode_draw(xcDRAW_FINAL, NULL);
	 if (op != XCF_Cancel) {
	    XPoint newpoints[5];
	    fscale = UGetRescaleBox(&areawin->save, newpoints);
	    if (fscale != 0.) {
	       elementrescale(fscale);
	       areawin->redraw_needed = True;
	    }
	 }
	 eventmode = NORMAL_MODE;
	 break;

      case(SELAREA_MODE):
	 selarea_mode_draw(xcDRAW_FINAL, NULL);

#ifdef TCL_WRAPPER
	 Tk_DeleteEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
	 xcRemoveEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, FALSE, (xcEventHandler)xlib_drag,
		NULL);
#endif
	 /* Zero-width boxes act like a normal selection.  Otherwise,	*/
 	 /* proceed with the area select.				*/

	 if ((areawin->origin.x == areawin->save.x) &&
	     (areawin->origin.y == areawin->save.y))
            select_add_element(ALL_TYPES);
	 else {
	    boxpts[0] = areawin->origin;
	    boxpts[1].x = areawin->save.x;
	    boxpts[1].y = areawin->origin.y;
	    boxpts[2] = areawin->save;
	    boxpts[3].x = areawin->origin.x;
	    boxpts[3].y = areawin->save.y;
	    selectarea(topobject, boxpts, 0);
	 }
	 break;

      case(PAN_MODE):
	 u2u_snap(&areawin->save);

#ifdef TCL_WRAPPER
	 Tk_DeleteEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#else
         xcRemoveEventHandler(areawin->area, PointerMotionMask |
		ButtonMotionMask, False, (xcEventHandler)xlib_drag,
		NULL);
#endif
	 areawin->panx = areawin->pany = 0;
	 if (op != XCF_Cancel)
	    panbutton((u_int) 5, (areawin->width >> 1) - (x - areawin->origin.x),
			(areawin->height >> 1) - (y - areawin->origin.y), 0);
	 break;
      default:
	 break;
   }

   /* Remove any selections */
   if ((eventmode == SELAREA_MODE) || (eventmode == PAN_MODE)
		|| (eventmode == MOVE_MODE))
   {
      eventmode = NORMAL_MODE;
      areawin->redraw_needed = True;
   }
   else if (eventmode != MOVE_MODE && eventmode != EPATH_MODE &&
		eventmode != EPOLY_MODE && eventmode != ARC_MODE &&
		eventmode != EARC_MODE && eventmode != SPLINE_MODE &&
		eventmode != ESPLINE_MODE && eventmode != WIRE_MODE &&
		eventmode != ETEXT_MODE && eventmode != TEXT_MODE) {
      unselect_all();
   }

   if (eventmode == NORMAL_MODE) {
      
      /* Return any highlighted networks to normal */
      highlightnetlist(topobject, areawin->topinstance, 0);

      XDefineCursor(dpy, areawin->window, DEFAULTCURSOR);
   }

   snap(x, y, &snappt);
   printpos(snappt.x, snappt.y);
}

/*--------------------------------------------------------------*/
/* Edit operations for instances.  This is used to allow	*/
/* numeric parameters to be adjusted from the hierarchical	*/
/* level above, shielding the the unparameterized parts from	*/
/* change.							*/
/*--------------------------------------------------------------*/

void inst_op(genericptr editpart, int op, int x, int y)
{
   UNUSED(editpart); UNUSED(op); UNUSED(x); UNUSED(y);
}

/*--------------------------------------------------------------*/
/* Operations for path components				*/
/*--------------------------------------------------------------*/

void path_op(genericptr editpart, int op, int x, int y)
{
   polyptr newpoly;
   splineptr newspline;
   Boolean donecycles = False;
   UNUSED(x); UNUSED(y);

   /* Don't allow point cycling in a multi-part edit.	*/
   /* Allowing it is just confusing.  Instead, we treat	*/
   /* button 1 (cycle) like button 2 (finish).		*/
   if (op == XCF_Continue_Element && areawin->selects > 1)
      op = XCF_Finish_Element;

   switch(ELEMENTTYPE(editpart)) {
      case (PATH): {
	 pathptr newpath = (pathptr)editpart;
         short dotrack = True;
         pathptr editpath;

         areawin->attachto = -1;

	 if (op != XCF_Continue_Element) {
	    dotrack = False;
	 }
	 if (op == XCF_Continue_Element) {
	    nextpathcycle(newpath, 1);
	    patheditpush(newpath);
	 }
	 else if (op == XCF_Finish_Element) {
   	    path_mode_draw(xcDRAW_FINAL, newpath);
	    incr_changes(topobject);
	 }
	 else {		/* restore previous path from edit stack */
	    free_single((genericptr)newpath);
	    if (areawin->editstack->parts > 0) {
	       if (op == XCF_Cancel) {
	          editpath = TOPATH(areawin->editstack->plist);
		  pathcopy(newpath, editpath);
	          reset(areawin->editstack, NORMAL);
	       }
	       else {
	          editpath = TOPATH(areawin->editstack->plist +
				areawin->editstack->parts - 1);
		  pathcopy(newpath, editpath);
		  free_single((genericptr)editpath);
		  free(editpath);
	          areawin->editstack->parts--;
	       }
	       if (areawin->editstack->parts > 0) {
		  dotrack = True;
	          nextpathcycle(newpath, 1);
   	          path_mode_draw(xcDRAW_EDIT, newpath);
	       }
	       else {
	       	  XPoint warppt;
		  user_to_window(areawin->origin, &warppt);
		  warppointer(warppt.x, warppt.y);
   	          path_mode_draw(xcDRAW_FINAL, newpath);
	       }
	    }
	    else {
   	       path_mode_draw(xcDRAW_EMPTY, newpath);
	       topobject->parts--;
	       free_single((genericptr)newpath);
	       free(newpath);
	    }
	 }
	 pwriteback(areawin->topinstance);

	 if (!dotrack) {
	    /* Free the editstack */
	    reset(areawin->editstack, NORMAL);
	    xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
			(xcEventHandler)trackelement, NULL);
	    eventmode = NORMAL_MODE;
	    donecycles = True;
	 }
      } break;

      case (POLYGON): {
	 if (eventmode == BOX_MODE) {
            polyptr   newbox;

            newbox = (polyptr)editpart;

            /* prevent length and/or width zero boxes */
            if (newbox->points->x != (newbox->points + 2)->x && (newbox->points 
	         + 1)->y != (newbox->points + 3)->y) {
	       if (op != XCF_Cancel) {
   	    	  poly_mode_draw(xcDRAW_FINAL, newbox);
	          incr_changes(topobject);
		  if (!nonnetwork(newbox)) invalidate_netlist(topobject);
		  register_for_undo(XCF_Box, UNDO_MORE, areawin->topinstance,
					newbox);
	       }
	       else {
   	    	  poly_mode_draw(xcDRAW_EMPTY, newbox);
		  free_single((genericptr)newbox);
		  free(newbox);
	          topobject->parts--;

	       }
            }
	    else {
   	       poly_mode_draw(xcDRAW_EMPTY, newbox);
	       free_single((genericptr)newbox);
	       free(newbox);
	       topobject->parts--;
	    }

            xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
               (xcEventHandler)trackbox, NULL);
            eventmode = NORMAL_MODE;
         }
	 else {   /* EPOLY_MODE */
            polyptr      editpoly;
            short        dotrack = True;
  
            newpoly = (polyptr)editpart;
            areawin->attachto = -1;
   
            if (op != XCF_Continue_Element) {
	       dotrack = False;
            }

            if (op == XCF_Continue_Element) {
	       nextpolycycle(&newpoly, 1);
	       polyeditpush(newpoly);
	    }
            else if (op == XCF_Finish_Element) {

	       /* Check for degenerate polygons (all points the same). */
	       int i;
	       for (i = 1; i < newpoly->number; i++)
		  if ((newpoly->points[i].x != newpoly->points[i - 1].x) ||
			(newpoly->points[i].y != newpoly->points[i - 1].y))
		     break;
	       if (i == newpoly->number) {
   	          poly_mode_draw(xcDRAW_EMPTY, newpoly);
		  /* Remove this polygon with the standard delete	*/
		  /* method (saves polygon on undo stack).		*/
		  newpoly->type |= REMOVE_TAG;
	          delete_tagged(areawin->topinstance);
	       }
	       else {
   	          poly_mode_draw(xcDRAW_FINAL, newpoly);
                  if (!nonnetwork(newpoly)) invalidate_netlist(topobject);
		  incr_changes(topobject);
	       }
	    }
            else {
	       XPoint warppt;
	       free_single((genericptr)newpoly);
	       if (areawin->editstack->parts > 0) {
	          if (op == XCF_Cancel) {
		     editpoly = TOPOLY(areawin->editstack->plist);
		     polycopy(newpoly, editpoly);
	             reset(areawin->editstack, NORMAL);
	          }
		  else {
		     editpoly = TOPOLY(areawin->editstack->plist +
				areawin->editstack->parts - 1);
		     polycopy(newpoly, editpoly);
		     free_single((genericptr)editpoly);
		     free(editpoly);
	             areawin->editstack->parts--;
		  }
	          if (areawin->editstack->parts > 0) {
		     dotrack = True;
		     nextpolycycle(&newpoly, -1);
   	             poly_mode_draw(xcDRAW_EDIT, newpoly);
	          }
	          else {
         	     XcTopSetForeground(newpoly->color);
		     user_to_window(areawin->origin, &warppt);
		     warppointer(warppt.x, warppt.y);
   	             poly_mode_draw(xcDRAW_FINAL, newpoly);
	          }
	       }
	       else {
   	          poly_mode_draw(xcDRAW_EMPTY, newpoly);
	          topobject->parts--;
		  free(newpoly);
	       }
	    }
	    pwriteback(areawin->topinstance);

            if (!dotrack) {
	       /* Free the editstack */
	       reset(areawin->editstack, NORMAL);

               xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
                   (xcEventHandler)trackelement, NULL);
               eventmode = NORMAL_MODE;
	       donecycles = True;
            }
 	 }
      } break;

      case (ARC): {
         arcptr   newarc, editarc;
         short    dotrack = True;

	 newarc = (arcptr)editpart;

	 if (op != XCF_Continue_Element) {
	    dotrack = False;
         }

	 if (op == XCF_Continue_Element) {
	    nextarccycle(&newarc, 1);
	    arceditpush(newarc);
	 }

         else if (op == XCF_Finish_Element) {
	    dotrack = False;

            if (newarc->radius != 0 && newarc->yaxis != 0 &&
		   (newarc->angle1 != newarc->angle2)) {
               XTopSetForeground(newarc->color);
	       incr_changes(topobject);
               if (eventmode == ARC_MODE) {
	          register_for_undo(XCF_Arc, UNDO_MORE, areawin->topinstance,
				newarc);
	       }
   	       arc_mode_draw(xcDRAW_FINAL, newarc);
	    }
	    else {

               /* Remove the record if the radius is zero.  If we were	*/
	       /* creating the arc, just delete it;  it's as if it	*/
	       /* never existed.  If we were editing an arc, use the	*/
	       /* standard delete method (saves arc on undo stack).	*/

   	       arc_mode_draw(xcDRAW_EMPTY, newarc);
	       if (eventmode == ARC_MODE) {
	          free_single((genericptr)newarc);
	          free(newarc);
		  topobject->parts--;
	       }
	       else {
	          newarc->type |= REMOVE_TAG;
	          delete_tagged(areawin->topinstance);
	       }
	    } 
	 }
	 else {	 /* Cancel: restore previous arc from edit stack */
	    free_single((genericptr)newarc);
	    if (areawin->editstack->parts > 0) {
	       if (op == XCF_Cancel) {
	          editarc = TOARC(areawin->editstack->plist);
		  arccopy(newarc, editarc);
		  copycycles(&(newarc->cycle), &(editarc->cycle));
	          reset(areawin->editstack, NORMAL);
	       }
	       else {
	          editarc = TOARC(areawin->editstack->plist +
				areawin->editstack->parts - 1);
		  arccopy(newarc, editarc);
		  copycycles(&(newarc->cycle), &(editarc->cycle));
		  free_single((genericptr)editarc);
		  free(editarc);
	          areawin->editstack->parts--;
	       }
	       if (areawin->editstack->parts > 0) {
		  dotrack = True;
	          nextarccycle(&newarc, -1);
   	       	  arc_mode_draw(xcDRAW_EDIT, newarc);
	       }
	       else {
		  if (eventmode != ARC_MODE) {
	       	     XPoint warppt;
		     user_to_window(areawin->origin, &warppt);
		     warppointer(warppt.x, warppt.y);
		  }
   	       	  arc_mode_draw(xcDRAW_FINAL, newarc);
	       }
	    }
	    else {
   	       arc_mode_draw(xcDRAW_EMPTY, newarc);
	       topobject->parts--;
	    }
         }
	 pwriteback(areawin->topinstance);

	 if (!dotrack) {
	    /* Free the editstack */
	    reset(areawin->editstack, NORMAL);

            xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
               (xcEventHandler)trackarc, NULL);
            eventmode = NORMAL_MODE;
	 }
      } break;

      case (SPLINE): {
	 splineptr	editspline;
	 short 		dotrack = True;

	 newspline = (splineptr)editpart;

	 if (op != XCF_Continue_Element) {
	    dotrack = False;
	 }

	 if (op == XCF_Continue_Element) {
	    /* Note:  we work backwards through spline control points.	*/
	    /* The reason is that when creating a spline, the sudden	*/
	    /* move from the endpoint to the startpoint	(forward	*/
	    /* direction) is more disorienting than moving from the	*/
	    /* endpoint to the endpoint's control point.		*/

	    nextsplinecycle(&newspline, -1);
	    splineeditpush(newspline);
	 }

	 /* unlikely but possible to create zero-length splines */
	 else if (newspline->ctrl[0].x != newspline->ctrl[3].x ||
	      	newspline->ctrl[0].x != newspline->ctrl[1].x ||
	      	newspline->ctrl[0].x != newspline->ctrl[2].x ||
	      	newspline->ctrl[0].y != newspline->ctrl[3].y ||
	      	newspline->ctrl[0].y != newspline->ctrl[1].y ||
	      	newspline->ctrl[0].y != newspline->ctrl[2].y) {
	    if (op == XCF_Finish_Element) {
	       incr_changes(topobject);
	       if (eventmode == SPLINE_MODE) {
		  register_for_undo(XCF_Spline, UNDO_MORE, areawin->topinstance,
				newspline);
	       }
	       spline_mode_draw(xcDRAW_FINAL, newspline);
	    }
	    else {	/* restore previous spline from edit stack */
	       free_single((genericptr)newspline);
	       if (areawin->editstack->parts > 0) {
		  if (op == XCF_Cancel) {
	             editspline = TOSPLINE(areawin->editstack->plist);
		     splinecopy(newspline, editspline);
	             reset(areawin->editstack, NORMAL);
		  }
		  else {
	             editspline = TOSPLINE(areawin->editstack->plist +
				areawin->editstack->parts - 1);
		     splinecopy(newspline, editspline);
		     free_single((genericptr)editspline);
		     free(editspline);
	             areawin->editstack->parts--;
		  }
		  if (areawin->editstack->parts > 0) {
		     dotrack = True;
	             nextsplinecycle(&newspline, 1);
		     spline_mode_draw(xcDRAW_EDIT, newspline);
		  }
		  else {
		     if (eventmode != SPLINE_MODE) {
	       		XPoint warppt;
		        user_to_window(areawin->origin, &warppt);
		        warppointer(warppt.x, warppt.y);
		     }
		     spline_mode_draw(xcDRAW_FINAL, newspline);
		  }
	       }
	       else {
	    	  spline_mode_draw(xcDRAW_EMPTY, newspline);
		  topobject->parts--;
	       }
	    }
	 }
	 else {
	    spline_mode_draw(xcDRAW_EMPTY, newspline);
	    free_single((genericptr)newspline);
	    free(newspline);
	    topobject->parts--;
	 }
	 pwriteback(areawin->topinstance);

	 if (!dotrack) {
	    /* Free the editstack */
	    reset(areawin->editstack, NORMAL);

	    xcRemoveEventHandler(areawin->area, PointerMotionMask, False,
	            (xcEventHandler)trackelement, NULL);
	    eventmode = NORMAL_MODE;
	    donecycles = True;
	 }
      } break;
   }
   calcbbox(areawin->topinstance);

   /* Multiple-element edit:  Some items may have been moved as	*/
   /* opposed to edited, and should be registered here.	 To do	*/
   /* this correctly, we must first unselect the edited items,	*/
   /* then register the move for the remaining items.		*/

   if (donecycles) {
      short *eselect;

      for (eselect = areawin->selectlist; eselect < areawin->selectlist +
		areawin->selects; eselect++)
	 checkcycle(SELTOGENERIC(eselect), 0);
	
      /* Remove all (remaining) cycles */
      for (eselect = areawin->selectlist; eselect < areawin->selectlist +
		areawin->selects; eselect++)
	 removecycle(SELTOGENERICPTR(eselect));

      /* Remove edits from the undo stack when canceling */
      if (op == XCF_Cancel || op == XCF_Cancel_Last) {
         if (xobjs.undostack && (xobjs.undostack->type == XCF_Edit)) {
	    undo_finish_series();
	    undo_action();
	 }
      }
   }
}

/*-------------------------------------------------------*/
/* Recalculate values for a drawing-area widget resizing */
/*-------------------------------------------------------*/

void resizearea(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
#ifndef TCL_WRAPPER
   Arg	wargs[2];
#endif
   XEvent discard;
   int savewidth = areawin->width, saveheight = areawin->height;
   XCWindowData *thiswin;
#ifndef HAVE_CAIRO
   XGCValues values;
#endif /* !HAVE_CAIRO */
   UNUSED(w); UNUSED(clientdata); UNUSED(calldata);

   if ((dpy != NULL) && xcIsRealized(areawin->area)) {

#ifdef TCL_WRAPPER
      areawin->width = Tk_Width(w);
      areawin->height = Tk_Height(w);
#else
      XtSetArg(wargs[0], XtNwidth, &areawin->width);
      XtSetArg(wargs[1], XtNheight, &areawin->height);
      XtGetValues(areawin->area, wargs, 2);
#endif

      if (areawin->width != savewidth || areawin->height != saveheight) {

	 int maxwidth = 0, maxheight = 0;
         for (thiswin = xobjs.windowlist; thiswin != NULL; thiswin = thiswin->next) {
            if (thiswin->width > maxwidth) maxwidth = thiswin->width;
            if (thiswin->height > maxheight) maxheight = thiswin->height;
         }
#if !defined(HAVE_CAIRO)
         if (dbuf != (Pixmap)NULL) XFreePixmap(dpy, dbuf);
         dbuf = XCreatePixmap(dpy, areawin->window, maxwidth, maxheight,
		DefaultDepthOfScreen(xcScreen(w)));
#endif
#ifdef HAVE_CAIRO
	 /* TODO: probably make this a generalized function call, which	*/
	 /* should be handled depending on the surface in the xtgui.c,	*/
	 /* xcwin32.c, etc. files.					*/
	 /* For now only support xlib surface				*/
	 cairo_xlib_surface_set_size(areawin->surface, areawin->width,
		areawin->height);
#else
         if (areawin->clipmask != (Pixmap)NULL) XFreePixmap(dpy, areawin->clipmask);
         areawin->clipmask = XCreatePixmap(dpy, areawin->window,
		maxwidth, maxheight, 1);

         if (areawin->pbuf != (Pixmap)NULL) {
	    XFreePixmap(dpy, areawin->pbuf);
            areawin->pbuf = XCreatePixmap(dpy, areawin->window,
		maxwidth, maxheight, 1);
	 }

         if (areawin->cmgc != (GC)NULL) XFreeGC(dpy, areawin->cmgc);
	 values.foreground = 0;
	 values.background = 0;
	 areawin->cmgc = XCreateGC(dpy, areawin->clipmask,
                GCForeground | GCBackground, &values);
#endif /* !HAVE_CAIRO */

	 /* Clear fixed_pixmap */
	 if (areawin->fixed_pixmap) {	
#ifdef HAVE_CAIRO
            cairo_pattern_destroy(areawin->fixed_pixmap);
            areawin->fixed_pixmap = NULL;
#else /* !HAVE_CAIRO */
	    XFreePixmap(dpy, areawin->fixed_pixmap);
	    areawin->fixed_pixmap = (Pixmap) NULL;
#endif /* !HAVE_CAIRO */
	 }

         reset_gs();

	 /* Re-compose the directores to match the new dimensions */
	 composelib(LIBLIB);
	 composelib(PAGELIB);

         /* Re-center image in resized window */
         zoomview(NULL, NULL, NULL);
      }

      /* Flush all expose events from the buffer */
      while (XCheckWindowEvent(dpy, areawin->window, ExposureMask, &discard) == True);
   }
}

/*----------------------*/
/* Draw the grids, etc. */
/*----------------------*/

#ifndef HAVE_CAIRO
void draw_grids(void)
{
   float x, y, spc, spc2, i, j, fpart;
   float major_snapspace, spc3;

   spc = xobjs.pagelist[areawin->page]->gridspace * areawin->vscale;
   if (areawin->gridon && spc > 8) {
      fpart = (float)(-areawin->pcorner.x)
		     / xobjs.pagelist[areawin->page]->gridspace;
      x = xobjs.pagelist[areawin->page]->gridspace *
	    (fpart - (float)((int)fpart)) * areawin->vscale;
      fpart = (float)(-areawin->pcorner.y)
		     / xobjs.pagelist[areawin->page]->gridspace;
      y = xobjs.pagelist[areawin->page]->gridspace *
	    (fpart - (float)((int)fpart)) * areawin->vscale;

      SetForeground(dpy, areawin->gc, GRIDCOLOR);
      for (i = x; i < (float)areawin->width; i += spc) 
	 DrawLine (dpy, areawin->window, areawin->gc, (int)(i + 0.5),
	       0, (int)(i + 0.5), areawin->height);
      for (j = (float)areawin->height - y; j > 0; j -= spc)
         DrawLine (dpy, areawin->window, areawin->gc, 0, (int)(j - 0.5),
	       areawin->width, (int)(j - 0.5));
   }

   if (areawin->axeson) {
      XPoint originpt, zeropt;
      zeropt.x = zeropt.y = 0;
      SetForeground(dpy, areawin->gc, AXESCOLOR);
      user_to_window(zeropt, &originpt);
      DrawLine(dpy, areawin->window, areawin->gc, originpt.x, 0,
	       originpt.x, areawin->height);
      DrawLine(dpy, areawin->window, areawin->gc, 0, originpt.y,
	       areawin->width, originpt.y);
   }

   /* bounding box goes beneath everything except grid/axis lines */
   UDrawBBox();

   /* draw a little red dot at each snap-to point */

   spc2 = xobjs.pagelist[areawin->page]->snapspace * areawin->vscale;
   if (areawin->snapto && spc2 > 8) {
      float x2, y2;

      fpart = (float)(-areawin->pcorner.x)
      	       / xobjs.pagelist[areawin->page]->snapspace;
      x2 = xobjs.pagelist[areawin->page]->snapspace *
	       (fpart - (float)((int)fpart)) * areawin->vscale;
      fpart = (float)(-areawin->pcorner.y)
	       / xobjs.pagelist[areawin->page]->snapspace;
      y2 = xobjs.pagelist[areawin->page]->snapspace *
	       (fpart - (float)((int)fpart)) * areawin->vscale;

#if defined(TCL_WRAPPER) && defined(XC_WIN32)
      {
	 HDC hdc = CreateCompatibleDC(NULL);
	 SelectObject(hdc, Tk_GetHWND(areawin->window));
#endif
         SetForeground(dpy, areawin->gc, SNAPCOLOR);
         for (i = x2; i < areawin->width; i += spc2)
            for (j = areawin->height - y2; j > 0; j -= spc2)
#if defined(TCL_WRAPPER) && defined(XC_WIN32)
	       SetPixelV(hdc, (int)(i + 0.5), (int)(j - 0.05), areawin->gc->foreground);
#endif
               DrawPoint (dpy, areawin->window, areawin->gc, (int)(i + 0.5),
			(int)(j - 0.5));
#if defined(TCL_WRAPPER) && defined(XC_WIN32)
	 DeleteDC(hdc);
      }
#endif
   };

   /* Draw major snap points (code contributed by John Barry) */

   major_snapspace = xobjs.pagelist[areawin->page]->gridspace * 20;
   spc3 = major_snapspace * areawin->vscale;
   if (spc > 4) {
      fpart = (float)(-areawin->pcorner.x) / major_snapspace;
      x = major_snapspace * (fpart - (float)((int)fpart)) * areawin->vscale;
      fpart = (float)(-areawin->pcorner.y) / major_snapspace;
      y = major_snapspace * (fpart - (float)((int)fpart)) * areawin->vscale;

      SetForeground(dpy, areawin->gc, GRIDCOLOR);
      for (i = x; i < (float)areawin->width; i += spc3) {
	 for (j = (float)areawin->height - y; j > 0; j -= spc3) {
	    XDrawArc(dpy, areawin->window, areawin->gc, (int)(i + 0.5) - 1,
		  (int)(j - 0.5) - 1, 2, 2, 0, 360*64);
	 }
      }
   }

   SetBackground(dpy, areawin->gc, BACKGROUND);
}
#endif /* !HAVE_CAIRO */

/*------------------------------------------------------*/
/* Draw fixed parts of the primary graphics window	*/
/*------------------------------------------------------*/

void draw_fixed(void)
{
   Boolean old_ongoing;
#ifndef HAVE_CAIRO
   Window old_window;
#endif /* !HAVE_CAIRO */

   if (xobjs.suspend >= 0) return;
   old_ongoing = areawin->redraw_ongoing;
   areawin->redraw_ongoing = True;

   /* Set drawing context to fixed_pixmap */
#ifdef HAVE_CAIRO
   cairo_identity_matrix(areawin->cr);
   cairo_push_group(areawin->cr);
#else /* HAVE_CAIRO */
   old_window = areawin->window;
   areawin->window = areawin->fixed_pixmap;
#endif /* HAVE_CAIRO */

   /* Clear background */
#ifdef HAVE_CAIRO
   if (xobjs.pagelist[areawin->page]->background.name != (char *)NULL) {
#ifdef HAVE_GS
      copybackground();
#else /* HAVE_GS */
      SetForeground(dpy, areawin->gc, BACKGROUND);
      cairo_paint(areawin->cr);
#endif /* HAVE_GS */
   }
   else {
      SetForeground(dpy, areawin->gc, BACKGROUND);
      cairo_paint(areawin->cr);
   }
#else /* HAVE_CAIRO */
   if (xobjs.pagelist[areawin->page]->background.name == (char *)NULL
	|| (copybackground() < 0)) {
      SetForeground(dpy, areawin->gc, BACKGROUND);
      XFillRectangle(dpy, areawin->window, areawin->gc, 0, 0, areawin->width,
	   areawin->height);
   }
   SetThinLineAttributes(dpy, areawin->gc, 0, LineSolid, CapRound, JoinBevel);
#endif /* !HAVE_CAIRO */
   
   newmatrix();

   /* draw GRIDCOLOR lines for grid; mark axes in AXESCOLOR */

   if (eventmode != CATALOG_MODE && eventmode != ASSOC_MODE
	&& eventmode != FONTCAT_MODE && eventmode != EFONTCAT_MODE
	&& eventmode != CATMOVE_MODE && eventmode != CATTEXT_MODE) {

      draw_grids();

      /* Determine the transformation matrix for the topmost object */
      /* and draw the hierarchy above the current edit object (if   */
      /* "edit-in-place" is selected).				    */

      if (areawin->editinplace == True) {
         if (areawin->stack != NULL) {
	    pushlistptr lastlist = NULL, thislist;
	    Matrix mtmp;

	    UPushCTM();	/* save our current state */

	    /* It's easiest if we first push the current page onto the stack, */
	    /* then we don't need to treat the top-level page separately.  We */
	    /* pop it at the end.					      */
	    push_stack(&areawin->stack, areawin->topinstance, NULL);

	    thislist = areawin->stack;

	    while ((thislist != NULL) &&
			(is_library(thislist->thisinst->thisobject) < 0)) {

	       /* Invert the transformation matrix of the instance on the stack */
	       /* to get the proper transformation matrix of the drawing one	*/
	       /* up in the hierarchy.						*/

	       UResetCTM(&mtmp);
	       UPreMultCTM(&mtmp, thislist->thisinst->position,
			thislist->thisinst->scale, thislist->thisinst->rotation);
	       InvertCTM(&mtmp);
	       UPreMultCTMbyMat(DCTM, &mtmp);

	       lastlist = thislist;
	       thislist = thislist->next;

	       /* The following will be true for moves between schematics and symbols */
	       if ((thislist != NULL) && (thislist->thisinst->thisobject->symschem
			== lastlist->thisinst->thisobject))
	          break;
	    }

	    if (lastlist != NULL) {
	       pushlistptr stack = NULL;
	       SetForeground(dpy, areawin->gc, OFFBUTTONCOLOR);
               UDrawObject(lastlist->thisinst, SINGLE, DOFORALL, 
			xobjs.pagelist[areawin->page]->wirewidth, &stack);
	       /* This shouldn't happen, but just in case. . . */
	       if (stack) free_stack(&stack);
	    }

	    pop_stack(&areawin->stack); /* restore the original stack state */
	    UPopCTM();			  /* restore the original matrix state */
	 }
      }
   }

   /* draw all of the elements on the screen */

   SetForeground(dpy, areawin->gc, FOREGROUND);

   /* Initialize hierstack */
   if (areawin->hierstack) free_stack(&areawin->hierstack);
   UDrawObject(areawin->topinstance, TOPLEVEL, FOREGROUND,
		xobjs.pagelist[areawin->page]->wirewidth, &areawin->hierstack);
   if (areawin->hierstack) free_stack(&areawin->hierstack);

#ifdef HAVE_CAIRO 
   if (areawin->fixed_pixmap)
      cairo_pattern_destroy(areawin->fixed_pixmap);
   areawin->fixed_pixmap = cairo_pop_group(areawin->cr);
#else /* HAVE_CAIRO */
   areawin->window = old_window;
#endif /* HAVE_CAIRO */
   areawin->redraw_ongoing = old_ongoing;
}

/*--------------------------------------*/
/* Draw the primary graphics window	*/
/*--------------------------------------*/

void drawwindow(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   XEvent discard;
   xcDrawType redrawtype = xcDRAW_EDIT;
   UNUSED(w); UNUSED(clientdata); UNUSED(calldata);

   if (areawin->area == NULL) return;
   if (!xcIsRealized(areawin->area)) return;
   if (xobjs.suspend >= 0) return;

#ifndef HAVE_CAIRO
   /* Make sure a fixed pixmap exists */
   if (!areawin->fixed_pixmap) {
      areawin->fixed_pixmap = XCreatePixmap(dpy, areawin->window,
            areawin->width, areawin->height,
            DefaultDepthOfScreen(xcScreen(areawin->area)));
   }
#endif /* !HAVE_CAIRO */

   /* Sanity check---specifically to track down an error */
   if ((areawin->selects == 1) && *(areawin->selectlist) >= topobject->parts) {
      Wprintf("Internal error!");
      areawin->selects = 0;
      unselect_all();
   }

   if (areawin->redraw_needed)
      redrawtype = xcREDRAW_FORCED;

   switch (eventmode) {
      case ARC_MODE: case EARC_MODE:
	 arc_mode_draw(redrawtype, TOARC(EDITPART));
	 break;
      case SPLINE_MODE: case ESPLINE_MODE:
	 spline_mode_draw(redrawtype, TOSPLINE(EDITPART));
	 break;
      case BOX_MODE: case EPOLY_MODE: case WIRE_MODE:
	 poly_mode_draw(redrawtype, TOPOLY(EDITPART));
	 break;
      case EPATH_MODE:
	 path_mode_draw(redrawtype, TOPATH(EDITPART));
	 break;
      case TEXT_MODE: case CATTEXT_MODE: case ETEXT_MODE:
	 text_mode_draw(redrawtype, TOLABEL(EDITPART));
	 break;
      case SELAREA_MODE:
	 selarea_mode_draw(redrawtype, NULL);
	 break;
      case RESCALE_MODE:
	 rescale_mode_draw(redrawtype, NULL);
	 break;
      case CATMOVE_MODE: case MOVE_MODE: case COPY_MODE:
	 move_mode_draw(redrawtype, NULL);
	 break;
      case ASSOC_MODE: case EINST_MODE: case FONTCAT_MODE: case EFONTCAT_MODE:
      case PAN_MODE: case NORMAL_MODE: case UNDO_MODE: case CATALOG_MODE:
	 normal_mode_draw(redrawtype, NULL);
   }

   /* flush out multiple expose/resize events from the event queue */
   while (XCheckWindowEvent(dpy, areawin->window, ExposureMask, &discard));

   /* end by restoring graphics state */
   SetForeground(dpy, areawin->gc, areawin->gccolor);

   areawin->redraw_needed = False;
}

/*----------------------------------------------------------------------*/
/* Draw the current window (areawin).  Check if other windows contain	*/
/* the same object or one of its ancestors.  If so, redraw them, too.	*/
/*----------------------------------------------------------------------*/

void drawarea(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   XCWindowDataPtr thiswin, focuswin;

   if (xobjs.suspend >= 0) {
      if (xobjs.suspend == 0)
	 xobjs.suspend = 1;	/* Mark that a refresh is pending */
      return;
   }

   focuswin = areawin;

   for (thiswin = xobjs.windowlist;  thiswin != NULL; thiswin = thiswin->next) {
      if (thiswin == focuswin) continue;

      /* Note:  need to check ancestry here, not just blindly redraw	*/
      /* all the windows all the time.					*/
      areawin = thiswin;

#ifdef HAVE_CAIRO
      /* Don't respond to an expose event if the graphics context  */
      /* has not yet been created.                                 */
      if (areawin->cr != NULL)
#endif
         drawwindow(NULL, NULL, NULL);
   }
   areawin = focuswin;
   drawwindow(w, clientdata, calldata);
}

/*-------------------------------------------------------------------------*/

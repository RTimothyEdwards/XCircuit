/*-------------------------------------------------------------------------*/
/* rcfile.c --- Load initialization variables from the .xcircuitrc file	   */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

/* This file is made obsolete by the Python interpreter, if compiled in */
#if !defined(HAVE_PYTHON) && !defined(TCL_WRAPPER)
   
/*-------------------------------------------------------------------------*/
/*      written by Tim Edwards, 9/9/96                                     */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>     /* for usleep() */

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
#include "menudep.h"
#include "colordefs.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* External variable declarations					*/
/*----------------------------------------------------------------------*/

extern char _STR2[250], _STR[150];
extern fontinfo *fonts;
extern short fontcount;
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern short beeper;
extern Widget menuwidgets[];
extern Display *dpy;

/*----------------------------------------------------------------------*/

#define LIBOVERRIDE	1
#define LIBLOADED	2
#define COLOROVERRIDE	4
#define FONTOVERRIDE	8
#define KEYOVERRIDE	16

/*----------------------------------------------------------------------*/
/* Execute a single command from a script or from the command line	*/
/*----------------------------------------------------------------------*/

short execcommand(short pflags, char *cmdptr)
{
   short flags = pflags;
   Boolean a;
   char type[50], value[50];
   char *argptr;
   short cpage = areawin->page;

   /* Note:  areawin->page is set to 0 before loading the .xcircuitrc	*/
   /*file, so .xcircuitrc file commands act on page 0 only;  from the	*/
   /* command line they act on the current page.			*/

   for(argptr = cmdptr; isspace(*argptr); argptr++)
      if (*argptr == '\0') return flags;
   cmdptr = argptr;

   for(argptr = cmdptr; (!isspace(*argptr)) && (*argptr != '\0'); argptr++);
   *argptr++ = '\0';
   while(isspace(*argptr)) argptr++;

   /* very liberal about comment line characters */

   if (*cmdptr == '#' || *cmdptr == '%' || *cmdptr == ';');

   /* startup configuration: enables/disables */

   else if (!(a = strcmp(cmdptr, "enable")) || !strcmp(cmdptr, "disable")) {
      sscanf(argptr, "%149s", _STR);
      if (!strncmp(_STR, "grid", 4)) {
	 areawin->gridon = a ? True : False;
	 toggle(GridGridButton, (pointertype)&areawin->gridon, NULL);
      }
      else if (!strncmp(_STR, "snap", 4) || !strncmp(_STR, "snap-to", 4)) {
	 areawin->snapto = a ? True : False;
	 toggle(SnaptoSnaptoButton, (pointertype)&areawin->snapto, NULL);
      }
      else if (!strcmp(_STR, "axis") || !strcmp(_STR, "axes")) {
	 areawin->axeson = a ? True : False;
	 toggle(GridAxesButton, (pointertype)&areawin->axeson, NULL);
      }
      else if (!strcmp(_STR, "xschema") || !strcmp(_STR, "schema")) {
	 /* Do nothing---retained for backward compatibility only */
      }
#ifdef HAVE_XPM
      else if (!strcmp(_STR, "toolbar")) {
	 areawin->toolbar_on = a ? True : False;
	 dotoolbar(OptionsDisableToolbarButton, NULL, NULL);
      }
#endif
   }

   /* startup configuration: overrides */

   else if (!strcmp(cmdptr, "override")) {

      /* "default" is now considered redundant */
      if (!strncmp(argptr, "default", 7)) {
	 argptr += 7;
	 while (isspace(*argptr) && (*argptr != '\0')) argptr++;
	 if (*argptr == '\0') return flags;	/* bad syntax */
      }

      sscanf(argptr, "%49s", value);
      if (!strcmp(value, "library") || !strcmp(value, "libraries")) {
	 flags |= LIBOVERRIDE;
      }
      else if (!strcmp(value, "color") || !strcmp(value, "colors")) {
	 flags |= COLOROVERRIDE;
      }
      else if (!strcmp(value, "font") || !strcmp(value, "fonts")) {
	 flags |= FONTOVERRIDE;
      }
      else if (!strcmp(value, "key") || !strcmp(value, "keybindings")) {
	 flags |= KEYOVERRIDE;
      }
   }

   /* load extra files */

   else if (!strcmp(cmdptr, "load")) {
      int pageno = areawin->page;
      int savepage = areawin->page;
      if (sscanf(argptr, "%249s %d", _STR, &pageno) >= 1) {
	 if (pageno != savepage) newpage(pageno);
	 startloadfile(-1);
	 if (pageno != savepage) newpage(savepage);
      }
      else
	 Wprintf("Error in load statement.");
   }

   /* load builtin libraries */

   else if (!strcmp(cmdptr, "library")) {
      int libnum;

      /* if loading of default libraries is not overridden, load them first */

      if (!(flags & (LIBOVERRIDE | LIBLOADED))) {
	 defaultscript();
	 flags |= LIBLOADED;
      }

      /* Optional third argument is the library number */
      if (sscanf(argptr, "%149s %d", _STR, &libnum) == 1)
	 libnum = LIBRARY;
      else {
	 if (libnum >= xobjs.numlibs)
	    libnum = createlibrary(FALSE);
	 else
	    libnum += LIBRARY - 1;
      }
      loadlibrary(libnum);
   }

   /* load extra fonts */

   else if (!strncmp(cmdptr, "font", 4)) {

      /* if default font is not overridden, load it first */

      if (!(flags & FONTOVERRIDE)) {
	 loadfontfile("Helvetica");
	 flags |= FONTOVERRIDE;
      }
      if (sscanf(argptr, "%149s", _STR) == 1)
	 loadfontfile(_STR);
      else
	 Wprintf("Error in font statement.");
   }

   /* load extra colors */

   else if (!strcmp(cmdptr, "color")) {
      sscanf(argptr, "%249s", _STR2);
      addnewcolorentry(xc_alloccolor(_STR2));
   }

   /* create macro */

   else if (!strcmp(cmdptr, "bind")) {
      int args;
      char *sptr;

      if (!(flags & KEYOVERRIDE)) {
	 default_keybindings();
         flags |= KEYOVERRIDE;
      }
      if ((args = sscanf(argptr, "%149s", _STR)) == 1) {
	 char *binding;
	 int func = -1, keywstate;
	 keywstate = string_to_key(_STR);
	 if (keywstate == 0) { /* Perhaps 1st arg is a function? */
	    keywstate = -1;
	    func = string_to_func(_STR, NULL);
	 }
	 if (keywstate == -1)
	    binding = function_binding_to_string(0, func);
	 else
	    binding = key_binding_to_string(0, keywstate);
	 Wprintf("%s", binding);
	 free(binding);
      }
      else if (args == 0)
	 Wprintf("See help window for list of bindings");
      else {
	 /* Use entire line after 1st argument, so multiple words are allowed */
	 sptr = argptr + strlen(_STR);
         if (add_keybinding(areawin->area, _STR, sptr) < 0) {
	    Wprintf("Function \"%s\" is not known\n", sptr);
	 }
      }
   }

   /* destroy macro */

   else if (!strcmp(cmdptr, "unbind")) {
      if (!(flags & KEYOVERRIDE)) {
	 default_keybindings();
         flags |= KEYOVERRIDE;
      }
      sscanf(argptr, "%149s %249s", _STR, _STR2);
      remove_keybinding(areawin->area, _STR, _STR2);
   }

   /* beep mode */

   else if (!strcmp(cmdptr, "beep")) {
      sscanf(argptr, "%49s", type);
      beeper = (!strcmp(type, "off")) ? 0 : 1;
   }

   /* active delay */

   else if (!strcmp(cmdptr, "pause")) {
      float dval = 0;
      sscanf(argptr, "%f", &dval);
      usleep((int)(1e6 * dval));
   }

   /* active refresh */

   else if (!strcmp(cmdptr, "refresh")) {
      XEvent event;

      refresh(NULL, NULL, NULL);

      while(XCheckWindowEvent(dpy, areawin->window, ~NoEventMask, &event))
         XtDispatchEvent(&event);
   }

   /* set variables */
 
   else if (!strcmp(cmdptr, "set") || !strcmp(cmdptr, "select")) {

      /* "default" is now considered redundant, but accepted for */
      /* backward compatibility.				 */

      if (!strncmp(argptr, "default", 7)) {
	 argptr += 7;
	 while (isspace(*argptr) && (*argptr != '\0')) argptr++;
	 if (*argptr == '\0') return flags;	/* bad syntax */
      }

      if (!strncmp(argptr, "font", 4)) {
	 short i;
	 if (strstr(argptr + 4, "scale")) {
	    sscanf(argptr + 4, "%*s %f", &areawin->textscale);
	 }
	 else {
	    sscanf(argptr, "%*s %49s", value);
	    for (i = 0; i < fontcount; i++)
	       if (!strcmp(fonts[i].psname, value)) break;

	    if (i == fontcount) {
	       loadfontfile(value);
	    }
   	    areawin->psfont = i;
	    setdefaultfontmarks();
	 }
      }
      else if (!strncmp(argptr, "boxedit", 7)) {
	 sscanf(argptr, "%*s %49s", value);
	 if (!strcmp(value, "rhomboid-x")) boxedit(NULL, RHOMBOIDX, NULL);
	 else if (!strcmp(value, "rhomboid-y")) boxedit(NULL, RHOMBOIDY, NULL);
	 else if (!strcmp(value, "rhomboid-a")) boxedit(NULL, RHOMBOIDA, NULL);
	 else if (!strcmp(value, "manhattan")) boxedit(NULL, MANHATTAN, NULL);
	 else if (!strcmp(value, "normal")) boxedit(NULL, NORMAL, NULL);
      }
      else if (!strncmp(argptr, "line", 4)) {
	 if (strstr(argptr + 4, "width")) {
	    sscanf(argptr + 4, "%*s %f", &areawin->linewidth);
	 }
      }
      else if (!strncmp(argptr, "colorscheme", 11)) {
	 sscanf(argptr, "%*s %49s", value);
	 if (!strcmp(value, "inverse") || !strcmp(value, "2"))
	    areawin->invert = False;
	 inversecolor(NULL, (pointertype)&areawin->invert, NULL);
      }
      else if (!strncmp(argptr, "coordstyle", 10)) {
	 sscanf(argptr, "%*s %49s", value);
	 if (!strcmp(value, "cm") || !strcmp(value, "centimeters")) {
	    xobjs.pagelist[cpage]->coordstyle = CM;
	    xobjs.pagelist[cpage]->pagesize.x = 595;  /* A4 size */
	    xobjs.pagelist[cpage]->pagesize.y = 842;
	    togglegrid((u_short)xobjs.pagelist[cpage]->coordstyle);
         }
      }
      else if (!strncmp(argptr, "orient", 6)) {   /* "orient" or "orientation" */
	 sscanf(argptr, "%*s %49s", value);
	 if (!strncmp(value, "land", 4))
	    xobjs.pagelist[cpage]->orient = 90;	/* Landscape */
	 else
	    xobjs.pagelist[cpage]->orient = 0;  /* Portrait */
      }
      else if (!strncmp(argptr, "page", 4)) {
	 if (strstr(argptr + 4, "type") || strstr(argptr + 4, "style")) {
	     sscanf(argptr + 4, "%*s %49s", value);
	     if (!strcmp(value, "encapsulated") || !strcmp(value, "eps"))
		xobjs.pagelist[cpage]->pmode = 0;
	     else
		xobjs.pagelist[cpage]->pmode = 1;
	 }
      }
      else if (!strncmp(argptr, "grid", 4)) {
	 if (strstr(argptr + 4, "space")) {
            sscanf(argptr + 4, "%*s %f", &xobjs.pagelist[cpage]->gridspace);
	 }
      }
      else if (!strncmp(argptr, "snap", 4)) {
	 if (strstr(argptr + 4, "space")) {
            sscanf(argptr + 4, "%*s %f", &xobjs.pagelist[cpage]->snapspace);
	 }
      }
   }
   return flags;
}

/*-------------------------------------------------------------------------*/
#define TEMPLEN 128

short readcommand(short mode, FILE *fd)
{
   char *temp, *tmpptr, *endptr;
   short templen = TEMPLEN, tempmax = TEMPLEN;
   short flags = (mode == 0) ? 0 : LIBOVERRIDE | LIBLOADED | FONTOVERRIDE;

   temp = (char *)malloc(templen);
   tmpptr = temp;

   while (fgets(tmpptr, templen, fd) != NULL) {
      endptr = tmpptr;
      while ((*endptr != '\0') && (*endptr != '\\')) {
         if (*endptr == '\n') {
	    endptr = temp;
	    templen = tempmax;
            flags = execcommand(flags, temp);
	    break;
	 }
	 endptr++;
	 templen--;
      }
      if (templen == 0) {
	 templen = (int)(endptr - temp);
	 tempmax += TEMPLEN;
	 temp = (char *)realloc(temp, tempmax);
	 tmpptr = temp + templen;
	 templen = TEMPLEN;
      }
      else
         tmpptr = endptr;
   }

   free(temp);
   fclose(fd);
   return flags;
}

/*----------------------------------------------------------------------*/
/* Load the default script (like execscript() but don't allow recursive */
/* loading of the startup script)					*/
/*----------------------------------------------------------------------*/

void defaultscript()
{
   FILE *fd;
   char *tmp_s = getenv((const char *)"XCIRCUIT_SRC_DIR");

   if (!tmp_s) tmp_s = SCRIPTS_DIR;
   sprintf(_STR2, "%s/%s", tmp_s, STARTUP_FILE);

   if ((fd = fopen(_STR2, "r")) == NULL) {
      sprintf(_STR2, "%s/%s", SCRIPTS_DIR, STARTUP_FILE);
      if ((fd = fopen(_STR2, "r")) == NULL) {
         Wprintf("Failed to open startup script \"%s\"\n", STARTUP_FILE);
	 return;
      }
   }
   readcommand(1, fd);
}

/*----------------------------------------------------------------------*/
/* Execute a script							*/
/*----------------------------------------------------------------------*/

void execscript()
{
   FILE *fd;

   xc_tilde_expand(_STR2, 249);
   if ((fd = fopen(_STR2, "r")) != NULL) readcommand(0, fd);
   else {
      Wprintf("Failed to open script file \"%s\"\n", _STR2);
   }
}

/*----------------------------------------------------------------------*/
/* Execute the .xcircuitrc startup script				*/
/*----------------------------------------------------------------------*/

void loadrcfile()
{
   char *userdir = getenv((const char *)"HOME");
   FILE *fd;
   short i, flags = 0;

   sprintf(_STR2, "%s", USER_RC_FILE);     /* Name imported from Makefile */

   /* try first in current directory, then look in user's home directory */

   xc_tilde_expand(_STR2, 249);
   if ((fd = fopen(_STR2, "r")) == NULL) {
      if (userdir != NULL) {
         sprintf(_STR2, "%s/%s", userdir, USER_RC_FILE);
         fd = fopen(_STR2, "r");
      }
   }
   if (fd != NULL) flags = readcommand(0, fd);

   /* Add the default font if not loaded already */

   if (!(flags & FONTOVERRIDE)) {
      loadfontfile("Helvetica");
      for (i = 0; i < fontcount; i++)
	 if (!strcmp(fonts[i].psname, "Helvetica")) areawin->psfont = i;
   }
   setdefaultfontmarks();

   /* arrange the loaded libraries */

   if (!(flags & (LIBOVERRIDE | LIBLOADED)))
      defaultscript();

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
   }

   /* These colors must be enabled whether or not colors are overridden, */
   /* because they are needed by the schematic capture system.           */

   addnewcolorentry(xc_getlayoutcolor(LOCALPINCOLOR));
   addnewcolorentry(xc_getlayoutcolor(GLOBALPINCOLOR));
   addnewcolorentry(xc_getlayoutcolor(INFOLABELCOLOR));
   addnewcolorentry(xc_getlayoutcolor(RATSNESTCOLOR));
   addnewcolorentry(xc_getlayoutcolor(BBOXCOLOR));


   /* Add the default key bindings */

   if (!(flags & KEYOVERRIDE))
      default_keybindings();
}

#endif
/* #endif for if !defined(HAVE_PYTHON) && !defined(TCL_WRAPPER) */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* filelist.c --- Xcircuit routines for the filelist widget		   */ 
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>	/* For isspace() */

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#include <unistd.h>
#define direct dirent
#elif !defined(_MSC_VER)
#include <sys/dir.h>
#endif

#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#endif
#ifdef TCL_WRAPPER 
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/Xw.h"
#include "Xw/WorkSpace.h"
#include "Xw/TextEdit.h"
#include "Xw/Toggle.h"
#endif
#endif

#if defined(XC_WIN32) && defined(TCL_WRAPPER)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#endif


/*-------------------------------------------------------------------------*/
/* Local includes							   */
/*-------------------------------------------------------------------------*/

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*-------------------------------------------------------------------------*/
/* Local definitions							   */
/*-------------------------------------------------------------------------*/

#define INITDIRS 10

/*-------------------------------------------------------------------------*/
/* Global Variable definitions						   */
/*-------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif

extern Display    *dpy;
extern XCWindowData *areawin;
extern ApplicationData appdata;
extern colorindex *colorlist;
extern short	  popups;     /* total number of popup windows */
extern char	  _STR2[250];
extern char	  _STR[150];
extern Globaldata xobjs;

Pixmap   flistpix = (Pixmap)NULL;    /* For file-selection widget */
short    flstart, flfiles, flcurrent;
int	 flcurwidth;

GC	 hgc = NULL, sgc = NULL;
char	 *cwdname = NULL;
fileliststruct *files;

#if !defined(XC_WIN32) || defined(TCL_WRAPPER)

/*-------------------------------------------------------------------------*/
/* Compare two filenames (for use by qsort())				   */
/*-------------------------------------------------------------------------*/

int fcompare(const void *a, const void *b)
{
   return (strcmp((char *)(((fileliststruct *)a)->filename),
	(char *)(((fileliststruct *)b)->filename)));
}

/*-------------------------------------------------------------------------*/
/* Routines for drawing a box around the currently selected file	   */
/*-------------------------------------------------------------------------*/

void dragfilebox(xcWidget w, caddr_t clientdata, XMotionEvent *event)
{
   short filenum;
   int twidth;
   Window lwin = xcWindow(w);

   filenum = (event->y - 10 + FILECHARHEIGHT) / FILECHARHEIGHT + flstart - 1;
   if (filenum < 0) filenum = 0;
   else if (filenum >= flfiles) filenum = flfiles - 1;
  
   if (filenum == flcurrent) return;

   if (flcurrent >= 0) 		/* erase previous box */
      XDrawRectangle(dpy, lwin, areawin->gc, 5,
	   10 + FILECHARHEIGHT * (flcurrent
	   - flstart), flcurwidth + 10, FILECHARHEIGHT);

   if (files == NULL) return;

   twidth = XTextWidth(appdata.filefont, files[filenum].filename,
	    strlen(files[filenum].filename));
   XDrawRectangle(dpy, lwin, areawin->gc, 5,
	   10 + FILECHARHEIGHT * (filenum
	   - flstart), twidth + 10, FILECHARHEIGHT);

   flcurrent = filenum;
   flcurwidth = twidth;
}

/*-------------------------------------------------------------------------*/
/* Begin tracking the cursor position relative to the files in the list    */
/*-------------------------------------------------------------------------*/

void startfiletrack(xcWidget w, caddr_t clientdata, XCrossingEvent *event)
{
#ifdef TCL_WRAPPER
   Tk_CreateEventHandler(w, PointerMotionMask,
		(Tk_EventProc *)xctk_dragfilebox, (ClientData)w);
#else
   xcAddEventHandler(w, PointerMotionMask, False, (xcEventHandler)dragfilebox, NULL);
#endif

   XSetFunction(dpy, areawin->gc, GXcopy);
   XSetForeground(dpy, areawin->gc, colorlist[AUXCOLOR].color.pixel);
   XSetLineAttributes(dpy, areawin->gc, 1, LineSolid, CapRound, JoinMiter);

   /* draw initial box */

   flcurrent = -1;
   dragfilebox(w, NULL, (XMotionEvent *)event);

   XSetFunction(dpy, areawin->gc, GXxor);
   XSetForeground(dpy, areawin->gc, colorlist[AUXCOLOR].color.pixel ^
		colorlist[BACKGROUND].color.pixel);
}

/*-------------------------------------------------------------------------*/
/* Stop tracking the cursor and return to default state			   */
/*-------------------------------------------------------------------------*/

void endfiletrack(xcWidget w, caddr_t clientdata, XCrossingEvent *event)
{
   Window lwin = xcWindow(w);

   XDrawRectangle(dpy, lwin, areawin->gc, 5,
	   10 + FILECHARHEIGHT * (flcurrent
	   - flstart), flcurwidth + 10, FILECHARHEIGHT);

#ifdef TCL_WRAPPER
   Tk_DeleteEventHandler(w, PointerMotionMask,
		(Tk_EventProc *)xctk_dragfilebox, (ClientData)w);
#else
   xcRemoveEventHandler(w, PointerMotionMask, False,
		(xcEventHandler)dragfilebox, NULL);
#endif

   /* Restore graphics state values */
   XSetForeground(dpy, areawin->gc, colorlist[areawin->gccolor].color.pixel);
   XSetFunction(dpy, areawin->gc, GXcopy);
}

#endif

/*----------------------------------------------------------------------*/
/* Read a crash file to find the name of the original file.		*/
/*----------------------------------------------------------------------*/

char *getcrashfilename()
{
   FILE *fi;
   char temp[256];
   char *retstr = NULL, *tpos, *spos;
   int slen;

   if ((fi = fopen(_STR2, "r")) != NULL) {
      while (fgets(temp, 255, fi) != NULL) {
	 if ((tpos = strstr(temp, "Title:")) != NULL) {
	    ridnewline(temp);
	    tpos += 7;
	    if ((spos = strrchr(temp, '/')) != NULL)
	       tpos = spos + 1;
	    retstr = (char *)malloc(1 + strlen(tpos));
	    strcpy(retstr, tpos);
	 }
	 else if ((tpos = strstr(temp, "CreationDate:")) != NULL) {
	    ridnewline(temp);
	    tpos += 14;
	    slen = strlen(retstr);
	    retstr = (char *)realloc(retstr, 4 + slen + strlen(tpos));
	    sprintf(retstr + slen, " (%s)", tpos);
	    break;
	 }
      }
      fclose(fi);
   }
   return retstr;
}

/*----------------------------------------------------------------------*/
/* Crash recovery:  load the file, and link the tempfile name to it.    */
/*----------------------------------------------------------------------*/

void crashrecover()
{
   if (xobjs.tempfile != NULL) {
      unlink(xobjs.tempfile);
      free(xobjs.tempfile);
      xobjs.tempfile = NULL;
   }
   if (strlen(_STR2) == 0) {
      Wprintf("Error: No temp file name for crash recovery!");
   }
   else {
      xobjs.tempfile = strdup(_STR2);
      startloadfile(-1);
   }
}

/*----------------------------------------------------------------------*/
/* Look for any files left over from a crash.                           */
/*----------------------------------------------------------------------*/

void findcrashfiles()
{
   DIR *cwd;
   struct direct *dp;
   struct stat sbuf;
#ifndef _MSC_VER
   uid_t userid = getuid();
#endif
   time_t recent = 0;
   char *snptr;
   int pid;

   cwd = opendir(xobjs.tempdir);
   if (cwd == NULL) return;	/* No tmp directory, no tmp files! */

   while ((dp = readdir(cwd)) != NULL) {
      sprintf(_STR, "%s/%s", xobjs.tempdir, dp->d_name);
      snptr = _STR + strlen(xobjs.tempdir) + 1;
      if (!strncmp(snptr, "XC", 2)) {
	 char *dotptr = strchr(snptr, '.');
	 pid = -1;
	 if (dotptr && dotptr > snptr + 2) {
	    *dotptr = '\0';
	    if (sscanf(snptr + 2, "%d", &pid) != 1) 
	       pid = -1;
	    *dotptr = '.';
         }
         if ((!stat(_STR, &sbuf))
#ifndef _MSC_VER
	     && (sbuf.st_uid == userid)
#endif
	     ) {
	    if ((recent == 0) || (sbuf.st_ctime > recent)) {

	       /* Check if the PID belongs to an active process	*/
	       /* by sending a CONT signal and checking if 	*/
	       /* there was no error result.			*/

#ifndef _MSC_VER
	       if (pid != -1)
		  if (kill((pid_t)pid, SIGCONT) == 0)
		     continue;
#endif

	       recent = sbuf.st_ctime;
	       strcpy(_STR2, _STR);
	    }
	 }
      }
   }
   closedir(cwd);
   
   if (recent > 0) {	/* There exists at least one temporary file */
			/* belonging to this user.  Ask to recover  */
			/* the most recent one.			    */

      /* Warn user of existing tempfile, and ask user if file	*/
      /* should be recovered immediately.			*/

#ifdef TCL_WRAPPER
      char *cfile = getcrashfilename();

      sprintf(_STR, ".query.title.field configure -text "
		"\"Recover file \'%s\'?\"", 
		(cfile == NULL) ? "(unknown)" : cfile);
      Tcl_Eval(xcinterp, _STR);
      Tcl_Eval(xcinterp, ".query.bbar.okay configure -command "
		"{filerecover; wm withdraw .query}; wm deiconify .query");
      if (cfile != NULL) free(cfile);
#else
      getfile(NULL, (pointertype)RECOVER, NULL);   /* Crash recovery mode */
#endif
   }
}  

/*----------------------------------------------------------------------*/
/* Match a filename extension against the file filter list.		*/
/*----------------------------------------------------------------------*/

#if !defined(XC_WIN32) || defined(TCL_WRAPPER)

Boolean match_filter(char *fname, char *filter)
{
   char *dotptr = strrchr(fname, '.');
   char *filtptr, *endptr;
   int filtlen, extlen;

   if (filter == NULL) return False;
   if (dotptr == NULL) return False;

   /* New 11/08:  Empty string for filter is a wildcard match, the same */
   /* as turning off the filter.					*/

   if (filter[0] == '\0') return True;

   dotptr++;
   extlen = strlen(dotptr);
   filtptr = filter;

   while (*filtptr != '\0') {
      endptr = filtptr;
      while (*endptr != '\0' && !isspace(*endptr)) endptr++;
      filtlen = (int)(endptr - filtptr);
      if (filtlen == extlen)
         if (!strncmp(dotptr, filtptr, extlen))
	    return True;

      filtptr = endptr;
      while (*filtptr != '\0' && isspace(*filtptr)) filtptr++;
   }
   return False;
}

/*----------------------------------------------------------------------*/
/* Make a list of the files in the list widget window			*/
/*----------------------------------------------------------------------*/

void listfiles(xcWidget w, popupstruct *okaystruct, caddr_t calldata)
{
   XGCValues	values;
#ifndef TCL_WRAPPER
   Arg wargs[2];
#endif
   DIR *cwd;
   char *filter;
   Window lwin = xcWindow(w);
   short allocd = INITDIRS;
   short n = 0;
   struct direct *dp;
   struct stat statbuf;
   int pixheight;
   Dimension textwidth, textheight;

   filter = okaystruct->filter;

   if (sgc == NULL) {
      values.foreground = colorlist[FOREGROUND].color.pixel;
      values.font = appdata.filefont->fid;
      values.function = GXcopy;
      values.graphics_exposures = False;
      sgc = XCreateGC(dpy, lwin, GCForeground | GCFont | GCFunction
		| GCGraphicsExposures, &values);
   }

#ifdef TCL_WRAPPER
   textwidth = Tk_Width(w);
   textheight = Tk_Height(w);
#else
   XtnSetArg(XtNwidth, &textwidth);
   XtnSetArg(XtNheight, &textheight);
   XtGetValues(w, wargs, n);
#endif

   /* Generate a new flistpix pixmap if currently nonexistent */

   if (!flistpix) {

      /* get list of files in the current directory (cwd) */

      flfiles = 0;
      if (cwdname == NULL) {
	 cwdname = (char *) malloc (sizeof(char));
	 cwdname[0] = '\0';
      }
      if (cwdname[0] == '\0')
         cwd = opendir(".");
      else
         cwd = opendir(cwdname);

      /* If current directory cannot be accessed for some reason, */
      /* print "Invalid Directory" to the file list window.	  */

      if (cwd == NULL) {
         XSetForeground(dpy, sgc, colorlist[BACKGROUND].color.pixel);
         XFillRectangle(dpy, lwin, sgc, 0, 0, textwidth, textheight);
         XSetForeground(dpy, sgc, colorlist[AUXCOLOR].color.pixel);
         XDrawString(dpy, lwin, sgc, 10, textheight / 2,
	    "(Invalid Directory)", 19);
	 return;
      }
      else {
	 if (files == NULL) 
	    files = (fileliststruct *) malloc (INITDIRS * sizeof(fileliststruct));

	 /* write the contents of the current directory into the   */
	 /* array "filenames[]" (except for current directory ".") */

         while ((dp = readdir(cwd)) != NULL) {
	    /* don't put current directory in list */
	    if (!strcmp(dp->d_name, ".")) continue;

	    /* record the type of file */
	
	    sprintf(_STR2, "%s%s", cwdname, dp->d_name); 
	    if (stat(_STR2, &statbuf)) continue;
	    /* if ((statbuf.st_mode & S_IFDIR) != 0) */ // deprecated usage
	    if (S_ISDIR(statbuf.st_mode))	/* is a directory */
	       files[flfiles].filetype = DIRECTORY;
	    else if (match_filter(dp->d_name, filter))
	       files[flfiles].filetype = MATCH;
	    else {
	       if (xobjs.filefilter)
		  continue;
	       else
	          files[flfiles].filetype = NONMATCH;
	    }

	    /* save the filename */

	    files[flfiles].filename = (char *) malloc ((strlen(dp->d_name) + 
		 ((files[flfiles].filetype == DIRECTORY) ? 2 : 1)) * sizeof(char));
	    strcpy(files[flfiles].filename, dp->d_name);
	    if (files[flfiles].filetype == DIRECTORY)
	       strcat(files[flfiles].filename, "/");
	    if (++flfiles == allocd) {
	       allocd += INITDIRS;
	       files = (fileliststruct *) realloc(files,
			allocd * sizeof(fileliststruct));
	    }
         }
      }
      closedir(cwd);

      /* sort the files[] array into alphabetical order (like "ls") */

      qsort((void *)files, (size_t)flfiles, sizeof(fileliststruct), fcompare);

      pixheight = flfiles * FILECHARHEIGHT + 25;
      if (pixheight < textheight) pixheight = textheight;

      flistpix = XCreatePixmap(dpy, areawin->window, textwidth, pixheight,
	   DefaultDepthOfScreen(xcScreen(w)));

      /* Write the filenames onto the pixmap */

      XSetForeground(dpy, sgc, colorlist[BACKGROUND].color.pixel);
      XFillRectangle(dpy, flistpix, sgc, 0, 0, textwidth, pixheight);
      XSetForeground(dpy, sgc, colorlist[FOREGROUND].color.pixel);
      for (n = 0; n < flfiles; n++) {
	 switch (files[n].filetype) {
	    case DIRECTORY:
	       XSetForeground(dpy, sgc, colorlist[SELECTCOLOR].color.pixel);
	       break;
	    case MATCH:
	       XSetForeground(dpy, sgc, colorlist[FILTERCOLOR].color.pixel);
	       break;
	    case NONMATCH:
	       XSetForeground(dpy, sgc, colorlist[FOREGROUND].color.pixel);
	       break;
	 }
         XDrawString(dpy, flistpix, sgc, 10, 10 + FILECHARASCENT + n * FILECHARHEIGHT,
	    files[n].filename, strlen(files[n].filename));
      }
   }

   /* Copy the pixmap of filenames into the file list window */

   XSetForeground(dpy, sgc, colorlist[BACKGROUND].color.pixel);
   XFillRectangle(dpy, lwin, sgc, 0, 0, textwidth, textheight);
   XCopyArea(dpy, flistpix, lwin, sgc, 0, flstart * FILECHARHEIGHT,
	textwidth, textheight, 0, 0);
}

/*-------------------------------------------------------------------------*/
/* Generate a new pixmap for writing the filelist and set the scrollbar    */
/* size accordingly.							   */
/*-------------------------------------------------------------------------*/

void newfilelist(xcWidget w, popupstruct *okaystruct)
{
   short n;

#ifdef TCL_WRAPPER
   int bval;
   int result;
   char *cstr = (char *)Tcl_GetVar2(xcinterp, "XCOps", "filter", 0);
   if (cstr == NULL) {
      Wprintf("Error: No variable $XCOps(filter) in Tcl!");
      return;
   }
   result = Tcl_GetBoolean(xcinterp, cstr, &bval);
   if (result != TCL_OK) {
      Wprintf("Error: Bad variable $XCOps(filter) in Tcl!");
      return;  
   }
   xobjs.filefilter = bval;
#else
   xcWidget textw = okaystruct->textw;
#endif

   for (n = 0; n < flfiles; n++)
      free(files[n].filename);
   free(files);
   if (flistpix != (Pixmap)NULL) XFreePixmap(dpy, flistpix);
   files = NULL;
   flistpix = (Pixmap)NULL;
   flstart = 0;
   listfiles(w, okaystruct, NULL);
#ifdef TCL_WRAPPER
   showlscroll(Tk_NameToWindow(xcinterp, ".filelist.listwin.sb", w), NULL, NULL);
   Tcl_Eval(xcinterp, ".filelist.textent.txt delete 0 end");
   sprintf(_STR2, ".filelist.textent.txt insert 0 %s", cwdname);
   Tcl_Eval(xcinterp, _STR2);
#else
   showlscroll(XtNameToWidget(xcParent(w), "LScroll"), NULL, NULL);
   XwTextClearBuffer(textw);
   XwTextInsert(textw, cwdname);
   XwTextResize(textw);
#endif
}

/*-------------------------------------------------------------------------*/
/* Button press handler for file list window				   */
/*-------------------------------------------------------------------------*/

void fileselect(xcWidget w, popupstruct *okaystruct, XButtonEvent *event)
{
   Window lwin = xcWindow(w);
   Dimension textwidth, textheight;
   short filenum;
   char *tbuf, *ebuf;

#ifdef TCL_WRAPPER
   textwidth = Tk_Width(w);
   textheight = Tk_Height(w);
#else
   Arg wargs[2];
   short n = 0;
   xcWidget textw = okaystruct->textw;

   XtnSetArg(XtNwidth, &textwidth);
   XtnSetArg(XtNheight, &textheight);
   XtGetValues(w, wargs, n);
#endif

   flcurrent = -1;

   if (files == NULL) return;	/* shouldn't happen */

   /* third mouse button cancels selection and reverts buffer to cwd name */

   if (event->button == Button3) {
      newfilelist(w, okaystruct);
      return;
   }

   filenum = (event->y - 10 + FILECHARHEIGHT) / FILECHARHEIGHT + flstart - 1;
   if (filenum < 0) filenum = 0;
   else if (filenum >= flfiles) filenum = flfiles - 1;

   /* Attempt to enter invalid directory. . . treat like button 3 */

   if (filenum < 0) {
      newfilelist(w, okaystruct);
      return;
   }
   
   /* check if this file is a directory or not */

   if (strchr(files[filenum].filename, '/') == NULL)	{

      /* highlight the entry. . . */

      XSetForeground(dpy, sgc, colorlist[AUXCOLOR].color.pixel);
      XDrawString(dpy, flistpix, sgc, 10, 10 + FILECHARASCENT + filenum * FILECHARHEIGHT,
   	   files[filenum].filename, strlen(files[filenum].filename));
      XCopyArea(dpy, flistpix, lwin, sgc, 0, flstart * FILECHARHEIGHT,
	   textwidth, textheight, 0, 0);

      /* . . .and append it to the text field */

#ifdef TCL_WRAPPER
      Tcl_Eval(xcinterp, ".filelist.textent.txt get");
      ebuf = (char *)Tcl_GetStringResult(xcinterp);
      tbuf = (char *)malloc((strlen(ebuf) +
		strlen(files[filenum].filename) + 6) * sizeof(char));
#else
      ebuf = (char *)XwTextCopyBuffer(textw);
      tbuf = (char *)malloc((XwTextGetLastPos(textw)
	     + strlen(files[filenum].filename) + 5) * sizeof(char));
#endif
      strcpy(tbuf, ebuf);

      /* add a comma if there is already text in the destination buffer */

      if (tbuf[0] != '\0') {
         if (tbuf[strlen(tbuf) - 1] != '/') strcat(tbuf, ",");
      }
      else {
	 if (cwdname != NULL) {
	    if (cwdname[0] != '\0') {
	       tbuf = (char *)realloc(tbuf, (strlen(cwdname) +
			strlen(files[filenum].filename) + 5) * sizeof(char));
	       strcpy(tbuf, cwdname);
	    }
 	 }
      }
      strcat(tbuf, files[filenum].filename);
#ifdef TCL_WRAPPER
      Tcl_Eval(xcinterp, ".filelist.textent.txt delete 0 end");
      sprintf(_STR2, ".filelist.textent.txt insert 0 %s", tbuf);
      Tcl_Eval(xcinterp, _STR2);
#else
      XwTextClearBuffer(textw);
      XwTextInsert(textw, tbuf);
      XwTextResize(textw);
#endif
      free(tbuf);
   }
   else {  /* move to new directory */

      if (!strcmp(files[filenum].filename, "../")) {
         char *cptr, *sptr = cwdname;
	 if (!strcmp(cwdname, "/")) return;	/* no way up from root dir. */
	 while(strstr(sptr, "../") != NULL) sptr += 3;
	 if ((cptr = strrchr(sptr, '/')) != NULL) {
	    *cptr = '\0';
	    if ((cptr = strrchr(sptr, '/')) != NULL) *(cptr + 1) = '\0';
	    else *sptr = '\0';
         }
	 else {
      	    cwdname = (char *)realloc(cwdname, (strlen(cwdname) +
	        4) * sizeof(char));
            strcat(cwdname, "../");
	 }
      }
      else {
	 cwdname = (char *)realloc(cwdname, (strlen(cwdname) +
	        strlen(files[filenum].filename) + 1) * sizeof(char));
         strcat(cwdname, files[filenum].filename);
      }
      newfilelist(w, okaystruct);
   }
}

/*-------------------------------------------------------------------------*/
/* Scrollbar handler for file list widget				   */
/*-------------------------------------------------------------------------*/

void showlscroll(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   Window swin = xcWindow(w);
   Dimension swidth, sheight;
   int pstart, pheight, finscr;

#ifdef TCL_WRAPPER
   swidth = Tk_Width(w);
   sheight = Tk_Height(w);
#else

   Arg wargs[2];
   short n = 0;

   XtnSetArg(XtNwidth, &swidth);
   XtnSetArg(XtNheight, &sheight);
   XtGetValues(w, wargs, n);
#endif

   XClearWindow(dpy, swin);

   if (flfiles > 0) {	/* no files, no bar */

      finscr = sheight / FILECHARHEIGHT;
      if (finscr > flfiles) finscr = flfiles;

      pstart = (flstart * sheight) / flfiles;
      pheight = (finscr * sheight) / flfiles;

      XSetForeground(dpy, sgc, colorlist[BARCOLOR].color.pixel);
      XFillRectangle(dpy, swin, sgc, 0, pstart, swidth, pheight);
   }
   flcurrent = -1;
}

/*-------------------------------------------------------------------------*/
/* Button Motion handler for moving the scrollbar up and down		   */
/*-------------------------------------------------------------------------*/

void draglscroll(xcWidget w, popupstruct *okaystruct, XButtonEvent *event)
{
   Dimension sheight;
   int phheight, finscr, flsave = flstart;
   xcWidget filew = okaystruct->filew;

#ifdef TCL_WRAPPER
   sheight = Tk_Height(w);
#else
   Arg wargs[1];
   short n = 0;

   XtnSetArg(XtNheight, &sheight);
   XtGetValues(w, wargs, n);
#endif

   finscr = sheight / FILECHARHEIGHT;
   if (finscr > flfiles) finscr = flfiles;

   /* center scrollbar on pointer vertical position */   

   phheight = (finscr * sheight) / (flfiles * 2);
   flstart = (event->y > phheight) ? ((event->y - phheight) * flfiles) / sheight : 0;
   if (flstart > (flfiles - finscr + 2)) flstart = (flfiles - finscr + 2);

   if (flstart != flsave) {
      showlscroll(w, NULL, NULL);
      listfiles(filew, okaystruct, NULL); 
   }
}

/*----------------------------------------------------------------------*/
/* Set/unset the file filtering function				*/
/*----------------------------------------------------------------------*/

#ifndef TCL_WRAPPER

void setfilefilter(xcWidget w, popupstruct *okaystruct, caddr_t calldata)
{
   xobjs.filefilter = (xobjs.filefilter) ? False : True;

   /* Force regeneration of the file list */

   newfilelist(okaystruct->filew, okaystruct);
}

#endif

/*----------------------------------------------------------------------*/
/* Generate the file list window					*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER

void genfilelist(xcWidget parent, popupstruct *okaystruct, Dimension width)
{
   xcWidget 	listarea, lscroll, entertext;

   entertext = okaystruct->textw;
   listarea = Tk_NameToWindow(xcinterp, ".filelist.listwin.win", parent);

   xcAddEventHandler(listarea, ButtonPressMask, False,
	  (xcEventHandler)fileselect, okaystruct);
   xcAddEventHandler(listarea, EnterWindowMask, False,
	  (xcEventHandler)startfiletrack, NULL);
   xcAddEventHandler(listarea, LeaveWindowMask, False,
	  (xcEventHandler)endfiletrack, NULL);
   flstart = 0;
   okaystruct->filew = listarea;

   lscroll = Tk_NameToWindow(xcinterp, ".filelist.listwin.sb", parent);

   Tk_CreateEventHandler(lscroll, Button1MotionMask | Button2MotionMask,
		(Tk_EventProc *)xctk_draglscroll, (ClientData)okaystruct);

   /* force new file list, in case the highlight filter changed */

   if (flistpix != (Pixmap)NULL) XFreePixmap(dpy, flistpix);
   flistpix = (Pixmap)NULL;
}

#else

void genfilelist(xcWidget parent, popupstruct *okaystruct, Dimension width)
{
   Arg		wargs[8];
   xcWidget 	listarea, lscroll, entertext, dofilter;
   short 	n = 0;
   int		wwidth;
   int		sbarsize;

#ifdef TCL_WRAPPER
   char *scale;
   scale = Tcl_GetVar2(xcinterp, "XCOps", "scale", TCL_GLOBAL_ONLY);
   sbarsize = SBARSIZE * atoi(scale);
#else
   sbarsize = SBARSIZE;
#endif

   XtnSetArg(XtNx, 20);
   XtnSetArg(XtNy, FILECHARHEIGHT - 10);
   XtnSetArg(XtNwidth, width - sbarsize - 40);
   XtnSetArg(XtNheight, LISTHEIGHT - FILECHARHEIGHT);
   XtnSetArg(XtNfont, appdata.filefont);

   entertext = okaystruct->textw;

   listarea = XtCreateManagedWidget("Filelist", XwworkSpaceWidgetClass,
	  parent, wargs, n); n = 0;
   XtAddCallback(listarea, XtNexpose, (XtCallbackProc)listfiles, okaystruct);

   xcAddEventHandler(listarea, ButtonPressMask, False,
	  (xcEventHandler)fileselect, okaystruct);
   xcAddEventHandler(listarea, EnterWindowMask, False,
	  (xcEventHandler)startfiletrack, NULL);
   xcAddEventHandler(listarea, LeaveWindowMask, False,
	  (xcEventHandler)endfiletrack, NULL);
   flstart = 0;
   okaystruct->filew = listarea;

   XtnSetArg(XtNx, width - sbarsize - 20);
   XtnSetArg(XtNy, FILECHARHEIGHT - 10);
   XtnSetArg(XtNwidth, sbarsize);
   XtnSetArg(XtNheight, LISTHEIGHT - FILECHARHEIGHT);
   XtnSetArg(XtNfont, appdata.xcfont);

   lscroll = XtCreateManagedWidget("LScroll", XwworkSpaceWidgetClass,
	     parent, wargs, n); n = 0;

   XtAddCallback(lscroll, XtNexpose, (XtCallbackProc)showlscroll, NULL);
   xcAddEventHandler(lscroll, Button1MotionMask | Button2MotionMask,
	     False, (xcEventHandler)draglscroll, okaystruct);

   /* Add a toggle widget to turn file filtering on/off */

   wwidth = XTextWidth(appdata.xcfont, "filter", strlen("filter"));
   XtnSetArg(XtNx, width - wwidth - 50);
   XtnSetArg(XtNy, LISTHEIGHT + 10);
   XtnSetArg(XtNset, xobjs.filefilter);
   XtnSetArg(XtNsquare, True);
   XtnSetArg(XtNborderWidth, 0);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNlabelLocation, XwLEFT);
   dofilter = XtCreateManagedWidget("Filter", XwtoggleWidgetClass,
		parent, wargs, n); n = 0;
   XtAddCallback(dofilter, XtNselect, (XtCallbackProc)setfilefilter, okaystruct);
   XtAddCallback(dofilter, XtNrelease, (XtCallbackProc)setfilefilter, okaystruct);

   /* force new file list, in case the highlight filter changed */

   if (flistpix != (Pixmap)NULL) XFreePixmap(dpy, flistpix);
   flistpix = (Pixmap)NULL;
}

#endif 	/* TCL_WRAPPER */

#endif

/*-------------------------------------------------------------------------*/
/* Look for a directory name in a string and update cwdname accordingly	   */
/*-------------------------------------------------------------------------*/

int lookdirectory(char *lstring, int nchars)
{
   int slen;
   DIR *cwd = NULL;

   xc_tilde_expand(lstring, nchars);
   slen = strlen(lstring);

   if (lstring[slen - 1] == '/' || ((cwd=opendir(lstring)) != NULL)) {
      if (cwd) closedir(cwd);
      if (lstring[slen - 1] != '/') strcat(lstring, "/");
      cwdname = (char *)realloc(cwdname, (slen + 2) * sizeof(char));
      strcpy(cwdname, lstring);
      return(1);
   }
   return(0);
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* fontfile.c --- Load font character and encoding defitions		   */
/* Copyright (c) 2001  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/
   
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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

/*-------------------------------------------------------------------------*/
/* Local includes                                                          */
/*-------------------------------------------------------------------------*/

#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*-------------------------------------------------------------------------*/
/* Global Variable definitions						   */
/*-------------------------------------------------------------------------*/

extern char _STR2[250], _STR[150];

extern short fontcount;
extern fontinfo *fonts;

extern Globaldata xobjs;
extern short beeper;
#ifdef HAVE_CAIRO
extern XCWindowData *areawin;
extern const char *utf8encodings[][256];
#endif

extern float version;

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif

/*----------------------------------------------------------------------*/
/* Find the file containing the encoding for the given font		*/
/*----------------------------------------------------------------------*/

FILE *findfontfile(char *fontname)
{
   size_t i;
   char tempname[256];
   FILE *fd;

   /* Add subdirectory "fonts".  We will try both with and	*/
   /* without the subdirectory.					*/

   sprintf(_STR, "fonts/%s", fontname);

   /* change string to all lowercase and remove dashes */

   for (i = 0; i < strlen(_STR); i++) {
      _STR[i] = tolower(_STR[i]);
      if (_STR[i] == '-') _STR[i] = '_';
   }

   /* Fprintf(stdout, "Searching for font file \"%s\"\n", _STR); */

   /* Use the mechanism of "libopen" to find the encoding file	*/
   /* in the search path					*/

   fd = libopen(_STR + 6, FONTENCODING, NULL, 0);

   if (fd == NULL) fd = libopen(_STR, FONTENCODING, NULL, 0);

   /* Some other, probably futile, attempts (call findfontfile recursively) */

   if (fd == NULL) {
      char *dashptr;

      /* If this font has a suffix, remove it and look for its root */
      /* font on the supposition that this font is derived from the */
      /* root font.						    */

      strncpy(tempname, fontname, 99);
      if ((dashptr = strrchr(tempname, '-')) != NULL) {
	 *dashptr = '\0';
	 if ((fd = findfontfile(tempname)) != NULL) return fd;

	 /* And finally, because it's a common case, try adding	*/
	 /* -Roman and trying again (but don't infinite loop!)	*/

	 if (strcmp(dashptr + 1, "Roman")) {
	    strcat(dashptr, "-Roman");
	    if ((fd = findfontfile(tempname)) != NULL) return fd;
	 }
      }

      Wprintf("No font encoding file found.");
      if (fontcount > 0) {	/* Make font substitution */
	 char *dchr, *psname = NULL;
	 short fval;

	 if ((dchr = strrchr(_STR, '.')) != NULL) *dchr = '\0';
	 if ((fval = findhelvetica()) == fontcount) {
	    /* This will cause some chaos. . . */
	    Fprintf(stderr, "Error:  No fonts available!  Check library path?\n");
	    exit(1);
	 }

	 psname = (char *)malloc((1 + strlen(fontname)) * sizeof(char));
	 strcpy(psname, fontname);
	 Wprintf("No encoding file found for font %s: substituting %s",
			psname, fonts[fval].psname);
         fonts = (fontinfo *)realloc(fonts, (fontcount + 1) * sizeof(fontinfo));
         fonts[fontcount].psname = psname;
         fonts[fontcount].family = psname;
         fonts[fontcount].encoding = fonts[fval].encoding;
         fonts[fontcount].flags = 0; 
         fonts[fontcount].scale = 1.0;
         fontcount++;
	 makenewfontbutton();
      }
      else {
	 Fprintf(stderr, "Error:  font encoding file missing for font \"%s\"\n",
		fontname);
	 Fprintf(stderr, "No fonts exist for a subsitution.  Make sure "
		"fonts are installed or that\nenvironment variable "
		"XCIRCUIT_LIB_DIR points to a directory of valid fonts.\n");
      }
      return (FILE *)NULL;
   }
   return fd;
}

/*----------------------------------------------------------------------*/
/* Load a named font							*/
/* Return 1 if successful, 0 if font is already present, and -1 if any	*/
/* other error occurred.						*/
/*----------------------------------------------------------------------*/

int loadfontfile(char *fname)
{
   FILE *fd;
   char temp[250], commandstr[30], tempname[100];
   char *psname = NULL, *family = NULL;
   char *cmdptr;
   int flags = 0;
   int i;
   float fontscale = 1.0;
   objectptr *j, *eptr;
   objectptr *encoding = NULL;
   float saveversion = version;
   char *fontname = strdup(fname);
#ifdef HAVE_CAIRO
   const char **utf8enc = NULL;
#endif /* HAVE_CAIRO */

   /* check to see if font name is in list of fonts */

   for (i = 0; i < fontcount; i++) {
      if (!strcmp(fonts[i].psname, fontname)) {
	 free(fontname);
	 return 0;
      }
   }
   if ((fd = findfontfile(fontname)) == NULL) return -1;

   while (fgets(temp, 249, fd) != NULL) {
      if (*temp == '\n') continue;
      sscanf(temp, "%29s", commandstr);
      for(cmdptr = commandstr; isspace(*cmdptr); cmdptr++);

      /* very liberal about comment line characters */

      if (*cmdptr == '#' || *cmdptr == '%' || *cmdptr == ';');

      else if (!strcmp(cmdptr, "name:")) {
	 sscanf(temp, "%*s %99s", tempname);

         /* If font is already in list, ignore it 			   */
	 /* This condition should be caught at the top of the routine. . . */

         for (i = 0; i < fontcount; i++) {
            if (!strcmp(fonts[i].psname, tempname)) {
	       /* Fprintf(stdout, "Font already loaded.\n"); */
	       fclose(fd);
	       free(fontname);
	       return 0;
	    }
	 }
	 psname = (char *)malloc((1 + strlen(tempname)) * sizeof(char));
	 strcpy(psname, tempname);
      }

      else if (!strcmp(cmdptr, "file:") || !strcmp(cmdptr, "load:")) {
	 /* Fprintf(stdout, "found file identifier in xfe file\n"); */
	 sscanf(temp, "%*s %149s", _STR);

	 /* since we can be in the middle of a file load, protect the	*/
	 /* current version number for the file load.			*/

	 version = PROG_VERSION;
	 if (loadlibrary(FONTLIB) == FALSE) {
	 };
	 version = saveversion;
      }

      else if (!strcmp(cmdptr, "family:")){
	 sscanf(temp, "%*s %99s", tempname);
	 family = (char *)malloc((1 + strlen(tempname)) * sizeof(char));
	 strcpy(family, tempname);
      }

      else if (!strcmp(cmdptr, "weight:")){
	 sscanf(temp, "%*s %99s", tempname);
	 tempname[0] = tolower(tempname[0]);
	 if (!strcmp(tempname, "bold"))
	    flags |= 0x01;
      }

      else if (!strcmp(cmdptr, "shape:")){
	 sscanf(temp, "%*s %99s", tempname);
	 tempname[0] = tolower(tempname[0]);
	 if (!strcmp(tempname, "italic") || !strcmp(tempname, "oblique")
		|| !strcmp(tempname, "slanted"))
	    flags |= 0x02;
      }

      else if (!strcmp(cmdptr, "scale:")){
	 sscanf(temp, "%*s %f", &fontscale);
      }

      else if (!strcmp(cmdptr, "type:")) {
	 sscanf(temp, "%*s %99s", tempname);
	 tempname[0] = tolower(tempname[0]);
	 if (!strcmp(tempname, "drawn") || !strcmp(tempname, "vectored"))
	    flags |= 0x08;
      }

      else if (!strcmp(cmdptr, "derived:")) {
	 if (encoding == NULL) {
	    Fprintf(stdout, "Font warning: \"derived\" statement must come "
			"after encoding\n");
	 }
	 else {
	    char *psname2;
	    sscanf(temp, "%*s %99s", tempname);
	    psname2 = (char *)malloc((1 + strlen(tempname)) * sizeof(char));
	    strcpy(psname2, tempname);
	    flags &= 0xffe0;		/* shares these flags with original */
	    flags |= 0x020;		/* derived font flag		    */

	    /* determine rest of flags */

	    sscanf(temp, "%*s %*s %99s", tempname);
	    tempname[0] = tolower(tempname[0]);
	    if (!strcmp(tempname, "bold")) {
	       flags |= 0x1;
	    }

	    sscanf(temp, "%*s %*s %*s %99s", tempname);
	    tempname[0] = tolower(tempname[0]);
	    if (!strcmp(tempname, "italic") || !strcmp(tempname, "oblique"))
	       flags |= 0x2;

	    sscanf(temp, "%*s %*s %*s %*s %99s", tempname);
	    tempname[0] = tolower(tempname[0]);
	    if (!strcmp(tempname, "drawn") || !strcmp(tempname, "vectored"))
	       flags |= 0x08;
	    else if (!strcmp(tempname, "special"))
	       flags |= 0x10;

	    /* generate a new fontinfo entry for the derived font */

            fonts = (fontinfo *) realloc (fonts, (fontcount + 1) * sizeof(fontinfo));
            fonts[fontcount].psname = psname2;
            if (family == NULL)
               fonts[fontcount].family = psname;
            else
               fonts[fontcount].family = family;
            fonts[fontcount].encoding = encoding;  /* use original encoding */
            fonts[fontcount].flags = flags; 
	    fonts[fontcount].scale = fontscale;
#ifdef HAVE_CAIRO
	    fonts[fontcount].utf8encoding = utf8enc;
	    if (areawin->area) {
	       xc_cairo_set_fontinfo(fontcount);
	    }
#endif /* HAVE_CAIRO */
            fontcount++;
	 }
      }

      else if (!strcmp(cmdptr, "encoding:")) {
	 #ifdef HAVE_CAIRO
	 const char *findencstr;
         #endif /* HAVE_CAIRO */
	 sscanf(temp, "%*s %99s", tempname);

	 if (!strcmp(tempname, "special") || !strcmp(tempname, "Special")) {
	    flags |= 0x80;
#ifdef TCL_WRAPPER
	    XcInternalTagCall(xcinterp, 3, "label", "encoding", "Special");
#else
	    makenewencodingbutton("Special", (char)1);
#endif
	 }
	
#ifdef HAVE_CAIRO
	    utf8enc = utf8encodings[0];
	    /* treat encoding of symbol font in a special way */
	    if (!strcmp(family, "Symbol"))
	       findencstr = "Symbol";
	    else
	       findencstr = tempname;

	    /* Now try to find the encoding, */
	    /* default to the first encoding (stdenc) if not found */
	    utf8enc = utf8encodings[0];
	    for (i = 0; utf8encodings[i][0]; i++)
	       if (!strcmp(utf8encodings[i][0], findencstr)) {
		  utf8enc = utf8encodings[i];
		  break;
	       }
#endif /* HAVE_CAIRO */

	 /* ISO-LatinX encodings where X=1 to 6 */

	 if (strstr(tempname, "Latin") != NULL) {
	    char estr[12];
	    for (i = 0; i < 6; i++) {
	       if (strchr(tempname, '1' + (char)i) != NULL) {
	          flags |= ((i + 2) << 7);
#ifdef TCL_WRAPPER
		  snprintf(estr, sizeof(estr), "ISOLatin%d", i + 1);
		  XcInternalTagCall(xcinterp, 3, "label", "encoding", estr);
#else
		  snprintf(estr, sizeof(estr), "ISO-Latin%d", i + 1);
	          makenewencodingbutton(estr, (char)(i + 2));
#endif
		  break;
	       }
	    }
	 }
	 /* i == 6 maps to ISO8859-5 */
	 if (strstr(tempname, "8859-5") != NULL) {
#ifndef TCL_WRAPPER
	    char estr[12];
#endif
	    flags |= ((6 + 2) << 7);
#ifdef TCL_WRAPPER
	    XcInternalTagCall(xcinterp, 3, "label", "encoding", "ISO8859-5");
#else
	    snprintf(estr, sizeof(estr), "ISO-8859-5");
	    makenewencodingbutton(estr, (char)8);
#endif
	 }

         /* Make space for the font encoding vector */

         encoding = (objectptr *)malloc(256 * sizeof(objectptr));
         eptr = encoding;

	 while (fgets(temp, 249, fd) != NULL) {
	    char *temp2 = temp;
	    while (*temp2 != '\0') {
	       if ((int)(eptr - encoding) == 256) break;
	       sscanf(temp2, "%99s", tempname);
	       *eptr = (objectptr) NULL;
	       for (j = xobjs.fontlib.library; j < xobjs.fontlib.library +
			xobjs.fontlib.number; j++) {
	          if (!strcmp(tempname, (*j)->name)) {
	             *eptr = (*j);
		     break;
	          }
	       }
	       if (j == xobjs.fontlib.library + xobjs.fontlib.number) {
	          Fprintf(stdout, "Font load warning: character \"%s\" at code ",
				tempname);
	 	  Fprintf(stdout, "position %d not found.\n", (int)(eptr - encoding));
	       }
	       eptr++;
	       while (*temp2 != ' ' && *temp2 != '\n' && *temp2 != '\0') temp2++;
	       while (*temp2 == ' ' || *temp2 == '\n') temp2++;
	    }
	    if ((int)(eptr - encoding) == 256) break;
	 }
	 if ((int)(eptr - encoding) != 256) {
	    Fprintf(stdout, "Font load warning: Only %d characters encoded.\n", 
		  (int)(eptr - encoding));
	    while (eptr < encoding + 256)
	       *eptr++ = (objectptr) NULL;
	 }

         /* If successful, register the font */

         fonts = (fontinfo *) realloc (fonts, (fontcount + 1) * sizeof(fontinfo));
         fonts[fontcount].psname = psname;
         if (family == NULL)
            fonts[fontcount].family = psname;
         else
            fonts[fontcount].family = family;
         fonts[fontcount].encoding = encoding;
         fonts[fontcount].flags = flags; 
         fonts[fontcount].scale = fontscale;
#ifdef HAVE_CAIRO
	 fonts[fontcount].utf8encoding = utf8enc;
         if (areawin->area) {
	    xc_cairo_set_fontinfo(fontcount);
	 }
#endif /* HAVE_CAIRO */
         fontcount++;

         /* Create a new menu button for the font family, if this is the first	*/
	 /* (In Tcl, this just registers the family name for cycling through	*/
	 /* via Alt-F; the menu update is done via command tag callback).	*/

	 for (i = 0; i < fontcount - 1; i++)
	    if (!strcmp(fonts[i].family, fonts[fontcount - 1].family))
	       break;
    
	 if (i == fontcount - 1)
	    makenewfontbutton();
      }
   }
   fclose(fd);
   free(fontname);
   return 1;
}

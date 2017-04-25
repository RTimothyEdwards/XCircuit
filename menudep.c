/*----------------------------------------------------------------------*/
/* menudep.c								*/
/*   Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	*/
/*----------------------------------------------------------------------*/
/* Parse file menus.h and create file of definitions for use by		*/
/* "NameToWidget" to find the menu in the widget menu tree.		*/
/*----------------------------------------------------------------------*/
/*									*/
/* example:  For  menustruct Fonts[], 					*/
/*   #define FontsButton 1						*/
/*									*/
/* Name is the menustruct name appended with the word "Button".		*/
/* The Widget itself will be placed in the WidgetList "menuwidgets"	*/
/*									*/
/* NAMING CONVENTIONS:							*/
/*    Cascade Widget:  name of menustruct + "Button"			*/
/*    Button Widgets:  name of menustruct + modified name of button	*/
/*				+ "Button"				*/
/*									*/
/*       "modified name" means that only alphanumeric characters are	*/
/*	 used and the macro (if any) in parentheses is removed.		*/
/*----------------------------------------------------------------------*/

#ifndef TCL_WRAPPER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#endif

#include "xcircuit.h"

#define _MENUDEP
#include "menus.h"

int menucount;
FILE *mp;

/*----------------------------------------------------------------------*/

char *normalize(char *textin)
{
   char *tptr = textin;
   char *sptr;
   char *newptr = (char *)malloc((strlen(textin) + 1) * sizeof(char));
   sptr = newptr;

   while (*tptr != '\0') {
      switch(*tptr) {
	 case ' ': case ':': case '/': case '!': case '_': case '-':
	 case '&': case '.': case '<': case '>':
	    break;
	 case '(':
	    while (*tptr != '\0' && *tptr != ')') tptr++;
	    if (*tptr == '\0') {
	       printf("Open parenthesis in menu.h: %s\n", textin);
	       exit(1);
	    }
	    break;
	 default:
	    *sptr++ = *tptr;
	    break;
      }
      tptr++;
   }
   *sptr = '\0';
   return newptr;
}

/*----------------------------------------------------------------------*/

void log_entry(char *menuname, char *buttonname)
{
   char *n1, *n2;
   n1 = normalize(menuname);
   n2 = normalize(buttonname);
   if (strlen(n2) == 0)
      menucount++;
   else
      fprintf(mp, "#define %s%sButton  (menuwidgets[%d])\n",
			n1, n2, menucount++);
   free(n1);
   free(n2);
}

/*----------------------------------------------------------------------*/

void toolbar_entry(char *buttonname)
{
   char *n1;
   n1 = normalize(buttonname);
   fprintf(mp, "#define %sToolButton  (menuwidgets[%d])\n", n1, menucount++);
   free(n1);
}

/*----------------------------------------------------------------------*/

void searchsubmenu(char *menuname, menuptr buttonmenu, int arraysize)
{
   menuptr p;

   for (p = buttonmenu; p < buttonmenu + arraysize; p++) {
      if (p->submenu != NULL)
	 searchsubmenu(p->name, p->submenu, p->size);
      else if (p->name[0] != ' ') /* anything but a separator */
	 log_entry(menuname, p->name);
   } 
}

/*----------------------------------------------------------------------*/

int main()
{
   short i;

   menucount = 0;

   if ((mp = fopen("menudep.h", "w")) == NULL) {
      printf("Unable to open menudep.h for output\n");
      exit(1);
   }

   fprintf(mp, "/*-------------------------------------------------------*/\n");
   fprintf(mp, "/* menudep.h  generated automatically by program menudep */\n");
   fprintf(mp, "/*            from file menus.h.  Do not edit.           */\n");
   fprintf(mp, "/*-------------------------------------------------------*/\n\n");

   /* run the same routine as "createmenus()" in xcircuit.c so that the */
   /* ordering will be the same.					*/

   for (i = 0; i < maxbuttons; i++)
      searchsubmenu(TopButtons[i].name, TopButtons[i].submenu,
            TopButtons[i].size);

#ifdef HAVE_XPM
   for (i = 0; i < toolbuttons; i++)
      toolbar_entry(ToolBar[i].name);
#endif

   fprintf(mp, "#define MaxMenuWidgets %d\n", menucount);
   fprintf(mp, "\n/*-------------------------------------------------------*/\n\n");
	       
   fclose(mp);
   exit(0);
}

#else

#include <stdio.h>

int main()
{
   /* create empty file menudep.h */
   FILE *fid = fopen("menudep.h", "w");
   fclose(fid);
   return 0;
}

#endif /* TCL_WRAPPER */

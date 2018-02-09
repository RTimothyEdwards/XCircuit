/*--------------------------------------------------------------*/
/* xcwrap.c:							*/
/*	Tcl module generator for xcircuit			*/
/*--------------------------------------------------------------*/

#ifdef TCL_WRAPPER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tk.h>

Tcl_Interp *xcinterp;
Tcl_Interp *consoleinterp;

/*------------------------------------------------------*/
/* Redefine all of the callback functions for Tcl/Tk    */    
/*------------------------------------------------------*/    

extern int Tk_SimpleObjCmd();
extern int xctcl_standardaction();
extern int xctcl_action();
extern int xctcl_refresh();
extern int xctcl_library();
extern int xctcl_tech();
extern int xctcl_font();
extern int xctcl_cursor();
extern int xctcl_bind();
extern int xctcl_filerecover();
extern int xctcl_color();
extern int xctcl_dofill();
extern int xctcl_doborder();
extern int xctcl_promptquit();
extern int xctcl_quit();
extern int xctcl_promptsavepage();
extern int xctcl_eventmode();
extern int xctcl_delete();
extern int xctcl_undo();
extern int xctcl_redo();
extern int xctcl_copy();
extern int xctcl_move();
extern int xctcl_edit();
extern int xctcl_param();
extern int xctcl_select();
extern int xctcl_deselect();
extern int xctcl_push();
extern int xctcl_pop();
extern int xctcl_rotate();
extern int xctcl_flip();
extern int xctcl_config();
extern int xctcl_page();
extern int xctcl_spice();
extern int xctcl_svg();
extern int xctcl_zoom();
extern int xctcl_pan();
extern int xctcl_element();
extern int xctcl_label();
extern int xctcl_polygon();
extern int xctcl_instance();
extern int xctcl_spline();
extern int xctcl_path();
extern int xctcl_arc();
extern int xctcl_graphic();
extern int xctcl_object();
extern int xctcl_here();
extern int xctcl_start();
extern int xctcl_netlist();
extern int xctcl_symschem();
extern int xctcl_tag();

typedef struct {
   const char	*cmdstr;
   void		(*func)();
} cmdstruct;

static cmdstruct xc_commands[] =
{
   {"action", (void *)xctcl_action},
   {"standardaction", (void *)xctcl_standardaction},
   {"start", (void *)xctcl_start},
   {"tag", (void *)xctcl_tag},
   {"refresh", (void *)xctcl_refresh},
   {"library", (void *)xctcl_library},
   {"technology", (void *)xctcl_tech},
   {"loadfont", (void *)xctcl_font},
   {"cursor", (void *)xctcl_cursor},
   {"bindkey", (void *)xctcl_bind},
   {"filerecover", (void *)xctcl_filerecover},
   {"color", (void *)xctcl_color},
   {"fill", (void *)xctcl_dofill},
   {"border", (void *)xctcl_doborder},
   {"quit", (void *)xctcl_promptquit},
   {"quitnocheck", (void *)xctcl_quit},
   {"promptsavepage", (void *)xctcl_promptsavepage},
   {"eventmode", (void *)xctcl_eventmode},
   {"delete", (void *)xctcl_delete},
   {"undo", (void *)xctcl_undo},
   {"redo", (void *)xctcl_redo},
   {"select", (void *)xctcl_select},
   {"deselect", (void *)xctcl_deselect},
   {"copy", (void *)xctcl_copy},
   {"move", (void *)xctcl_move},
   {"edit", (void *)xctcl_edit},
   {"parameter", (void *)xctcl_param},
   {"push", (void *)xctcl_push},
   {"pop", (void *)xctcl_pop},
   {"rotate", (void *)xctcl_rotate},
   {"flip", (void *)xctcl_flip},
   {"config", (void *)xctcl_config},
   {"page", (void *)xctcl_page},
   {"zoom", (void *)xctcl_zoom},
   {"pan", (void *)xctcl_pan},
   {"element", (void *)xctcl_element},
   {"label", (void *)xctcl_label},
   {"polygon", (void *)xctcl_polygon},
   {"instance", (void *)xctcl_instance},
   {"spline", (void *)xctcl_spline},
   {"path", (void *)xctcl_path},
   {"arc", (void *)xctcl_arc},
   {"graphic", (void *)xctcl_graphic},
   {"object", (void *)xctcl_object},
   {"here", (void *)xctcl_here},
   {"netlist", (void *)xctcl_netlist},
   {"symbol", (void *)xctcl_symschem},
   {"schematic", (void *)xctcl_symschem},
   {"spice", (void *)xctcl_spice},
   {"svg", (void *)xctcl_svg},
   {"", NULL}  /* sentinel */
};

extern Tcl_HashTable XcTagTable;

/*--------------------------------------------------------------*/
/* Initialization procedure for Tcl/Tk				*/
/*--------------------------------------------------------------*/

#ifdef _MSC_VER
__declspec(dllexport)
#endif
int
Xcircuit_Init(Tcl_Interp *interp)
{
   char command[256];
   int cmdidx;
   Tk_Window tktop;
   char *tmp_s, *tmp_l;
   char *cadhome;
   char version_string[20];

   /* Interpreter sanity checks */
   if (interp == NULL) return TCL_ERROR;

   /* Remember the interpreter */
   xcinterp = interp;

   if (Tcl_InitStubs(interp, "8.1", 0) == NULL) return TCL_ERROR;
   
   tmp_s = getenv("XCIRCUIT_SRC_DIR");
   if (tmp_s == NULL) tmp_s = SCRIPTS_DIR;

   tmp_l = getenv("XCIRCUIT_LIB_DIR");
   if (tmp_l == NULL) tmp_l = BUILTINS_DIR;
 
   strcpy(command, "xcircuit::");

   /* Create the start command */

   tktop = Tk_MainWindow(interp);

   /* Create all of the commands (except "simple") */

   for (cmdidx = 0; xc_commands[cmdidx].func != NULL; cmdidx++) {
      sprintf(command + 10, "%s", xc_commands[cmdidx].cmdstr);
      Tcl_CreateObjCommand(interp, command, 
		(Tcl_ObjCmdProc *)xc_commands[cmdidx].func,
		(ClientData)tktop, (Tcl_CmdDeleteProc *) NULL);
   }

   /* Command which creates a "simple" window (this is a top-	*/
   /* level command, not one in the xcircuit namespace which	*/
   /* is why I treat it separately from the other commands).	*/

   Tcl_CreateObjCommand(interp, "simple",
		(Tcl_ObjCmdProc *)Tk_SimpleObjCmd,
		(ClientData)tktop, (Tcl_CmdDeleteProc *) NULL);

   sprintf(command, "lappend auto_path %s", tmp_s);
   Tcl_Eval(interp, command);
   if (!strstr(tmp_s, "tcl")) {
      sprintf(command, "lappend auto_path %s/tcl", tmp_s);
      Tcl_Eval(interp, command);
   }
   
   if (strcmp(tmp_s, SCRIPTS_DIR))
      Tcl_Eval(interp, "lappend auto_path " SCRIPTS_DIR );

   /* Set $XCIRCUIT_SRC_DIR and $XCIRCUIT_LIB_DIR as Tcl variables */

   Tcl_SetVar(interp, "XCIRCUIT_SRC_DIR", tmp_s, TCL_GLOBAL_ONLY);
   Tcl_SetVar(interp, "XCIRCUIT_LIB_DIR", tmp_l, TCL_GLOBAL_ONLY);

   /* Set $CAD_ROOT as a Tcl variable */

   cadhome = getenv("CAD_ROOT");
   if (cadhome == NULL) cadhome = CAD_DIR;
   Tcl_SetVar(interp, "CAD_ROOT", cadhome, TCL_GLOBAL_ONLY);

   /* Set $XCIRCUIT_VERSION and $XCIRCUIT_REVISION as Tcl variables */

   sprintf(version_string, "%s", PROG_REVISION);
   Tcl_SetVar(interp, "XCIRCUIT_REVISION", version_string, TCL_GLOBAL_ONLY);

   sprintf(version_string, "%s", PROG_VERSION);
   Tcl_SetVar(interp, "XCIRCUIT_VERSION", version_string, TCL_GLOBAL_ONLY);

#ifdef ASG
   /* Set a variable in Tcl indicating that the ASG module is compiled in */
   Tcl_SetVar(interp, "XCIRCUIT_ASG", "1", TCL_GLOBAL_ONLY);
#endif

   /* Export the namespace commands */

   Tcl_Eval(interp, "namespace eval xcircuit namespace export *");
   Tcl_PkgProvide(interp, "Xcircuit", version_string);

   /* Initialize the console interpreter, if there is one. */

   if ((consoleinterp = Tcl_GetMaster(interp)) == NULL)
      consoleinterp = interp;

   /* Initialize the command tag table */

   Tcl_InitHashTable(&XcTagTable, TCL_STRING_KEYS);

   return TCL_OK;
}

#endif /* TCL_WRAPPER */

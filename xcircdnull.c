/*----------------------------------------------------------------------*/
/* xcircdnull.c								*/
/*									*/
/* See comments for "xcircexec.c".  This has the same function as	*/
/* xcircexec.c, but does not initialize the Tk package.  This avoids	*/
/* problems attempting to run in an environment without a DISPLAY	*/
/* variable set (true batch mode).					*/
/*----------------------------------------------------------------------*/

#include <stdio.h>

#include <tcl.h>

/*----------------------------------------------------------------------*/
/* Application initiation.  This is exactly like the AppInit routine	*/
/* for "wish", minus the cruft, but with "tcl_rcFileName" set to	*/
/* "xcircuit.tcl" instead of "~/.wishrc".				*/
/*----------------------------------------------------------------------*/

int
xcircuit_AppInit(Tcl_Interp *interp)
{
    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tcl", Tcl_Init, Tcl_Init);

    /* This is where we replace the home ".tclshrc" file with	*/
    /* xcircuit's startup script.				*/

    Tcl_SetVar(interp, "tcl_rcFileName", SCRIPTS_DIR "/xcircuit.tcl",
		TCL_GLOBAL_ONLY);

    /* Additional variable can be used to tell if xcircuit is in batch mode */
    Tcl_SetVar(interp, "batch_mode", "true", TCL_GLOBAL_ONLY);

    return TCL_OK;
}

/*----------------------------------------------------------------------*/
/* The main procedure;  replacement for "wish".				*/
/*----------------------------------------------------------------------*/

int
main(int argc, char **argv)
{
    Tcl_Main(argc, argv, xcircuit_AppInit);
    return 0;
}

/*----------------------------------------------------------------------*/

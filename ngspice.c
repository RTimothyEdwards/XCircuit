/*----------------------------------------------------------------------*/
/* ngspice.c --- Handling of background ngspice process.		*/
/* Copyright (c) 2004  Tim Edwards, MultiGiG, Inc.			*/
/* These routines work only if ngspice is on the system.		*/
/*----------------------------------------------------------------------*/

/* #undef SPICE_DEBUG */
#define SPICE_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <time.h>
#ifndef _MSC_VER
#include <sys/wait.h>   /* for waitpid() */
#include <sys/time.h>
#include <ctype.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

/* This code should be able to run without requiring Tcl-based XCircuit	*/
/* but this has not been worked out, so instead it is just disabled.	*/

#ifdef TCL_WRAPPER

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include <tk.h>

#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations					*/
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*------------------------------------------------------------------------*/
/* External Variable definitions                                          */
/*------------------------------------------------------------------------*/

extern char _STR2[250], _STR[150];
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern Display *dpy;
extern int number_colors;
extern colorindex *colorlist;
extern Cursor appcursors[NUM_CURSORS];

#ifndef HAVE_VFORK
#define vfork fork
#endif

int spice_state;	/* Track state of the simulator */

#define SPICE_INIT 0	/* Initial state; ngspice interpreter is idle.	*/
#define SPICE_BUSY 1	/* Simulation in progress; ngspice is busy.	*/
#define SPICE_READY 2	/* Simulation at break; ngspice is waiting for	*/
			/* "resume".					*/

#define EXPECT_ANY    0	/* Read routine accepts all input until input	*/
			/* buffer is empty, then returns.		*/
#define EXPECT_PROMPT 1	/* Read routine accepts all input up to the	*/
			/* next ngspice prompt, removes the prompt,	*/
			/* sets the Tcl return value to the history	*/
			/* number, and returns.				*/
#define EXPECT_REF    2	/* Read routine accepts all input until the	*/
			/* first "Reference value" line, truncates the	*/
			/* output, and sets the Tcl return value to the	*/
			/* reference value seen.			*/

#ifndef CONST84
#define CONST84
#endif

/*------------------------------------------------------*/
/* Global variable definitions				*/
/*------------------------------------------------------*/

#ifndef _MSC_VER
pid_t spiceproc = -1;	   	/* ngspice process	*/
#else
HANDLE spiceproc = INVALID_HANDLE_VALUE;
#endif

int   pipeRead, pipeWrite; /* I/O pipe pair for ngspice	*/

/*------------------------------------------------------*/
/* Start a ngspice process and arrange the I/O pipes  	*/
/* (Commented lines cause ngspice to relay its stderr 	*/
/* to xcircuit's stderr)				*/
/*							*/
/* Return codes:					*/
/*   0	success						*/
/*  -1	failed---ngspice exited with error condition	*/
/*  -2	exec failed---e.g., unable to find ngspice	*/
/*   1	ngspice is already running			*/
/*------------------------------------------------------*/

#ifdef _MSC_VER

int start_spice()
{
   STARTUPINFO st_info;
   PROCESS_INFORMATION pr_info;
   SECURITY_ATTRIBUTES attr;
   HANDLE child_stdin, child_stdout, tmp;
   char cmd[4096];

   attr.nLength = sizeof(SECURITY_ATTRIBUTES);
   attr.lpSecurityDescriptor = NULL;
   attr.bInheritHandle = FALSE;

   if (!CreatePipe(&tmp, &child_stdout, &attr, 0))
      goto spice_error;

   SetHandleInformation(tmp, HANDLE_FLAG_INHERIT, 0);
   pipeRead = _open_osfhandle(tmp, _O_RDONLY);
   if (!CreatePipe(&child_stdin, &tmp, &attr, 0))
		goto spice_error;
   SetHandleInformation(tmp, HANDLE_FLAG_INHERIT, 0);
   pipeWrite = _open_osfhandle(tmp, _O_WRONLY);

   ZeroMemory(&pr_info, sizeof(PROCESS_INFORMATION));
   ZeroMemory(&st_info, sizeof(STARTUPINFO));
   st_info.cb = sizeof(STARTUPINFO);
   st_info.dwFlags = STARTF_USESTDHANDLES;
   st_info.hStdError = st_info.hStdOutput = child_stdout;
   st_info.hStdInput = child_stdin;

   snprintf(cmd, 4095, "%s -p", SPICE_EXEC);
   if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL,
		&st_info, &pr_info) == 0)
      goto spice_error;

   CloseHandle(pr_info.hThread);
   spiceproc = pr_info.hProcess;
   return 0;

spice_error:
   Fprintf(stderr, "Exec of ngspice failed\n");
   return -2;
}

#else /* ! MSC_VER */

int start_spice()
{
   int std_in[2], std_out[2], ret;

   ret = pipe(std_in);
   ret = pipe(std_out);

   /* Input goes from xcircuit	to ngspice to provide commands to the	*/
   /* ngspice interpreter front-end.  Output goes from ngspice back to	*/
   /* xcircuit to provide data for displaying.				*/

   if (spiceproc < 0) {  /* ngspice is not running yet */
      spiceproc = vfork();
      if (spiceproc == 0) {		/* child process (ngspice) */
#ifdef SPICE_DEBUG
         fprintf(stdout, "Calling %s\n", SPICE_EXEC);
#endif
	 close(std_in[0]);
	 close(std_out[1]);

	 dup2(std_in[1], fileno(stdout));
	 dup2(std_in[1], fileno(stderr));
	 dup2(std_out[0], fileno(stdin));

	 Flush(stderr);

	 /* Force interactive mode with "-p".  Otherwise, ngspice will	*/
	 /* detect that the pipe is not a terminal and switch to batch-	*/
	 /* processing mode.						*/

         execlp(SPICE_EXEC, "ngspice", "-p", (char *)NULL);
	 spiceproc = -1;
	 Fprintf(stderr, "Exec of ngspice failed\n");
	 return -2;
      }
      else if (spiceproc < 0) {
	 Wprintf("Error: ngspice not running");
	 close(std_in[0]);
	 close(std_in[1]);
	 close(std_out[0]);
	 close(std_out[1]);
	 return -1;			/* error condition */
      }
      else {	/* parent process */
	 close(std_in[1]);
	 close(std_out[0]);
	 pipeRead = std_in[0];
	 pipeWrite = std_out[1];
	 return 0;			/* success */
      }
   }
   return 1;	/* spice is already running */
}

#endif /* ! MSC_VER */

/*------------------------------------------------------*/
/* Send text to the ngspice simulator			*/
/*------------------------------------------------------*/

void send_to_spice(char *text)
{
   int tlen = strlen(text);
   write(pipeWrite, text, tlen);
   if (*(text + tlen - 1) != '\n')
      write(pipeWrite, "\n", 1);

   if (!strncmp(text, "run", 3) || !strncmp(text, "resume", 6))
      spice_state = SPICE_BUSY;
   else if (!strncmp(text, "quit", 4) || !strncmp(text, "exit", 4))
      spice_state = SPICE_INIT;

#ifdef SPICE_DEBUG
   /* fprintf(stdout, "writing: %s\n", text); */
#endif
}

/*------------------------------------------------------*/
/* Get text from the ngspice simulator			*/
/*------------------------------------------------------*/

#define RECV_BUFSIZE 1024

char *recv_from_spice(Tcl_Interp *interp, int expect)
{
   int n, numc, totc, nfd;
   fd_set readfds, writefds, exceptfds;
   static char *buffer = NULL;
   struct timeval timeout;
   char *pptr, *bufptr;
   float refval;

   if (buffer == NULL)
      buffer = (char *)malloc(RECV_BUFSIZE);

   /* 2-second timeout on reads */
   timeout.tv_sec = (expect == EXPECT_ANY) ? 0 : 2;
   timeout.tv_usec = 0;

   FD_ZERO(&readfds);
   FD_ZERO(&exceptfds);

   bufptr = buffer;
   totc = 0;
   numc = RECV_BUFSIZE - 1;
   while (numc == RECV_BUFSIZE - 1) {

      nfd = pipeRead + 1;
      FD_ZERO(&writefds);
      FD_SET(pipeRead, &readfds);
      FD_SET(pipeRead, &exceptfds);

      *bufptr = '\0';
      n = select(nfd, &readfds, &writefds, &exceptfds, &timeout);
      if (n == 0) {
	 if (expect != EXPECT_ANY)
            Fprintf(stderr, "Timeout during select()\n");
         return buffer;	/* Timeout, or no data */
      }
      else if (n < 0) {
         /* Interrupt here? */
         Fprintf(stderr, "Exception received by select()\n");
         return buffer;
      }
      numc = read(pipeRead, bufptr, RECV_BUFSIZE - 1);
      *(bufptr + numc) = '\0';	/* Terminate the line just read */
      totc += numc;

      switch(expect) {
	 case EXPECT_PROMPT:
	    for (pptr = bufptr + numc - 1; pptr >= buffer; pptr--)
	       if (*pptr == '\n')
		  break;
	    if (!strncmp(pptr + 1, "ngspice", 7)) {
	       *(pptr) = '\0';
	       if (sscanf(pptr + 8, "%d", &numc) == 1) {
		  sprintf(_STR2, "%d", numc);
		  Tcl_SetResult(interp, _STR2, TCL_STATIC); 
	       }
	       return buffer;
	    }
	    /* Force the process to continue reading until the	*/
	    /* prompt is received.				*/
	    numc = RECV_BUFSIZE - 1;
	    break;

	 case EXPECT_REF:
	    for (pptr = bufptr + numc - 1; pptr > buffer; pptr--) {
	       if (*pptr == '\r') {
	          while ((--pptr >= buffer) && !isspace(*pptr));
	          if (sscanf(pptr + 1, "%g", &refval)) {
	             sprintf(_STR2, "%g", refval);
	             Tcl_SetResult(interp, _STR2, TCL_STATIC); 
	          }
		  return buffer;
	       }
	    }
	    /* Force the process to continue reading until a	*/
	    /* reference value is received.			*/
	    numc = RECV_BUFSIZE - 1;
	
	    /* Drop through to below to remove the control chars */

	 case EXPECT_ANY:
	    /* Remove control characters from the input */
	    pptr = bufptr;
	    while (*pptr != '\0') {
	       if (*pptr == '\r') *pptr = '\n';
	       else if (!isprint(*pptr)) *pptr = ' ';
	       pptr++;
 	    }
	    break;

      }
      if (numc == RECV_BUFSIZE - 1) {
	 buffer = (char *)realloc(buffer, totc + RECV_BUFSIZE);
	 bufptr = buffer + totc;
      }
   }

#ifdef SPICE_DEBUG
   /* fprintf(stdout, "read %d characters: %s", totc, buffer); */
#endif
   return buffer;
}

/*------------------------------------------------------*/
/* Send break (ctrl-c) to the ngspice simulator		*/
/* Return 0 on success, -1 if ngspice didn't return a	*/
/* prompt within the timeout period.			*/
/*------------------------------------------------------*/

int break_spice(Tcl_Interp *interp)
{
   char *msg;

#ifdef _MSC_VER
   if (spiceproc == INVALID_HANDLE_VALUE) return 0;	 /* No process to interrupt */
#else
   if (spiceproc == -1) return 0;	 /* No process to interrupt */
#endif

   /* Sending SIGINT in any state other than "busy" will kill	*/
   /* the ngspice process, not interrupt it!			*/

   if (spice_state == SPICE_BUSY) {
#ifndef _MSC_VER
      kill(spiceproc, SIGINT);
#else
      TerminateProcess(spiceproc, -1);
#endif
      msg = recv_from_spice(interp, EXPECT_PROMPT);	/* Flush the output */
      if (*msg == '\0')
	 return -1;
   }

   spice_state = SPICE_READY;
#ifdef SPICE_DEBUG
   /* fprintf(stdout, "interrupt ngspice\n"); */
#endif
   return 0;
}

/*------------------------------------------------------*/
/* Resume the ngspice simulator				*/
/*------------------------------------------------------*/

void resume_spice()
{
   spice_state = SPICE_BUSY;
   write(pipeWrite, "resume\n", 7);
#ifdef SPICE_DEBUG
   /* fprintf(stdout, "resume ngspice\n"); */
#endif
}

/*------------------------------------------------------*/
/* Exit ngspice. . . not so gently			*/
/*------------------------------------------------------*/

int exit_spice()
{
#ifdef _MSC_VER
   if (spiceproc == INVALID_HANDLE_VALUE) return -1;	/* ngspice not running */
#else
   if (spiceproc < 0) return -1;	/* ngspice not running */
#endif

#ifdef SPICE_DEBUG
   fprintf(stdout, "Waiting for ngspice to exit\n");
#endif
#ifndef _MSC_VER
   kill(spiceproc, SIGKILL);
   waitpid(spiceproc, NULL, 0);      
#else
   TerminateProcess(spiceproc, -1);
#endif
#ifdef SPICE_DEBUG
   fprintf(stdout, "ngspice has exited\n");
#endif

#ifdef _MSC_VER
   spiceproc = INVALID_HANDLE_VALUE;
#else
   spiceproc = -1;
#endif
   spice_state = SPICE_INIT;

   return 0;
}

/*------------------------------------------------------*/
/* Spice command					*/
/*							*/
/* Run the ngspice simulator from the tcl command line.	*/
/* Currently, the Tcl command-line interface is the	*/
/* *only* way to access the ngspice simulator through	*/
/* XCircuit.  ngspice runs as a separate process via a	*/
/* fork call, and sends and receives through the	*/
/* "spice" command in Tcl-XCircuit.			*/
/*------------------------------------------------------*/

int xctcl_spice(ClientData clientData, Tcl_Interp *interp, 
        int objc, Tcl_Obj *CONST objv[])
{
   int result, idx;
   char *msg, *msgin, *eptr;

   static char *subCmds[] = {
	"start", "send", "get", "break", "resume", "status",
	"flush", "exit", "run", "print", NULL
   };
   enum SubIdx {
	StartIdx, SendIdx, GetIdx, BreakIdx, ResumeIdx, StatusIdx,
	FlushIdx, ExitIdx, RunIdx, PrintIdx
   };

   if (objc == 1 || objc > 3) {
      Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
      return TCL_ERROR;
   }
   else if ((result = Tcl_GetIndexFromObj(interp, objv[1],
	(CONST84 char **)subCmds,
        "option", 0, &idx)) != TCL_OK) {
      return result;
   }

   switch(idx) {
      case StartIdx:
	 if (spice_state != SPICE_INIT) {
	    Tcl_SetResult(interp, "ngspice process already running", NULL);
	    return TCL_ERROR;
	 }
	 result = start_spice();
	 if (result != 0) {
	    Tcl_SetResult(interp, "unable to run ngspice", NULL);
	    return TCL_ERROR;
	 }
	 msg = recv_from_spice(interp, EXPECT_PROMPT);
	 if (*msg == '\0')
	    return TCL_ERROR;
	 Fprintf(stdout, "%s", msg);
	 Flush(stdout);

	 /* Force the output not to use "more" */
	 send_to_spice("set nomoremode true");
	 msg = recv_from_spice(interp, EXPECT_PROMPT);

	 /* This needs to be considerably more clever.  Should	*/
	 /* also generate the spice file if it does not exist.	*/
	 /* Finally, needs to add models and some statement	*/
	 /* like "trans" to the end to start the simulation.	*/

	 sprintf(_STR2, "source %s.spc", topobject->name);
	 send_to_spice(_STR2);
	 msg = recv_from_spice(interp, EXPECT_PROMPT);
	 if (*msg == '\0')
	    return TCL_ERROR;
	 else {
	    Fprintf(stdout, "%s", msg);
	    Flush(stdout);
	    spice_state = SPICE_READY;
	 }
	 break;

      case RunIdx:
	 if (spice_state != SPICE_READY) {
	    Tcl_SetResult(interp, "Spice process busy or nonexistant", NULL);
	    return TCL_ERROR;
	 }

	 send_to_spice("run");
	 msg = recv_from_spice(interp, EXPECT_REF);
	 if (*msg == '\0')
	    return TCL_ERROR;
	 else {
	    spice_state = SPICE_BUSY;
	    Fprintf(stdout, "%s", msg);
	    Flush(stdout);
	 }
	 break;

      case SendIdx:
	 /* Allow commands in INIT mode, because they may come from */
	 /* parameterized labels.  No output will be generated.	    */
	 if (spice_state == SPICE_INIT)
	    break;
	 else if (spice_state == SPICE_BUSY)	
	    if (break_spice(interp) < 0)
	       return TCL_ERROR;

	 if (objc == 2) break;

	 /* Prevent execution of "run" and "resume" using this	*/
	 /* method, so we keep control over the output and the	*/
	 /* state of the simulator.				*/

	 msgin = Tcl_GetString(objv[2]);
	 if (!strncmp(msgin, "run", 3) || !strncmp(msgin, "resume", 6)) {
	    Tcl_SetResult(interp, "Do not use \"send\" with "
			"\"run\" or \"resume\"\n", NULL);
	    return TCL_ERROR;
	 }
	 else
	    send_to_spice(msgin);

	 msg = recv_from_spice(interp, EXPECT_PROMPT);
	 if (*msg == '\0')
	    return TCL_ERROR;
	 else {
	    char *mp = msg;
            /* ngspice echos the terminal, so walk past input */
            while (*mp++ == *msgin++);
	    Tcl_SetResult(interp, mp, TCL_STATIC);
         }
	 break;

      case FlushIdx:
	 if (spice_state == SPICE_INIT)
	    break;
	 msg = recv_from_spice(interp, EXPECT_ANY);	/* Grab input and discard */
	 break;

      case GetIdx:
	 if (spice_state == SPICE_INIT)
	    break;
	 msg = recv_from_spice(interp, EXPECT_ANY);	/* Grab input and return */
	 if (msg)
	    Tcl_SetResult(interp, msg, TCL_STATIC);
	 break;

      case BreakIdx:
	 if (spice_state == SPICE_INIT)
	    break;
	 else if (spice_state == SPICE_BUSY)
	    if (break_spice(interp) < 0)
	       return TCL_ERROR;

	 /* Return the value of the stepsize from the function. */
	 send_to_spice("print length(TIME)");	
	 goto process_result;

      case PrintIdx:
	 /* Allow command in INIT mode, because it may come from */
	 /* parameterized labels.  No output will be generated.	 */
	 if (spice_state == SPICE_INIT)
	    break;

	 /* Do *not* allow this command to be sent while spice	*/
	 /* is running.						*/
	 else if (spice_state == SPICE_BUSY)	
	    if (break_spice(interp) < 0)
	       return TCL_ERROR;

	 if (objc == 2) break;

	 /* Similar to "spice send {print ...}", but processes	*/
	 /* the output like "spice break" does to return just	*/
	 /* the result.	 However, if no index is provided for 	*/
	 /* the variable, we determine the timestep and set the	*/
	 /* index accordingly.					*/

	 msg = Tcl_GetString(objv[2]);
	 if (strchr(msg, '[') != NULL)
	    sprintf(_STR2, "print %s", msg);
	 else {
	    float refval;
	    char *stepstr;

	    send_to_spice("print length(TIME)");	
	    stepstr = recv_from_spice(interp, EXPECT_PROMPT);
	    if (stepstr && ((eptr = strrchr(stepstr, '=')) != NULL)) {
	       eptr++;
	       while (isspace(*eptr)) eptr++;
	       if (sscanf(eptr, "%g", &refval) == 1)
		  sprintf(_STR2, "print %s[%d]", msg, (int)(refval - 1));
	       else
		  sprintf(_STR2, "print %s", msg);
	    }
	    else
	       sprintf(_STR2, "print %s", msg);
	 }
	 send_to_spice(_STR2);

process_result:
	 msg = recv_from_spice(interp, EXPECT_PROMPT);	/* Grab the output */
	 if (msg && ((eptr = strrchr(msg, '=')) != NULL)) {
	    eptr++;
	    while (isspace(*eptr)) eptr++;
	    Tcl_SetResult(interp, eptr, TCL_STATIC);
	 }
	 else if (*msg == '\0')
	    return TCL_ERROR;
	 break;

      case ResumeIdx:
	 if (spice_state != SPICE_READY) {
	    Tcl_SetResult(interp, "Spice process busy or nonexistant", NULL);
	    return TCL_ERROR;
	 }
	 resume_spice();
	 break;

      case StatusIdx:
	 switch(spice_state) {
	    case SPICE_INIT:
	       Tcl_SetResult(interp, "init", NULL);
	       break;
	    case SPICE_BUSY:
	       Tcl_SetResult(interp, "busy", NULL);
	       break;
	    case SPICE_READY:
	       Tcl_SetResult(interp, "ready", NULL);
	       break;
	 }
	 break;

      case ExitIdx:
	 exit_spice();
	 break;
   }
   return XcTagCallback(interp, objc, objv);
}

#endif /* TCL_WRAPPER */
/*----------------------------------------------------------------------*/

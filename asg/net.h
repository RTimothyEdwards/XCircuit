
/************************************************************
**
**       COPYRIGHT (C) 1988 UNIVERSITY OF PITTSBURGH
**       COPYRIGHT (C) 1988-1989 Alan R. Martello
**                  ALL RIGHTS RESERVED
**
**        This software is distributed on an as-is basis
**        with no warranty implied or intended.  No author
**        or distributor takes responsibility to anyone 
**        regarding its use of or suitability.
**
**        The software may be distributed and modified 
**        freely for academic and other non-commercial
**        use but may NOT be utilized or included in whole
**        or part within any commercial product.
**
**        This copyright notice must remain on all copies 
**        and modified versions of this software.
**
************************************************************/

/*
**   Global Include File
**
**                vsim VHDL Simulator
**    Copyright (C) 1988, University of Pittsburgh
**
*/

/*
**   File:        net.h
**
**   Functions:   N/A
**
**   Description:
**
**
**   Author:      Lyn Ann Mears    (LAM) - University of Pittsburgh, EE Dept.
**   Modified by: Alan R. Martello (ARM) - University of Pittsburgh, EE Dept.
**
**   Revision History
**   ----------------
**   Date      Initials   Description of change
**
**   thru 7/88   LAM      Initial modification of RSIM along with writing
**                        new routines, including debug of code.
**   08/04/88    ARM      Code cleanup; added comments.
**
**
**   Some code taken and adapted from RSIM work performed by Chris Terman.
**
*/

#include <stdio.h>

#define INCLUDE_TIMER 1  /*  switch to include timer calls / instruction  */


 /*
  * return values for all 'cmd_xxxx' calls
  */
#define  CMD_SAME_LEVEL  0	/* stay on the same command level   */
#define  CMD_UP_LEVEL    1	/* pop up one command level (file)  */

#define  FIRST_LEVEL_CALL 0	/* first call to print node's critical path  */

#ifndef TRUE
#define  TRUE  1
#endif
#ifndef FALSE
#define  FALSE 0
#endif

 /* alias definitions  */
#define  MAX_ALIAS 50		/* the maximum number of allowed alias */
#define  MAX_ALIAS_NAME 10	/* the maximum length of an alias name */

#define BIG_NUMBER 0x7FFFFFFF	/* used as a delta value to perform a sort  */

/*  delay equates  */
#define  TRANSPORT_DELAY  1
#define  INERTIAL_DELAY   0

#define  NO_DELTA         0L	/* the delta value for the current time  */

/*  definitions for determining what vector list we want to deal with  */
#define  VLIST_CLOCK  1
#define  VLIST_USER   2

 /* usage: xxxxx command strings appear below  */
#define USAGE_ACTIVITY "usage: activity starting_time [ending_time]"
#define USAGE_ALIAS "usage: alias [alias_name  string_to_alias]"
#define USAGE_CL "usage: cl [number_of_clock_cycles]"
#define USAGE_CHANGES "usage: changes starting_time [ending_time]"
#define USAGE_CLOCK "usage: clock [node_name vector_string] ..."
#define USAGE_CLRVEC "usage: clrvec clock  OR  clrvec user"
#define USAGE_DEBUG "usage: debug [debug_level]"
#define USAGE_LOGFILE   "usage: logfile [new_logfile_name]"
#define USAGE_NODE "usage: node [-l] node_name"
#define USAGE_PATH "usage: path node ..."
#define USAGE_RUNVEC "usage: runvec [number_of_cycles to run]"
#define USAGE_ST "usage: st [new_step_size]"
#define USAGE_STEPSIZE "usage: stepsize [new_step_size]"
#define USAGE_TRACE "usage: trace node ..."
#define USAGE_UVEC "usage: uvec [node_name vector_string] ..."
#define USAGE_WATCH "usage: watch [-]node_name ..."
#define USAGE_CMDFILE "usage: @ command_file_name"
#define USAGE_EFFECT_OTHERS "usage ? node ..."
#define USAGE_EFFECT_ME "usage: ! node ..."


#define MAXBITS 32		/* max number of bits in a bit vector   */

/* possible values for nflags  */
#define TRACED  0x0001
#define NAMED  0x0004
#define ALIAS  0x0008
#define DELETED  0x0010
#define STOPONCHANGE 0x0020
#define USERDELAY 0x0040
#define VISITED  0x0080
#define FORCE_VALUE 0x0100	/* set => node tied high/low   */
#define FORCE_ONCE      0x0200	/* set => process as event to proogate
				 * results once  */

/*  definitions for 'oflags' below  */
#define NORMAL_GATE  0x0001
#define PROCESS_GATE 0x0002

/*  misc definitions  */
#define NBUCKETS 20		/* number of buckets in histogram */
#define HASHSIZE 731		/* size of hash table (before chaining)  */
#define MAXGLIST 10		/* maximum number of nodes in a gate list */
#define NFREE  4096
#define LSIZE  2000		/* max size of command line (in chars)   */
#define MAXARGS 100		/* maximum number of command-line arguments */
#define MAXCOL  60		/* maximum width of print line   */

#define INITED DELETED		/* this flag used by init */
#define SETTLETIME 500		/* time in delta's to settle network   */
#define MULT_LINE_ALIAS ';'     /* the command for multiple line aliases  */

#define TSIZE 1024		/* size of event array, must be power of two  */
 /* this is because we use (TSIZE - 1) to do a FAST modulo      */
 /* function in advance_simulated_time in queue.c               */

#define NBUNCH 10		/* number of event structs to allocate at
				 * once  */


/*
*************************************************************
**    typedefs and structure definitions appear below      **
*************************************************************
*/

typedef struct Event *evptr;
typedef struct Node *nptr;
typedef struct Object *obptr;
typedef struct Input *iptr;

typedef struct nlist *nlptr;
typedef struct oblist *oblptr;

struct Event
{
    evptr           flink, blink;	/* doubly-linked event list */
    evptr           nlink;	/* link for list of events for this node */
    nptr            enode;	/* node this event is all about */
    nptr            cause;	/* node which caused this event to happen */
    long            ntime;	/* time, in DELTAs, of this event */
    char            eval;	/* new value */
};

struct Node
{
    char           *nname;	/* name of node, as found in .ivf file */
    nptr            nlink;	/* sundries list    */
    evptr           events;	/*  */
    oblptr          in;		/* list of objects with node as in */
    oblptr          out;	/* list of objects with node as out */
    nptr            hnext;	/* link in hash bucket */
    long            ctime;	/* time in DELTAs of last transition */
    nptr            cause;	/* node which caused last transition  */
    char            nvalue;	/* current value (lm npot in rsim)  */
    unsigned int    nflags;	/* flag word (see defs below)  */
};

struct Object
{
    char           *oname;	/* name of object */
    char           *otype;	/* type of object */
    nlptr           in;		/* list of nodes of mode in for object */
    nlptr           out;	/* list of nodes of mode out for object */
    char           *eval;	/* name of evaluation function for object */
    int             (*fun) ();	/* evaluation function for object */
    long            delay;	/* delay in deltas for object */
    obptr           hnext;	/* link in hash bucket */
    char            oflags;	/* flag word */
    int             dly_type;	/* type of delay (transport or inertial) */
};

/* linked list of inputs  */
struct Input
{
    iptr            next;	/* next element of list  */
    nptr            inode;	/* pointer to this input node  */
};

struct nlist
{
    nlptr           next;
    nptr            this;
};

struct oblist
{
    oblptr          next;
    obptr           this;
};

struct command
{
    char           *name;	/* name of this command  */
    char           *help_txt;	/* help text for this command                   */
    int             (*handler) ();	/* handler for this command  */
};

typedef struct Bits *bptr;

struct Bits
{
    bptr            bnext;	/* next bit vector in chain  */
    int             bnbits;	/* number of bits in this vector  */
    nptr            bbits[MAXBITS];	/* pointers to the bits (nodes)  */
    char           *bname;	/* name of this vector of bits  */
};


 /* define the vector structure  */
typedef struct VectorRecord Vector;

struct VectorRecord
{
    char           *vec_values;	/* the vector of values  */
    nptr            node;	/* pointer to the node this vector refers to  */
    int             num_elements;	/* the number of elements in the
					 * vector  */
    int             repeat_count;	/* TRUE => '*' at end, FALSE
					 * otherwise  */
    Vector         *next;	/* pointer to next vector in vector list,
				 * NULL in none  */
};

 /* definition of command and '$x' alias structure  */
struct alias_str
{
    char            alias_name[MAX_ALIAS_NAME];
    char            actual_cmd[LSIZE];
};

 /* definition of sequential function names and addresses  */
struct functions
{
    char           *name;
    int             (*fun) ();
};

/*
*******************************************************
**    externs for global variables appear below      **
*******************************************************
*/

extern struct functions funs[];
extern struct functions proc_funs[];

extern oblptr   seq_obs;
extern oblptr   end_seq;

 /* the clock vector list  */
extern Vector  *vec_clock;

 /* the user-defined vector list  */
extern Vector  *vec_user;

extern struct alias_str alias_list[];	/* the alias list    */
extern FILE    *outfile;	/* current output file     */
extern char    *targv[];	/* pointer to tokens on current command line   */
extern int      debug;		/* <>0 means print lotsa interesting info */
extern int      nfree;		/* # of bytes of free storage remaining */
extern int      nindex;		/* number of nodes    */
extern int      pending;	/* <>0 means events are still pending  */
extern long     stepsize;	/* the current stepsize    */
extern int      targc;		/* number of args on command line  */
extern iptr     hinputs;	/* list of nodes to be driven high  */
extern iptr     infree;		/* list of free input nodes  */
extern iptr     linputs;	/* list of nodes to be driven low  */
extern iptr     wlist;		/* list of watched nodes  */

extern long     cur_delta;	/* the current time, in 0.1 ns increments */

extern long     nevent;		/* number of current event    */
extern obptr    curobj;		/* pointer to current object   */

extern int      column;
extern int      lineno;
extern char    *filename;

extern struct Bits *blist;
extern FILE    *logfile;

extern struct command cmdtbl[];
extern nptr     hash[];


/*
*******************************************************
**    external funtion definitions appear below      **
*******************************************************
*/

#ifndef CXREF
#define MAX(x,y) ( ((x)<(y)) ? (y) : (x) )

char           *pnode ();
char           *pnode_lc ();
double          atof ();

int             cmd_alias ();
int             cmd_cmdfile ();
int             cmd_comment ();
int             cmd_cont ();
int             cmd_display ();
int             cmd_doactivity ();
int             cmd_dochanges ();
int             cmd_doclock ();
int             cmd_doexit ();
int             cmd_domsg ();
int             cmd_dopath ();
int             cmd_dostep ();
int             cmd_excl ();
int             cmd_help ();
int             cmd_pnlist ();
int             cmd_print_node ();
int             cmd_quest ();
int             cmd_quit ();
int             cmd_sb_vec ();
int             cmd_set_bits ();
int             cmd_set_hi ();
int             cmd_set_lo ();
int             cmd_set_xinput ();
int             cmd_setdbg ();
int             cmd_setlog ();
int             cmd_setstep ();
int             cmd_settrace ();
#if INCLUDE_TIMER 
int             cmd_timer ();
#endif
int             cmd_vec_clear ();
int             cmd_vec_clock ();
int             cmd_vec_run ();
int             cmd_vec_user ();
long            atol ();

nptr            find ();

void            info ();
void            setin ();
void            clear_vectors ();
void            idelete ();
void            iinsert ();
void            init_hash ();
void            init_nodes ();
void            n_insert ();
void            new_out ();
void            presim ();
void            prnode ();
void            step ();

#endif

#define DEBUG_EVENTS  0x01
#define DEBUG_SYSTEM  (~(DEBUG_EVENTS))

#if INCLUDE_TIMER

#define TIMER_ON_STR    "on"
#define TIMER_OFF_STR   "off"
#define TIMER_SHOW_STR  "show"

#define TIMER_ON  1
#define TIMER_OFF 2

extern int timer_state;
extern long int elapsed_time;

#endif

extern int interrupt_flag;

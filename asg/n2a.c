/************************************************************
**
**       COPYRIGHT (C) 1993 UNIVERSITY OF PITTSBURGH
**       COPYRIGHT (C) 1996 GANNON UNIVERSITY
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


/* file n2a.c */
/* ------------------------------------------------------------------------
 * main file manipulation routines                         spl 7/89
 *
 * ------------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>	
#include <sys/resource.h>

/*---------------------------------------------------------------
 * External functions
 *---------------------------------------------------------------
 */

#include "externs.h"

/*---------------------------------------------------------------
 * Global Variable Definitions
 *---------------------------------------------------------------
 */

mlist *modules;		/* M the set of all modules */
nlist *nets;		/* N the set of all nets */
tlist *ext_terms;	/* ST the set of all system terminals */
mlist *ext_mods;	/* the modules associated with each element of ST */

/* NOT USED*/
tlist *int_terms;	/* T the set of all inter-module terminals */

/* FOR THE ROUTER (only) */
int currentPartition;
tile **routingRoot[2];	  

/* NOT USED ENOUGH */
int lcount = 0;			   /* number of input lines read */
int module_count = 0;		   /* number of modules/objects read in */
int node_count = 0;		   /* number of nodes/signals read in */
int terminal_count = 0;		   /* number of terminals read in */
int connected[MAX_MOD][MAX_MOD];   /* BIG array of connection counts */
info_type module_info[MAX_MOD];/* re-used array of inter module connections */
float partition_ratio=DEF_PARTITION_RATIO; 	/* -r qualifier, used for clustering */
/* command-line parameters: */
int compute_aspect_ratios = FALSE;		/* -a flag, in parsing - compute aspect ratios*/
int Xcanvas_scaling = FALSE;	   		/* -b flag (use XCanvas scaling) */
int max_partition_conn =  MAX_PARTITION_CONN; 	/* -c qualifier, used for clustering */
int debug_level = 11;			      	/* -d qualifier, sets the debug level */
int stopAtFirstCut = FALSE;			/* -f flag; terminates GR early */
int congestion_rule = 2; 			/* -g qualifier, used for conGestion */
int do_hand_placement = FALSE;			/* -h flag */
int includeSysterms = FALSE;			/* -i flag; excludes system terminals */

int cc_search_level = CC_SEARCH_LEVEL;		/* -l qualifier, limits cc search */
int matrix_type = UNSIGNED;			/* -m qualifier, sets signal-flow style*/
int do_routing = TRUE;				/* -n qualifier, inhibits routing */
FILE *outputFile;				/* -o qualifier, opens an Output file */
int partition_rule =  DEF_PARTITION_RULE; 	/* -p qualifier, used for clustering */
int recordStats = FALSE;			/* -q flag; quality stats collection on*/
int max_partition_size =  MAX_PARTITION_SIZE; 	/* -s qualifier, used for clustering */
int turn_mode = TIGHT;			        /* Used to choose the cornering alg */
int useSlivering = TRUE;			/* -v flag; removes sliVer corrections */
int useAveraging = FALSE;			/* -w flag; turns Weighted averaging on*/
int latex = FALSE;				/* -x flag; modified PostScript header */
int recordTiming = FALSE;			/* -y flag; turns time collection on */
int xcircuit= TRUE;				/* -z flag;to launch spar into xcircuit*/
FILE *stats;					/* quality statistics file */

/* NOT USED MUCH */
mlist **boxes;		/* the sets of boxes */
mlist **strings;	/* the set of strings */
ctree **parti_stats;	/* partition statistics */
int *str_length;	/* the length of each string */
int *str_height;	/* the height of each string */
int *x_sizes;		/* the max x size of each partition */
int *y_sizes;		/* the max y size of each partition */
int *x_positions;	/* and their postions, after placement */
int *y_positions;
int xfloor, yfloor;	/* the origin for the current partition */

mlist **partitions;	/* the set of PARTITIONS (lists) of modules */
tile **rootTiles;	/* The tile spaces that correspond to the partitions */
int partition_count;	/* the total number of partitions created */

#define USEC 1000000

/*--------------------------------------------------------------*/
/* record the time, and mark it in the <timingFile>: 		*/
/*--------------------------------------------------------------*/

mark_time(tf, name, dTime)
    FILE *tf;
    char *name;
    long *dTime;
{
    double t;
    int cTime, maxMemory = 0;
    struct rusage ru;

    getrusage(RUSAGE_SELF, &ru);
    cTime = USEC * ru.ru_utime.tv_sec + ru.ru_utime.tv_usec; 
    maxMemory = MAX(ru.ru_maxrss, maxMemory);
    t = (double)(cTime - *dTime)/(double)USEC;
    *dTime = cTime;

    fprintf(tf, "%s time = %f, time elapsed = %f\n", 
	    name, t, (double)cTime/(double)USEC);
}

/*------------------------------------------------------------------*/
/* Given a list of modules, count the number of terminals contained */
/*------------------------------------------------------------------*/

int count_terminals(mods)
    mlist *mods;
{
    int c = 0;
    mlist *ml;
    for (ml = mods; ml != NULL; ml = ml->next)
	c += list_length(ml->this->terms);
    return(c);
}

/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

int allocate_globals(p)
    int p;			/* Number of partitions */
{
    x_sizes = (int *)calloc((p+1), sizeof(int));
    y_sizes = (int *)calloc((p+1), sizeof(int));
    x_positions = (int *)calloc((p+1), sizeof(int));
    y_positions = (int *)calloc((p+1), sizeof(int));
    partitions = (mlist **)calloc((p+3), sizeof(mlist *));
    rootTiles = (tile **)calloc((p+1), sizeof(tile *));
}

/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

int separate_systerms(intModules, sysModules, sysNets)	
    mlist **intModules, **sysModules;
    nlist **sysNets;
{
    mlist *ml, *sysMods = NULL;

    /* First, Do some of the background work done in "place.c" */
    allocate_globals(currentPartition);
    partitions[0] = partitions[1] = partitions[2] = NULL;

    for (ml = modules; ml != NULL; ml = ml->next) {
	if ((!strcmp(ml->this->type, INOUT_TERM)) ||
		(!strcmp(ml->this->type, INPUT_TERM))) {
	    push(ml->this, sysModules);
	    push(ml->this, &partitions[2]);
	}
	else if (!strcmp(ml->this->type, OUTPUT_TERM)) {
	    push(ml->this, sysModules);
	    push(ml->this, &partitions[1]);
	}
	else push(ml->this, intModules);
    }
    partitions[0] = *intModules;


    for (ml = *sysModules; ml != NULL; ml = ml->next) {
	/* Collect the nets that connect to the system terminals: */
	/* Assume there is only one terminal for each system terminal: */
	push(ml->this->terms->this->nt, sysNets);
    }
}	

/*--------------------------------------------------------------*/
/* Routine to set the debug level from an external routine.	*/
/*--------------------------------------------------------------*/

int SetDebugLevel(int *level)
{
    if (level != NULL)
       debug_level = *level;
    return debug_level;
}

/*--------------------------------------------------------------*/
/* Route --							*/
/*								*/
/* This method is used to route all the modules.... 		*/
/*--------------------------------------------------------------*/

void Route(XCWindowData *areastruct, Boolean bIsSparmode) 
{
    FILE *timingFile;
    int c,  errflg = 0, x1, y1, x2, y2;
    nlist *nl = NULL, *systermNets = NULL;
    mlist *modulesJustPlaced, *ml;
    mlist *systerms = NULL, *internalModules = NULL;

    /* for date and time */

    long starTime, diffTime;
    tile *r;
    extern int optind;
    extern char *optarg;
    int collapseNullModules = FALSE;
    int useBlockPartitioning = FALSE;

    outputFile = stdout;

    if (recordTiming == TRUE)
       mark_time(timingFile, "parse", &diffTime);

    /* place */

    if (do_hand_placement == FALSE) {
	fprintf(stderr,"Placing internal modules...");
	if (useBlockPartitioning == FALSE)
	   partition_count = partition();
	else partition_count = blockPartition();

	if (collapseNullModules == TRUE) clip_null_gates();
	
	erase_systerms();
	place_first_strings();
	box_placement();
	placement_prep();
	modulesJustPlaced = partition_placement();
	/* Setup the bounding box */
	systerms = make_room_for_systerm_placement(&systermNets, &x1, &y1, &x2, &y2);
	fprintf(stderr,"\nAutomatic placement ");
    }
    fprintf(stderr, "completed.\n");

    if (includeSysterms == TRUE) {

	/* routing */
	if (do_routing == TRUE) {

	    currentPartition = 0;
	    for(ml = systerms; ml != NULL; ml = ml->next) pull_terms_from_nets(ml->this);
	    routingRoot[VERT] = (tile **)calloc(partition_count + 2, sizeof(tile *));
	    routingRoot[HORZ] = (tile **)calloc(partition_count + 2, sizeof(tile *));
	    
	    fprintf(stderr,"Routing for Systerm Placement...");

	    create_routing_space(modulesJustPlaced, currentPartition, 
				 x_sizes[currentPartition], y_sizes[currentPartition], 
				 xfloor, yfloor);

	    nl = first_global_route(modulesJustPlaced, currentPartition, 
				    TRUE, congestion_rule);
	    if (recordTiming == TRUE)
	        mark_time(timingFile, "central global route", &diffTime);    

	    /* Create a local route to use to locate the system terminals */
	    local_route(systermNets, currentPartition, FALSE);
	    if (recordTiming == TRUE)
	        mark_time(timingFile, "central local route", &diffTime);    
	    fprintf(stderr,"placing systerms...");

	    /* Add the system terminals (systerms) to the diagram: */
	    modulesJustPlaced = fine_systerm_placement(&x1, &y1, &x2, &y2); 
	    if (recordTiming == TRUE)
	       mark_time(timingFile, "systerm placement", &diffTime);    

	    fprintf(stderr,"completed. \n");
	}

	if (do_routing != TRUE) {
	    gross_systerm_placement(&x1, &y1, &x2, &y2);
	    if (recordTiming == TRUE)
	       mark_time(timingFile, "gross systerm placement", &diffTime);    
	}

	/* Add the system terminals back to their respective nets: */

	for (ml = systerms; ml != NULL; ml = ml->next)
	   add_terms_to_nets(ml->this); 

	fprintf(stderr,"System Terminals being added to diagram...\n");

	/* finish the routing */

	if (do_routing == TRUE) {
	    fprintf(stderr,"Adding systerms to routing space, "
			"Global Routing commenced...");

	    /* Deal with resetting the boundaries to include the system terminals: */

	    reset_boundaries(routingRoot[HORZ][currentPartition], x1, y1, x2, y2);
	    reset_boundaries(routingRoot[VERT][currentPartition], y1, x1, y2, x2);
	    xfloor = x1;
	    x_sizes[currentPartition] = x2 - x1;
	    yfloor = y1;
	    y_sizes[currentPartition] = y2 - y1;	    
	    
	    nl = incremental_global_route(modulesJustPlaced, modules, nl,
				currentPartition);

	    if (recordTiming == TRUE)
	       mark_time(timingFile, "complete global route", &diffTime);    

	    fprintf(stderr,"completed...Local Routing commenced...");
	    local_route(nets, currentPartition, TRUE);
	    if (recordTiming == TRUE)
	       mark_time(timingFile, "complete local route", &diffTime);    
	    fprintf(stderr,"completed.\n");
	}
    }
    else if (do_routing == TRUE) {

	/* TEST! Tim 4/30/2012 --- put a large buffer area around the	*/
	/* area.  Area calculation seems to assume that lower bound is	*/
	/* zero and _sizes seem to be calculated from zero to max so	*/
	/* that negative numbers are not represented.  Below is just a	*/
	/* hack---we need to calculate the bounding box of the space	*/
	/* needed for the routing, correctly.				*/

	xfloor = yfloor = -72;

	/* No terminals are to be placed, hence no incremental routing. */

	routingRoot[VERT] = (tile **)calloc(partition_count + 3, sizeof(tile *));
	routingRoot[HORZ] = (tile **)calloc(partition_count + 3, sizeof(tile *));
	
	fprintf(stderr,"Adding modules to routing space, Global Routing commenced...");

	create_routing_space(modulesJustPlaced, currentPartition,
			x_sizes[currentPartition] - xfloor,
			y_sizes[currentPartition] - yfloor, 
			xfloor, yfloor);

	nl = first_global_route(modulesJustPlaced, currentPartition, 
				FALSE, congestion_rule);  
	if (recordTiming == TRUE)
	   mark_time(timingFile, "complete global route", &diffTime);    

	fprintf(stderr,"completed...Local Routing commenced...");
	local_route(nets, currentPartition, TRUE);
	fprintf(stderr,"completed.\n");      
	if (recordTiming == TRUE)
	   mark_time(timingFile, "complete local route", &diffTime);    
    }

    if (recordStats == TRUE) print_distance_matricies();

    /* Finish up */

    if (debug_level > 20) {
       print_modules(stderr);
       print_nets(stderr);
       print_info(stderr);
       print_strings(stderr);
    }

    if (do_routing != TRUE) {
	draft_statistics();
	print_module_placement(currentPartition, outputFile);
    }
    else {
	if (turn_mode != TIGHT)
	   print_global_route(outputFile);
	else
    	   // Dump the spar structures into Xcircuit
	   xc_print_local_route(areastruct, nets);

        if (recordStats == TRUE) {
	    draft_statistics();
	    collect_congestion_stats(currentPartition);
        }
    
        fprintf(stderr, "\n %d Lines read, %d Modules, %d Nodes, %d Partitions\n",
		lcount, module_count, node_count, partition_count);
        fprintf(stderr, "\nMax partition size: %d Max Connections: "
		"%d Ratio: %-5.3f Rule #%d\n",
		max_partition_size, max_partition_conn,
		partition_ratio, partition_rule);

        if (recordTiming == TRUE) {
	    mark_time(timingFile, "Generation time", &diffTime);    
	    fprintf(timingFile,"%d Modules, %d Nets, %d Terminals, %d partitions\n\n",
		list_length(modules), list_length(nets), 
		count_terminals(modules), partition_count);
	    fclose(timingFile);
        }
    }
}

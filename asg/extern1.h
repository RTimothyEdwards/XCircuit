#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>	
#include <sys/resource.h>


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
/* command-line parameters: (see n2a.c---some settings here are overridden there) */
int compute_aspect_ratios = FALSE;		/* -a flag, in parsing - compute aspect ratios*/
int Xcanvas_scaling = FALSE;	   		/* -b flag (use XCanvas scaling) */
int max_partition_conn =  MAX_PARTITION_CONN; 	/* -c qualifier, used for clustering */
int debug_level = 0;			      	/* -d qualifier, sets the debug level */
int stopAtFirstCut = FALSE;			/* -f flag; terminates GR early */
int congestion_rule = 2; 			/* -g qualifier, used for conGestion */
int do_hand_placement = FALSE;			/* -h flag */
int includeSysterms = TRUE;			/* -i flag; excludes system terminals */

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
char *ivffilename; 			      	/* input file name */
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







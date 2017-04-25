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
  
/* file place.c */
/* ------------------------------------------------------------------------
 * Placement routines                         spl 7/89
 *
 * "(Level #)" refers to calling depth from main()
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
#include "externs.h"
#include "psfigs.h"

/*---------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------
 */

mlist *trace_path();
int init_string_arrays();
module *select_next_box();
int accept_node1(), accept_node2(), accept_node3(), accept_node4();

/*---------------------------------------------------------------
 * Global (to this file)  Declarations
 *---------------------------------------------------------------
 */

static float xgr = 0; /* center of "gravity" of current box */
static float ygr = 0;
static float xgbr = 0; /* center of "gravity" of currently placed boxes */
static float ygbr = 0;

static int xSideCount;   /* number of conections for the dominant count in 
                            "best_side..." */
static int ySideCount;    /* number of conections for the next highest count in 
                            "best_side..." */

static ctree *c_tree;
/* the stem upon which the cluster tree is built and maintained. */

static int clus1 = 0, clus2 = 0;
/* identifies the clusters that should be combined in the next clustering pass*/

static clist *c_list, *partition_tree;
/* lists of module numbers, and other stats needed for the clustering calculations */

static int d1 = 0, d2 = 0;
/* used to evaluate how cross-coupled modules/partitions are placed; <d1> rep-
 * resents the depth from <m1> to <m2>, and <d2> the reverse.  (see cross_coupled_p())
 */

int rpl_merge_row[MAX_MOD];	/* Used to restore the connection matrix if a */
int rpl_scratch_row[MAX_MOD];	/* partition is clipped from the tree */
int rpl_merge_col[MAX_MOD];
int rpl_scratch_col[MAX_MOD];

/* ------------------------------------------------------------------------
 * Clustering routines                         stf 8/89
 * ------------------------------------------------------------------------
 */

/*---------------------------------------------------------------
 * Partition (Level 1)
 * Cut up the set of modules into a set of sets.
 * This is done by building the cluster tree until some limit is reached. 
 * Then the cluster tree is picked apart to assign the modules to their 
 * apropriate partitions.  (These assignments are made to the .used 
 * portion of module_info[].)
 *---------------------------------------------------------------
 */

int partition()
{	int p, done = 0;
	
	int p_count = 0, cont = FALSE;
	FILE *temp = (debug_level < -3)?fopen("tree_stats", "a"):NULL;
	
	init_info();

	build_clusters();
    
/* debugging - */
	if (debug_level < -3)
	{ 
	    fprintf(temp,"\nmax breadth = %d, height = %d\n",
		    print_ctree(temp,c_tree), tree_depth(c_tree,0));
	    fclose(temp);
	}	

	partition_count = ilist_length(c_list);

	init_string_arrays(partition_count);

	if (debug_level < 0) 			/* debug */ 
	    fprintf(stderr,"tree broken into %d partitions.\n", partition_count); 

	for (p = partition_count; p > 0; p--)
	{
	    assign_partitions(p);
	}

	build_connections(matrix_type);	/* rebuild the connection table for modules */
	
	return(partition_count);
}

/*---------------------------------------------------------------
 * blockPartition() (Level 1)
 * Cut up the set of modules into a set of sets.
 * This is done by taking advantage of blockingclues in the namespace of the
 * .ivf file.  Modules are assigned to their apropriate partitions, and these
 * assignments are made to the .used  * portion of module_info[].
 *---------------------------------------------------------------
 */

int blockPartition()
{	
    int p, done = 0;
    int p_count = 0, cont = FALSE;

    init_info();

    partition_count = interpret_block_assignments();

    init_string_arrays(partition_count);

    install_modules();

    if (debug_level < 0) 			/* debug */ 
        fprintf(stderr,"%d block partitions discovered.\n", partition_count); 

    build_connections(matrix_type);	/* rebuild the connection table for modules */
	
    return(partition_count);
}


/*----------------------------------------------------------------------------------*/
int nstrcomp(c1,c2)	
    /* Return TRUE if the two strings are equal, ignoring case */
    char *c1,*c2;
{
    if (strcasecmp(c1,c2) == 0) return(TRUE);
    else return(FALSE);
}
/*---------------------------------------------------------------------------------*/
int interpret_block_assignments()
{
    mlist *ml;
    int i, p, partCount;       /* <partCount> corresponds to the length of <table> */
    list *l, *table = NULL;
    char *blkName;
    
    /* Loop through all of the names for the modules, and collect block names */
    /* for each unique block name, create a separate partition */
    partCount = 0;

    for (ml = modules; ml != NULL; ml = ml->next)
    {
	i = strcspn(ml->this->name,"/");
	blkName = strdup(ml->this->name);
	blkName = strncpy(blkName, ml->this->name, i);
	blkName[i] = '\0';

	/* Handle the special case where no block name is given: */
	if (nstrcomp(blkName,ml->this->name) == TRUE) blkName = "BLANK";

	l = member((int *)blkName, table, nstrcomp);
	if (l == NULL) /* New block name encountered */
	{
	    /* Insert the new name into the table */
	    queue((int *)blkName, &table);
	    p = partCount += 1;
	}
	else /* Old block name encountered, calculate the corresponding partition */
	{
	    p = partCount - list_length(l) + 1;
	}
	/* set the module information for use by the boxing and string routines */
	module_info[ml->this->index].used = p;
    }

    for (l = table; l != NULL; l = l->next)	/* Cleanup strings */
    {
	my_free(l->this);
    }
    free_list(table);	/* Cleanup */
    return(partCount);
}


/*---------------------------------------------------------------------------------*/
int install_modules()
{
    mlist *ml;
    int p;

    for (ml = modules; ml != NULL; ml = ml->next)
    {
    /* Loop through all of the names for the modules, and collect block names */

	p = module_info[ml->this->index].used; 		/* (Set previously) */

	/* add the module to the appropriate partition:  */
	partitions[p] = (mlist *)concat_list(ml->this, partitions[p]);
    }
}
/*---------------------------------------------------------------
 * breadth_search (Level 2)
 * This measures the maximum breadth of the given ctree, executing (*fn)()
 * On each node element.
 *--------------------------------------------------------------
 */
int breadth_search(t, fn)
    ctree *t;
    int (*fn)();	/* the function to perform at every node */
    
{      
    clist *queue = NULL;
    ctree *temp;
    int this, next, breadth = 0;
    
    if (t == NULL) return 0;

/* Load the queue and begin: */
    indexed_list_insert(t,0,&queue);
    this = 1; 	next = 0;	breadth = 1;

/* Yank the first person off of the list, and add any children to the back */
    do
    {
	temp = (ctree *)remove_indexed_element(0, &queue);   /* dequeue the next item */
	if (temp != NULL)
	{
	    if (temp->left!=NULL)		/* enque the left child */
	    {
		indexed_list_insert(temp->left,ilist_length(queue),&queue);
		next += 1;
	    }
	    
	    if (temp->right!=NULL)  		/* enque the right child */
	    {
		indexed_list_insert(temp->right,ilist_length(queue),&queue);
		next += 1;
	    }
	    /* Now do something with the element: */
	    (*fn)(temp->this);
	    if (--this == 0) 			/* end of a layer */
	    {
		breadth = MAX(breadth, next);
		this = next;	next = 0;
	    }
      	}
/* go on to the next thing in the queue */
    } while (ilist_length(queue)!= 0);
    return (breadth);
}
/*---------------------------------------------------------------
 * tree_breadth (Level 2)
 * This measures the maximum breadth of the given ctree
 *--------------------------------------------------------------
 */
int tree_breadth(t)
    ctree *t;
{      
    clist *queue = NULL;
    ctree *temp;
    int this, next, breadth = 0;
    
    if (t == NULL) return 0;

/* Load the queue and begin: */
    indexed_list_insert(t,0,&queue);
    this = 1; 	next = 0;	breadth = 1;

/* Yank the first person off of the list, and add any children to the back */
    do
    {
	temp = (ctree *)remove_indexed_element(0, &queue);   /* dequeue the next item */
	if (temp != NULL)
	{
	    if (temp->left!=NULL)		/* enque the left child */
	    {
		indexed_list_insert(temp->left,ilist_length(queue),&queue);
		next += 1;
	    }
	    
	    if (temp->right!=NULL)  		/* enque the right child */
	    {
		indexed_list_insert(temp->right,ilist_length(queue),&queue);
		next += 1;
	    }
	    if (--this == 0) 			/* end of a layer */
	    {
		breadth = MAX(breadth, next);
		this = next;	next = 0;
	    }
      	}
/* go on to the next thing in the queue */
    } while (ilist_length(queue)!= 0);
    return (breadth);
}

/*---------------------------------------------------------------
 * tree_depth (Level 2)
 * This measures the maximum depth of the given ctree
 *--------------------------------------------------------------
 */
int tree_depth(t, d)
    ctree *t;	int d;
{    
    int r=d, l=d;
    
    if (t == NULL) return d;
    else
    {
	if (t->right != NULL)
	{
	    r = tree_depth(t->right, ++r);
	}
	if (t->left != NULL)
	{
	    l = tree_depth(t->left, ++l);
	}	
	return MAX(r, l);
    }
}

/*---------------------------------------------------------------
 * accept_node1 (Level 3)
 * This examines the given tree node, and returns a binary acceptance
 * as to whether or not it will be an acceptable partition.
 * IT IS IMPORTANT TO NOTE how this function is used: As tree limbs are 
 * built, they are checked for acceptance.  Limbs/leaves are added to the
 * branch one at a time, and if the new addition makes a previously 
 * acceptable branch unacceptable, the old branch is saved as a partition,
 * and the new addition takes it's place in the growing tree.

 * VARIANT 1:  A partition is acceptable if it has LE the preset number
 * of connections (-c flag) AND it has LE the preset number of modules
 * enclosed (-s flag).
 *---------------------------------------------------------------
 */
int accept_node1(limb)
    ctree *limb;
{	/* recall that this will be called on partial trees */
    int l = (limb->left != NULL) ? limb->left->this->size : 0;
    int r = (limb->right!= NULL) ? limb->right->this->size : 0;
    int c;

    int s = MIN(limb->this->size, l + r);
    l = (limb->left != NULL) ? limb->left->this->connects : 0;
    r = (limb->right != NULL) ? limb->right->this->connects : 0;
    c = MIN(limb->this->connects, l + r);

    if ((s <= max_partition_size) && (c <= max_partition_conn)) return(TRUE);
    else if (s == 1) return(TRUE);
    else return(FALSE);
}

/*---------------------------------------------------------------
 * VARIANT 2:  A partition is acceptable if the ratio of the number
 * of connections to the number of modules is greater than some value
 * partition_ratio (set by the -r flag).
 *----------------------------------------------------------------
 */
int accept_node2(limb)
    ctree *limb;
{	/* recall that this will be called on partial trees */
    int c = limb->this->connects;
    int s = limb->this->size;
    float ratio;

    ratio = (s > 0) ? (float)c/(float)s : 0.0;
    
    if ((s == 1) || (ratio >= partition_ratio))	return(TRUE);
    else return(FALSE);
}
/*---------------------------------------------------------------
 * VARIANT 3:  A partition is acceptable if the parent represents 
 * an increace in the total number of connections represented by the 
 * branch.  (So a good partition will represent a local maxima)
 *---------------------------------------------------------------
 */
int accept_node3(limb)
    ctree *limb;
{	/* recall that this will be called on partial trees */
    int s = limb->this->size;
    int c = limb->this->connects;
    int l = (limb->left != NULL) ? limb->left->this->connects : 0;
    int r = (limb->right!= NULL) ? limb->right->this->connects : 0;

    if ((s == 1) || ((c >= MAX(l,r)) && (c <= max_partition_conn)))
	return(TRUE);
    else return(FALSE);
}
/*---------------------------------------------------------------
 * VARIANT 4:  A partition is acceptable if the parent represents 
 * an increace in the ratio of the c/s among the branches...
 * (So a good partition will represent a local c/s minima)
 *---------------------------------------------------------------
 */
int accept_node4(limb)
    ctree *limb;
{	/* recall that this will be called on partial trees */
    int s = limb->this->size;
    int c = limb->this->connects;
    int cl = (limb->left != NULL) ? limb->left->this->connects : 0;
    int cr = (limb->right!= NULL) ? limb->right->this->connects : 0;
    int sl = (limb->left != NULL) ? limb->left->this->size : 0;
    int sr = (limb->right!= NULL) ? limb->right->this->size : 0;
    float OVratio, Lratio, Rratio, maxChildRatio;

    OVratio = (s != 0) ? (float)c/(float)s : 0.0;
    Lratio = (sl != 0) ? (float)cl/(float)sl : 0.0;
    Rratio = (sr != 0) ? (float)cr/(float)sr : 0.0;
    
    maxChildRatio = (Lratio > Rratio) ? Lratio : Rratio;

    if ((s == 1) || (OVratio >= maxChildRatio))	return(TRUE);
    else return(FALSE);
}

/*---------------------------------------------------------------
 * assign_partitions (Level 2)
 * This takes the first tree branch on the list, and assigns all of the 
 * modules therin the given partition number.
 *---------------------------------------------------------------
 */
int assign_partitions(pnum)
    int pnum;
{
    ctree *scrap = (ctree *)ipop(&c_list);
    make_part_assignments(pnum, scrap);

    /* save the cluster pointer for posterity */
    parti_stats[pnum] = scrap;
    
}

/*---------------------------------------------------------------
 * make_part_assignments (Level 3)
 * recursive tree traverse, at each leaf node, it assigns the module
 * the given partition number.
 *---------------------------------------------------------------
 */
int make_part_assignments(p, cnode)
    int p;
    ctree *cnode;
{
    if ((int *)cnode == (int *)NULL) return;               /* return (NULL); */
    
    if ((cnode->left == NULL) && (cnode->right == NULL)) /*leaf node*/
    {
	/* set the module information for use by the boxing and string routines */
	module_info[cnode->this->contents->index].used = p;

	/* add the module to the appropriate partition:  */
	partitions[p] = (mlist *)concat_list(cnode->this->contents, partitions[p]);
    }
    else
    {
	make_part_assignments(p, cnode->left);
	make_part_assignments(p, cnode->right);
    }
}
/*---------------------------------------------------------------
 * build_clusters (Level 2)
 * modifies the connected mtrx, the c_list,
 * and reassigns the cluster number(s) of any modules that are moved.
 * this function represents one step of the clustering method, as applied 
 * to schematic modules.  
 *---------------------------------------------------------------
 */

build_clusters()
{
    int c_count, continueFlag = FALSE, (*acceptance_fn)();
    clist *initClusters = NULL;	      
    FILE *tfile = (debug_level < -2)?fopen("con", "a"):NULL;
    
/* Determine which of the acceptance functions should be used to pull apart the cluster 
   tree: */

    if (debug_level < -2)		/* debug info */
    {
	fprintf(tfile, "\n[Rule# %d]  max partition size: %d, max connections: %d, ratio: %-5.3f\n",
		partition_rule, max_partition_size, max_partition_conn, partition_ratio);
	
    }
    
    switch(partition_rule)
    {
	case 1 :	acceptance_fn = (int (*)())accept_node1;	break;
	
	case 2 :	acceptance_fn = (int (*)())accept_node2;	break;
	
	case 3 :        acceptance_fn = (int (*)())accept_node3;	break;
	 
	case 4 :        acceptance_fn = (int (*)())accept_node4;	break;
	 
	default:	error("build_clusters - bade rule choice ");
	                break;
    }	
	
/* Setup the connection matrix */
    build_connections(matrix_type);
    overlay_connections(module_count, matrix_type);
    init_clusters(module_count, &initClusters);
    
    c_count = ilist_length(initClusters);
    do
    {

	if (debug_level < -2)		/* debug info */
	{ 
	    fprintf(tfile,"Clusters to be merged correspond to rows %d & %d  ", clus1, clus2);
	    p_cons(tfile, c_count, initClusters);
	    fprintf(tfile,"\n");
	}    

	reset_connections(c_count, MIN(clus1, clus2), MAX(clus1, clus2));
	/* Builds the <c_list> */
	merge_clusters(clus1, clus2, c_count, &initClusters, acceptance_fn, tfile);   
	continueFlag = find_next_cluster(c_count = ilist_length(initClusters));

	if (continueFlag == FALSE) 
	{
	    /* Nothing left is connected, so force them into partitions */
	    c_list = (clist *)append_ilist(c_list, initClusters);
	    initClusters = NULL;
	    break;
	}
    } while(c_count > 1);

    if ((continueFlag  == TRUE) && (initClusters != NULL) && 
	((initClusters->this != NULL) && ((*acceptance_fn)(initClusters->this) == TRUE)))
	c_list = (clist *)append_ilist(c_list, initClusters);
    else if ((initClusters != NULL) && 
	     ((initClusters->this != NULL) && 
	      ((*acceptance_fn)(initClusters->this) != TRUE)))
    {
	error("build_clusters: tree pruned badly.");
	exit(-1);
    }

    if (debug_level < -2) fclose(tfile);
}

/*---------------------------------------------------------------
 * init_clusters (Level 3)
 * set up a cluster for each module, set the cluster_count, and 
 * set up the c_list, which is used to keep track of where
 * the modules have been placed.
 *---------------------------------------------------------------
 */

init_clusters(m_count, lst)
    int m_count;
    clist **lst;
    
{	
    mlist *m;
    ctree *c_leaf;
    cluster *c_ptr;
    
/* setup the cluster list (c_list) with all of the cluster leaf nodes
 * (Each module becomes a seperate leaf node.)	Use the global module list
 * as the information source.			*/

    for (m = modules; m != NULL; m = m->next)
    {
	c_ptr = (cluster *)calloc(1, sizeof(cluster));
	c_ptr->index = m->this->index;
	c_ptr->size = 1;
	c_ptr->connects = connected[m->this->index][m->this->index];
	c_ptr->contents = m->this;
	
	c_leaf = (ctree *)build_leaf((int *)c_ptr);
	ipush((int *)c_leaf, (ilist **)lst);
    }		

/* find the two clusters that should be merged:				      */
    find_next_cluster(m_count);
}

/*------------------------------------------------------------------------
 * reset_connections (Level 3)
 * This takes the column numbers from clus1 and clus2 and combines the two
 * columns.  The side effect of this process is to remove a column and row 
 * from the connection matrix.
 *------------------------------------------------------------------------
 */
int reset_connections(cl_count, merge_col, scratch_col)
    int cl_count, merge_col, scratch_col;
{	
    int i, j;
    
    for (i=0; i < cl_count - 1; i++)
    {
	/* first save the original merged row and scratch row, & the
	   original scratch row and scratch columns in case this merger
	   results in a new partition.  If a new partition is formed, one of these  
	   row/col pairs will be used to restore the connection matrix.	*/
	rpl_merge_row[i] = connected[merge_col][i];
	rpl_merge_col[i] = connected[i][merge_col];
	rpl_scratch_row[i] = connected[scratch_col][i];
	rpl_scratch_col[i] = connected[i][scratch_col];

	if (i == cl_count - 2)	/* need to get the full row */
	{
	    rpl_merge_row[i+1] = connected[merge_col][i+1];
	    rpl_merge_col[i+1] = connected[i+1][merge_col];
	    rpl_scratch_row[i+1] = connected[scratch_col][i+1];
	    rpl_scratch_col[i+1] = connected[i+1][scratch_col];
	}

/* Now reset the connection matrix for this set of clusters, combining clus1 & clus2: */
/* Note that one column and one row are dropped from the connection matrix.           */
	for (j=0; j < cl_count - 1; j++)
	{
	    /* reset the array as if nothing were to be removed: */
	    if (i == merge_col)
	    {
		if (j == merge_col)
	        connected[i][j] = connected[i][j] + connected[scratch_col][scratch_col] 
		- connected[i][scratch_col] - connected[scratch_col][j];
		    
		else if (j < scratch_col) connected[i][j] = 
		    connected[i][j] + connected[scratch_col][j];
		
		else connected[i][j] = connected[i][j+1] + connected[scratch_col][j+1]; 
	    }
	    
	    else if (i < scratch_col) 
	    {
		if (j >= scratch_col) connected[i][j] = connected[i][j+1];
		else if (j == merge_col) connected[i][j] = 
		    connected[i][j] + connected[i][scratch_col];
		                                          
/*		else connected[i][j] = connected[i][j];   */
	    }
	    
	    else /* (i >= scratch_col) */
	    {
		if (j == merge_col) connected[i][j] = 
		    connected[i+1][j] + connected[i+1][scratch_col];
		else if (j < scratch_col) connected[i][j] = connected[i+1][j];
		else connected[i][j] = connected[i+1][j+1];
	    }
	
	}
    }
}

/*---------------------------------------------------------------
 * good_partition (Level 4)
 * This returns TRUE iff one of the two branches of the given tree
 * should be pruned to form a partition.  The severed branch is
 * then placed on the given cluster list (lst).
 *---------------------------------------------------------------
 */
int good_partition(tree, cCount, parts, actv, acceptance_fn, row1, row2, tf)
    ctree **tree;	/* The cluster tree in question */
    int cCount;		/* Number of clusters extant */
    clist **parts;	/* ordered list of clusters that are now partitions */
    clist **actv;	/* ordered list of actve clusters */
    int (*acceptance_fn)(), row1, row2;
    FILE *tf;		/* Temp File - for debug information */
{
    ctree *temp, *parent = *tree;
    int r, l, p;
    
    if ((int *)parent == (int *)NULL) return;		    /* return(NULL); */
    
    r = (parent->right != NULL) ? (*acceptance_fn)(parent->right) : FALSE;
    l = (parent->left != NULL) ? (*acceptance_fn)(parent->left) : FALSE;
    p = (parent != NULL) ? (*acceptance_fn)(parent) : FALSE;
    
    if (p == FALSE)				/* clip some branch, and try again */
    {
	if ((l == TRUE) && 			/* clip the left branch */
	    ((r == FALSE) || (parent->left->this->size > parent->right->this->size)))	
	{		    
	    if (debug_level < -2)
	    {
		fprintf(tf, "Cluster ");
		print_ctree(tf, parent->left);
		fprintf(tf,"\nWas saved as a partition #%d.(L)\n",ilist_length(*parts)+1);
	    }

	    indexed_list_insert((int *)parent->left, ilist_length(*parts), 
				(ilist **)parts);
	    if (parent->right != NULL)	/* finish clipping the left branch */
	    {	
		*tree = parent->right;
		my_free(parent);
	    }
	    else parent->left = NULL;		/* now reset the parent */
	    /* replace <row1> with the old contents of <row2>: */
	    reset_connected_matrix(row1, row2, cCount, actv,
	                      &rpl_scratch_col[0], &rpl_scratch_row[0], matrix_type);
	    return(TRUE);	
	}
	else if ((r == TRUE) && 		/* clip the right branch */
		 ((l == FALSE) || 
		  (parent->right->this->size >= parent->left->this->size)))
	{
	    if (debug_level < -2)
	    {
		print_ctree(tf, parent->right);
		fprintf(tf," being saved as a partition.(R)\n");
	    }

	    indexed_list_insert((int *)parent->right, ilist_length(*parts),
				(ilist **)parts);
	    if (parent->left != NULL)	/* finish clipping the right branch */
	    {
		*tree = parent->left;
		my_free(parent);
	    }
	    else parent->right = NULL;	
	    /* replace <row1> with the old contents of <row1> */
	    reset_connected_matrix(row1, row1, cCount, actv,
				   &rpl_merge_col[0], &rpl_merge_row[0], matrix_type);
	    return(TRUE);	
	}
    }
    
    else return(FALSE);
}

/*---------------------------------------------------------------
 * merge_clusters   (Level 3)
 * Take the two given column numbers, look up their corresponding clusters, 
 * and build another tree node from them.  Place this node in the cluster list
 * position indexed by c1.  Remove the position indexed by c2.  
 *----------------------------------------------------------------
 */

int merge_clusters(c1, c2, cCount, t, acceptance_fn, tf)
    int c1, c2, cCount;
    clist **t;			/* Indexed list of active clusters */
    int (*acceptance_fn)();
    FILE *tf;    
{
    ctree *temp;
    ctree *r = (ctree *)remove_indexed_element(c2, t);
    clist *merge = (clist *)find_indexed_element(c1, *t);
    ctree *l;

    if (merge == NULL) {
	error("merge_clusters: Passed bad element c1","");
	return 0;
    }
    l = merge->this;

/* build the new treenode: */
/* combine the two clusters into one (clus1 and clus2) and add them to the tree: */
    temp = (ctree *)build_node(l, r, (int *)calloc(1, sizeof(cluster)));
    temp->this->index = c1;
    temp->this->size = r->this->size + l->this->size;
    temp->this->connects = connected[c1][c1];

/* check to see if this branch should be clipped, and saved as a partition: */
    good_partition(&temp, cCount, &c_list, t, acceptance_fn, c1, c2, tf);
    merge->this = temp;    
}
	    
/*---------------------------------------------------------------
 * find_next_cluster   (Level 3)
 * keep track of which of the next clusters should be combined on the next pass:
 * (calculate the cluster values, noting which is the smallest - then set clus1 and 
 * clus2 accordingly)							 
 *----------------------------------------------------------------
 */
int find_next_cluster(c_count)
    int c_count;
{
    int i, j, flag= 0;
    float cvalue, smallest;

/* a hack to start off the search */
/* find the first two connected things : */
    for (i=0; i < c_count; i++)
    {
	for  (j=0; j < c_count; j++)	
	{
	    if ((j != i) && (connected[i][j] > 0))
	    {
		smallest = 
	       (float)(connected[i][i] + connected[j][j] - connected[i][j] 
		       - connected[j][i])/
               (float)(connected[i][i] + connected[j][j]);
		clus1 = MIN(i,j);
		clus2 = MAX(i,j);
		i = j = flag = c_count;	/* exit the loops */
	    }
	}
    }
/* check to see that smallest was set to something: */
    if (flag == 0)
    {
	clus1 = 0;
	clus2 = 1;
	return (FALSE);
    }
    
     
    for (i=0; i < c_count; i++)	
    {
	for (j=0; j < c_count; j++)	
	if ((j > i) && (connected[i][i] != 0) && (connected[j][j] != 0)) 
	{ 
	    cvalue = 
	       (float)(connected[i][i] + connected[j][j] - connected[i][j] 
		       - connected[j][i]) /
               (float)(connected[i][i] + connected[j][j]);

	    if ((smallest >= cvalue) && (connected[i][j] > 0))
	    { 
		smallest = cvalue;    /* exit with clus1, clus2 set */
		clus1 = i;
		clus2 = j;
	    }			
	}
    }
    return(TRUE);
}

/*---------------------------------------------------------------
 * internal_placement (Level 1)
 * This places the first <partition_count> parititions
 * These placements are on a virtual page [origin = (0,0)], and no
 * attention is paid to modules outside of this partition.
 * (EXCEPTION: systerminals are considered)
 * The metric used is as follows:  
 *	- All cross-coupled modules are placed in above each other
 *	- modules with systerm inputs are to the left
 *	- modules with systerm outputs are to the right
 *	- modules with a common output are placed above each other
 *	- modules with a common input are placed above each other
 *---------------------------------------------------------------
 */
/*---------------------------------------------------------------
 * cross_coupled_p(m1, m2, l) return TRUE iff <m1> has terminals connected
 * to both the inputs and outputs of <m2> at <l> levels of removal.
 *--------------------------------------------------------------
 */
int cross_coupled_p(m1,m2,l)
    module *m1, *m2;
    int l;
{
    tlist *t, *tt;
    int r1;		/* The depth from <m1> to <m2> */    
    int r2;		/* The depth from <m2> to <m1> */
    
    for (t = m1->terms; t != NULL; t = t->next)
    {
	if ((t->this->type == OUT) && 
	    (r1 = on_output_net_p(m2, t->this->nt, l, 1) != FALSE)) 
	/* found <m2> somewhere */
	{
	    /* This prevents the string layer from trying to use modules that are 
	       already well positioned */
	    if (m2->placed == PLACED) return;		    /* return(NULL); */
	    
	    for(tt = m2->terms; tt != NULL; tt = tt->next)
	    {
		if ((tt->this->type == OUT) &&
		    (r2 = on_output_net_p(m1, tt->this->nt, l, 1) != FALSE))
		{   /* found <m1> somewhere */
		    d1 = r1;
		    d2 = r2;
		    return(TRUE); /* return both <r1> & <r2> for non-trivial <l> */
		}
	    }
	}
    }
    return(FALSE);
}
/*---------------------------------------------------------------
 * on_output_net_p(m, n, l) return TRUE iff <m1> has terminals connected
 * to both the inputs and outputs of <m2> at <l>levels of removal.
 *--------------------------------------------------------------
 */

int on_output_net_p(m, n, l, s)
    module *m;
    net *n;
    int l, s;
{
    tlist *t, *tt;
    
    /* See if the module in question is on this net */
    for (t = n->terms; t != NULL; t = t->next) 
    {
	if (t->this->type == IN)
	{
	    if (t->this->mod == m) return(s);	/* found something */
	    else if (l > 1) 			/* keep looking */
	    {
		/* This prevents the string layer from trying to use modules that are 
		 * already well positioned */
		if (t->this->mod->placed == PLACED) return(FALSE);
		
		for (tt = t->this->mod->terms; tt != NULL; tt = tt->next)
		{
		    if ((tt->this->type == OUT) &&
			(s = on_output_net_p(m, tt->this->nt, l-1, s+1) != FALSE)) 
		    return(s+1);
		}
	    }
	}
    }
    return(FALSE);
}

/*---------------------------------------------------------------
 * better_inheritence (path1, path2, p)
 * return TRUE if <path1> shows better inheritence then <path2>
 *---------------------------------------------------------------
 */
int better_inheritence (path1, path2, p)
    mlist *path1, *path2;
    int p;
{
    /* <path1> shows better inheritence then <path2> if outputs of modules in 
       <path1> feed inputs of modules in <path2> that are not in <path1>. */
    module *m, *candidateMod;
    mlist *ml;
    tlist *tl, *ttl;

    for (ml = path1; ml != NULL; ml = ml->next)
    {
	m = ml->this;
	for (tl = m->terms; tl != NULL; tl = tl->next)
	{
	    if ((tl->this->type == OUT) || (tl->this->type == INOUT))
	    {
		for(ttl = tl->this->nt->terms; ttl != NULL; ttl = ttl->next)
		{
		    if ((ttl->this->type == IN) &&
			(ttl->this->mod->index == m->index) && /* same partition */
			(member_p(ttl->this->mod, path2, identity) == TRUE))
		    {
			candidateMod = ttl->this->mod;
			if (member_p(candidateMod, path1, identity) == FALSE) 
			    return(TRUE);
		    }
		}
	    }
	}
    }
    return(FALSE);
}


/*---------------------------------------------------------------
 * Original spl code:
 *---------------------------------------------------------------
 */
/*---------------------------------------------------------------
 * longest_path (Level 3)
 * For this root in the partition try to construct the
 * longest path. If it is, use it for the string for this partition.
 * stf addition:  This is not the case if <m> is an output terminal,
 * AND there are other items in the string [output terminals should be 
 *at the end of non-trivial strings]; AND this is not the case
 * if <m> is not IN/INOUT terminal, AND there is some other IN/INOUT
 * terminal along <m>'s path. [strings should begin with their input
 * terminals, if ther are any in the string]
 

 * The problem is with ties - there are (or should be) a number of 
 * modules that would make equally-long strings out of the partition,
 * but don't make it.  Some modules of equally-long strings are better
 * than others: zb. A module that connects only to the input systerms
 * and internally is better than one that connects to input and output 
 * systerms .
 
 *---------------------------------------------------------------
 */
mlist *longest_path(m, p, len, cmplxLen)
    module *m;
    int p, len;
    int *cmplxLen;
{
    mlist *path = NULL, *x, *xprev;
    int cmplxPathLength = 0;

    reset_path_trace(partitions[p]);		 /* reset the flags for trace_path */

    /* the order of this list corresponds to the placement order within a box */
    path = trace_path(m, p, 0, &cmplxPathLength);  
	    
    if (path == NULL) 
    {
	*cmplxLen = 0;
	return(NULL);
    }
    
    if ((cmplxPathLength > len) ||
	((strings[p] != NULL) && (any_placed_p(strings[p]) == TRUE)))
    {
	/* NOW- clip any systerms from the list:
	 * (This is to see that the strings do not get oversized for modules that will
	 *  be placed outside of the partitioning scheme.)
	 */
	*cmplxLen = cmplxPathLength;
	xprev = path;
	for (x = path->next; x != NULL; x = x->next)
	{
	    if ((!strcmp(x->this->type, "OUT")) || 
		(!strcmp(x->this->type, "INOUT")) || 
		(!strcmp(x->this->type, "IN")))	
	    {
		xprev->next = x->next;
	    }
	}
	/* now check the head */
	if ((path->next != NULL) && ((!strcmp(path->this->type, "OUT")) || 
				     (!strcmp(path->this->type, "INOUT")) || 
				     (!strcmp(path->this->type, "IN"))))
	{		
	    path = path->next;
	}
	
	strings[p] = path;
		
      	if(debug_level > 10) /* debug */	
	{
	    fprintf(stderr,
		    "\nNew String: Partition: %d, Root: %s\n",
		    p, m->name);
	    for(x = path; x != NULL; x = x->next)
	    {
		fprintf(stderr, "Module: %s\n", x->this->name);
	    }
	}
    }
    else *cmplxLen = len;
    return(strings[p]);    
}

/*---------------------------------------------------------------
 * longest_path_in_part
 *---------------------------------------------------------------
 */
mlist *longest_path_in_part(p)
    int p;
{
    /* return the longest path of unplaced modules in partition <p>. */
    mlist *bestPath = NULL, *path, *ml;
    int len, connCount, bestLen = 0;

    for (ml = partitions[p]; ml != NULL; ml = ml->next)
    {
	// if (ml->this->placed == FALSE)
	if (ml->this->placed == UNPLACED)
	{
	    path = longest_path(ml->this, p, bestLen, &len);
	    if (len > bestLen) 
	    {
		bestLen = len;
		bestPath = path;
		connCount = count_connections(bestPath, p);
	    }
	    else if ((len == bestLen) && 
		     (count_connections(path, p) > connCount))
	    {
		bestLen = len;
		bestPath = path;
		connCount = count_connections(bestPath, p);
	    }
	    else if ((len == bestLen) &&
		     (better_inheritence(path, bestPath, p) == TRUE))
	    {
		bestLen = len;
		bestPath = path;
		connCount = count_connections(bestPath, p);
	    }
	}
    }
    strings[p] = bestPath;
    str_length[p] = 0;
    str_height[p] = 0;
    return(bestPath);
}

/* ---------------------------------------------------------------------------*/
void insert_boxtile(t, w)
    tile *t;
    mlist *w;
{
    mlist *ml;
    /* Mark where the new tile went: */
    for(ml = w; ml != NULL; ml = ml->next) ml->this->htile = t;
    t->this = (int *)w;
}
/* ---------------------------------------------------------------------------*/
void manage_string_contents_for_merge(newTile, oldTile)
    tile *newTile, *oldTile;
{
    mlist *ml;
    /* Mark where the new tile went: */
    for(ml = (mlist *)oldTile->this; ml != NULL; ml = ml->next) 
        ml->this->htile = newTile;
    newTile->this = oldTile->this;
}    
/*---------------------------------------------------------------
 * place_first_strings (Level 1)
 * This places the non-zero strings.  (whatever's in strings[])
 * We assume there is only one of them per partition.
 *---------------------------------------------------------------
 */
void place_first_strings()
{
    int i, start_x, start_y;
    tile *boxTile;
    
    xfloor = 0;	   /* initialize the offsets for the placement routines */
    yfloor = 0;
    
    /* Set the content-managing functions for this tilespace: */
    insert_contents = insert_boxtile;
    manage_contents__hmerge = manage_string_contents_for_merge;
    manage_contents__vmerge = manage_string_contents_for_merge;

    for (i = 1; i <= partition_count; i++)
    {
	if (partitions[i] != NULL)
	{
	    init_string(i);
	    
	    place_head(i);

	    start_x = strings[i]->this->x_pos + strings[i]->this->x_size + 
	              WHITE_SPACE * count_terms(strings[i]->this, RIGHT);	    
	    start_y = strings[i]->this->y_pos + strings[i]->this->y_size;

	    place_string(i);

	    if (yfloor < 0)  fixup_string(i, xfloor, yfloor - 1);

	    x_sizes[i] = str_length[i];
	    y_sizes[i] = str_height[i];
	    
	    boxTile = insert_tile(rootTiles[i], 0, 0, str_length[i], str_height[i], 
				  strings[i]); 
	}
    }
}
/*---------------------------------------------------------------
 * init_string (Level 2)
 * Look through the partitions, identifying which modules which are roots.
 * Find the longest path from any root in each partition, make this the 
 * (single) non-trivial string for that partition.
 *---------------------------------------------------------------
 */
init_string(p)
    int p;
{
    mlist *ml;
    int len = 0;

    /* lookup the actual module */
    for (ml = partitions[p]; ml != NULL; ml = ml->next)
    {
	/* if it is a root, see if it is the head of the longest
	 * possible path within its own partition.
	 */
	if (is_root(ml->this, ml->this->index))
	{
	    longest_path(ml->this, p, len, &len);
	}
    }
    if (strings[p] == NULL) longest_path(partitions[p]->this, p, len, &len);
}

/*---------------------------------------------------------------
 * place_head	 (Level 2)
 * perform the rotation and placement of head of the strings
 * Note we actually do the rotation math here, since
 * we should only have to do it once. If we ever do 
 * hierarical rotation, we might just want to keep the fact
 * that it was rotated.
 *---------------------------------------------------------------
 */
int place_head(part)
    int part;
{   
    mlist *ml;
    module *m;
    int rot = 0;

    if (strings[part] == NULL) return(0);
    
    ml = strings[part];
    m = ml->this;
    
    /* For analog devices, m may not have a primary output */

    if ((ml->next != NULL) && (m->primary_out != NULL)) {

	/* non-singular string */
	switch (side_of(m->primary_out)) {

	    /* rotate module to PUT PRIMARY OUTPUT ON THE RIGHT */
	    
	    case LEFT:	rot = ONE_EIGHTY;	break;	
	    case UP: 	rot = NINETY;		break;	
	    case RIGHT:	rot = ZERO;		break;
	    case DOWN:	rot = TWO_SEVENTY;	break;
	    
	    default: error("place_head: bad side_of primary terminal",
			   m->name);
	}
	if (rot != ZERO)
	{/* update all the terminal postions and sides */
	    
	    rotate_module(m, rot);
	}
    }
    m->x_pos = WHITE_SPACE * count_terms(m, LEFT);
    m->y_pos = WHITE_SPACE * count_terms(m, DOWN) + 1;

    /* Create the fuzzy (delayed-evaluation) position: */
    m->fx->item1 = create_rng_varStruct
                    (create_srange(m->x_pos - XFUZZ, m->x_pos + XFUZZ, X));
    m->fy->item1 = create_rng_varStruct
                    (create_srange(m->y_pos - YFUZZ, m->y_pos + YFUZZ, Y));
    
    /* mark the module as placed */
    m->placed = PLACED;

    str_length[part] = m->x_pos + m->x_size + 	/* CHANNEL_LENGTH + */
                     (WHITE_SPACE * (count_terms(m, RIGHT) + 1));
    str_height[part] = m->y_pos + m->y_size + /* CHANNEL_HEIGHT + */
                      (WHITE_SPACE * (count_terms(m, UP) + 1));
}

/*------------------------------------------------------------------------------------
 * output_term_connecting (Level 3)
 * Locate the first terminal that is an output of <m1> that ties to <m2>:
 *------------------------------------------------------------------------------------
 */
term *output_term_connecting(m1, m2)
    module *m1, *m2;
{
    term *t;
    tlist *tl, *tll;

    if (m1 == NULL) return(NULL);
    else if (m2 == NULL) return(m1->primary_out);

    for (tl = m1->terms; tl != NULL; tl  = tl->next)
    {
	if ((tl->this->type == OUT) || (tl->this->type == INOUT))
	{
	    t = tl->this;
	    for (tll = m2->terms; tll != NULL; tll  = tll->next)
	    {
		if (t->nt == tll->this->nt) return(t);
	    }
	}
    }
    /* Default : (Probably will cause problems... */
    return(m1->primary_out);
}
    

/*---------------------------------------------------------------
 * place_string (Level 2)
 * perform the rotation and placement of the rest of each string
 * return the value of the lowest module's y postion.
 * This monster is very convoluted, and difficult to explain, mainly
 * due to the complexities of complex-string placement.
 *---------------------------------------------------------------
 */
int place_string(part)
    int part;
{   
    mlist *ml, *t, *given_string, *top_string = NULL;
    module *m, *cc_m, *last_m, *top_cc_mod = NULL, *bot_cc_mod = NULL;
    term *prevt;
    int tsLength, lev, margin, rot = 0, cc_flag = FALSE, curHeight;
    int offset = 0;

    if (strings[part] == NULL) return(0);
    
    given_string = strings[part];
    last_m = given_string->this;
    prevt = output_term_connecting(last_m, (given_string->next != NULL) ? given_string->next->this : NULL);

    /* skip the first one, we just did it in init_place above */

    for(ml = given_string->next; ml != NULL; ml = ml->next)
    {
	m = cc_m = ml->this;
	if (m->placed == UNPLACED)
	{
	
	    /* Set up all the strangeness for cross-coupled modules: */
	    for (lev = 1; (lev <= cc_search_level) && (cc_m != NULL); lev++)
	    {
		if (cross_coupled_p(last_m, cc_m, lev) != FALSE)
		{
		    /* <d1> & <d2> (which represent the depths from <last_m> to <m> & vv) 
                     * are now set */
		    cc_flag = TRUE; 
		    top_cc_mod = last_m;
		    bot_cc_mod = cc_m;
		    break;
		}
		cc_m = (ml->next != NULL) ? ml->next->this : NULL;
	    }
	
/* This creates two modes to operate in:  either cc mode, or in-line mode, all centered 
   around the <cc_flag>.  Very strange contortions happen for multi-level cc modules.
   <lev> is the level of removal from <last_m> at which the cross-coupling was discovered
   <cc_m> is the module that is coupled to <last_m> (AKA <bot_cc_mod>).
   <last_m> is the last module to be succesfully placed from string<ml> 
   (AKA <top_cc_mod>).

   <m> is the next module to be placed behind <last_m>, and all modules between <m> & 
   <cc_m> belong behind <last_m>.
   Any modules remaining in string<ml> (behind <cc_m>) belong behind <cc_m>. <cc_m> 
   should be placed directly beneath <last_m>.
 */

	    
	    /* Analog devices do not necessarily have a primary input */

	    if (m->primary_in != NULL) {
	        switch (side_of(m->primary_in)) {
	            /* rotate module to put PRIMARY INPUT ON THE LEFT */
		
		    case LEFT:	rot = ZERO;		break;
		    case UP: 	rot = TWO_SEVENTY;     	break;
		    case RIGHT:	rot = ONE_EIGHTY;	break;
		    case DOWN:	rot = NINETY;		break;
	    
		    default: error("place_string: bad side_of primary terminal",
				m->name);
	        }
	    }
	    /* update all the terminal postions and sides */
	    if (rot != ZERO) rotate_module(m, rot);
	    
	    /* move the module to the right of the space already allocated.  
	       NOTE: At this point the function becomes recursive, so as to handle the 
	             top string completely.  There is a top string to be handled iff a 
		     pair of cc modules has been discovered, AND we are about to place 
		     the bottom module. */

	    if ((cc_flag == TRUE) && (m == bot_cc_mod)) 	
	    {
		/* mark <bot_cc_mod> as PLACED, so path_trace() will not step on him. */
		bot_cc_mod->placed = PLACED;
		top_cc_mod->placed = UNPLACED;	/* This is so trace_path() will work properly */
		set_flag_value(partitions[part], UNSEEN);
		set_flag_value(ml, SEEN);
		given_string = strings[part];

		/* reset strings[part] with the top-string info: */
		top_string = longest_path(top_cc_mod, part, 0, &tsLength);
		top_cc_mod->placed = PLACED;	/* it really is placed, so note it. */

		if ((top_string != NULL) && (top_string->next != NULL))
		{
		    offset = place_string(part);
		    /* add the modules in <given_string> back to stings[part] */
		    for(t = given_string; t != NULL; t = t->next)
		    {
			pushnew(t->this, &strings[part]);
		    }		    
		}
		else strings[part] = given_string;	/* restore strings[part] */
		
		/* now that the top string has been handled, finish up with the bottom 
		   string: */
		m->x_pos = top_cc_mod->x_pos;
		m->y_pos = top_cc_mod->y_pos - m->y_size - CC_SPACE - 
		           (WHITE_SPACE * (count_terms(m, DOWN) + 1)) + offset;

		/* Create the fuzzy (delayed-evaluation) position: */
		m->fx->item1 = create_var_varStruct(top_cc_mod->fx);
		/* Short version (not quite right) */
		m->fy->item1 = create_rng_varStruct
		                (create_srange(m->y_pos - YFUZZ, 
					       m->y_pos + YFUZZ, Y));

	    }
	
	    else 
	    {
		margin = WHITE_SPACE * (count_terms_between_mods(last_m, m) + 1);
		m->x_pos = last_m->x_pos + last_m->x_size + margin;
		m->y_pos = line_up_mod(m, prevt);

		/* Create the fuzzy (delayed-evaluation) position: */
		/* fx = last_m->x_pos + last_m->x_size + margin +/- XFUZZ	*/
		m->fx->item1 = 
		    create_var_struct
		        (ADD, last_m->fx,
			 create_var_struct
			     (ADD, create_int_varStruct(&last_m->x_size),
			      create_rng_varStruct
			      (create_srange(margin - XFUZZ, margin + XFUZZ, X))));
		/* Short version (not quite right) */
		m->fy->item1 = 
		    create_rng_varStruct(create_srange(m->y_pos - YFUZZ, 
						       m->y_pos + YFUZZ, Y));
	    }
	    
	    /* mark the module as placed */
	    m->placed = PLACED;

	    /* but we might have gone below the bottom, remember it */	
	    offset = (bot_cc_mod != NULL) ? 
	      bot_cc_mod->y_pos - (WHITE_SPACE * count_terms(bot_cc_mod, DOWN)) : 0;
	    yfloor = MIN(yfloor, (m->y_pos - (WHITE_SPACE * count_terms(m, DOWN))));

	    /* We might have gone above the current top */	
	    curHeight = m->y_pos + m->y_size + CHANNEL_HEIGHT + 
	           (WHITE_SPACE * (count_terms(m, UP) + 1));
	    str_height[part] = MAX(curHeight, str_height[part]);

	    str_length[part] = MAX(str_length[part],
				   m->x_pos + m->x_size + CHANNEL_LENGTH +
				   (WHITE_SPACE * (count_terms(m, RIGHT) + 1)));
	}
	
	/* get ready for the next iteration */
	prevt = output_term_connecting(m, (ml->next != NULL) ? ml->next->this : NULL);
	last_m = m;
    
    }
    return(offset);
}


/*---------------------------------------------------------------
 * box_placement (Level 1)
 * should follow module_placement()
 *---------------------------------------------------------------
 */

void box_placement()
{
    int i, len, start_x, start_y;
    module *mod;
    mlist *box;
    
    /* for each partition, find the next string.  Then place this string within its box.
     * Then place this box around the other boxes (the PLACED modules within the given
     * partition)
     */

    /* remove old connection counts */
    reset_info();

    /* initialize lower left corner offset */
    xfloor = 0;
    yfloor = 0;
    
    for(i = 1; i <= partition_count;i++)
    {	
	mod = select_next_box(i);       /* NOT IN USE yields a mod not already placed */

	while (mod != NULL)
	{
	    box = longest_path_in_part(i);	
/*NEW	    box = longest_path(mod, i, 0, &len);	/* find the next string */
	    place_head(i);			/* place the first module in the box */
	                                        /* box == strings[i] */
	    /* Added by Tim to cope with a segfault */
	    if (strings[i] == NULL) {
	       error("box_placement: NULL strings entry", "");
	       break;
	    }
	    start_x = strings[i]->this->x_pos + strings[i]->this->x_size + 
	              WHITE_SPACE * count_terms(strings[i]->this, RIGHT);
	    start_y = strings[i]->this->y_pos;
	    place_string(i);			/* place the rest of the string */
	    if( yfloor < 0) fixup_string(i, xfloor, yfloor - 1);

	    /* move the box to its position about the boxes already placed in the 
	       partition */	
	    box = strings[i];		/* Pick up anything added in "place_string" */
	    place_box(box, i);			    
	    mod = select_next_box(i);
	}
    }
}
/*---------------------------------------------------------------
 * old_select_next_box (Level 2)
 *---------------------------------------------------------------
 */
module *old_select_next_box(part)
    int part;
{
    /* Select the "most connected" unplaced module within the partition */
    mlist *ml;
    module *mod, *best = NULL;
    
    int ci = -1;

    for(ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	mod = ml->this;

	/* re-count connections just for this module */
	count_inout(mod->index, part);	
	
	if ((mod->placed == UNPLACED) && 	   /* skip things already down */
	    (module_info[mod->index].in > ci) &&
	    (strcmp(mod->type, "OUT")) &&	   /* skip systerms */
	    (strcmp(mod->type, "INOUT")) &&
	    (strcmp(mod->type, "IN"))) 
	{
	    ci = module_info[mod->index].in;
	    best = mod;
	}
    }
    return(best);
}
/*---------------------------------------------------------------
 * select_next_box (Level 2)
 * This determines the order that modules are placed within a partition.
 *---------------------------------------------------------------
 */
module *select_next_box(part)
    int part;
{
    /* Select the module in the partition that is best connected to the
       already-placed modules within the partition.  */
    mlist *ml, *mll;
    module *mod, *best = NULL;
    int count, bestCount = 0;

    for(ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	mod = ml->this;
	if ((mod->placed == UNPLACED) && 	   /* skip things already down */
	    (strcmp(mod->type, "OUT")) &&	   /* skip systerms */
	    (strcmp(mod->type, "INOUT")) &&
	    (strcmp(mod->type, "IN"))) 
	{
	    count = 0;
	    for(mll = partitions[part]; mll != NULL; mll = mll->next)	
	    {
		if (mll->this->placed == PLACED) 
		    count += connected[mod->index][mll->this->index];
	    }
	    if (count > bestCount)
	    {
		bestCount = count;
		best = mod;
	    }
	}
    }
    if (best == NULL) return(old_select_next_box(part));
    return(best);
}
/*---------------------------------------------------------------
 *  default_place_box(Level 2)
 *  what to do when there is no connectivity information: set x, y
 * to have this box abut the current partition:
 *---------------------------------------------------------------
 */
default_place_box(box, part, x, y)
    mlist *box;
    int part, *x, *y;
{
    int side;
    
    /* decide which side of the bounding box to add this part onto */
    /* NOTE: The call to "best_side_box" also sets the side/count globals, 
             including their signs.  */

    side = best_side_box(part, box);

    switch(side)
    {
	case LEFT:
	{
	    *x = xfloor - str_length[part];
	    *y = (int)(ygbr - ygr);	
	    break;
	}
	case UP:
	{
	    *x = (int)(xgbr - xgr) - xfloor;
	    *y = y_sizes[part];
	    break;
	}
	case RIGHT:
	{
	    *x = x_sizes[part];
	    *y = (int)(ygbr - ygr);
	    break;
	}
	case DOWN:
	{
	    *x = (int)(xgbr - xgr) - xfloor; 
	    *y = yfloor - str_height[part];
	    break;
	}
    }
}
/*-------------------------------------------------------------------------
 * Pivot and anchor determination:  For left and right-cominated movement,
 * the pivots and anchors need to be set on the edge of a filled tile, so
 * that any desired lineup can occur.   This is deemed unneccessary for 
 * up- and downward movement.

 * NOTE: set_pivot differs from det_pivot_point in that set_pivot does not
 *       have a tile space to work in, rather simply the dimensions of the 
 *       tile that will contain the pivot point.
 *-------------------------------------------------------------------------*/
det_anchor_points(root, xCount, yCount, gravX, gravY, anchX, anchY)
    tile *root;
    int xCount, yCount, gravX, gravY;
    int *anchX, *anchY;
{
    /* set the anchor point */
    tile *tt, *t = locate(root, gravX, gravY);
    
    if (t == NULL)
    {
	/* Just use the gravity points, as calculated. */
	*anchX = gravX;
	*anchY = gravY;
    }
    else if ((abs(yCount) > abs(xCount)) || (xCount == 0))
    {
	/* Just use the gravity points, as calculated. */
	*anchX = gravX;
	*anchY = gravY;	
    }
    else
    {
	/* find the appropriate edge of the tile space: */
	if (xCount > 0)	       /* right movement, so the anchor goes on the right edge */
	{
	    *anchY = gravY;
	    *anchX = (t->this == NULL) ? gravX : t->x_pos + t->x_len;
	    /* The piv/anchor points should be on the egde of a filled tile, not between 
	       filled tiles */
	    tt = locate(t, *anchX + 1, *anchY);
	    if ((tt != NULL) && (tt->this != NULL))
	    fprintf(stderr, "bad anchor chosen for part @(%d, %d)\n", *anchX, *anchY);
	}
	else			/* left movement, so the anchor goes on the left edge. */
	{
	    *anchY = gravY;
	    *anchX = (t->this == NULL) ? gravX : t->x_pos;
	    /* The piv/anchor points should be on the egde of a filled tile, not between 
	       filled tiles */
	    tt = locate(t, *anchX - 1, *anchY);
	    if ((tt != NULL) && (tt->this != NULL))
	    fprintf(stderr, "bad anchor chosen for part @(%d, %d)\n", *anchX, *anchY);
	}
    }   
}

/*-------------------------------------------------------------------------*/
int horizontally(m1, m2)
    module *m1, *m2;
{
    if (m1->x_pos <= m2->x_pos) return(TRUE);
    else return(FALSE);
}
/*-------------------------------------------------------------------------*/
int vertically(m1, m2)
    module *m1, *m2;
{
    if (m1->y_pos <= m2->y_pos) return(TRUE);
    else return(FALSE);
}
/*-------------------------------------------------------------------------*/
det_pivot_points(root, xCount, yCount, gravX, gravY, p, pivX, pivY)
    tile *root;
    int xCount, yCount, gravX, gravY;
    int p;				/* partition number */
    int *pivX, *pivY;
{
    /* set the pivot point */
    tile *tt, *t = locate(root, gravX, gravY);
    int offsetX = 0, offsetY = 0;

    /* all modules in the partition need to be offset by the difference between the
       LLH Module's location and (0,0).  This accounts for differences between the
       tile spaces, and should not be a large number.  This is most critical for
       small partitions.  */

    if ((t == NULL) || (abs(yCount) > abs(xCount)) || (xCount == 0))
    {
	/* Just use the gravity points, as calculated along with the desired offsets: */
	*pivX = gravX + offsetX;
	*pivY = gravY + offsetY;
    }
    else
    {
	/* find the appropriate edge of the tile space: */
	if (xCount < 0)		/* left movement, so the pivot goes on the right edge */
	{
	    *pivY = gravY + offsetY;
	    *pivX = (t->this == NULL) ? gravX : t->x_pos + t->x_len;
	    tt = locate(t, *pivX + 1, *pivY);
	    /* The piv/anchor points should be on the edge of a filled tile, not between 
	       filled tiles */
	    if ((tt != NULL) && (tt->this != NULL))
	    fprintf(stderr, "bad pivot chosen for partition @(%d, %d)\n", *pivX, *pivY);
	}
	else			/* right movement, so the pivot goes on the left edge. */
	{
	    *pivY = gravY + offsetY;
	    *pivX = (t->this == NULL) ? gravX : t->x_pos;
	    /* The piv/anchor points should be on the egde of a filled tile, not between 
	       filled tiles */
	    tt = locate(t, *pivX - 1, *pivY);
	    if ((tt != NULL) && (tt->this != NULL))
	    fprintf(stderr, "bad pivot chosen for partition @(%d, %d)\n", *pivX, *pivY);
	}
    }   
}
/*-------------------------------------------------------------------------*/
set_pivot(xLen, yLen, xCount, yCount, gravX, gravY, pivX, pivY)
    int xLen, yLen, xCount, yCount, gravX, gravY;
    int *pivX, *pivY;

{
    /* set the pivot point */
    
    if ((abs(yCount) > abs(xCount)) || (xCount == 0))
    {
	/* Just use the gravity points, as calculated. */
	*pivX = gravX;
	*pivY = gravY;
    }
    else
    {
	/* find the appropriate edge of the tile space: */
	if (xCount < 0)		/* left movement, so the pivot goes on the right edge */
	{
	    *pivY = gravY;
	    *pivX = xLen;
	}
	else			/* right movement, so the pivot goes on the left edge. */
	{
	    *pivY = gravY;
	    *pivX = 0;		/* default origin */
	    
	}
    }
    
}
/*-------------------------------------------------------------------------*/
normalize_side_counts(xCount,yCount)
    int *xCount, *yCount;
{
    /* Normalize the count values.  This is only important if one of them is 0. */
    if (*xCount == 0) *yCount = *yCount/abs(*yCount);
    else if (*yCount == 0) *xCount = *xCount/abs(*xCount);
}

/*-------------------------------------------------------------------------*/


/*---------------------------------------------------------------
 *  place_box(Level 2)
 * 
 *---------------------------------------------------------------
 */
place_box(box, part)
    mlist *box;
    int part;
{
    tile *boxTile;
    int side, loc, anchX, anchY, pivX, pivY;
    int x, y;
    
    /* decide which side of the bounding box to add this part onto */
    /* NOTE: The call to "best_side_box" also sets the side/count globals, 
             including their signs.  */

    side = best_side_box(part, box);

    if ((xSideCount == 0) && (ySideCount == 0))
    {
	default_place_box(box, part, &x, &y);
	boxTile = insert_tile(rootTiles[part], x, y, str_length[part], 
			      str_height[part], box);
    }
    else
    {
	det_anchor_points(rootTiles[part], xSideCount, ySideCount,
			  (int)xgbr, (int)ygbr, &anchX, &anchY);
	set_pivot(str_length[part], str_height[part], xSideCount, ySideCount,
		  (int)xgr, (int)ygr, &pivX, &pivY);	
	normalize_side_counts(&xSideCount, &ySideCount);
	

	boxTile = angled_tile_insertion(rootTiles[part], 
					anchX, anchY,	 	     /* Anchor point */
					xSideCount, ySideCount,      /* Slope of axis of motion */
					pivX, pivY,	 	     /* Pivot point (in new tile)*/
					str_length[part], str_height[part], /* tile size */
					box);
    }
    
    if (boxTile == NULL)	/* This shouldn't happen */
    {
	error("place_box: No valid insertion found!!","");
	exit(1);
    }
    
    x = boxTile->x_pos;
    y = boxTile->y_pos;
    
    move_string(part, x, y);  

    /* adjust the bounding box, based on the new partition configuration: */
    boxTile = best_tile_in_space(rootTiles[part], most_left);
    xfloor = boxTile->x_pos;

    boxTile = best_tile_in_space(rootTiles[part], most_down);
    yfloor = boxTile->y_pos;

    boxTile = best_tile_in_space(rootTiles[part], most_right);
    x_sizes[part] = boxTile->x_pos + boxTile->x_len;

    boxTile = best_tile_in_space(rootTiles[part], most_up);
    y_sizes[part] = boxTile->y_pos + boxTile->y_len;

    /* put the lower left corner of the string back at 0,0 */
    if((xfloor < 0)||(yfloor < 0)) fixup_partition(part, xfloor, yfloor);
}


/*---------------------------------------------------------------
 * placement_prep (Level 1)
 * Do the prep work for placing the partitions...
 *---------------------------------------------------------------
 */
void placement_prep()
{
    /* reset offset */
    xfloor = 0;
    yfloor = 0;	    

/* Start by removing the system terminals, and building a connection matrix that
 * will allow all of the internal modules to be placed.  Place them, and then go
 * back to handle the two new partitions (partition_count + 1 and partition_count + 2)
 * that have not been accounted for as of yet.  Finally, these two new paritions will
 * need to be placed.
 */
    initialize_partition_markings(partition_count);
    erase_systerms();
/*    convert_boxes_to_module_sets(partition_count); 			*/
    build_connections_amongst_partitions(partition_count, UNSIGNED);

    /* UNSIGNED used to be matrix_type */
}

/*---------------------------------------------------------------
 * partition_placement (Level 1)
 * Put all the partitions into one bounding box in partition 0, 
 * based on picking the largest number of connections off the diagonal 
 * of the connection matrix; (which has been built to match the 
 * partitions, seen as clusters)
 *---------------------------------------------------------------
 */
mlist *partition_placement()
{
    int tx,ty;
    ilist *part_map = NULL;
    int next, c_count, i, *part, order = 1;
    FILE *tfile = (debug_level < -1)?fopen("con", "a"):NULL;

    if (debug_level < -1)		/* debug info */
    {
	fprintf(tfile, "\n-----?? [Rule# %d] -----------------------------------\n",
		partition_rule);
    }

    /* initialize the mapping from partition number to the column within the 
       connection matrix: */

    for (i = 1; i <= partition_count; i++)
    {
	/* unmark all of the modules, as none have been added to partition[0]: */
	set_flag_value(partitions[i], UNPLACED);

	/* continue setting up the cluster-matrix to partition number mapping: */
	part = (int *)calloc(1, sizeof (int));
	*part = i;	

	/* Partitions are numbered [1..partition_count], the array is 
	   [0..partition_count]  */
	indexed_list_insert(part, i ,&part_map);	
    }
    
/* Pick the next partition to go down, and place it in partition[0]:  
 * NOTE: this is the only time that <next> corresponds directly to the actual partition 
 * number being placed.
 */
    c_count = partition_count;
    next = find_max_diagonal(c_count);
    
    while (c_count >= 1)
    {
	if (next == 0)	/* couldn't find anything */
	{
	    next = 1; 	/* Take the next partition in the part_map */
	}

	part = remove_indexed_element(next, &part_map);

	if (debug_level < -1)		/* debug info */
	{ 
	    fprintf(tfile,"merging P0 and P%d", *part);
	    p_cons(tfile, c_count+1, NULL);
	    fprintf(tfile,"\n");
	}    
	
	place_partition(*part, order++);   	 
	reset_connections(c_count+1, 0, next);
	next = find_max_connections_to_P0(--c_count);
	
    }
    if (debug_level < -1) fclose(tfile);

    /* cleanup */

    /* put the lower left corner of the box back at 0,0 */
    if((xfloor < 0)||(yfloor < 0))
    {
	reset_borders(xfloor,yfloor);
	fixup_partition(0, xfloor, yfloor);	
    }

    build_connections(matrix_type);	/* rebuild the connection table for modules */
    
    return(partitions[0]);	
}	
/*===================================================================================*/
/*---------------------------------------------------------------
 * initialize_partition_markings	 (Level 2)
 * This unmarks the ->placed field for all modules, so the gravity_
 * partition() will work properly.
 *--------------------------------------------------------------
 */
initialize_partition_markings(pc)
    int pc;
{
    mlist *ml;
    int i;
    for(i = 1; i <= pc; i++)
    {
	for (ml = partitions[i]; ml != NULL; ml = ml->next)
	{
	    ml->this->flag = UNPLACED;
	}
    }
}
/*----------------------------------------------------------------------------*/
provide_module_spacing(m, xPos, yPos, xLen, yLen)
    module *m;
    int *xPos, *yPos, *xLen, *yLen;
{
    int leftSpacing = (WHITE_SPACE * count_terms(m, LEFT));
    int rightSpacing = (WHITE_SPACE * count_terms(m, RIGHT));
    int topSpacing = (WHITE_SPACE * count_terms(m, UP));

    switch(validate(m->type))
    {
	case BUFFER : 
	case NOT_  : 
	case AND  : 
	case NAND : 
	case OR   : 
	case NOR  : 
	case XOR  : 
	case XNOR : *xPos = m->x_pos - leftSpacing;
	            *yPos = m->y_pos - WHITE_SPACE;
	            *xLen = m->x_size + rightSpacing + leftSpacing;
		    *yLen = m->y_size + WHITE_SPACE * 2;
	            break;
	case INPUT_SYM : 
	case OUTPUT_SYM : *xPos = m->x_pos;
	                  *yPos = m->y_pos;
	                  *xLen = m->x_size = rightSpacing;
		          *yLen = m->y_size;
	                  break;
        case BLOCK : 
        default : 
                     *xPos = m->x_pos - leftSpacing;
                     *yPos = m->y_pos - WHITE_SPACE;
                     *xLen = m->x_size + leftSpacing + rightSpacing;
                     *yLen = m->y_size + topSpacing + 2 * WHITE_SPACE;
	  	     break;
   }
}
/*===================================================================================*/
/*---------------------------------------------------------------
 * convert_boxes_to_module_sets (Level 2)
 * This converts the tiles from representing strings to representing modules.
 *--------------------------------------------------------------
 */
convert_boxes_to_module_sets(pc)
    int pc;
{
    module *m;
    mlist *string, *ml;
    tile *t;
    tilist *tl, *boxes = NULL;
    int i, x, y, xSize, ySize;

    for(i = 1; i <= pc; i++)
    {
	boxes = nonFreespace(rootTiles[i], &boxes);
	for (tl = boxes; tl != NULL; tl = tl->next)
	{
	    string = (mlist *)tl->this->this;	/* the string is the tile contents */

	    rootTiles[i] = delete_tile(tl->this);	        /* scrap the big tile */

	    for(ml = string; ml != NULL; ml = ml->next)
	    {
		/* Insert the small tiles: NOTE:  THE SPACING IS INCORRECT!! */
		m = ml->this;
	        provide_module_spacing (m, &x, &y, &xSize, &ySize);
		insert_tile(rootTiles[i], x, y, xSize, ySize, m);
	    }
	}
	boxes = (tilist *)free_list(boxes);
    }
}
/*===================================================================================*/
/*---------------------------------------------------------------
 * erase_syterms	(Level 2)
 * The systerm modules have been placed within the various partitions.
 * remove them from their partitions so that they may be placed seperately
 * along the edges of the diagram.  These are broken into two groups,
 * the INs and the OUTs.  
 *
 * NOTE:  This also removes the terminals from the nets. 
 *
 *---------------------------------------------------------------
 */
erase_systerms()
{
    int p = partition_count, old_part;             /* number of partitions */
    mlist *systerm;
    int ins = p + 2, outs = p + 1;
    
    for (systerm = ext_mods; systerm != NULL; systerm = systerm->next)
    {
	systerm->this->placed = UNPLACED;	/* unplace the module */
	old_part = module_info[systerm->this->index].used;

	if (!strcmp(systerm->this->type, "OUT"))
	{
	    rem_item(systerm->this, &partitions[old_part]);
	    module_info[systerm->this->index].used = outs;
	    partitions[outs] = (mlist *)concat_list(systerm->this, partitions[outs]);
	}
	else /*( (!strcmp(systerm->this->type, "IN")) || 
	         (!strcmp(systerm->this->type, "INOUT")) )  */
	{
	    rem_item(systerm->this, &partitions[old_part]);
	    module_info[systerm->this->index].used = ins;
	    partitions[ins] = (mlist *)concat_list(systerm->this, partitions[ins]);
	}	    
    }

/* NOTE:  the global <partition_count> is not updated -
          <partition_count> + 1 refers to the output terminals, and 
	  <partition_count> + 2 indexes the input/inout terminals.  */
}


/*---------------------------------------------------------------
 * default_place_partition (Level 2)
 * Put a partition down along an edge of partition[0].
 *---------------------------------------------------------------
 */
default_place_partition(part, x, y)
    int part, *x, *y;
{
    /* Unangled, straight-up adjacent placement.  All this does is to calculate where 
       the partition's new origin should be placed at. */
    int side;
    
    if(partitions[0] == NULL)
    {
	/* this is the first partition to be placed */
	*x = 0;
	*y = 0;
    }
    else
    {
	if (partitions[part] == NULL) return; 		  /* return()NULL); */

	/* decide which side of the bounding box to add this partition onto */
	side = best_side_part(part); 

	switch(side)
	{
	    case LEFT:
	    {
		*x = xfloor - (x_sizes[part] + CHANNEL_LENGTH +
		     WHITE_SPACE * count_terms_part(part, RIGHT));
		*y = (int)(ygbr - ygr);
		break;
	    }
	    case UP:
	    {
		*x = (int)(xgbr - xgr);
		*y = y_sizes[0] + CHANNEL_HEIGHT +
		     WHITE_SPACE * count_terms_part(part, DOWN);
		break;
	    }
	    case RIGHT:
	    {
		*x = x_sizes[0] + CHANNEL_LENGTH +
		                    WHITE_SPACE * count_terms_part(part, LEFT);
		*y = (int)(ygbr - ygr);
		break;
	    }
	    case DOWN:
	    {
		*x = (int)(xgbr - xgr);
		*y = yfloor - (y_sizes[part] + CHANNEL_HEIGHT +
					      WHITE_SPACE * count_terms_part(part, UP));
		break;
	    }
	}	
    }
}
/* ---------------------------------------------------------------------------*/
void insert_boxtile_for_partitions(t, w)
    tile *t;
    mlist *w;
{
    mlist *ml;
    /* Mark where the new tile went: */
    for(ml = w; ml != NULL; ml = ml->next) ml->this->vtile = t;
    t->this = (int *)w;
}
/* ---------------------------------------------------------------------------*/
void manage_partition_contents_for_merge(newTile, oldTile)
    tile *newTile, *oldTile;
{
    mlist *ml;
    /* Mark where the new tile went: */
    for(ml = (mlist *)oldTile->this; ml != NULL; ml = ml->next) 
        ml->this->vtile = newTile;
    newTile->this = oldTile->this;
}    
/* ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------
 * place_partition (Level 2)
 * Put a partition down along an edge of partition[0].
 *---------------------------------------------------------------
 */
place_partition(part, order)
    int part, order;
{
    int i, anchX, anchY, pivX, pivY;
    tilist *tl = NULL;
    tile *temp;
    mlist *ml;
    int x, y, side;
    
    /* list the tiles associated with this partition: */
    tl = nonFreespace(rootTiles[part], &tl);
    
    /* Set the content-managing functions for this tilespace: */
    insert_contents = insert_boxtile_for_partitions;
    manage_contents__hmerge = manage_partition_contents_for_merge;
    manage_contents__vmerge = manage_partition_contents_for_merge;

    if(partitions[0] == NULL)
    {
	/* this is the first partition to be placed */
	x_positions[part] = 0;
	y_positions[part] = 0;

	/* Add these tiles to partition[0]: */
	for (; (tl != NULL); tl = tl->next)
	{
	    temp = insert_tile(rootTiles[0], tl->this->x_pos, tl->this->y_pos, 
			       tl->this->x_len, tl->this->y_len, tl->this->this);
	}
    }
    else
    {
        /* skip empty partitions */
	if (partitions[part] == NULL) return;		   /* return(NULL); */

	/* decide where to add this partition: (set the <xSideCount> & <ySideCount> 
	   globals) */
	side = best_side_part(part);

	if ((xSideCount == 0) && (ySideCount == 0))
	{
	    default_place_partition(part, &x, &y);
	    for (; (tl != NULL); tl = tl->next)
	    {
		temp = insert_tile(rootTiles[0], x + tl->this->x_pos, 
				   y + tl->this->y_pos, 
				   tl->this->x_len, tl->this->y_len, tl->this->this);
	    }
	}
	
	else
	{
	    det_anchor_points(rootTiles[0], xSideCount, ySideCount,
			      (int)xgbr, (int)ygbr, &anchX, &anchY);
	    det_pivot_points(rootTiles[part], xSideCount, ySideCount,
			     (int)xgr, (int)ygr, part, &pivX, &pivY);	
	    normalize_side_counts(&xSideCount, &ySideCount);
	    
	    temp = angled_group_tile_insertion
	           (rootTiles[0], anchX, anchY,       /* Anchor point */
		    xSideCount, ySideCount,           /* Slope of axis of motion */
		    pivX, pivY, 	     	      /* Pivot point (in new tile)*/
		    &x, &y,			      /* new origin for this partition */
		    tl);			      /* Tiles to insert */
	}
    
	if (temp == NULL)			      /* This shouldn't happen */
	{
	    error("place_partition: No valid insertion found!!","");
	    exit(1);
	}
	
	x_positions[part] = x;
	y_positions[part] = y;
    }
    
    /* The rest we also do the first time */

	/* adjust the bounding box(es) */

    x_sizes[part] += x_positions[part];
    y_sizes[part] += y_positions[part];
 
    x_sizes[0] = MAX(x_sizes[0], x_sizes[part]);
    y_sizes[0] = MAX(y_sizes[0], y_sizes[part]);

    xfloor = MIN(xfloor, x_positions[part]);
    yfloor = MIN(yfloor, y_positions[part]);

    /* NOTE: we actually update the modules's position, so we do not
     * have to keep adding in an offset for the partition's position
     * in all future calculations, (like fixup, and for new gravity calcs)
     * If we keep this idea, we do not need the positions array, it will
     * just be a temp in this routine.
     */

    /* move the partition to partition 0 */
    for(ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	if (ml->this->placed == PLACED)
	{
	    partitions[0] = (mlist *)concat_list(ml->this, partitions[0]);

	    /* update the module's position */
	    ml->this->x_pos += x_positions[part];
	    ml->this->y_pos += y_positions[part];
	    
	    /* and mark it as having been added to partitions[0]*/
	    ml->this->flag = PLACED;
	    module_info[ml->this->index].order = order++;
	}
    }
}
/*---------------------------------------------------------------
 * set_flag_value(string, flag_val) 	(Level 3)
 * This is to set the ->flag fields within the given module_string
 *---------------------------------------------------------------
 */
int set_flag_value(string, flag_val)
    mlist *string;
    int flag_val;
{
    mlist *ml;
    for(ml = string; ml != NULL; ml = ml->next)
    {
	ml->this->flag = flag_val;
    }
}
/*---------------------------------------------------------------
 * set_placed_value(string, flag_val) 	(Level 3)
 * This is to set the ->flag fields within the given module_string
 *---------------------------------------------------------------
 */
int set_placed_value(string, flag_val)
    mlist *string;
    int flag_val;
{
    mlist *ml;
    for(ml = string; ml != NULL; ml = ml->next)
    {
	ml->this->placed = flag_val;
    }
}

/*---------------------------------------------------------------
 * reset_path_trace(path) 	(Level 3)
 * This is to set up the ->flag fields within the given partition 
 * such that trace_path() will discover the next string that exists
 * within the partition.  
 *---------------------------------------------------------------
 */
int reset_path_trace(mlist *path)
   {
    mlist *ml;
    for(ml = path; ml != NULL; ml = ml->next)
    {
	if (ml->this->placed == UNPLACED) ml->this->flag = UNSEEN;
    }
}

/*---------------------------------------------------------------
 * any_placed_p(string) 	(Level 3)
 * This returns TRUE if any module within the given string is PLACED
 *---------------------------------------------------------------
 */
int any_placed_p(string)
    mlist *string;
{
    mlist *ml;
    for(ml = string; ml != NULL; ml = ml->next)
    {
	if (ml->this->placed == PLACED) return(TRUE);
    }
    return(FALSE);
}

/*---------------------------------------------------------------
 * Stuff below here should probably be put in utility or another
 * helper file
 *---------------------------------------------------------------
 */


/*---------------------------------------------------------------
 * best_side_part (Level 3)
 * Given partition <part>, determin, based on the signal flow, to
 * which side of partition[0] it should be placed.
 * The new partition is placed on one of the four sides of the bounding 
 * box of the placed partitions (partition [0]).    ALSO,
 * as we need to walk through the net anyway, calculate the centers-
 * of-gravity for the two partitions (the xl,xb, xt, xr... values)
 *
 * --------------------------------------------------------------
 */
int best_side_part(part)
    int part;
{
    int top = 0, bot = 0, lef = 0, rig = 0, side, temp;
    int xl=0,yl=0,xll=0,yll=0,xr=0,yr=0,xrr=0,yrr=0;
    int xt=0,yt=0,xtt=0,ytt=0,xb=0,yb=0,xbb=0,ybb=0;
    int ooTemp, iiTemp, ioTempXX, ioTempX, xlSpot = 0, ioTempYY, oiTempX, oiTempY;
    int io=0, xio=0, yio=0, xioio=0, yioio=0;
    mlist *ml;
    tlist *t, *tt;

/* calculate the inputs/outputs between this partition and partition[0]: */
    for(ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	for(t = ml->this->terms; t != NULL; t = t->next)
	{
	    if (t->this->nt == NULL) break; 
	    for(tt = t->this->nt->terms; tt != NULL; tt = tt->next)
	    {
		/* Count how many terminals with nets into the set of
		 * placed modules we have, and also their postion.
		 *
		 * Also, get the ones from the placed modules
		 * to this one.
		 *
		 * For x determinations, use the box locations, as these limits
		 * determine the insertion of new tiles.
		 *
		 */

		if((tt->this->mod->flag == PLACED) &&   /* its down 
							   (see place_partition())*/
		   (t->this->mod != tt->this->mod)) 	/* don't bother with yourself */

		{  
		    /* Handle inout terminal combinations: */
		    if ((t->this->type == INOUT) || (tt->this->type == INOUT))
		    {
			if (t->this->type == IN)
			{ 
			    xr += t->this->mod->htile->x_pos;
			    yr += t->this->y_pos + t->this->mod->y_pos;
			    yrr += tt->this->mod->vtile->y_pos + 
			           tt->this->mod->vtile->y_len;
			    yrr += tt->this->x_pos + tt->this->mod->x_pos;
			    rig += 1;
			}
			else if (tt->this->type == OUT)
			{
			    xr += t->this->mod->htile->x_pos;
			    yr += t->this->y_pos + t->this->mod->y_pos;
			    xrr += tt->this->x_pos + tt->this->mod->x_pos;
			    yrr += tt->this->mod->vtile->y_pos + 
			           tt->this->mod->vtile->y_len;
			    rig += 1;
			}
			else if (t->this->type == OUT)
			{	
			    ioTempYY = tt->this->mod->vtile->y_pos +
				      tt->this->mod->vtile->y_len;
			    ioTempX = t->this->mod->htile->x_pos + 
			              t->this->mod->htile->x_len;
			    ioTempXX = tt->this->x_pos + tt->this->mod->x_pos;
			    if ((((ioTempXX <= xll) || (xll == 0)) ||
				 ((ioTempXX <= xll) && 
				  ((ioTempYY <= yll) || (yll == 0))))&&
				 ((ioTempX >= xlSpot) || (xlSpot == 0)))
			    {
				xlSpot = ioTempX;			    
				xl = t->this->mod->htile->x_pos + 
				     t->this->mod->htile->x_len;
				yl = t->this->y_pos + t->this->mod->y_pos;
				xll = tt->this->x_pos + tt->this->mod->x_pos;
				yll = tt->this->mod->vtile->y_pos +
				      tt->this->mod->vtile->y_len;
			    }
			    lef += 1;
			}
			else if  (tt->this->type == IN)
			{
			    ioTempYY = tt->this->y_pos + tt->this->mod->y_pos;
			    ioTempX = t->this->x_pos + t->this->mod->x_pos;
			    ioTempXX = tt->this->mod->vtile->x_pos;
			    if ((((ioTempXX <= xll) || (xll == 0)) ||
				 ((ioTempXX <= xll) && 
				  ((ioTempYY <= yll) || (yll == 0))))&&
				 ((ioTempX >= xlSpot) || (xlSpot == 0)))
			    {
				xlSpot = ioTempX;
				yl = t->this->mod->htile->y_pos + 
				     t->this->mod->htile->y_len;
				xl = t->this->x_pos + t->this->mod->x_pos;
				xll = tt->this->mod->vtile->x_pos;
				yll = tt->this->y_pos + tt->this->mod->y_pos;
			    }
			    lef += 1;
			}
			else /* INOUT vs INOUT */
			{
			    yio = t->this->mod->htile->y_pos + 
			         t->this->mod->htile->y_len;
			    xio = t->this->x_pos + t->this->mod->x_pos;
			    xioio = tt->this->x_pos + tt->this->mod->x_pos;
			    yioio = tt->this->mod->vtile->y_pos +
			          tt->this->mod->vtile->y_len;
			    io += 1;
			}
		    }

/* Normal terminal interactions: */
		    /* count in-out connections (from P[0] to <part>) */
		    if (((t->this->type == OUT) && (tt->this->type == IN)) ||
			((t->this->type == INOUT) && (tt->this->type == INOUT)))
		    {
			if (useAveraging == TRUE)
			{
			    /* xl += t->this->x_pos + t->this->mod->x_pos; */
			    xl += t->this->mod->htile->x_pos + 
			          t->this->mod->htile->x_len;
			    yl += t->this->y_pos + t->this->mod->y_pos;
			    /* xll += tt->this->x_pos + tt->this->mod->x_pos; */
			    xll += tt->this->mod->vtile->x_pos;
			    yll += tt->this->y_pos + tt->this->mod->y_pos;
			}
			else /* choose the lowest, furthest left to line up on */
			{
			    ioTempYY = tt->this->y_pos + tt->this->mod->y_pos;
			    ioTempX = t->this->mod->x_pos;
			    ioTempXX = tt->this->mod->vtile->x_pos;
			    if ((((ioTempXX <= xll) || (xll == 0)) ||
				 ((ioTempXX <= xll) && 
				  ((ioTempYY <= yll) || (yll == 0))))&&
				 ((ioTempX >= xlSpot) || (xlSpot == 0)))
			    {
				xlSpot = ioTempX;
				xl = t->this->mod->htile->x_pos + 
				     t->this->mod->htile->x_len;
				yl = t->this->y_pos + t->this->mod->y_pos;
				xll = tt->this->mod->vtile->x_pos;
				yll = tt->this->y_pos + tt->this->mod->y_pos;
			    }
			}
			lef += 1;
		    }
		    
		    /* count out-out connections */
		    if (((t->this->type == OUT) && (tt->this->type == OUT)) ||
			((t->this->type == OUTIN) && (tt->this->type == INOUT)))
		    {
			temp = tt->this->x_pos + tt->this->mod->x_pos;
			if (temp > xbb)
			{
			    xb = t->this->mod->htile->x_pos + 
			         t->this->mod->htile->x_len;
			    yb = t->this->y_pos + t->this->mod->y_pos;
			    /* xbb = tt->this->x_pos + tt->this->mod->x_pos; */
			    xbb += tt->this->mod->vtile->x_pos + 
			           tt->this->mod->vtile->x_len;
			    ybb = tt->this->y_pos + tt->this->mod->y_pos;
			}
			bot += 1;
		    }
		    
		    /* count out-in connections */
		    if (((t->this->type == IN) && (tt->this->type == OUT)) ||
			((t->this->type == OUTIN) && (tt->this->type == OUTIN)))
		    {
			if (1)
			{
			    xr += t->this->mod->htile->x_pos;
			    yr += t->this->y_pos + t->this->mod->y_pos;
			    xrr += tt->this->mod->vtile->x_pos + 
			           tt->this->mod->vtile->x_len;
			    yrr += tt->this->y_pos + tt->this->mod->y_pos;
			}
			else /* choose the highest to line up on */
			{
			    oiTempY = tt->this->y_pos + tt->this->mod->y_pos;
			    oiTempX = tt->this->mod->vtile->x_pos + 
				      tt->this->mod->vtile->x_len;
			    if (oiTempX > xrr)
			    {
				xr = t->this->mod->htile->x_pos;
				yr = t->this->y_pos + t->this->mod->y_pos;
				xrr = tt->this->mod->vtile->x_pos + 
				       tt->this->mod->vtile->x_len;
				yrr = tt->this->y_pos + tt->this->mod->y_pos;
			    }
			    else if (oiTempY > yrr)
			        yrr = tt->this->y_pos + tt->this->mod->y_pos;
			}
			rig += 1;
		    }
		    
		    /* count in-in connections */
		    if (((t->this->type == IN) && (tt->this->type == IN)) ||
			((t->this->type == INOUT) && (tt->this->type == OUTIN)))
		    {
			iiTemp = tt->this->y_pos + tt->this->mod->y_pos;
			if (ytt > iiTemp)
			{
			    xt = t->this->mod->htile->x_pos;
			    yt = t->this->y_pos + t->this->mod->y_pos;
			    xtt = tt->this->mod->vtile->x_pos;
			    ytt = tt->this->y_pos + tt->this->mod->y_pos;
			}
			top += 1;
		    }
		}
	    }
	}
    }
    if (top + bot + lef + rig == 0)
    {
	/* use inout information */ 
	lef = io;
	xl = xio; 	yl = yio;
	xll = xioio;	yll = yioio;
    }

/* now figure out which side to put the partition on: */
/* settle any conflicts, otherwise key off of the max value: */
    side = choose_side(top, bot, lef, rig);
    switch(side) 				   /* ALSO: Handle the tie breakers: */
    {
	case UP    : { if (top == lef) 
		       {   set_gravity_part(part, xl, yl, xll, yll, 
					    (useAveraging == TRUE) ? lef : 1);
			   side = LEFT;
			   xSideCount = -1 * lef;	/* Overwrite old <xSideCount> */
			   ySideCount = 0;
			   break;
		       }
		       else if (top == rig) 
		       {   set_gravity_part(part, xr, yr, xrr, yrr, 
					    (1) ? rig : 1);
			   side = RIGHT;
			   xSideCount = rig;		/* Overwrite old <xSideCount> */
			   ySideCount = 0;
			   break;
		       }
		       else if ((lef > 0) && (rig > 0))
		       { set_gravity_part(part, xl+xr+xt, yl+yr+yt, xll+xrr+xtt, 
					  yll+yrr+ytt, (useAveraging == TRUE) ? 
					  top+lef+rig : 1+rig+1);
			 break;
		       }
	               else 
		       {   set_gravity_part(part, xt, yt, xtt, ytt, 1);
			   break;
		       }
		     }
	
	case DOWN  : { if (bot == rig)
		       { 
			   set_gravity_part(part, xr, yr, xrr, yrr,
					    (1) ? rig : 1);
			   side = RIGHT;
			   xSideCount = rig;		/* Overwrite old <xSideCount> */
			   ySideCount = 0;
			   break;
		       }
		       else if (bot == lef) 
		       {   set_gravity_part(part, xl, yl, xll, yll, 
					    (useAveraging == TRUE) ? lef : 1);
			   side = LEFT;
			   xSideCount = -1 * lef;
			   ySideCount = 0;
			   break;
		       }
	               else
		       { set_gravity_part(part, xb, yb, xbb, ybb, 1);
			 break;
		       }
		     }
	case LEFT  : { if (lef == rig) 
		       { set_gravity_part(part, xl+xr, yl+yr, xll+xrr, yll+yrr, 
					    (useAveraging == TRUE) ? lef+rig : rig + 1);
			 side = UP;
			 xSideCount = 0;		/* Overwrite old <xSideCount> */
			 ySideCount = 1;
			 break;
		       }
	               else 		      /* No need to change global side counts */
		       { set_gravity_part(part, xl, yl, xll, yll,
					  (useAveraging == TRUE) ? lef : 1);
		         break;
		       }
		     }
	
	case RIGHT : { set_gravity_part(part, xr, yr, xrr, yrr,
					(1) ? rig : 1);
		       break;
		     }
    }
    return (side);
}
	       
/*---------------------------------------------------------------
 * choose_side
 * rules for angle selection (placement of boxes & partitions as tiles)
 *---------------------------------------------------------------
 */
int choose_side(common_ins, common_outs, in_to, out_of)
    int common_ins, common_outs, in_to, out_of;
    
{
    /* This sets the proper signs for use as the side globals <secSideCount>; */
    int max, next_max, mainSide;
    
    if (MAX(in_to, common_ins) >= MAX(out_of, common_outs))
    {
	/* inputs (up/left) dominate: */
	if (in_to > common_ins)
	{
	    mainSide = LEFT;	    max = -1 * in_to;
	    next_max = (common_outs > common_ins) ? -1 * common_outs : common_ins;
	}
	else
	{
	    mainSide = UP;	    max = common_ins;	next_max =  -1 * in_to;
	    /* next_max = (out_of > in_to) ? out_of : -1 * in_to; */
	}
    }
    else
    /* outputs (left/bottom) dominate */
    {
	if (out_of > common_outs)
	{
	    mainSide = RIGHT;		max = out_of;
	    next_max = (common_outs > common_ins) ? -1 * common_outs : common_ins;
	}
	else 
	{
	    mainSide = DOWN;	max = -1 * common_outs;		next_max = -1 * in_to;
	    /* next_max = (out_of > in_to) ? out_of : -1 * in_to;	*/
	}
    }
    
    /* Set the globals <xSideCount> & ySideCount>, etc: */
    xSideCount = ((mainSide == LEFT) || (mainSide == RIGHT)) ? max : next_max;
    ySideCount = ((mainSide == UP) || (mainSide == DOWN)) ? max : next_max;

    return(mainSide);
}

/*---------------------------------------------------------------
 * set_gravity_part()
 *---------------------------------------------------------------
 */
set_gravity_part(p,xb,yb,xbb,ybb,count)
    int p,xb,yb,xbb,ybb,count;
{
    if(count > 0)
    {
	xgr = xb/(float)count;
	ygr = yb/(float)count;
	xgbr = xbb/(float)count;
	ygbr = ybb/(float)count;
    }
    else
    {	/* stack them, as we don't know what else to do */
	xgr  = (float)x_sizes[p]/2.0;
	ygr  = (float)(y_sizes[p] + yfloor);
	xgbr = (float)x_sizes[0]/2.0;
	ygbr = 0.0;
	error("set_gravity_part: partition has no connections to P[0]?"); 
    }
}

/*---------------------------------------------------------------
 * set_gravity_box()
 *---------------------------------------------------------------
 */
set_gravity_box(p,xb,yb,xbb,ybb,count)
    int p,xb,yb,xbb,ybb,count;
{	/* Set the globals used to place two boxes */
    if(count > 0)
    {
	xgr = xb/(float)count;
	ygr = yb/(float)count;
	xgbr = xbb/(float)count;
	ygbr = ybb/(float)count;
    }
    else
    {	/* stack these two boxes, as we don't know what else to do */

	xgr  = (float)(str_length[p]) / 2.0;
	ygr  = (float)(str_height[p]) / 2.0;
	xgbr = (float)(x_sizes[p] - xfloor) / 2.0;
	ygbr = (float)(y_sizes[p] - yfloor) / 2.0;
	error("set_gravity_box: partition has no connections to P[0]?",""); 
    }
}
/*--------------------------------------------------------------------------------
 * Count connections (between a box and a partition)
 *---------------------------------------------------------------------------------
 */
int count_connections(box, part)
    mlist *box;
    int part;
{
    tlist *tl, *ttl;
    mlist *ml;
    int lef = 0, rig = 0, top = 0, bot = 0;

/* calculate the inputs/outputs between this box and the placed modules in 
   partition[part]: */
    for(ml = box; ml != NULL; ml = ml->next)
    {
	for(tl = ml->this->terms; tl != NULL; tl = tl->next)
	{
 	    if (tl->this->nt == NULL) break;
	    for(ttl = tl->this->nt->terms; ttl != NULL; ttl = ttl->next)
	    {
		/* Count how many terminals with nets into the set of
		 * placed modules we have, and also their postion.
		 *
		 * And while we are at it, get the ones from the placed modules
		 * to this one.
		 */

		if((ttl->this->mod->placed == PLACED) && 	/* in partitions[part] */
		   (module_info[tl->this->mod->index].used == part) &&
		   (module_info[ttl->this->mod->index].used == part) &&
		   (tl->this->mod != ttl->this->mod) &&  /* don't bother with yourself */
		   (find_item(ttl->this->mod->flag, box) == NULL))    /* not in the box */
		{  
		    /* count in-out connections (from P[0] to <part>) */
		    if (((tl->this->type == OUT) || (tl->this->type == INOUT)) &&
			((ttl->this->type == IN) || (ttl->this->type == INOUT)))
		    {
			lef += 1;
		    }
		    
		    /* count out-out connections */
		    if (((tl->this->type == OUT) || (tl->this->type == INOUT)) &&
			((ttl->this->type == OUT) || (ttl->this->type == INOUT)))
		    {
			bot += 1;
		    }
		    
		    /* count out-in connections */
		    if (((tl->this->type == IN) || (tl->this->type == INOUT)) &&
			((ttl->this->type == OUT) || (ttl->this->type == INOUT)))
		    {
			rig += 1;
		    }
		    
		    /* count in-in connections */
		    if (((tl->this->type == IN) || (tl->this->type == INOUT)) &&
			((ttl->this->type == IN) || (ttl->this->type == INOUT)))
		    {
			top += 1;
		    }
		}
	    }
	}
    }
    return(top + bot + lef + rig);
}
/*---------------------------------------------------------------
 * best_side_box (Level 3)
 * Given partition <part> & some subset of those modules <box>, decide
 * which side of the placed partitions in <partitions[]> the given box
 * should be placed. 
 * ALSO:  we need to walk through the net anyway, calculate the centers-
 * of-gravity for the two partitions (the xl,xb, xt, xr... values)
 *
 * --------------------------------------------------------------
 */
int best_side_box(part, box)
    int part;
    mlist * box;
    
{
    int top = 0, bot = 0, lef = 0, rig = 0, max, side;
    int xl=0,yl=0,xll=0,yll=0,xr=0,yr=0,xrr=0,yrr=0;
    int xt=0,yt=0,xtt=0,ytt=0,xb=0,yb=0,xbb=0,ybb=0;
    int ooTemp, iiTemp, ioTempX, ioTempY, oiTempX, oiTempY;
    mlist *ml;
    tlist *t, *tt;

/* calculate the inputs/outputs between this box and the placed modules in 
   partition[part]:  */
    for(ml = box; ml != NULL; ml = ml->next)
    {
	for(t = ml->this->terms; t != NULL; t = t->next)
	{
	    if (t->this->nt == NULL) break;
	    for(tt = t->this->nt->terms; tt != NULL; tt = tt->next)
	    {
		/* Count how many terminals with nets into the set of
		 * placed modules we have, and also their postion.
		 *
		 * And while we are at it, get the ones from the placed modules
		 * to this one.
		 */

		if((tt->this->mod->placed == PLACED) && 	/* in partitions[part] */
		   (module_info[t->this->mod->index].used == part) &&
		   (module_info[tt->this->mod->index].used == part) &&
		   (t->this->mod != tt->this->mod) && 	 /* don't bother with yourself */
		   (find_item(tt->this->mod->flag, box) == NULL)) /* not in the box */
		{  
		    /* count in-out connections (ouput of box to partitions[part]) */
/* old		    if (((t->this->type == OUT) || (t->this->type == INOUT)) &&
			((tt->this->type == IN) || (tt->this->type == INOUT)))  */

		    if (((t->this->type == OUT) && (tt->this->type == IN)) ||
			((t->this->type == INOUT) && (tt->this->type == INOUT)))
		    {
			if (useAveraging == TRUE)
			{
			    xl += t->this->x_pos + t->this->mod->x_pos;
			    yl += t->this->y_pos + t->this->mod->y_pos;
			    xll += tt->this->x_pos + tt->this->mod->x_pos;
			    yll += tt->this->y_pos + tt->this->mod->y_pos;
			}
			else /* choose the lowest to line up on */
			{
			    ioTempY = tt->this->y_pos + tt->this->mod->y_pos;
			    ioTempX = tt->this->x_pos + tt->this->mod->x_pos;
			    if ((ioTempX < xll) || (xll == 0))
			    {
				xl = t->this->x_pos + t->this->mod->x_pos;
				yl = t->this->y_pos + t->this->mod->y_pos;
				xll = tt->this->x_pos + tt->this->mod->x_pos;	
				yll = tt->this->y_pos + tt->this->mod->y_pos;
			    }
			    else if (ioTempY < yll) 
			        yll = tt->this->y_pos + tt->this->mod->y_pos;
			}
			lef += 1;
		    }
		    
		    /* count out-out connections */
/* old		    if (((t->this->type == OUT) || (t->this->type == INOUT)) &&
			((tt->this->type == OUT) || (tt->this->type == INOUT))) */
		    if (((t->this->type == OUT) && (tt->this->type == OUT)) ||
			((t->this->type == OUTIN) && (tt->this->type == INOUT)))
		    {
			ooTemp = tt->this->x_pos + tt->this->mod->x_pos;
			if (ooTemp > xbb)
			{
			    xb = t->this->x_pos + t->this->mod->x_pos;
			    yb = t->this->y_pos + t->this->mod->y_pos;
			    xbb = tt->this->x_pos + tt->this->mod->x_pos;
			    ybb = tt->this->y_pos + tt->this->mod->y_pos;
			}
			bot += 1;
		    }
		    
		    /* count out-in connections */
/* old		    if (((t->this->type == IN) || (t->this->type == INOUT)) &&
			((tt->this->type == OUT) || (tt->this->type == INOUT))) */
		    if (((t->this->type == IN) && (tt->this->type == OUT)) ||
			((t->this->type == OUTIN) && (tt->this->type == OUTIN)))
		    {
			if (useAveraging == TRUE)
			{
			    xr += t->this->x_pos + t->this->mod->x_pos;
			    yr += t->this->y_pos + t->this->mod->y_pos;
			    xrr += tt->this->x_pos + tt->this->mod->x_pos;
			    yrr += tt->this->y_pos + tt->this->mod->y_pos;
			}
			else /* choose the highest to line up on */
			{
			    oiTempY = tt->this->y_pos + tt->this->mod->y_pos;
			    oiTempX = tt->this->x_pos + tt->this->mod->x_pos;
			    if (oiTempX > xrr)
			    {
				xr = t->this->x_pos + t->this->mod->x_pos;
				yr = t->this->y_pos + t->this->mod->y_pos;
				xrr = tt->this->x_pos + tt->this->mod->x_pos;
				yrr = tt->this->y_pos + tt->this->mod->y_pos;
			    }
			    else if (oiTempY > yrr)
			        yrr = tt->this->y_pos + tt->this->mod->y_pos;
			}
			rig += 1;
		    }
		    
		    /* count in-in connections */
/* old		    if (((t->this->type == IN) || (t->this->type == INOUT)) &&
			((tt->this->type == IN) || (tt->this->type == INOUT))) */
		    if (((t->this->type == IN) && (tt->this->type == IN)) ||
			((t->this->type == INOUT) && (tt->this->type == OUTIN)))
		    {
			/* choose the most forward terminal: */
			iiTemp = tt->this->x_pos + tt->this->mod->x_pos;
			if (xtt < iiTemp)
			{
			    xt = t->this->x_pos + t->this->mod->x_pos;
			    yt = t->this->y_pos + t->this->mod->y_pos;
			    xtt = tt->this->x_pos + tt->this->mod->x_pos;
			    ytt = tt->this->y_pos + tt->this->mod->y_pos;
			}
			top += 1;
		    }
		}
	    }
	}
    }
/* now figure out which side to put the partition on: */
/* settle any conflicts, otherwise key off of the max value: */
    side = choose_side(top, bot, lef, rig);
    switch(side)
    {
	case UP    : {
	               if (top == lef) 
		       {   set_gravity_box(part, xl, yl, xll, yll, 
					   (useAveraging == TRUE) ? lef : 1);
			   side = LEFT;
			   xSideCount = -1 * lef;	/* Overwrite old <xSideCount> */
			   ySideCount = top;
			   break;
		       }
	               else if (top == rig) 
		       {   
			   set_gravity_box(part, xr, yr, xrr, yrr, 
				       (useAveraging == TRUE) ? rig : 1);
			   side = RIGHT;
			   xSideCount = rig;	/* Overwrite old <xSideCount> */
			   ySideCount = (rig != 1) ? top: 0;
			   break;
		       }
	               else 		      /* No need to change global side counts */
		       {   set_gravity_box(part, xt, yt, xtt, ytt, 1);
			   break;
		       }
		     }
	
	case DOWN  : { 
	               if (bot == lef) 
		       {   set_gravity_box(part, xl, yl, xll, yll,
					   (useAveraging == TRUE) ? lef : 1);
			   side = LEFT;
			   xSideCount = lef;
			   ySideCount = -1 * bot;
			   break;
		       }
	               else if (bot == rig) 
		       {   
			   set_gravity_box(part, xr, yr, xrr, yrr, 
				       (useAveraging == TRUE) ? rig : 1);
			   side = RIGHT;
			   xSideCount = rig;	/* Overwrite old <xSideCount> */
			   ySideCount = (rig != 1) ? -1 * bot : 0;
			   break;
		       }
	               else		       /* No need to change global side counts */
		       {	
			   set_gravity_box(part, xb, yb, xbb, ybb, 1);
			   break;
		       }
		     }
	case LEFT : { if (lef == rig) 
		       { if (useAveraging == TRUE)
			     set_gravity_box(part, xl+xr, yl+yr, xll+xrr, yll+yrr, lef+rig);
			 else set_gravity_box(part, xl+xr, yl+yr, xll+xrr, yll+yrr, 2);
			 side = UP;
			 xSideCount = -1 * lef;		/* Overwrite old <xSideCount> */
			 ySideCount = 0;
			 break;
		       }
	               else 		       /* No need to change global side counts */
		       { set_gravity_box(part, xl, yl, xll, yll, 
					 (useAveraging == TRUE) ? lef : 1);
		         break;
		       }
		     }
	
	case RIGHT : { 			       /* No need to change global side counts */
	               set_gravity_box(part, xr, yr, xrr, yrr, 
				       (useAveraging == TRUE) ? rig : 1);
		       break;
		     }
    }
    return (side);
}


/*---------------------------------------------------------------
 * fixup_string (Level 2)
 * Go back through the string and move everyone up, and over by the amount
 * necessary to put the lowest one on the 0 line. 
 * and the leftmost one on the 0 line. adjust the size too.
 *---------------------------------------------------------------
 */
fixup_string(part, xflr, yflr)
    int part, xflr, yflr;
{
    mlist *ml;

    /* fixup the overal size */
    str_length[part] += -(xflr);
    str_height[part] += -(yflr);
    xfloor = 0;
    yfloor = 0;

    for(ml = strings[part]; ml != NULL; ml = ml->next)
    {
	ml->this->x_pos += -(xflr);
	ml->this->y_pos += -(yflr);
    }
}
/*---------------------------------------------------------------
 * move_string (Level 3)
 * Go back through the string and move everyone up, and over by the amount
 * necessary to put the lowest one on the 0 line. 
 *---------------------------------------------------------------
 */
move_string(part, xflr, yflr)
    int part, xflr, yflr;
{
    mlist *ml;

    for(ml = strings[part]; ml != NULL; ml = ml->next)
    {
	ml->this->x_pos += xflr;
	ml->this->y_pos += yflr;
    }
}
/*---------------------------------------------------------------
 * reset_borders (Level 2)
 * Go back through the partition corners, and readjust them.
 *---------------------------------------------------------------
 */
reset_borders(xflr, yflr)
    int xflr, yflr;
{
    int p;
    for (p = 0; p <= partition_count; p++)
    {
	/* fixup the overal size */
	x_sizes[p] -= xflr;
	y_sizes[p] -= yflr; 
	x_positions[p] -= xflr;
	y_positions[p] -= yflr;
    }
    x_sizes[0] += xflr;
    y_sizes[0] += yflr;
    
}
/*---------------------------------------------------------------
 * fixup_partition (Level 2)
 * Go back through the string and move everyone up, and over by the amount
 * necessary to put the lowest one on the 0 line. 
 * and the leftmost one on the 0 line. adjust the size too.
 *---------------------------------------------------------------
 */
fixup_partition(part, xflr, yflr)
    int part, xflr, yflr;
{
    mlist *ml = NULL;
    tilist *tl = NULL;

    /* fixup the overal size */
    x_sizes[part] -= xflr;
    y_sizes[part] -= yflr; 
    
    xfloor = 0;
    yfloor = 0;

    /* Reset each module individually: */
    for(ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	if (ml->this->placed == PLACED)
	{
	    ml->this->x_pos -= xflr;
	    ml->this->y_pos -= yflr;
	}
    }
    /* Reset all the asociated tiles: */
    tl = allspace(rootTiles[part], &tl);
    tl = translate_origin(tl, -1*xflr, -1*yflr);
}

/*---------------------------------------------------------------
 * line_up_mod (Level 3)
 * This calculates the y postion of a module in a string.
 * Line up the current module with the previous
 * module's output (<prevt>). If the previous module is turned away
 * from us, we must allow for bends and jogs.
 *---------------------------------------------------------------
 */
int line_up_mod(m, prevt)
    module *m;
    term *prevt;
{
    int y = 0;
    tlist *tl;
    term *curr;
    
    if(prevt == NULL)
    {
	error("line_up_mod: previous primary terminal NULL","m->name");
    }

    /* find the terminal in <m> to line up on: */
    for (tl = m->terms; tl != NULL; tl  = tl->next)
    {
	if (tl->this->nt == prevt->nt)
	{
	    curr = tl->this;
	    break;
	}
    }
	
    switch (prevt->side)
    {
	case RIGHT:
	    y = prevt->y_pos + prevt->mod->y_pos - curr->y_pos;
	    break;
	case UP:
	    y = prevt->y_pos + prevt->mod->y_pos - curr->y_pos +
	        WHITE_SPACE;
	    break;
	case DOWN:
	    y = prevt->mod->y_pos - WHITE_SPACE - curr->y_pos;
	    break;
	case LEFT:
	    if(prevt->mod->y_size > (2 * prevt->y_pos))
	    {   /* go around the bottom */
		y = prevt->mod->y_pos - WHITE_SPACE -
		    curr->y_pos;
	    }
	    else
	    {   /* go around the top */
		y = prevt->mod->y_pos + prevt->mod->y_size +
		    WHITE_SPACE - curr->y_pos;
	    }
	    break;
	default:
	{
	    error("line_up_modules: bad side for previous terminal","");
	    break;
	}
    }
    return(y);
}

/*---------------------------------------------------------------
 * rotate_module (Level 3)
 * perform the rotation of the module and terminal positions and sides
 *---------------------------------------------------------------
 */

rotate_module(m, rot)
    module *m;
    int rot;
{
    int xs = m->x_size;
    int ys = m->y_size;
    tlist *t;
        
    /* Mark the rotation done */
    m->rot = rot;

    /* update the terminal postions and their sides */
    for (t = m->terms; t != NULL; t = t->next)
    {
	rot_x_y(rot, xs, ys, t->this);
	t->this->side = rot_side(rot, t->this->side);
    }
    
    /* Also update the module's x y sizes (for non-square modules) */
    if ((rot == NINETY) || (rot == TWO_SEVENTY))
    {
	m->x_size = ys;
	m->y_size = xs;
    }
}
/*---------------------------------------------------------------
 * trace_path (Level 4)
 *
 * This is a recursive function that performs a depth-first search of
 * the modules in the partition that are unplaced, and follow the
 * standard output-input (left-right) convention.  Modules are marked to
 * avoid cycle problems.  From the lists generated by traversing the
 * child nodes, the longest list seen so far is returned to the parent,
 * with the module visited added to the front.  
 *
 *---------------------------------------------------------------
 */
mlist *trace_path(mod, p, depth, lenWithCC)
    module *mod;
    int p, depth;
    int *lenWithCC;
{
    tlist *t, *tt;
    module *nextMod;
    mlist *testPath = NULL, *pathSoFar = NULL;
    int testLength = 0, lengthSoFar = 0;

    depth++;
   
    if(debug_level > 20)
    {
	fprintf(stderr,
		"Trace_path: Module: %s, Partition: %d, Depth: %d, flag: %d\n",
		mod->name, p, depth, mod->flag);
    }
    
    if ((depth <= MAX_DEPTH) && (mod->flag == UNSEEN))
    {
	if (mod->placed == UNPLACED) 		/* only consider unplaced modules */
	{
	    mod->flag = SEEN;			/* don't visit the same node twice */
	    
	    for (t = mod->terms; t !=NULL; t = t->next)
	    {
		/* only go from OUT/INOUTs to IN/INOUTs */
		if((t->this->type == OUT)||(t->this->type == INOUT))
		{
		    for(tt = t->this->nt->terms; tt != NULL; tt = tt->next)
		    {		    
			nextMod = tt->this->mod;

			/* Check each input term on the net */
			if((nextMod->index != mod->index) /* not looking at head */
			   &&				        /* am in partition */
			   (module_info[nextMod->index].used == p) 
			   &&					/* Only follow inputs */
			   ((tt->this->type == IN)||(tt->this->type == INOUT)) 
			   &&				/* not connected to systerm IN */
			   (strcmp(nextMod->type, "IN"))	
			   &&			     	/* not conn'd to systerm INOUT */
			   (strcmp(nextMod->type, "INOUT"))	
			   &&				/* not conn'd to a systerm OUT */
			   (strcmp(nextMod->type, "OUT"))
			   &&
			   (nextMod->flag != SEEN))
			{
			    if (debug_level > 18)/* debug */
			    {
				fprintf(stderr,
					"\t%s, finds: %s through net: %s\n",
					mod->name,
					nextMod->name,
					t->this->nt->name);
			    }
			    /* mark these connections (INOUTs are inelligible) */
			    if (t->this->type == OUT) mod->primary_out = t->this;
			    if (tt->this->type == IN) nextMod->primary_in = tt->this;
			    /* recursive call, adding <nextMod> & all progeny: */
			    testPath = trace_path(nextMod, p, depth, &testLength); 

			    /* Check to see if this is a cc module */
			    if((cc_search_level > 0) &&
				    (cross_coupled_p(mod, nextMod, 1) == TRUE))
			    {
				pathSoFar = testPath; 
				lengthSoFar += testLength;
			    }
			    /* if the recursive call found anything new, save the info.*/
			    else if (testLength > lengthSoFar)
			    {
				pathSoFar = testPath; 
				lengthSoFar = testLength;
			    }
			    else /* Clean up markers set while making <testPath> */
			    {				
				reset_path_trace(testPath);
			    }
			}
		    }
		}
	    }
	    *lenWithCC = lengthSoFar + 1;
	    return ((mlist *)concat_list(mod, pathSoFar));/* OK-> add this mod to the list */
	}
    }
    
    /* we hit the max length or we hit a node we have already seen */
    *lenWithCC = lengthSoFar;
    return(NULL);
}

/*---------------------------------------------------------------
 * init_string_arrays (Level 2)
 * initialize the boxes, strings, sizes, and positions arrays
 * Note: we do one more (number 0) since we do not use partition 0
 * (until the very end) and we want to number them 1 to pn.
 *---------------------------------------------------------------
 */
int init_string_arrays(p)
    int p;
{
    int i;
    
    /* allocate space for the partitions[] matrix, which is built as the partition 
       assignments are made: */
    partitions = (mlist **)calloc((p+3), sizeof(mlist *));
    rootTiles = (tile **)calloc((p+3), sizeof(tile *));

    strings = (mlist **)calloc((p+1), sizeof(mlist *));
    parti_stats = (ctree **)calloc((p+1), sizeof(ctree *));
    x_sizes = (int *)calloc((p+1), sizeof(int));
    y_sizes = (int *)calloc((p+1), sizeof(int));
    x_positions = (int *)calloc((p+1), sizeof(int));
    y_positions = (int *)calloc((p+1), sizeof(int));
    str_height = (int *)calloc((p+1), sizeof(int));
    str_length = (int *)calloc((p+1), sizeof(int));

    for(i = 0; i <= p; i++)
    {
	rootTiles[i] = create_tile(TILE_SPACE_X_LEN, TILE_SPACE_Y_LEN, NULL, 
				   TILE_SPACE_X_POS, TILE_SPACE_Y_POS);
	partitions[i] = NULL;
	parti_stats[i] = NULL;
	strings[i] = NULL;
	x_sizes[i] = 0;
	y_sizes[i] = 0;
	x_positions[i] = 0;
	y_positions[i] = 0;
	str_height[i] = 0;
	str_length[i] = 0;	
    }
}

/*---------------------------------------------------------------
 * is_root (Level 3)
 * discover if the module should be used as the root of 
 * a list of modules (string) which make up a box. The rules in the
 * tech-report don't make a lot of sense.
 * " a module is allowed to be a root if it has a connection with
 *   another module in another partion, or if it is connected with
 *   an in or inout terminal, or if it has just one outgoing net."
 * The rule we are using is:
 * if it has an in or inout terminal to a module in another partition, 
 * or if it has an in or inout terminal to a system input net."
 *---------------------------------------------------------------
 */
int is_root(m, j)
    module *m;	/* the module */
    int j;	/* its index */
{
    int root = 0;
    tlist *t, *tt;
    int out_count = 0;

#if 0
/* I do not understand this rule: 
   "if the module has any connections
    outside this partition then it is a root" 
*/
    module_info[j].in = 0;
    module_info[j].out = 0;
    count_inout(j, module_info[j].used);
    if (module_info[j].out)
    {
	root = 1;
	return(root);
    }
#endif

    /* Check to see if it is a system input terminal module */
    if ((!strcmp(m->type, "IN")) || (!strcmp(m->type, "INOUT")))
    {
	root = 1;
	return (root);
    }
	

    /* check all the terminals for this module */
    for (t = m->terms; t != NULL; t = t->next) 
    {
	/* check to see if its an input */
	if ((t->this->type == IN)||(t->this->type == INOUT))	
	{	    
	    /* check to see if its a system input */		
	    if ((t->this->nt->type == IN) || (t->this->nt->type == INOUT))
	    {
		root = 1;
		return(root);
	    }
	    /* see if it connects to a module in another partition */
	    for(tt = t->this->nt->terms; tt != NULL; tt = tt->next)
	    {
		if (module_info[tt->this->mod->index].used != module_info[j].used)
		{
		    root=1;
		    return(root);
		}
	    }
	}
#if 0
	else if (t->this->type = OUT)
	{
	    out_count++;
	}
#endif

    }

#if 0	    
    /* I do not understand this rule either: "just one output" */
    if(out_count == 1)
    {
     	root = 1;
	return(root);
    }
#endif
    
    /* default, its not a root */
    return(root);
}


/*---------------------------------------------------------------
 * build_connections_amongst_partitions (Level 2)
 * run through all the nets and see which modules are connected to
 * which others, and by how many nets. This structure is key to 
 * the partitioning functions. We are hopfully trading space for speed.
 * ie, once this monster is built it can be used quickly.
 * time = |N| * |B|^2
 * NOTE: it might be faster to run from each module, down its
 * termlist, and then down their netlists. 
 * time = |M| * |F| * |B|.
 * M = number of modules
 * N = number of nets
 * B = average branching factor per net
 * F = average fanout + fanin per module
 *---------------------------------------------------------------
 */
build_connections_amongst_partitions(p, how)
    int p,how;
    
{
    nlist *nl;
    tlist *t, *tt;
    module *m1, *m2;
    int i, j, p1, p2;

    /* Zero the relavent portion of the connection matrix (incl. systerm cols/rows) */
    for (i = 0; i <= p + 2; i++)
    {
	for (j = 0; j <= p + 2; j++)
	{
	    connected[i][j] = 0;
	}
    }
    for (nl = nets; nl != NULL; nl = nl->next)
    {
	for (t = nl->this->terms; t != NULL; t = t->next)
	{
	    m1 = t->this->mod;
	    if (((m1->placed != UNPLACED) ||	           /* consider placed modules */
		 (module_info[m1->index].used == p+1) ||   /* and system terminals */
		 (module_info[m1->index].used == p+2)) &&
		((how == UNSIGNED)||
		 ((how == FORWARD) && (t->this->type == OUT) || 
		  (t->this->type == INOUT)) ||
		 ((how == REVERSE) && (t->this->type == IN) || 
		  (t->this->type == INOUT))))
	    for(tt = nl->this->terms; tt != NULL; tt = tt->next)
	    {
		m2 = tt->this->mod;
		if (((m2->placed != UNPLACED) ||            /* consider placed modules */
		     (module_info[m2->index].used == p+1) ||/* and system terminals */
		     (module_info[m2->index].used == p+2)) &&
		    ((how == UNSIGNED)||
		     ((how == REVERSE) && (tt->this->type == OUT) || 
		      (tt->this->type == INOUT)) ||
		     ((how == FORWARD) && (tt->this->type == IN) || 
		      (tt->this->type == INOUT))))
		{
		    p1 = module_info[m1->index].used;
		    p2 = module_info[m2->index].used;
		    if (p1 != p2) connected[p1][p2]++;
		}
	    }
	}
    }

/* now overlay this portion of the array, so that the clustering math works out: */
    overlay_connections(p + 3, how);
}

/*---------------------------------------------------------------
 * build_connections (Level 2)
 * This maps the type of connections being used (POS, UNSIGNED, NEG)
 * onto the function to call.
 *--------------------------------------------------------------
 */
build_connections(how)
    int how;
{
    switch(how)
    {
	case FORWARD: { forward_build_connections();
                        break;
		      }
	case REVERSE: { reverse_build_connections();
			break;
		      }
	case UNSIGNED: { unsigned_build_connections();
			 break;
		       }
	default: 
	{
	    error("build_connections: bad call (%d)");
	    break;
	}
    }
}

/*---------------------------------------------------------------
 * unsigned_build_connections (Level 3)
 * run through all the nets and see which modules are connected to
 * which others, and by how many nets. This structure is key to 
 * the partitioning functions. We are hopfully trading space for speed.
 * ie, once this monster is built it can be used quickly.
 * time = |N| * |B|^2
 * NOTE: it might be faster to run from each module, down its
 * termlist, and then down their netlists. 
 * time = |M| * |F| * |B|.
 * M = number of modules
 * N = number of nets
 * B = average branching factor per net
 * F = average fanout + fanin per module
 *---------------------------------------------------------------
 */
unsigned_build_connections()
{
    nlist *n;
    tlist *t, *tt;
    int i, j;

    for (i = 0; i < MAX_MOD; i++)
    {
	for (j = 0; j < MAX_MOD; j++)
	{
	    connected[i][j] = 0;
	}
    }
    for (n = nets; n != NULL; n = n->next)
    {
	for (t = n->this->terms; t != NULL; t = t->next)
	{
	    for(tt = n->this->terms; tt != NULL; tt = tt->next)
	    {
		connected[t->this->mod->index][tt->this->mod->index]++;
	    }
	}
    }
}
/*------------------------------------------------------------------------
 * Overlay_connections (Level 3)
 * This resets the diagonal entries of the useful columns in connected[][].
 * The value inserted is the sum of the column entries.
 *------------------------------------------------------------------------
 */
int overlay_connections(cl_count, how)
    int cl_count, how;
{	
    int i, j, sum=0;
    for (i=0; i < cl_count; i++)
    {
	sum = 0;
	
/* Now reset the connection matrix for this set of clusters, summing the column, and 
 * inserting the value on the diagonal:						      */

	for (j=0; j < cl_count; j++)
	{
	    if (how == FORWARD) sum += connected[j][i]; 	/* sum the column */
	    else sum += connected[i][j];			/* sum the row */
	}
	connected[i][i] = sum - connected[i][i];
    }
}

/*---------------------------------------------------------------
 * forward_build_connections (Level 2)
 * run through all the nets and see which modules are connected to
 * which others, and by how many nets.   This beast is built such that the
 * row item 'feeds' all of the positive entries in the row.  In this way, 
 * one-level (forward) signal flow is mapped.
 * 
 * This structure is key to the partitioning functions. We are hopfully 
 * trading space for speed, as this thing is slow to build, but is manipulated
 * fairly easily.
 *
 * time = |N| * |B|^2
 * NOTE: it might be faster to run from each module, down its
 * termlist, and then down their netlists. 
 * time = |M| * |F| * |B|.
 * M = number of modules
 * N = number of nets
 * B = average branching factor per net
 * F = average fanout + fanin per module
 *---------------------------------------------------------------
 */
forward_build_connections()
{
    mlist *m;
    
    net *n;
    tlist *t, *tt;
    int i, j;

    for (i = 0; i < MAX_MOD; i++)
    {
	for (j = 0; j < MAX_MOD; j++)
	{
	    connected[i][j] = 0;
	}
    }
    for (m = modules; m != NULL; m = m->next)
    {
	for (t = m->this->terms; t != NULL; t = t->next)
	{
	    n = t->this->nt;
	    if ((t->this->type == OUT) || (t->this->type == INOUT))
	    {
		for(tt = n->terms; tt != NULL; tt = tt->next)
		{
		    if ((tt->this->type == IN) || (tt->this->type == INOUT))
		        connected[t->this->mod->index][tt->this->mod->index]++;
		}
	    }
	}
    }
}
/*---------------------------------------------------------------
 * reverse_build_connections (Level 2)
 * run through all the nets and see which modules are connected to
 * which others, and by how many nets.   This beast is built such that the
 * row item 'is fed by' all of the positive entries in the row.  In this way, 
 * one-level (reverse) signal flow is mapped.
 * 
 * This structure is key to the partitioning functions. We are hopfully 
 * trading space for speed, as this thing is slow to build, but is manipulated
 * fairly easily.
 *
 * time = |N| * |B|^2
 * NOTE: it might be faster to run from each module, down its
 * termlist, and then down their netlists. 
 * time = |M| * |F| * |B|.
 * M = number of modules
 * N = number of nets
 * B = average branching factor per net
 * F = average fanout + fanin per module
 *---------------------------------------------------------------
 */
reverse_build_connections()
{
    mlist *m;
    
    net *n;
    tlist *t, *tt;
    int i, j;

    for (i = 0; i < MAX_MOD; i++)
    {
	for (j = 0; j < MAX_MOD; j++)
	{
	    connected[i][j] = 0;
	}
    }
    for (m = modules; m != NULL; m = m->next)
    {
	for (t = m->this->terms; t != NULL; t = t->next)
	{
	    n = t->this->nt;
	    if ((t->this->type == IN) || (t->this->type == INOUT))
	    {
		for(tt = n->terms; tt != NULL; tt = tt->next)
		{
		    if ((tt->this->type == OUT) || (tt->this->type == INOUT))
		        connected[t->this->mod->index][tt->this->mod->index]++;
		}
	    }
	}
    }
}
/*---------------------------------------------------------------
 *  reset_connected_matrix 	(Level 5)
 * overwrite the indicated row and column <i> with the information stored
 * in the given integer vectors (<col>, <row>).
 *---------------------------------------------------------------
 */
reset_connected_matrix(merge, scratch, cCount, actives, col, row, how)
    int merge;		/* <merge> is the row/column being replaced. */
    int scratch;	/* <scratch> is the (org) row/column replacing <merge> */
    int cCount;         /* Number of rows/columns to mess with */
    clist **actives;	/* Up-to-date ordered list of active clusters */
    int *col, *row;	/* These are arrays containing the data to replace with  */
    int how;
{
    int x,y, sum;
    clist *cl = *actives;
    for (x = 0; x < cCount-1; x++)
    {
	if (x < merge)
	{
	    connected[merge][x] = row[x];
	    connected[x][merge] = col[x];
	}
	else if (x == merge)
	{
	    connected[merge][x] = row[scratch];
	    connected[x][merge] = col[scratch];
	}
	else if ((x > merge) && ((x < scratch) || (merge == scratch)))
	{
	    connected[merge][x] = row[x+1];   
	    connected[x][merge] = col[x+1];
	}
	else if (x >= scratch)
	{
	    connected[merge][x] = row[x+1];
	    connected[x][merge] = col[x+1];
	}
	/* <row> & <col> are one element longer than a row/col in the connection mtrx */
    }
    
    /* Restore everyone's diagonal values: */
    for (x = 0; x < cCount-1; x++)
    {
	connected[x][x] = 0;
	sum = 0;
	for (y = 0; y < cCount-1; y++)
	{
	    if (x != y)	 /* ignore the current diagonal value */
	    {
		if (how == FORWARD) sum += connected[y][x]; 	/* sum the column */
		else sum += connected[x][y];			/* sum the row */
	    }
	}
	connected[x][x] = sum;
	cl->this->this->connects = sum;   /* Update the information in the cluster 
					     structure as well, otherwise the 
					     partitioning functions won't work right. */
	cl = cl->next;
    }
}
/*---------------------------------------------------------------
 * find_max_connections_to_P0	(Level 4)
 * Return the col/row number that has the highest number of connections
 * to partition 0 (the set of placed partitions)
 * NOTE: this ignores column 0 of the connection matrix.
 *---------------------------------------------------------------
 */
int find_max_connections_to_P0(range)
    int range;
{
    int i, partition_0_connections = 0, max = 0, index = 0;
    
    for (i = 1; i <= range; i++)
    {
	if ((connected[i][0] > 0) || (connected[0][i] > 0))
	{
	    partition_0_connections = MAX(connected[0][i], connected[i][0]);
	
	    if ((partition_0_connections > max) ||
		((partition_0_connections == max) && 
		 (connected[i][i] >= connected[index][index])))
	    {
		max = partition_0_connections;
		index = i;
	    }
	}
    }
    return(index);
}
/*---------------------------------------------------------------
 * find_max_diagonal	(Level 4)
 * Retrurn the col/row number that has the highest number of connections
 * to partition 0 (the set of placed partitions)
 * NOTE: this ignores column 0 of the connection matrix.
 *---------------------------------------------------------------
 */
int find_max_diagonal(range)
    int range;
{
    int i, max = 0, index = 0;
    
    for (i = 1; i <= range; i++)
    {
	if (connected[i][i] > max)
	{
	    max = connected[i][i];
	    index = i;
	}
    }
    return(index);
}

/*---------------------------------------------------------------
 * collect_info (Level 3)
 * count how modules are connected in/out of the 
 * current partition, or with pn=0 in/out of the free set
 * 
 *---------------------------------------------------------------
 */
collect_info(pn)
    int pn;
{
    int i;
    
    for(i = 0; i < module_count; i++)
    {
	if(module_info[i].used == 0)
	{
	    count_inout(i, pn);
	}
    }
}

/*---------------------------------------------------------------
 * count_inout (Level 4)
 * count how modules are connected in/out of this partition, to this one
 * NOTE: WE ONLY GIVE A COUNT OF 1 FOR ANY NON-ZERO AMOUNT.
 *---------------------------------------------------------------
 */
count_inout(m, pn)
int m, pn;
{
    int j;
    
    for(j = 0; j < module_count; j++)
    {
	if (m != j)
	{
	    if (connected[m][j] > 0)
	    {
		(module_info[j].used != pn ) ?
		(module_info[m].out)++ :
		(module_info[m].in)++;
	    }
	}
    }
}

/*---------------------------------------------------------------
 * reset_info (Level 3)
 * initialize temporary information about the partiions of modules
 *---------------------------------------------------------------
 */
reset_info()
{
    int i;
    
    for(i = 0; i < module_count; i++)
    {
	module_info[i].in = 0;
	module_info[i].out = 0;
    }
    
}
/*---------------------------------------------------------------
 * init_info (Level 2)
 * initialize accumulated information about the partitions of modules
 *---------------------------------------------------------------
 */
init_info()
{
    int i;
    
    for(i = 0; i < module_count; i++)
    {
	module_info[i].used = 0;
    }
    reset_info();
}
/*-------------------------------------------------------------------------------------*/
void print_module_placement(part, f)
    int part;				/* The partition to see... */
    FILE *f;				/* The file to dump it to (Better be open!) */
{
    /* This prints a file that can produce a PostScript form of a tile space containing 
       modules and empty tiles. */

    char labelBuffer[30];
    mlist *ml;
    int i, index;

    /* now start the dump file: */
    if (latex != TRUE) 
    {
	ps_print_standard_header(f);
	fprintf(f, "\n(%s: Module Placement for ??) %d %d %d %d init\n",
                getenv("USER"), xfloor, yfloor, x_sizes[0], y_sizes[0]);
    }
    else 
    {
	ps_print_standard_header(f);
	fprintf(f, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
	fprintf(f,"%%%%BoundingBox: %d %d %d %d\n",
		xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
		x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);
    }

    /* Insert all of the tiles into their own tile space */
    for (ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	ps_print_mod(f, ml->this);
    }

    /* Now print the bounderies that surround the various partitions: */
    if ((do_routing == FALSE) && (part == 0))
    {
	for (i=1; i <= partition_count; i++)
	{
	    if (partitions[i] != NULL)
	    {
		index = partitions[i]->this->index;
		ps_print_border(f,x_positions[i],y_positions[i],x_sizes[i],y_sizes[i]);
		sprintf(labelBuffer, "[part: #%d, placed:%d]",
			module_info[index].used, module_info[index].order);
		ps_put_label(f, labelBuffer, (float)(x_positions[i] + 1), 
			     (float)(y_sizes[i] - 2));
	    }
	}
    }
    else if (partitions[part] != NULL)
    {
	index = partitions[part]->this->index;
	ps_print_border(f, x_positions[part], y_positions[part],
			x_sizes[part], y_sizes[part]);
	sprintf(labelBuffer, "[part: #%d, placed:%d]\0",
		module_info[index].used, module_info[index].order);
	ps_put_label(f, labelBuffer, x_positions[part] + 1, y_sizes[part] - 2);
    }

    if (latex != TRUE) fprintf(f, "showpage\n");
}

/*---------------------------------------------------------------
 * END OF FILE
 *---------------------------------------------------------------
 */

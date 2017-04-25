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


/* file debug.c */
/* ------------------------------------------------------------------------
 * debugging routines for place and route                 spl 7/89
 *
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "externs.h"

/*---------------------------------------------------------------
 * Forward References
 *---------------------------------------------------------------
 */

void print_tlist();

/*---------------------------------------------------------------
 * print the full list of all modules
 *---------------------------------------------------------------
 */

void print_modules(f)
    FILE *f;
{
    mlist *m;
    for (m = modules; m != NULL ; m = m->next)
    {
	fprintf(f, "\nModule index: %d name: %s,\ttype: %s, \tflag: %d\n",
		m->this->index, m->this->name, m->this->type, m->this->flag);
	fprintf(f, "\tpostition: %d %d,\tsize: %d %d,\trotation: %d\n",
		m->this->x_pos, m->this->y_pos,
		m->this->x_size, m->this->y_size, m->this->rot);
	fprintf(f, "    Terminal List:\n");
	print_tlist(f,m->this->terms);
    }
}

/*
 *---------------------------------------------------------------
 * print the full list of all nets
 *---------------------------------------------------------------
 */

void print_nets(f)
    FILE *f;
{
    nlist *n;
    for (n = nets; n != NULL ; n = n->next)
    {
	fprintf(f, "\nNet name: %s Type: %d \n", n->this->name, n->this->type);
	fprintf(f, "    Terminal List:\n");
	print_tlist(f,n->this->terms);
    }
}

/*
 *---------------------------------------------------------------
 * print a terminal list
 *---------------------------------------------------------------
 */
void print_tlist(f,t)
    FILE *f;
    tlist *t;
{
    int i = 0;
    for(; t != NULL; t = t->next)
    {
	fprintf(f, "\t%s:%s: %d,%d type:%d", t->this->mod->name, t->this->name,
		t->this->x_pos, t->this->y_pos, t->this->type);
	if(++i >= 4)
	{
	    fprintf(f, "\n");
	    i = 0;
	}
    }
}
/*
 *---------------------------------------------------------------
 * print the list of "external/system" terminals
 *---------------------------------------------------------------
 */

void print_ext(f)
    FILE *f;
{
    tlist *t;
    int i = 0;
    
    fprintf(f, "\nExternal Ports:\n");

    for(t = ext_terms; t != NULL; t = t->next)
    {
	fprintf(f, "\t%s:", t->this->name);
	switch (t->this->type)
	{
	    case IN:	fprintf(f, "global_input"); break;
	    
	    case OUT:	fprintf(f, "global_output"); break;
	    	    
	    case INOUT:	fprintf(f, "global_inout"); break;

	    default:	fprintf(f, "bad type"); break;
	}
	
	if(++i >= 3)
	{
	    fprintf(f, "\n");
	    i = 0;
	}
    }
}

/*
 *---------------------------------------------------------------
 * print_connections (stf 9-89)
 *---------------------------------------------------------------
 */
void print_connections(f)
    FILE *f;
{
    int i, j;
    
    fprintf(f, "\nConnections:\n");
    
    for (i = 0; i < module_count; i++)
    {
	for (j = 0; j < module_count; j++)
	{
	    fprintf(f, "i,j:%d,%d:%d\t", i, j, connected[i][j]);
	    if ((j+1)%5 == 0) fprintf(f, "\n");
	}
	fprintf(f,"\n");
    }
}

/*
 *---------------------------------------------------------------
 * p_cons  (stf 9-89)
 *---------------------------------------------------------------
 */
void p_cons(f,q, limbs)
    FILE *f;
    int q;
    clist *limbs;
{
    int i, j;
    
    fprintf(f, "\nConnections:(%d)\n", q);
    
    for (i = 0; i < q; i++)
    {
	fprintf(f,"%2d | ", i);
	
	for (j = 0; j < q; j++)
	{
	    fprintf(f, "%2d ", connected[i][j]);
	    if ((j+1)%50 == 0) fprintf(f, "\n");
	}
	fprintf(f,"  | ");
	
	if (find_indexed_element(i, limbs) != NULL)
	    print_ctree(f, ( find_indexed_element(i, limbs) )->this);
	else
	    fprintf(f, "?");
	
	fprintf(f,"\n");
    }
}

/*
 *--------------------------------------------------------------
 * det_in_out_relationship(m1, m2)
 *--------------------------------------------------------------
 */
void det_in_out_relationship(m1, m2, in_x, out_x)
    module *m1, *m2;
    int *in_x, *out_x;
{
    tlist *tl, *tll;

    /* Check out the connection between <m1> and <m2> */
    for (tl = m1->terms; tl != NULL; tl = tl->next)
    {
	if (tl->this->nt == NULL) /* Don't Care */
	{
	    *out_x = 0;
	    *in_x = 0;
	    break;
	}

	for (tll = tl->this->nt->terms; tll != NULL; tll = tll->next)
	{
	    if (tll->this->mod == m2)
	    {
		if (((tl->this->type == IN) && (tll->this->type != IN)) ||
		    ((tl->this->type == INOUT) && (tll->this->type == OUT)))
		{
		    /* <m2> drives <m1> */
		    *in_x = m1->x_pos + tl->this->x_pos;	
		    *out_x = m2->x_pos + tll->this->x_pos;
		}    
		else if (((tll->this->type == IN) && (tl->this->type != IN)) ||
			 ((tll->this->type == INOUT) && (tl->this->type == OUT)))
		{
		    /* <m1> drives <m2> */
		    *out_x = m1->x_pos + tl->this->x_pos;
		    *in_x = m2->x_pos + tll->this->x_pos;	
		}  
		else /* Don't care: */
		{
		    *out_x = 0;
		    *in_x = 0;
		}
	    }
	}
    }
}
/*
 *--------------------------------------------------------------
 * float manhattan_PWS (stf 5-91)
 *--------------------------------------------------------------
 */
float manhattan_PWS()
{
    /* this function returns the manhattan (x + y) distances among the modules of 
       the design.  This number can be used to measure the quality of the
       placement, and is accomplished by doing a pairwise multiplication of the
       inverse distance with the connectivity for all of the modules in the 
       design. */

    /* Modified to only include those connections that are not xfouled */

    int rIndex, cIndex, match_module_index();
    int in_x, out_x, xNoise, yNoise;
    float connectivity, manhat_dist[MAX_MOD][MAX_MOD];
    float x, y, inv_manhat_dist[MAX_MOD][MAX_MOD], manhat_sum = 0.0;
    mlist *ml, *mll;

    /* Rebuild the connected[][] matrix for reference: */
    build_connections(matrix_type);
    overlay_connections(module_count, matrix_type);

    /* Build the two distance matricies */
    for (ml = modules; ml != NULL; ml = ml->next)
    {
	rIndex = ml->this->index;
	for (mll = ml->next; mll != NULL; mll = mll->next)
	{
	    cIndex = mll->this->index;
	    mll->this->placed = UNPLACED;	    /* Avoids screwups in the cc code */

	    if (connected[rIndex][cIndex] != 0) 
	    {
		/* Skip cross-coupled gates: */
		if (cross_coupled_p(ml->this, mll->this, cc_search_level) != FALSE)
		{		
		    break;
		}
		else
		{
		    /* Noise Reduction */
		    xNoise = ml->this->x_pos + ml->this->x_size/2 - 
		             mll->this->x_pos - mll->this->x_size/2;
		    
		    yNoise = ml->this->y_pos + ml->this->y_size/2 - 
		             mll->this->y_pos - mll->this->y_size/2;
		    x = (float)abs(xNoise);	y = (float)abs(yNoise);
		    in_x = out_x = 0;
		
		    manhat_dist[rIndex][cIndex] = x + y;
		    if (x + y > 0.0)
		    {
			inv_manhat_dist[rIndex][cIndex] =1.0/manhat_dist[rIndex][cIndex];
			
			/* Calculate the pairwise sums: Penalize for bad, non-cc 
			   (xfoul) connections: */
			det_in_out_relationship(ml->this, mll->this, &in_x, &out_x);
			if (in_x < out_x)
			{
			    connectivity = (float)connected[rIndex][cIndex];
			    manhat_sum -= inv_manhat_dist[rIndex][cIndex] * connectivity;
			}
			else
			{
			    connectivity = (float)connected[rIndex][cIndex];
			    manhat_sum += inv_manhat_dist[rIndex][cIndex] * connectivity;
			}
		    }
		}
	    }
	}
    }

    return(manhat_sum);
}
/*
 *--------------------------------------------------------------
 * print_distance_stats (stf 5-91)
 *--------------------------------------------------------------
 */
void print_distance_stats(mfile)
    FILE *mfile;	/* Where to write the info */
{
    /* this function prints the manhattan (x + y) and euclidian (sqrt(x^2 + y^2))
       distances among the modules of the design to a file "dist_stats"
       along with a measure of how this correlates with the connectivity
       of the design.  This number can be used to measure the quality of the
       placement, and is accomplished by doing a pairwise multiplication of the
       inverse distance with the connectivity for all of the modules in the 
       design. */

    /* Modified to only include those connections that are not xfouled */

    int rIndex, cIndex, match_module_index();
    int in_x, out_x, xNoise, yNoise;
    float connectivity, manhat_dist[MAX_MOD][MAX_MOD], euclid_dist[MAX_MOD][MAX_MOD];
    float x, inv_manhat_dist[MAX_MOD][MAX_MOD], manhat_sum = 0.0;
    float y, inv_euclid_dist[MAX_MOD][MAX_MOD], euclid_sum = 0.0;
    mlist *ml, *mll;

    /* Rebuild the connected[][] matrix for reference: */
    build_connections(matrix_type);
    overlay_connections(module_count, matrix_type);

    /* Build the two distance matricies */
    for (ml = modules; ml != NULL; ml = ml->next)
    {
	rIndex = ml->this->index;
	for (mll = ml->next; mll != NULL; mll = mll->next)
	{
	    cIndex = mll->this->index;
	    mll->this->placed = UNPLACED;	    /* Avoids screwups in the cc code */

	    if (connected[rIndex][cIndex] != 0) 
	    {
		/* Skip cross-coupled gates: */
		if (cross_coupled_p(ml->this, mll->this, cc_search_level) != FALSE)
		{		
		    break;
		}
		else
		{
		    /* Noise Reduction */
		    xNoise = ml->this->x_pos + ml->this->x_size/2 - 
		             mll->this->x_pos - mll->this->x_size/2;
		    
		    yNoise = ml->this->y_pos + ml->this->y_size/2 - 
		             mll->this->y_pos - mll->this->y_size/2;
		    x = (float)abs(xNoise);	y = (float)abs(yNoise);
		    in_x = out_x = 0;
		
		    manhat_dist[rIndex][cIndex] = x + y;
		    euclid_dist[rIndex][cIndex] = (float)sqrt((double)((x * x)+(y * y)));
		    if (x + y > 0.0)
		    {
			inv_manhat_dist[rIndex][cIndex] =1.0/manhat_dist[rIndex][cIndex];
			inv_euclid_dist[rIndex][cIndex] =1.0/euclid_dist[rIndex][cIndex];
			
			/* Calculate the pairwise sums: Penalize for bad, non-cc 
			   (xfoul) connections: */
			det_in_out_relationship(ml->this, mll->this, &in_x, &out_x);
			if (in_x < out_x)
			{
			    connectivity = (float)connected[rIndex][cIndex];
			    manhat_sum -= inv_manhat_dist[rIndex][cIndex] * connectivity;
			    euclid_sum -= inv_euclid_dist[rIndex][cIndex] * connectivity;
			}
			else
			{
			    connectivity = (float)connected[rIndex][cIndex];
			    manhat_sum += inv_manhat_dist[rIndex][cIndex] * connectivity;
			    euclid_sum += inv_euclid_dist[rIndex][cIndex] * connectivity;
			}
		    }
		}
	    }
	}
    }

    /* Now dump the two sums to the stats file: */
    fprintf(mfile,"\t%-8.7f \t%-8.7f ",
	    manhat_sum, euclid_sum);
}
/*
 *--------------------------------------------------------------
 * print_distance_matricies (stf 5-91)
 *--------------------------------------------------------------
 */
void print_distance_matricies()
{
    /* this function prints the manhattan (x + y) and euclidian (sqrt(x^2 + y^2))
       distances among the modules of the design to a file "dist_stats"
       along with a measure of how this correlates with the connectivity
       of the design.  This number can be used to measure the quality of the
       placement, and is accomplished by doing a pairwise multiplication of the
       inverse distance with the connectivity for all of the modules in the 
       design. */

    FILE *mfile = fopen("dist_stats","a");
    int rIndex, cIndex, match_module_index();
    int in_x, out_x, xNoise, yNoise;
    float connectivity, manhat_dist[MAX_MOD][MAX_MOD], euclid_dist[MAX_MOD][MAX_MOD];
    float x, inv_manhat_dist[MAX_MOD][MAX_MOD], manhat_sum = 0.0;
    float y, inv_euclid_dist[MAX_MOD][MAX_MOD], euclid_sum = 0.0;
    mlist *ml, *mll;

    /* Rebuild the connected[][] matrix for reference: */
    build_connections(matrix_type);
    overlay_connections(module_count, matrix_type);

    /* Build the two distance matricies */
    for (ml = modules; ml != NULL; ml = ml->next)
    {
	rIndex = ml->this->index;
	for (mll = ml->next; mll != NULL; mll = mll->next)
	{
	    cIndex = mll->this->index;
	    mll->this->placed = UNPLACED;	    /* Avoids screwups in the cc code */

	    if (connected[rIndex][cIndex] != 0) 
	    {
		/* Skip cross-coupled gates: */
		if (cross_coupled_p(ml->this, mll->this, cc_search_level) != FALSE)
		{		
		    break;
		}
		else
		{
		    /* Noise Reduction */
		    xNoise = ml->this->x_pos + ml->this->x_size/2 - 
		             mll->this->x_pos - mll->this->x_size/2;
		    
		    yNoise = ml->this->y_pos + ml->this->y_size/2 - 
		             mll->this->y_pos - mll->this->y_size/2;
		    x = (float)abs(xNoise);	y = (float)abs(yNoise);
		    in_x = out_x = 0;
		
		    manhat_dist[rIndex][cIndex] = x + y;
		    euclid_dist[rIndex][cIndex] = (float)sqrt((double)((x * x)+(y * y)));
		    if (x + y > 0.0)
		    {
			inv_manhat_dist[rIndex][cIndex] =1.0/manhat_dist[rIndex][cIndex];
			inv_euclid_dist[rIndex][cIndex] =1.0/euclid_dist[rIndex][cIndex];
			
			/* Calculate the pairwise sums: Penalize for bad, non-cc 
			   (xfoul) connections: */
			det_in_out_relationship(ml->this, mll->this, &in_x, &out_x);
			if (in_x < out_x)
			{
			    connectivity = (float)connected[rIndex][cIndex];
			    manhat_sum -= inv_manhat_dist[rIndex][cIndex] * connectivity;
			    euclid_sum -= inv_euclid_dist[rIndex][cIndex] * connectivity;
			}
			else
			{
			    connectivity = (float)connected[rIndex][cIndex];
			    manhat_sum += inv_manhat_dist[rIndex][cIndex] * connectivity;
			    euclid_sum += inv_euclid_dist[rIndex][cIndex] * connectivity;
			}
		    }
		}
	    }
	}
    }
    /* Now dump all of this to the stats file: */
    fprintf(mfile,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fprintf(mfile,"| -p%d -s%d -c%d -r%-4.2f -m%d %s %s %s\t ||",
	    partition_rule, max_partition_size, max_partition_conn,
	    partition_ratio, matrix_type, (stopAtFirstCut == TRUE) ? "-f" : "--", 
	    (useSlivering == FALSE) ? "-v" : "--", (useAveraging == TRUE) ? "-w" : "--");

    fprintf(mfile, "The Connection Matrix looks like: \n");
    for(rIndex = 0; rIndex < module_count; rIndex++)
    {
	for(cIndex = 0; cIndex < module_count; cIndex++)
	{
	    fprintf(mfile, "%6d ", connected[rIndex][cIndex]);
	}
	ml = (mlist*)member(rIndex, modules, match_module_index);
	fprintf(mfile," | %s\n", ml->this->name);
    }

    fprintf(mfile, "\n\nThe Manhattan Distance Matrix looks like: \n");
    for(rIndex = 0; rIndex < module_count; rIndex++)
    {
	for(cIndex = 0; cIndex < module_count; cIndex++)
	{
	    fprintf(mfile, "%-6.1f ", manhat_dist[rIndex][cIndex]);
	}
	ml = (mlist*)member(rIndex, modules, match_module_index);
	fprintf(mfile," | %s\n", ml->this->name);
    }

/*    fprintf(mfile, "\n\nThe Euclidian Distance Matrix looks like: \n");
    for(rIndex = 0; rIndex < module_count; rIndex++)
    {
	for(cIndex = 0; cIndex < module_count; cIndex++)
	{
	    fprintf(mfile, "%-6.3f ", euclid_dist[rIndex][cIndex]);
	}
	ml = (mlist*)member(rIndex, modules, match_module_index);
	fprintf(mfile," | %s\n", ml->this->name);
    }
*/  
    fprintf(mfile,"\n\nThe pairwise sum for inv. Manhattan vs. Connectivity is %-8.7f\n",
	    manhat_sum);
    fprintf(mfile,"\n\nThe pairwise sum for inv. Euclidian vs. Connectivity is %-8.7f\n",
	    euclid_sum);
    fclose(mfile);
}

int match_module_index(i, m)
    int i;
    module *m;
{
    /* Return TRUE if the module->index field matches the index <i> given: */
    if (m->index == i) return (TRUE);
    else return(FALSE);
}

/*
 *--------------------------------------------------------------
 * print_ctree (stf 9-89)
 *--------------------------------------------------------------
 */
int print_ctree(f,t)
	FILE *f;
	ctree *t;
{
    clist *queue = NULL;
    ctree *temp;
    int this, next, breadth = 0;
    
    if (t == NULL) return 0;

/* Load the queue and begin: */
    indexed_list_insert(t,0,&queue);
    this = 1;   next = 0;       breadth = 1;

    fprintf(f,"{");

/* Yank the first person off of the list, and add any children to the back */
    do
    {
        temp = (ctree *)remove_indexed_element(0, &queue);      /* dequeue the next item */
        if (temp != NULL)
        {
	    if (temp->right!=NULL)              /* enque the right child */
            {
                indexed_list_insert(temp->right,ilist_length(queue),&queue);
                next += 1;
            }
            if (temp->left!=NULL)               /* enque the left child */
            {
                indexed_list_insert(temp->left,ilist_length(queue),&queue);
                next += 1;
            }
            /* Now do something with the element: */
	    
	    if (temp->this->contents != NULL)
		fprintf(f,"[c=%d] %s, ", temp->this->connects, temp->this->contents->name);
	
	     else
	        fprintf(f,"[s=%d c=%d r=%-5.3f]", temp->this->size, temp->this->connects,
			(float)temp->this->connects/(float)temp->this->size);
	    
            if (--this == 0)                      /* end of a layer */
            {
                breadth = MAX(breadth, next);
                this = next;    next = 0;
		fprintf(f,": ");
		
            }
        }
/* go on to the next thing in the queue */  
    } while (ilist_length(queue)!= 0);

    fprintf(f,"}");
    return (breadth);
}



/*
 *---------------------------------------------------------------
 * print_info
 *---------------------------------------------------------------
 */
void print_info(f)
    FILE *f;
{
    int i;
    fprintf(f, "\nInfo:\n");
    
    for(i = 0; i< module_count; i++)
    {
	fprintf(f, "module:%d\t used:%d\t in:%d\t out:%d\n",
		i, module_info[i].used, module_info[i].in, module_info[i].out);
    }
}
/*
 *---------------------------------------------------------------
 * print_boxes
 *---------------------------------------------------------------
 */
void print_boxes(f)
    FILE *f;
{
    mlist *ml;
    int i;
    
    fprintf(f, "\nBoxes:\n");
    
    for(i = 0; i<= partition_count; i++)
    {
	fprintf(f, "Boxes for partition %d:\n", i);

	for (ml = boxes[i]; ml != NULL; ml = ml->next)
	{
	    fprintf(f, "\t\tModule: %s\n", ml->this->name);
	}
    }
}

/*
 *---------------------------------------------------------------
 * print_strings
 *---------------------------------------------------------------
 */
void print_strings(f)
    FILE *f;
{
    mlist *ml;
    int i;
    
    fprintf(f, "\nStrings:\n");
    
    for(i = 0; i<= partition_count; i++)
    {
	fprintf(f, "string for partition %d: total size: %d %d\n",
		i, x_sizes[i], y_sizes[i]);

	for (ml = strings[i]; ml != NULL; ml = ml->next)
	{
	    fprintf(f, "\t\tModule: %s, pos: %d %d, in: %s, out: %s\n",
		    ml->this->name,
		    ml->this->x_pos,
		    ml->this->y_pos,
		    (ml->this->primary_in) ?
		    ml->this->primary_in->nt->name : "", 
		    (ml->this->primary_out) ?
		    ml->this->primary_out->nt->name : "");
	    
	}
    }
}


/*---------------------------------------------------------------
 * print_range
 *---------------------------------------------------------------
 */
print_range(s, rl)
    char *s;
    rlist *rl;
{
    fprintf(stderr, "Range list: %s\n", s);
    
    for (; rl != NULL; rl = rl->next)
    {
	fprintf(stderr, "\t x-low:%d\t y-high:%d\t crosses:%d\n",
		rl->this->x, rl->this->y, rl->this->c);
    }
}

/*
 *---------------------------------------------------------------
 * print_result
 *---------------------------------------------------------------
 */
#define SIDE_CONVERT(n) ((n==0)?0:(n==1)?90:(n==2)?180:(n==3)?270:0)
#define SIDE_X(n)  ((n==0)?-4:(n==1)?0:(n==2)?0:(n==3)?0:0)
#define SIDE_Y(n)  ((n==0)?0:(n==1)?2:(n==2)?0:(n==3)?-2:0)

void print_result(f)
    FILE *f;
{
    mlist *ml;
    tlist *t;
    int i, index, c;

    llist *head, *hhead;
    

    /* for date and time */
    long time_loc;
    extern long int time();
    
    /* copy the Post Script header file to the output file */
    ps_print_standard_header(f);

    /* Define the bounding box for the post-script code: */
    fprintf(f,"%%%%BoundingBox: %d %d %d %d\n",
	    xfloor - CHANNEL_LENGTH - 1,
	    yfloor - CHANNEL_HEIGHT - 1,
	    x_sizes[0] + CHANNEL_LENGTH + 1, 
	    y_sizes[0] + CHANNEL_HEIGHT + 1);
 

    /* now dump the result */
    time_loc = time(0L);

    if (latex != TRUE) /* Not in latex mode */
    {
	fprintf(f,
		"\n(N2A %s %s input:? s:%d c:%d r:%-5.3f rule #%d part:%d dir:%d) %d %d %d %d init\n",
		getenv("USER"), ctime(&time_loc),
		max_partition_size, max_partition_conn, partition_ratio, partition_rule,
		partition_count, matrix_type,
		xfloor, yfloor, x_sizes[0], y_sizes[0]);
    }
    else /* This stuff replaces the init for latex mode */
    {
	fprintf(f, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
    }

    
    for (ml = partitions[0]; ml != NULL; ml = ml->next)
    {
	ps_print_mod(f, ml->this);
    }

    if (do_routing == FALSE)
    for (i=1; i<=partition_count; i++)
    {
	if (partitions[i] != NULL)
	{
	    index = partitions[i]->this->index;
	    
	    ps_print_border(f,x_positions[i], y_positions[i], x_sizes[i], y_sizes[i]);
	    fprintf(f,"%d %d ([part: #%d, placed:%d]) label\n", 
		    x_positions[i] + 1, y_sizes[i] - 2,
		    module_info[index].used, module_info[index].order);
	}
    }

    fprintf(f, "done\n");
}

/*
 *---------------------------------------------------------------
 * ps_box(x_len, y_len, x_org, y_org, rot)
 * --------------------------------------------------------------
 */
int ps_box(x_len, y_len, x_org, y_org, rot)
    int x_len, y_len, x_org, y_org, rot;
/* this function produces the relative code for a box of size (x_len, y_len)
 * starting at position (x_org, yorg)
 */
{
}

/* --------------------------------------------------------------
 * draft_statistics
 * This function provides statistics on how well the draft was 
 * done:  Statistics are # modules, # nets, # terminals, #cc @ level <l>...
 * # cc @ level 1, # foul-ups.
 * A signal flow foulup is defined as any non-cc module who's child is not
 * in an x-position greater-than its parent.
 * Other useful info is the filename, date, and all of the (specified?) 
 * command-line args.  All information is printed to a file "draft_stats"
 *---------------------------------------------------------------
 */
void draft_statistics()
{
    int cc_count[10];
    int xfouls = 0, yfouls = 0, term_count = 0, lev;
    int out_x, in_x, out_y, in_y, possibly_screwed, also_unscrewed;
    
    mlist *ml;
    module *m, *last_m, *cc_m, *top_cc_mod, *bot_cc_mod;
    tlist *t, *tt;

    FILE *f = fopen("draft_stats", "a"); 
    
    for (lev=0; lev < cc_search_level; lev++) cc_count[lev] = 0;
      
    last_m = modules->this;
    last_m->placed = UNPLACED;	/* for cross_coupled_p to work properly */
     
    term_count = list_length(last_m->terms);
    
    
    for(ml = modules->next; ml != NULL; ml = ml->next)
    {
	m = cc_m = ml->this;
	term_count += list_length(m->terms);
	
	/* Set up all the strangeness for cross-coupled modules: */
	for (lev = 1; (lev <= cc_search_level) && (cc_m != NULL); lev++)
	{
	    cc_m->placed = UNPLACED;
	    
	    if (cross_coupled_p(last_m, cc_m, lev) != FALSE)
	    {
		top_cc_mod = last_m;
		bot_cc_mod = cc_m;
		cc_count[lev-1] += 1;
		break;
	    }
	    cc_m = (ml->next != NULL) ? ml->next->this : NULL;
	}
	
	for (t = last_m->terms; t != NULL; t = t->next)
	{
	    if ((t->this->type == OUT) || (t->this->type == INOUT)) 
	    {
		possibly_screwed = FALSE;
		also_unscrewed = FALSE;
		
		for (tt = t->this->nt->terms; tt != NULL; tt = tt->next)
		{
		    if ((tt->this->type == IN) || (tt->this->type == INOUT)) 
		    {
			out_x = t->this->mod->x_pos + t->this->x_pos;
			in_x = tt->this->mod->x_pos + tt->this->x_pos;
			out_y = t->this->mod->y_pos + t->this->y_pos;
			in_y = tt->this->mod->y_pos + tt->this->y_pos;
			
			if (((t->this->mod == top_cc_mod)&&(tt->this->mod == bot_cc_mod)) ||
			    ((t->this->mod == bot_cc_mod)&&(tt->this->mod == top_cc_mod)))
			{  /* Skip cc modules */
			    also_unscrewed = TRUE;
			}
			
			else if (out_x > in_x) 
			{ xfouls += 1; }
			
			else if (out_y < in_y) 
			{ possibly_screwed = TRUE; }

			else 
			{ also_unscrewed = TRUE; }
			
		    }
		}
		if ((possibly_screwed == TRUE) && (also_unscrewed == FALSE)) yfouls += 1;
	    }
	}
	/* for next iteration: */
	last_m = m;
	last_m->placed = UNPLACED;
	
    }

    /* Done gathering statistics! */
    terminal_count = term_count;	/* Set the global so others can use it */

    fprintf(f,"------------------------------------------------------------------------------\n");
    
    fprintf(f,"? -p%d -s%d -c%d -r%-4.2f | %d\t ||",
	    partition_rule, max_partition_size, max_partition_conn,
	    partition_ratio, term_count);

    fprintf(f, "\t %d \t %d \t %d \t %dx%d = %d", 
	    xfouls, yfouls, xfouls+yfouls, x_sizes[0], y_sizes[0], x_sizes[0] * y_sizes[0]);
    print_distance_stats(f);
    fprintf(f," |\n");
    fclose(f);
    
}

/*--------------------------------------------------------------
 * stats_table_header (incomplete)
 *---------------------------------------------------------------
 */
void stats_table_header(f)
    FILE *f;
{		
    int lev, cc_count[10], term_count;
    
    fprintf(f, "Modules: %d, Nets: %d, Terminals: %d, Partitions: %d\n",
	    module_count, node_count, term_count, partition_count);

    fprintf(f, "CC Stats: ");
    
    for(lev = 1; lev <= cc_search_level; lev++)
    {
	fprintf(f, "lev%d: %d  ", lev, cc_count[lev-1]);
    }
    
}

/*
 *---------------------------------------------------------------
 * END OF FILE
 *---------------------------------------------------------------
 */

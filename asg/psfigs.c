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

/* 
 *----------------------------------------------------------------------
 * File 	psfigs.c
 * Author 	s.t.frezza 
 * 
 * This file contains supporting post-script generation routines to produce 
 * figures for predefined schematic symbols
 *
 *----------------------------------------------------------------------
 */
#include <stdio.h>
#include "network.h"
#include "route.h"
#include "psfigs.h"
#include "asg_module.h"

#define NAME_OFFSET -1.1
#define SIDE_CONVERT(n) ((n==0)?0:(n==1)?1:(n==2)?2:(n==3)?3:0)
#define PI 3.1415927


/* This definition is used to replace the old usage of "header.ps" file: */
#define HEADER "%!							    	\n\
%  head.ps									\n\
%    prologue file for ploting of schematics editor drawings			\n\
%										\n\
%  routines:									\n\
%    init    -- sets up the graphics state for the proper scaling		\n\
%										\n\
%  some constants that can be changed:						\n\
/swidth        .1 def			% width of lines			\n\
/fwidth	      1.6 def			% scale factor for labels		\n\
/hwidth        12 def			% type size of header			\n\
/hmargin        6 def			% header margin				\n\
/init {		 	 	%  (optional_header) xlo ylo xhi yhi -> -	\n\
	/yhi exch def /xhi exch def						\n\
	/ylo exch def /xlo exch def						\n\
	/lwunit 72 def			% LaserWriter units per inch		\n\
	/marginx 0.27 lwunit mul def    % x-margin				\n\
	/marginy 0.25 lwunit mul def    % y-margin				\n\
	/pagex 8.5 lwunit mul		% usable x-space on page		\n\
             2 marginx mul sub def						\n\
	/pagey 11.0 lwunit mul 		% usable y-space on page		\n\
	     2 marginy mul sub def						\n\
%										\n\
	count 1 ge {			% header left on stack?			\n\
	     /Helvetica-Bold findfont   % set header font			\n\
		 hwidth scalefont setfont					\n\
	     marginx marginy pagey add hwidth sub				\n\
		 moveto			% position to place text		\n\
	     show			% print header string			\n\
	     /pagey pagey hwidth hmargin add sub				\n\
		 def			% adjust page size for header		\n\
	} if									\n\
%										\n\
	count 1 ge {			% 2nd header left on stack?		\n\
	     marginx marginy pagey add hwidth sub				\n\
		 moveto			% position to place text		\n\
	     show			% print header string			\n\
	     /pagey pagey hwidth hmargin add sub				\n\
		 def			% adjust page size for header		\n\
	} if									\n\
%										\n\
%										\n\
	/Courier findfont               % set the drawing font			\n\
	    fwidth scalefont setfont				       		\n\
%								    		\n\
	xhi xlo sub yhi ylo sub div     % push delta x / delta y		\n\
	pagex pagey div			% push pagex / pagey			\n\
	gt {									\n\
	   90 rotate			% rotate page and set origin		\n\
	   marginy pagex marginx add neg translate				\n\
	   pagey xhi xlo sub div	% push x and y scales			\n\
	   pagex yhi ylo sub div						\n\
        }{							      		\n\
	   marginx marginy translate	% set origin				\n\
	   pagex xhi xlo sub div	% push x and y scales			\n\
	   pagey yhi ylo sub div						\n\
	} ifelse								\n\
%										\n\
	2 copy 				% dup the scales			\n\
	gt 									\n\
	   {dup scale pop}		% xscale > yscale: use yscale		\n\
	   {pop dup scale}              % yscale > xscale: use xscale		\n\
	ifelse									\n\
%								 		\n\
	xlo neg ylo neg translate		% move origin to proper place	\n\
	swidth setlinewidth		% set line width for strokes		\n\
} def										\n"


/*--------------------------------------------------------------------------------- */

int ps_print_seg(f,x1,y1,x2,y2)
    FILE *f;
    int x1,y1,x2,y2;
    
{
    fprintf(f, "newpath %d %d moveto %d %d lineto \n", x1, y1, x2, y2);
    fprintf(f, "closepath stroke [] 0 setdash\n");    
}
/*--------------------------------------------------------------------------------- */

int ps_print_border(f,x1,y1,x2,y2)
    FILE *f;
    int x1,y1,x2,y2;
    
{
    fprintf(f,"[4.0] 0 setdash\n");
    fprintf(f, "newpath %d %d moveto %d %d lineto \n", x1, y1, x2, y1);
    fprintf(f, "%d %d lineto %d %d lineto\n", x2, y2, x1, y2);
    fprintf(f, "closepath stroke [] 0 setdash\n");
    
}
/*--------------------------------------------------------------------------------- */
int ps_print_mod(f,m)
    FILE  *f;
    module *m;
{
    int type, n, i;
    float scale, arcLen, noOfExtensions, fractional, theta, mid_y, mid_x;
    
    if (m != NULL)
    {
	type = validate(m->type);
	 scale = (float)m->x_size/(float)(ICON_SIZE * ICON_SIZE/2);
	

	ps_print_sym(f, m->type, m->x_pos,  m->y_pos, m->x_size, m->y_size, 
		     m->rot, m->name,scale);

	if ((type == BLOCK) || (type == DONT_KNOW)) ps_attach_outerms(f, m);
	ps_attach_interms(f, m);

	/* Special case stuff for mulit-terminal ANSI AND, NAND, OR, NOR, XOR, and
	   XNOR gates: */
	if ((m->y_size > ICON_SIZE) && (type != BLOCK) && (type != DONT_KNOW))
	{
	    scale = (float)m->x_size/(float)(ICON_SIZE * ICON_SIZE/2);

	    if ((type == AND) || (type == NAND))
	    {   /* AND family - display a line to connect all inputs to the icon */
		mid_x = (float)m->x_pos + (float)m->x_size/2.0;
		fprintf(f, "newpath %f %d moveto  ", mid_x - scale * 14.0, m->y_pos);
		fprintf(f, "%d %d rlineto closepath  stroke\n", 0, m->y_size);
	    }

	    else if ((type == OR) || (type == NOR))
	    {   /* OR family -- Add line to the 1st Back Curve:  */
		mid_x = (float)m->x_pos + (float)m->x_size/2.0 - 0.4;
		arcLen = (18.8232 * scale);
		noOfExtensions = ((float)m->y_size - arcLen)/(2.0 * arcLen);
		n = (int)noOfExtensions;
		fractional = (noOfExtensions - (float)n) * arcLen;
		for (i = n; i >= 0; i--)
		{	/* put down one arc, or fraction thereof */
		    theta = (i > 0) ? 28.072 : 
		            (180.0/PI) * asin(fractional/(scale * 20)) - 28.072;
		    if (theta > (-28.072 * 1.10))	/* 10% margin */
		    {
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 + 
			        (float)(n - i + 1) * arcLen;
			fprintf(f, "newpath %f %f %f %f %f arc stroke\n", 
				mid_x - scale * 28.5, mid_y, scale * 20.0, 
				-28.072, theta);
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 - 
			        (float)(n - i + 1) * arcLen;
			fprintf(f, "newpath %f %f %f %f %f arc stroke\n", 
				mid_x - scale * 28.5, mid_y, scale * 20.0, 
			-(theta), 28.072);
		    }
		}
	    }
	    else if ((type == XOR) || (type == XNOR))
	    {   /* OR family -- Add line to the 2nd Back Curve:  */
		mid_x = (float)m->x_pos + (float)m->x_size/2.0;
		arcLen = (18.8232 * scale);
		noOfExtensions = ((float)m->y_size - arcLen)/(2.0 * arcLen);
		n = (int)noOfExtensions;
		fractional = (noOfExtensions - (float)n) * arcLen;
		for (i = n; i >= 0; i--)
		{	/* put down one arc, or fraction thereof */
		    theta = (i > 0) ? 28.072 : 
		            (180.0/PI) * asin(fractional/(scale * 20)) - 28.072;
		    if (theta > (-28.072 * 1.10))	/* 10% margin */
		    {
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 + 
			        (float)(n - i + 1) * arcLen;
			fprintf(f, "newpath %f %f %f %f %f arc stroke\n", 
				mid_x - scale * 31.5, mid_y, scale * 20.0, 
				-28.072, theta);
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 - 
			        (float)(n - i + 1) * arcLen;
			fprintf(f, "newpath %f %f %f %f %f arc stroke\n", 
				mid_x - scale * 31.5, mid_y, scale * 20.0, 
				-(theta), 28.072);
		    }
		}
	    }
	    /* height of arc in Y is 2*rad*sin(28.072) = 40*scale*.47058 = 
	       scale * 18.8232. */
	}
    }
}
/*--------------------------------------------------------------------------------- */
int ps_attach_interms(f, m)
    FILE *f;
    module *m;
{ 	
    /* attach all of the input terminals to the gate in question.
     * NOTE: see "insert_all_modules" in route.c for the definition of the
     * area in which the module must fit.  
     */
    tlist *tl;
    int mType = validate(m->type), x = m->x_pos, y = m->y_pos;
    int term_x, term_y, icon_term_x;
    char *temp = (char *)calloc(NET_NAME_SIZE + 1, sizeof(char));
    float scale = (float)m->x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float mid_x = (float)m->x_pos + (float)m->x_size/2.0;
    float w;

    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	term_x = tl->this->x_pos;
	term_y = tl->this->y_pos;
	if (tl->this->type == IN)
	{
	    icon_term_x = m->x_size/2 - 3;
	    
	    /* display a small line for each terminal */
	    fprintf(f, "newpath %d %d moveto ", term_x + x, term_y + y);
	    switch(mType)
	    {
		case DONT_KNOW :
		case BLOCK: fprintf(f, "%d %d lineto ", x, term_y + y);
		            break;
		case AND  :
		case NAND : fprintf(f, "%f %d lineto ", mid_x - 14.0 * scale, term_y+y);
                            break;
		default :   fprintf(f, "%d 0 rlineto ", icon_term_x - term_x);
		            break;
	    }
	    fprintf(f, "closepath  stroke\n ");

	    /* skip the dot: */
	    /* ps_print_contact(f,(float)term_x + x + 1, (float)term_y + y); */

	    if ((do_routing != TRUE) && (latex == FALSE)) 
	    {
		/* At the net name */
		str_end_copy(temp, tl->this->nt->name, NET_NAME_SIZE);
	        ps_put_label(f, temp,
			     (float)(term_x + x - strlen(tl->this->nt->name)),
			     (float)(term_y + y));
	    }
	}
	else if (tl->this->type == OUT)
	{
	    /* skip the dot: */
	    /* ps_print_contact(f,(float)(term_x + x) - .5, (float)term_y + y); */
	    
	    if (mType != INPUT_SYM)
	    {  /* add the net name as a terminal name: */
		str_end_copy(temp, tl->this->nt->name, NET_NAME_SIZE);
	        ps_put_label(f, temp,
			     (float)(term_x + x), (float)(term_y + y + 1));
	    }
	}
	else if (tl->this->type == INOUT)
	{
	    if (m->x_size > ICON_SIZE)
	    {
		/* display a line to connect all of the terminals to the icon */
	    }
	    /* Connect the line from the outside to the edge of the box... */
	    fprintf(f, "newpath %d %d moveto  ", x + term_x, y + term_y + 1);
	    fprintf(f, "%d %d rlineto closepath  stroke\n", 0, -(TERM_MARGIN));
	}
	else if (tl->this->type == OUTIN)
	{
	    if (m->x_size > ICON_SIZE)
	    {
		/* display a line to connect all of the terminals to the icon */
	    }
	    /* Connect the line from the outside to the edge of the box... */
	    fprintf(f, "newpath %d %d moveto  ", x + term_x, y + term_y);
	    fprintf(f, "%d %d rlineto closepath  stroke\n", 0, -(TERM_MARGIN));
	}
	else /* Special Terminals... */
	{
	    icon_term_x = m->x_size/2 - 3;
	    fprintf(f, "newpath %6.4f %d moveto ", 
		    (float)term_x/2.0 + (float)x, term_y + y);
	    switch(mType)
	    {
		case DONT_KNOW :
		case BLOCK: fprintf(f, "%d %d lineto ", x, term_y + y);
		            break;
		case AND  :
		case NAND : fprintf(f, "%f %d lineto ", mid_x - 14.0 * scale, term_y+y);
                            break;
		default :   fprintf(f, "%d 0 rlineto ", icon_term_x - term_x);
		            break;
	    }
	    fprintf(f, "closepath  stroke\n ");
	
	    w = 0.750;
	    if (tl->this->type == GND)
	    {
		/* display a small GND Symbol for this terminal */
		fprintf(f,"\n%s GND Symbol ->", "%%%");
		fprintf(f, "\n newpath %6.4f %d moveto ", 
			(float)term_x/2.0 + (float)x, term_y + y);
		fprintf(f,"0 -0.75 rlineto %6.5f 0 rmoveto %6.5f 0 rlineto ", w/4.0, w/-2.0);
		fprintf(f,"%6.5f 0.25 rmoveto %6.5f 0 rlineto ", w/-16.0, 0.75*w);
		fprintf(f,"%6.5f 0.25 rmoveto %6.5f 0 rlineto closepath stroke \n", 
			(-7.0/8.0)*w, w);
	    }
	    else if (tl->this->type == VDD)
	    {
		/* display a small VDD symbol for this terminal */
		fprintf(f,"\n%s Vdd Symbol on module \n", "%%%");
		fprintf(f, "newpath %6.4f %d moveto ", 
			(float)term_x/2.0 + (float)x, term_y + y);
		fprintf(f, "0 0.750 rlineto 0.1250 -0.250 rlineto ");
		fprintf(f, "-0.25 0 rlineto 0.1250 0.250 rlineto closepath stroke \n");
	    }
	    else /* (tl->this->type == DUMMY) */
	    {
		/* Simply put down a contact on the end of the stubbed terminal: */
		ps_print_contact(f,(float)term_x/2.0 + x, (float)term_y + y);
	    }
	}
    }
}
/*--------------------------------------------------------------------------------- */
int xc_attach_interms(areastruct, m)
    XCWindowData *areastruct;
    module *m;
{ 	
    /* attach all of the input terminals to the gate in question.
     * NOTE: see "insert_all_modules" in route.c for the definition of the
     * area in which the module must fit.  
     */
    tlist *tl;
    int mType = validate(m->type), x = m->x_pos, y = m->y_pos;
    int term_x, term_y, icon_term_x;
    char *temp = (char *)calloc(NET_NAME_SIZE + 1, sizeof(char));
    float scale = (float)m->x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float mid_x = (float)m->x_pos + (float)m->x_size/2.0;
    float w;

    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	term_x = tl->this->x_pos;
	term_y = tl->this->y_pos;
	if (tl->this->type == IN)
	{
	    icon_term_x = m->x_size/2 - 3;
	    
	    /* display a small line for each terminal */
	  //  fprintf(f, "newpath %d %d moveto ", term_x + x, term_y + y);
	    switch(mType)
	    {
		case DONT_KNOW :
		case BLOCK://code to be attached
		            break;
		case AND  :
		case NAND ://code to be attached
                            break;
		default :   //code to be attached
		            break;
	    }
		    if ((do_routing != TRUE) && (latex == FALSE)) 
	    {
		/* At the net name */
		str_end_copy(temp, tl->this->nt->name, NET_NAME_SIZE);
	        /*ps_put_label(f, temp,
			     (float)(term_x + x - strlen(tl->this->nt->name)),
			     (float)(term_y + y));*/
	    }
	}
	else if (tl->this->type == OUT)
	{
	   
	    
	    if (mType != INPUT_SYM)
	    {  /* add the net name as a terminal name: 
		str_end_copy(temp, tl->this->nt->name, NET_NAME_SIZE);
	        ps_put_label(f, temp,
			     (float)(term_x + x), (float)(term_y + y + 1));*/
	    }
	}
	else if (tl->this->type == INOUT)
	{
	    if (m->x_size > ICON_SIZE)
	    {
		/* display a line to connect all of the terminals to the icon */
	    }
	    /* Connect the line from the outside to the edge of the box... */
	   // fprintf(f, "newpath %d %d moveto  ", x + term_x, y + term_y + 1);
	    //fprintf(f, "%d %d rlineto closepath  stroke\n", 0, -(TERM_MARGIN));
	}
	else if (tl->this->type == OUTIN)
	{
	    if (m->x_size > ICON_SIZE)
	    {
		/* display a line to connect all of the terminals to the icon */
	    }
	    /* Connect the line from the outside to the edge of the box... 
	    fprintf(f, "newpath %d %d moveto  ", x + term_x, y + term_y);
	    fprintf(f, "%d %d rlineto closepath  stroke\n", 0, -(TERM_MARGIN));*/
	}
	else /* Special Terminals... */
	{
	    icon_term_x = m->x_size/2 - 3;
	  //  fprintf(f, "newpath %6.4f %d moveto ", 
	//	    (float)term_x/2.0 + (float)x, term_y + y);
	    switch(mType)
	    {
		case DONT_KNOW :
		case BLOCK: //fprintf(f, "%d %d lineto ", x, term_y + y);
		            break;
		case AND  :
		case NAND : //fprintf(f, "%f %d lineto ", mid_x - 14.0 * scale, term_y+y);
                            break;
		default :   //fprintf(f, "%d 0 rlineto ", icon_term_x - term_x);
		            break;
	    }
	   // fprintf(f, "closepath  stroke\n ");
	
	    w = 0.750;
	    if (tl->this->type == GND)
	    {
		/* display a small GND Symbol for this terminal 
		fprintf(f,"\n%s GND Symbol ->", "%%%");
		fprintf(f, "\n newpath %6.4f %d moveto ", 
			(float)term_x/2.0 + (float)x, term_y + y);
		fprintf(f,"0 -0.75 rlineto %6.5f 0 rmoveto %6.5f 0 rlineto ", w/4.0, w/-2.0);
		fprintf(f,"%6.5f 0.25 rmoveto %6.5f 0 rlineto ", w/-16.0, 0.75*w);
		fprintf(f,"%6.5f 0.25 rmoveto %6.5f 0 rlineto closepath stroke \n", 
			(-7.0/8.0)*w, w);*/
	    }
	    else if (tl->this->type == VDD)
	    {
		/* display a small VDD symbol for this terminal 
		fprintf(f,"\n%s Vdd Symbol on module \n", "%%%");
		fprintf(f, "newpath %6.4f %d moveto ", 
			(float)term_x/2.0 + (float)x, term_y + y);
		fprintf(f, "0 0.750 rlineto 0.1250 -0.250 rlineto ");
		fprintf(f, "-0.25 0 rlineto 0.1250 0.250 rlineto closepath stroke \n");*/
	    }
	    else /* (tl->this->type == DUMMY) */
	    {
		/* Simply put down a contact on the end of the stubbed terminal: 
		ps_print_contact(f,(float)term_x/2.0 + x, (float)term_y + y);*/
	    }
	}
    }
}



/*--------------------------------------------------------------------------------- */
int ps_attach_outerms(f, m)
    FILE *f;
    module *m;
{ 	
    /* attach all of the output terminals to the gate in question.
     * NOTE: see "insert_all_modules" in route.c for the definition of the
     * area in which the module must fit.  
     */
    tlist *tl;
    int x = m->x_pos;
    int y = m->y_pos;
    int term_x, term_y, icon_term_x;
    
    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	term_x = tl->this->x_pos;
	term_y = tl->this->y_pos;
	if (tl->this->type == OUT)
	{
	    /* display a small line for each terminal */
	    fprintf(f, "newpath ");
	    fprintf(f, "%d %d moveto  ", term_x + x - 1, term_y + y);
	    fprintf(f, "%d %d rlineto  ",TERM_MARGIN , 0);
	    fprintf(f, "closepath  stroke\n ");
	}
    }
}

/*--------------------------------------------------------------------------------- */
int xc_attach_outerms(areastruct, m)
    XCWindowData *areastruct;
    module *m;
{ 	
    /* attach all of the output terminals to the gate in question.
     * NOTE: see "insert_all_modules" in route.c for the definition of the
     * area in which the module must fit.  
     */
    tlist *tl;
    int x = m->x_pos;
    int y = m->y_pos;
    int term_x, term_y, icon_term_x;
    
    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	term_x = tl->this->x_pos;
	term_y = tl->this->y_pos;
	if (tl->this->type == OUT)
	{
	    /* display a small line for each terminal 
	    fprintf(f, "newpath ");
	    fprintf(f, "%d %d moveto  ", term_x + x - 1, term_y + y);
	    fprintf(f, "%d %d rlineto  ",TERM_MARGIN , 0);
	    fprintf(f, "closepath  stroke\n ");*/
	}
    }
}



/*--------------------------------------------------------------------------------- */
int ps_print_contact(f, x, y)
    FILE *f;
    float x,y;
    
/* add the dot: */
{
    fprintf(f, "newpath %f %f %f 0 360 arc closepath fill\n", 
	    x, y, CONTACT_WIDTH);
}
/*--------------------------------------------------------------------------------- */

int ps_print_box(f, x, y, x_dis, y_dis, rot)
    FILE *f;
    int x, y, x_dis, y_dis, rot;
{
    /* print a box of the given size at the given location : */
    fprintf(f, "newpath\n");
    fprintf(f, "%d %d %d moveto\n", rot, x, y);
    fprintf(f, "%d %d rlineto\n", 0, y_dis);
    fprintf(f, "%d %d rlineto\n", x_dis, 0);
    fprintf(f, "%d %d neg rlineto\n", 0, y_dis);
    fprintf(f, "closepath \nstroke\n ");
}

/*--------------------------------------------------------------------------------- */

int ps_print_sym(f, sym, x, y, x_dis, y_dis, rot, name,scale)
    FILE  *f;
    char *sym, *name;
    int x, y, x_dis, y_dis, rot;
	float scale;
{
    /* print the symbol that goes with this type: */
    switch(validate(sym))
    {
	case BUFFER : 
		     ps_put_label(f, name, (float)x + (float)x_dis/2. - (float)strlen(name)/2.0,
				   (float)y + 2.25);
		      break;
	case INVNOT_  : 
			ps_print_inverted_not(f, x, y, x_dis, y_dis);
	            ps_put_label(f, name, (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 +1.0,
				 (float)y + 2.25);
		    break;
	case NOT_  : 
                      
                 ps_print_not(f, x, y, x_dis, y_dis);
	          ps_put_label(f, name, (float)x + (float)x_dis/2. - (float)strlen(name)/2.0,
				 (float)y + 2.25);
		    break;
	case AND  : 
                    ps_print_and(f, x, y, x_dis, y_dis,scale);
	          ps_put_label(f, name, 
				 (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - .2,
				 (float)y + (float)y_dis/2. - NAME_OFFSET);
	            break;
	case NAND : 
                     
			ps_print_nand(f, x, y, x_dis, y_dis,scale);
	            ps_put_label(f, name, 
				 (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - .2,
				 (float)y + (float)y_dis/2. - NAME_OFFSET);
	            break;
	case OR   : ps_print_or(f, x, y, x_dis, y_dis);
	         ps_put_label(f, name, 
				 (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - .2,
				 (float)y + (float)y_dis/2. - NAME_OFFSET);
	            break;
	case NOR  : ps_print_nor(f, x, y, x_dis, y_dis);
	            ps_put_label(f, name, 
				 (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - .2,
				 (float)y + (float)y_dis/2. - NAME_OFFSET);
	            break;
	case XOR  : ps_print_xor(f, x, y, x_dis, y_dis);
	           ps_put_label(f, name, 
				 (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - .2,
				 (float)y + (float)y_dis/2. - NAME_OFFSET);
	            break;
	case XNOR :
                   ps_print_xnor(f, x, y, x_dis, y_dis);
	           ps_put_label(f, name, 
				 (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - .2,
				 (float)y + (float)y_dis/2. - NAME_OFFSET);
	            break;

	case INPUT_SYM : 
                   ps_print_interm(f, x, y, x_dis, y_dis);
	               ps_put_label(f, name, 
				      (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 - 3.2,
				      (float)y + (float)y_dis/1.5 - NAME_OFFSET);
	                 break;
	case OUTPUT_SYM : 
			ps_print_outterm(f, x, y, x_dis, y_dis);
	        ps_put_label(f, name, 
				       (float)x + (float)x_dis/2. - (float)strlen(name)/2.0 + .1,
				       (float)y + (float)y_dis/2. - NAME_OFFSET);
	                  break;
	 
        case BLOCK :
			ps_print_block(f, x, y, x_dis, y_dis, rot);
	             ps_put_label(f, name, (float)x + (float)x_dis/2. - 2.0,
			       (float)y + (float)y_dis/2. - NAME_OFFSET);
	             break;

        default :ps_print_box(f, x, y, x_dis, y_dis, SIDE_CONVERT(rot));
	         ps_put_label(f, name, (float)x + (float)x_dis/2. - 2.0,
			       (float)y + (float)y_dis/2. - NAME_OFFSET);
	
	          return(0);
	  	  break;
    }
     return(1);
}

/*---------------------------------------------------------------------- */

int ps_put_label(f, str, x, y)
    FILE *f;
    char *str;
    float x, y;
{
    fprintf(f,"%f %f moveto (%s) show\n", x,y,str);
}

/*--------------------------------------------------------------------------------- */

int ps_put_int(f, i, x, y)
    FILE *f;
    int i;
    float x, y;
{
    fprintf(f,"%f %f moveto (%d) show\n", x,y,i);
}


/*----------------------------------------------------------------------- */
int ps_print_outterm(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate the ps code for an output terminal */
    int mid_x = x + x_size/2;
    int mid_y = y + y_size/2;

    fprintf(f, "newpath %d %d moveto ", x - 1, mid_y);
    fprintf(f, "%d %d rlineto ", x_size/2, 0);
    fprintf(f, "%d %d rlineto ", -(x_size/2 - 1), y_size/2 - 2);
    fprintf(f, " stroke\n");
    fprintf(f, "%d %d moveto ", x - 1 + x_size/2, mid_y);
    fprintf(f, "%d %d rlineto ", -(x_size/2 - 1), -(y_size/2 - 2));
    fprintf(f, " stroke\n");

}
/*--------------------------------------------------------------------------------- */
int ps_print_interm(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate the ps code for an output terminal */
    int mid_x = x + x_size/2;
    int mid_y = y + y_size/2;

    fprintf(f, "newpath %d %d moveto ", x + x_size + 1, mid_y);
    fprintf(f, "%d %d rlineto ", -(x_size/2), 0);
    fprintf(f, "%d %d rlineto ", -(x_size - 2), y_size/2 - 2);
    fprintf(f, " stroke\n");
    fprintf(f, "%d %d moveto ", x + 1 + x_size/2, mid_y);
    fprintf(f, "%d %d rlineto ", -(x_size - 2), -(y_size/2 - 2));
    fprintf(f, " stroke\n");

}

/*--------------------------------------------------------------------------------- */
int ps_print_block(f, x_pos, y_pos, x_size, y_size, rot)
    FILE *f;
    int x_pos, y_pos, x_size, y_size, rot;
{
    ps_print_box(f, x_pos, y_pos, x_size, y_size, SIDE_CONVERT(rot));
}

/*--------------------------------------------------------------------------------- */
int ps_print_buffer(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate post-script code for a BUFFER_ gate to fit within the given box */

    float mid_x = (float)x + (float)x_size/2.0;
    float mid_y = (float)y + (float)(y_size/2);
    float tip = mid_x - 2.25 + 4.330127;			/* 2.55 * tan(60 deg) */

    fprintf(f, "\n%%%% Buffer centered at %f %f w/ tip at %f... \n", 
	    mid_x, mid_y, tip);

    /* first put down the triangle: */
    fprintf(f, "newpath %f %f moveto  ", mid_x - 2.25, mid_y - BUFFER_SIZE/2.0);
    fprintf(f, "%f %f lineto ", mid_x - 2.25, mid_y + BUFFER_SIZE/2.0);
    fprintf(f, "%f %f lineto closepath stroke\n", tip, mid_y);
        
    /* put down the terminal lines */
    fprintf(f, "newpath %d %f moveto  ", x + x_size + TERM_MARGIN, mid_y);
    fprintf(f, "%f %f lineto  closepath stroke\n", tip, mid_y);
    fprintf(f, "newpath %f %f moveto ", mid_x - 2.25, mid_y);
    fprintf(f, "%d %f lineto  closepath stroke\n", x - TERM_MARGIN, mid_y);

    return(1);
}
/*--------------------------------------------------------------------------------- */
int ps_print_inverted_not(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate post-script code for an inverted NOT gate to fit within the given box */
    float mid_x = (float)x + (float)x_size/2.0;
    float diam = 2 * CIRCLE_SIZE;
    float base_x = mid_x - 2.25 + diam;
    float mid_y = (float)y + (float)(y_size/2);
    float tip = base_x + 4.330127;		/* 4.330127 = 2.55 * tan(60 deg) */

    fprintf(f, "\n%%%% Inverter centered at %f %f w/ tip at %f... \n", 
	    mid_x, mid_y, tip);

    /* first put down the triangle: */
    fprintf(f, "newpath %f %f moveto  ", base_x, mid_y - BUFFER_SIZE/2.0);
    fprintf(f, "%f %f lineto ", base_x, mid_y + BUFFER_SIZE/2.0);
    fprintf(f, "%f %f lineto closepath stroke\n", tip, mid_y);
    
    /* now put down the little circle: */
    fprintf(f, "newpath %f %f %f 0 360 arc closepath stroke\n", 
	    base_x - CIRCLE_SIZE, mid_y, CIRCLE_SIZE);
    
    /* put down the terminal line */
    fprintf(f, "newpath %d %f moveto  ", x + x_size + TERM_MARGIN, mid_y);
    fprintf(f, "%f %f lineto closepath  stroke\n", tip, mid_y);
    fprintf(f, "newpath %f %f moveto ", base_x - diam, mid_y);
    fprintf(f, "%d %f lineto closepath stroke\n", x - TERM_MARGIN, mid_y);

    return(1);
}
/*--------------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------------- */
int ps_print_not(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate post-script code for a NOT gate to fit within the given box */
    float mid_x = (float)x + (float)x_size/2.0;
    float mid_y = (float)y + (float)(y_size/2);
    float tip = mid_x - 2.25 + 4.330127;			/* 2.55 * tan(60 deg) */

    fprintf(f, "\n%%%% Inverter centered at %f %f w/ tip at %f... \n", 
	    mid_x, mid_y, tip);

    /* first put down the triangle: */
    fprintf(f, "newpath %f %f moveto  ", mid_x - 2.25, mid_y - BUFFER_SIZE/2.0);
    fprintf(f, "%f %f lineto ", mid_x - 2.25, mid_y + BUFFER_SIZE/2.0);
    fprintf(f, "%f %f lineto closepath stroke\n", tip, mid_y);
    
    /* now put down the little circle: */
    fprintf(f, "newpath %f %f %f 0 360 arc closepath stroke\n", 
	    tip + CIRCLE_SIZE, mid_y, CIRCLE_SIZE);
    
    /* put down the terminal line */
    fprintf(f, "newpath %d %f moveto  ", x + x_size + TERM_MARGIN, mid_y);
    fprintf(f, "%f %f lineto closepath  stroke\n", tip + (2.0 * CIRCLE_SIZE), mid_y);
    fprintf(f, "newpath %f %f moveto ", mid_x - 2.25, mid_y);
    fprintf(f, "%d %f lineto closepath stroke\n", x - TERM_MARGIN, mid_y);

    return(1);
}
/*--------------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------------- */
int ps_print_and(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate postscript code for an AND gate within the given box: */
    float mid_x = (float)x + (float)x_size/2.0;
    float mid_y = (float)y + (float)y_size/2.0;
    float scale = (float)x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float rad = scale * 10.0;
    
    /* first put down the figure: */
    fprintf(f, "\n%%%% AND Gate centered at %f %f... \n", mid_x, mid_y);

    /* Front Curve*/
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",
	    mid_x + (scale * 2.0), mid_y, rad, -90.081, 90.081);
    
    /* Connecting Lines: */
    fprintf(f, "newpath %f %f moveto %f %f lineto %f %f lineto %f %f lineto stroke\n ",
	    mid_x + scale * 2.0, mid_y + scale * 10, 
	    mid_x - scale * 13.0, mid_y + scale * 10,
	    mid_x - scale * 13.0, mid_y - scale * 10,
	    mid_x + scale * 2.0, mid_y - scale * 10);

    /* put down the terminal line */
    fprintf(f, "newpath %f %f moveto ", mid_x + scale * 12.0, mid_y);
    fprintf(f, "%d %f lineto stroke \n", x + x_size + TERM_MARGIN, mid_y);
	    
    return(1);
}
/*--------------------------------------------------------------------------------- */
int ps_print_nand(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate postscript code for a NAND gate within the given box: */
    float mid_x = (float)x + (float)x_size/2.0;
    float mid_y = (float)y + (float)y_size/2.0;
    float scale = (float)x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float rad = scale * 10.0;
    
    /* first put down the figure: */
    fprintf(f, "\n%%%% NAND Gate centered at %f %f... \n", mid_x, mid_y);

    /* Front Curve */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",
	    mid_x + (scale * 1.0), mid_y, rad, -90.081, 90.081);
    
    /* Connecting Lines: */
    fprintf(f, "newpath %f %f moveto %f %f lineto %f %f lineto %f %f lineto stroke\n ",
	    mid_x + scale * 1.0, mid_y + scale * 10, 
	    mid_x - scale * 14.0, mid_y + scale * 10,
	    mid_x - scale * 14.0, mid_y - scale * 10,
	    mid_x + scale * 1.0, mid_y - scale * 10);

    /* now put down the little circle: */
    fprintf(f, "newpath  ");
    fprintf(f, "%f %f %f 0 360 arc closepath stroke\n", 
	    mid_x + scale * 11.0 + CIRCLE_SIZE, mid_y, CIRCLE_SIZE);

    /* put down the terminal line */
    fprintf(f, "newpath %f %f moveto ", 
	    mid_x + scale * 11.0 + (2.0 * CIRCLE_SIZE), mid_y);
    fprintf(f, "%d %f lineto stroke \n", x + x_size + TERM_MARGIN, mid_y);
	    
    return(1);
}
/*--------------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------------- */
int ps_print_nor(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate postscript code for a NOR gate within the given box: */
    float mid_x = (float)x + (float)x_size/2.0 - 0.4;
    float mid_y = (float)y + (float)y_size/2.0;
    float scale = (float)x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float rad = scale * 20.0;
    
    /* first put down the figure: */
    fprintf(f, "\n%%%% NOR Gate centered at %f %f... \n", mid_x, mid_y);

    /* Top Curve ...newpath 383.167 283.167 195.877 91.219 29.291 arcn stroke */
    fprintf(f, "newpath %f %f %f %f %f arcn stroke\n",mid_x - scale * 5.0, 
	    mid_y - scale * 10.0, rad, 91.219, 29.219);
    
    /* Bottom Curve: ...newpath 381.151 478.360 199.372 -90.618 -29.892 arc stroke */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",mid_x - scale * 5.0, 
	    mid_y + scale * 10.0, rad, -90.618, -29.892);

    /* Back Curve: newpath 126.500 379.000 212.500 -31.072 31.072 arc stroke */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",mid_x - scale * 29.0, 
	    mid_y, rad, -28.072, 28.072);

    /* Connecting Lines: */
    fprintf(f, "newpath %f %f moveto %f %f lineto stroke\n ",
	    mid_x - scale * 12.3, mid_y + scale * 10, 
	    mid_x - scale * 3.0, mid_y + scale * 10);

    fprintf(f, "newpath %f %f moveto %f %f lineto  stroke\n",
	    mid_x - scale * 12.3, mid_y - scale * 10, 
	    mid_x - scale * 3.0, mid_y - scale * 10);

    /* now put down the little circle: */
    fprintf(f, "newpath  ");
    fprintf(f, "%f %f %f 0 360 arc closepath stroke\n", 
	    mid_x + (scale * 13.50) + CIRCLE_SIZE, mid_y, CIRCLE_SIZE);

    /* put down the terminal line */
    fprintf(f, "newpath %f %f moveto ", 
	    mid_x + (scale * 13.50) + (2 * CIRCLE_SIZE), mid_y);
    fprintf(f, "%d %f lineto stroke \n", x + x_size + TERM_MARGIN, mid_y);
	    
    return(1);
}
/*--------------------------------------------------------------------------------- */
int ps_print_or(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate postscript code for an OR gate within the given box: */
    float mid_x = (float)x + (float)x_size/2.0 - 0.4;
    float mid_y = (float)y + (float)y_size/2.0;
    float scale = (float)x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float rad = scale * 20.0;
    
    /* first put down the figure: */
    fprintf(f, "\n%%%% OR Gate centered at %f %f... \n", mid_x, mid_y);

    /* Top Curve ...newpath 383.167 283.167 195.877 91.219 29.291 arcn stroke */
    fprintf(f, "newpath %f %f %f %f %f arcn stroke\n",mid_x - scale * 5.0, 
	    mid_y - scale * 10.0, rad, 91.219, 29.219);
    
    /* Bottom Curve: ...newpath 381.151 478.360 199.372 -90.618 -29.892 arc stroke */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",
	    mid_x - scale * 5.0, mid_y + scale * 10.0, rad, -90.618, -29.892);

    /* Back Curve: newpath 126.500 379.000 212.500 -31.072 31.072 arc stroke */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n", 
	    mid_x - scale * 29.0, mid_y, rad, -28.072, 28.072);

    /* Connecting Lines: */
    fprintf(f, "newpath %f %f moveto %f %f lineto stroke\n ",
	    mid_x - scale * 12.3, mid_y + scale * 10, 
	    mid_x - scale * 3.0, mid_y + scale * 10);

    fprintf(f, "newpath %f %f moveto %f %f lineto  stroke\n",
	    mid_x - scale * 12.3, mid_y - scale * 10,
	    mid_x - scale * 3.0, mid_y - scale * 10);
	    
    /* put down the terminal line */
    fprintf(f, "newpath %f %f moveto %d %f lineto stroke\n", 
	    mid_x + scale * 13.00, mid_y,
	    x + x_size + TERM_MARGIN, mid_y);
    return(1);
}
/*--------------------------------------------------------------------------------- */
int ps_print_xnor(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate postscript code for a XNOR gate within the given box: */
    float mid_x = (float)x + (float)x_size/2.0;
    float mid_y = (float)y + (float)y_size/2.0;
    float scale = (float)x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float rad = scale * 20.0;
    
    /* first put down the figure: */
    fprintf(f, "\n%%%% XNOR Gate centered at %f %f... \n", mid_x, mid_y);

    /* Top Curve */
    fprintf(f, "newpath %f %f %f %f %f arcn stroke\n",mid_x - scale * 4.5, 
	    mid_y - scale * 10.0, rad, 91.219, 29.219);
    
    /* Bottom Curve */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",mid_x - scale * 4.5, 
	    mid_y + scale * 10.0, rad, -90.618, -29.892);

    /* 1st Back Curve */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",mid_x - scale * 28.5, 
	    mid_y, rad, -28.072, 28.072);

    /* 2nd Back Curve */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n",mid_x - scale * 31.5, 
	    mid_y, rad, -28.072, 28.072);

    /* Connecting Lines: */
    fprintf(f, "newpath %f %f moveto %f %f lineto stroke\n ",
	    mid_x - scale * 11.8, mid_y + scale * 10, 
	    mid_x - scale * 2.5, mid_y + scale * 10);

    fprintf(f, "newpath %f %f moveto %f %f lineto  stroke\n",
	    mid_x - scale * 11.8, mid_y - scale * 10, 
	    mid_x - scale * 2.5, mid_y - scale * 10);

    /* now put down the little circle: */
    fprintf(f, "newpath %f %f %f 0 360 arc closepath stroke\n", 
	    mid_x + (scale * 14.00) + CIRCLE_SIZE, mid_y, CIRCLE_SIZE);

    /* put down the terminal line */
    fprintf(f, "newpath %f %f moveto ", 
	    mid_x + (scale * 14.50) + (2.0 * CIRCLE_SIZE), mid_y);
    fprintf(f, "%d %f lineto stroke \n", x + x_size + TERM_MARGIN, mid_y);
	    
    return(1);
}
/*--------------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------------- */
int ps_print_xor(f, x, y, x_size, y_size)
    FILE *f;
    int x, y, x_size, y_size;
{
    /* generate postscript code for a XOR gate within the given box: */
    float mid_x = (float)x + (float)x_size/2.0;
    float mid_y = (float)y + (float)y_size/2.0;
    float scale = (float)x_size/(float)(ICON_SIZE * ICON_SIZE/2);
    float rad = scale * 20.0;
    
    /* first put down the figure: */
    fprintf(f, "\n%%%% XOR Gate centered at %f %f... \n", mid_x, mid_y);

    /* Top Curve */
    fprintf(f, "newpath %f %f %f %f %f arcn stroke\n", mid_x - scale * 4.5, 
	    mid_y - scale * 10.0, rad, 91.219, 29.219);
    
    /* Bottom Curve: */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n", mid_x - scale * 4.5, 
	    mid_y + scale * 10.0, rad, -90.618, -29.892);

    /* 1st Back Curve:  */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n", mid_x - scale * 28.5, 
	    mid_y, rad, -28.072, 28.072);

    /* 2nd Back Curve: */
    fprintf(f, "newpath %f %f %f %f %f arc stroke\n", mid_x - scale * 31.5, 
	    mid_y, rad, -28.072, 28.072);

    /* Connecting Lines: */
    fprintf(f, "newpath %f %f moveto %f %f lineto stroke\n ",
	    mid_x - scale * 11.8, mid_y + scale * 10, 
	    mid_x - scale * 2.5, mid_y + scale * 10);

    fprintf(f, "newpath %f %f moveto %f %f lineto  stroke\n",
	    mid_x - scale * 11.8, mid_y - scale * 10, 
	    mid_x - scale * 2.5, mid_y - scale * 10);

    /* put down the terminal line */
    fprintf(f, "newpath %f %f moveto ", mid_x + scale * 14.0, mid_y);
    fprintf(f, "%d %f lineto stroke \n", x + x_size + TERM_MARGIN, mid_y);

    return(1);
}
/*--------------------------------------------------------------------------------- */
ps_print_standard_header(f)
    FILE *f;
{
    fprintf(f, "%s\n", HEADER);
}
/*--------------------------------------------------------------------------------- */



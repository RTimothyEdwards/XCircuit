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
 *      file terminals.h
 * As part of MS Thesis Work, S T Frezza, UPitt Engineering
 * February, 1991
 *
 * This file contains structures to aid in the accurate placement of terminals. 
 */

#include <stdio.h>	
#include "route.h"	/* Routing Structures needed for terminal placement */

/* Structure Definitions and type definitions: */
typedef struct connect_struct    connect;
typedef struct connect_list_str	connlist;

struct connect_struct
{
    srange *exit;	/* Range where the wire exits the central part of the diagram */
    int up, down;	/* distance above/below connection point that must be observed */
    module *mod;	/* System terminal to whom this is assosiated. */
    crnlist *corners;	/* Corners that set this range */
};

struct connect_list_str
{
    connect *this;
    connlist *next;
};


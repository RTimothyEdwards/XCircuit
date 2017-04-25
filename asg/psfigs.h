
/************************************************************
**
**       COPYRIGHT (C) 2004 Gannon University
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
**   Author:      Steve Frezza (STF)	Gannon University
**   Revision History
**   ----------------
**   Date      Initials   Description of change
**
**   thru 2/04   STF      Initial modification of ASG code in merger with
**			  XCircuit.
**   Some code taken and adapted from SPRA work performed by Al Martello.
**
*/

/* The following icons are directly supported: 
   (See "validate" in "psfigs.c") */
#define NOT_ 1
#define AND 2
#define NAND 3
#define OR 4
#define NOR 5
#define XOR 6
#define XNOR 7
#define BUFFER 8
#define INPUT_SYM 9
#define OUTPUT_SYM 10
#define INOUT_SYM 11
#define BLOCK 12
#define INVNOT_ 13
#define NMOS 14
#define PMOS 15
#define CAPC 16
#define INDR 17
#define RESTR 18
#define VAMP	19
#define MSFET	20

#define DONT_KNOW 20

#define CONTACT_WIDTH .350
#define ICON_LEN 4
#define ICON_HEIGHT 6
#define CIRCLE_SIZE .75
#define ANDARC_RAD 3.0625
#define ANDARC_THETA 69.09
#define ANDARC_CENTER_X_OFFSET 3.854 - 4.0

#define ORARC_RAD 3.75
#define ORARC_THETA 53.13
#define ORARC_CENTER_X_OFFSET 2.75 - 8.0

#define XOR_OFFSET .5

#define BUFFER_SIZE 6.0
#define GATE_LENGTH 8
#define GATE_HEIGHT 6
#define SYSTERM_SIZE 6
#define BLOCK_SPACES_PER_PIN 3
#define SPACES_PER_PIN 2

/*- end psfigs support ------------------------------------------------------*/

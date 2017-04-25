
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
**  some system dependent definitions
*/
#ifdef apollo
#define const
#endif


/*
**  common definitions needed by more vcomp and vsim
*/
#define STR_DUMMY_SIGNAL  "_DUMMY_SIGNAL_"
#define STR_GUARD_SIGNAL    "guard" /*  the signal name for a block guard  */
#define SIG_ZERO   "$0"
#define SIG_ONE    "$1"

#define ATTRIB_SEP  '\''  /*  the attribute separator char  */
/*  list of supported attributes (case MUST be lc)  */
#define ATT_RISING   "rising"
#define ATT_FALLING  "falling"



#define MAX_BIT_NUMBER  32

#define INIT_FUN_PREFACE "init_"


/*
**  signal assignment option definitions
*/
#define STR_DELAY_INERTIAL  "I"    /*  default option  */
#define STR_DELAY_TRANSPORT "T"    /*  'TRANSPORT' only option  */
#define STR_GUARD_OPTION    "G"    /*  'GUARDED' only option  */
#define STR_GUARD_TRANSPORT "GT"   /*  both GUARD and TRANSPORT option  */


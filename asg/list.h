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


/* file list.h */
/* ------------------------------------------------------------------------
 * List processing facilities based on spl place & route routines (1/90 stf)
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*---- --- for use with these definitions of TRUE and FALSE -------------- */
#define NOT(x) (((x) == FALSE) ? TRUE : FALSE)

#ifdef TCL_WRAPPER
  #define malloc Tcl_Alloc
  #define calloc(a,b) my_calloc(a, b)			/* see utility.c */
  #define free(a) Tcl_Free((char *)(a))
  #define realloc(a,b) Tcl_Realloc((char *)(a), b)

  #define fprintf tcl_printf			/* defined by xcircuit */
  #define fflush tcl_stdflush
#endif

/*------------------------------------------------------------------------
 * These list processing functions are based on the following two
 * data structures:
 *------------------------------------------------------------------------*/
typedef struct list_struct list;
typedef struct indexed_list ilist;

struct list_struct
{
    int *this;
    list *next;
};

struct indexed_list
{
    int index;
    int *this;
    ilist *next;
};

/*------------------------------------------------------------------------ */
/* NOTE:  It is highly advisable to initialize ALL list pointers to NULL,
 * 	  Otherwise, 'random' values that point off into undesirable sections
 *        of memory may be become "->next" pointers.  THIS IS A BAD THING. */
/*------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------
 * concat_list: append a generic thing to the head of a list.
 * works for null lists.
 * ------------------------------------------------------------------------
 */

extern list *concat_list();
/*    int *e;
 *    list *l;   */

/* ------------------------------------------------------------------------
 * append_list: append two generic lists together. (<l2> is attached to the
 * end of <l1>.  If both lists are null, it returns null.
 * ------------------------------------------------------------------------
 */

extern list *append_list();
/*    list *l1, *l2;  */

/*---------------------------------------------------------------
 * remove_list_element
 * cut out a generic list element from the middle of a list
 * note the parameter must be the address of the real thing, not
 * a copy. so this should be called with the address of a 
 * subfield ie remove_list_element(&(thing->next))
 * both remove_list_element(thing->next)
 * and remove_list_element(&thing) (unless thing is global) will not work.
 *---------------------------------------------------------------
 */
extern void remove_list_element();
/*    list **lptr; */

/*--------------------------------------------------------------------------
 * delete_if
 * Clip all of the elements of the given list who's ->this field return TRUE 
 * to the given <testFn>. (Recursive)
 *--------------------------------------------------------------------------
 */
extern list *delete_if();
/*    list **lptr;	 List to work on. */
/*    int (*testFn)();   Should take a single *int as its arg, return TRUE/FALSE */

/* ------------------------------------------------------------------------
 * pushnew - add a unique element to the front of a linked list
 * ------------------------------------------------------------------------
 */
extern void pushnew();
/*    int *what;
 *    list **l; */

/* ------------------------------------------------------------------------
 * push - add to the front of a linked list
 * ------------------------------------------------------------------------
 */
extern void push();
/*    int *what;
 *    list **l; */

/* ------------------------------------------------------------------------
 * pop - (FILO) pop from the front of a linked list
 * ------------------------------------------------------------------------
 */
extern int *pop();
/*     list **l; */

/* ------------------------------------------------------------------------
 * queue - add to the back of a linked list
 * ------------------------------------------------------------------------
 */
extern void queue();
/*    int *what;
 *    list **l; */

/* ------------------------------------------------------------------------
 * trans_item - pull from an unknown position within the first list, push 
 *              onto second list
 * ------------------------------------------------------------------------
 */
extern int *trans_item();
/*    int *what;
 *    list **sList; 
 *    list **dList; */

/* ------------------------------------------------------------------------
 * rem_item - pull from an unknown position within the list
 * ------------------------------------------------------------------------
 */
extern int *rem_item();
/*    int *what;
 *    list **l; */

/* ------------------------------------------------------------------------
 * repl_item - pull from an unknown position within the list, replace with 
 * 	       given element.
 * ------------------------------------------------------------------------
 */
extern int *repl_item();
/*    int *what, *new;
 *    list *l; */

/* ------------------------------------------------------------------------
 * find_item - see if the item is in the list
 * ------------------------------------------------------------------------
 */
extern list *find_item();
/*    int *what;
 *    list *l; */
    
/* ------------------------------------------------------------------------
 * nth - Return the Nth element of the list <l> given that there are at
 *       least <n> elements on <l>.	
 * ------------------------------------------------------------------------
 */
extern int *nth();
/*    list *l;  */
/*    int n;    */

/* ------------------------------------------------------------------------
 * nth_cdr - Return the list pointer containing the Nth element of the 
 *       list <l> given that there are at least <n> elements on <l>.	
 * ------------------------------------------------------------------------
 */
extern list *nth_cdr();
/*    list *l;  */
/*    int n;    */

/* ------------------------------------------------------------------------
 * list_length
 * ------------------------------------------------------------------------
 */
extern int list_length();
/*    list *l; */

/* ------------------------------------------------------------------------
 * member_p - returns TRUE iff the given pointer returns TRUE when compared
 *            with the ->this slot of some list element.
 * ------------------------------------------------------------------------
 */
extern int member_p();
/*    int *e;		The element to look for */
/*    list *l;		The list to look in  */
/*    int (*testFn)();	The function to test by - <e> is the first arg, 
                        l-this second (expected to take two *int's as args, 
			return TRUE/FALSE */
/* ------------------------------------------------------------------------
 * member - returns list pointer to the first item that returns TRUE to 
 *          the testFn(e, ll->this) query.  Returns NULL otherwise.
 * ------------------------------------------------------------------------
 */
extern list *member();
/*    int *e;		The element to look for */
/*    list *l;		The list to look in  */
/*    int (*testFn)();	The function to test by - <e> is the first arg, 
                        l-this second (expected to take two *int's as args, 
			return TRUE/FALSE */

/* ------------------------------------------------------------------------
 * rcopy_list: make a copy of a list, reversing it for speed/convience
 * ------------------------------------------------------------------------
 */
extern list *rcopy_list();
/*    list *l;  */

/* ------------------------------------------------------------------------
 * copy_list: make a copy of a list, preserving order
 * ------------------------------------------------------------------------
 */
extern list *copy_list();
/*    list *l; */

/* ------------------------------------------------------------------------
 * delete_duplicates  Modify the given list by deleting duplicate elements:
 * ------------------------------------------------------------------------
 */
extern list *delete_duplicates();
/*    list **l;         */
/*    int (*testFn)();  Should take two *int's as args, return TRUE/FALSE */

/* ------------------------------------------------------------------------
 * my_free	wipe this pointer from  memory.
 * ------------------------------------------------------------------------
 */
extern int *my_free();
/* int *p;  */

/* ------------------------------------------------------------------------
 * free_list	wipe this list from memory.  Do not disturb any list elements.
 * ------------------------------------------------------------------------
 */
extern list *free_list();
/*    list *l; */

/* ------------------------------------------------------------------------
 * sort_list (destructive) sort the given list via orderFn.
 * ------------------------------------------------------------------------
 */
list *sort_list();
/*    list *l;			*/
/*    int (*orderFn)();	   Should take two *int's as args, return TRUE/FALSE */
                        /* TRUE implies the first element belongs in front of the second */

/* ------------------------------------------------------------------------
 * merge_sort_list (destructive) merge sort the given list via orderFn.
 * ------------------------------------------------------------------------
 */
list *merge_sort_list();
/*    list **l;			*/
/*    int (*orderFn)();	   Should take two *int's as args, return TRUE/FALSE */
                        /* TRUE implies the first element belongs in front of the second */

/* ------------------------------------------------------------------------
 * slow_sort_list (destructive) sort the given list via orderFn.
 * ------------------------------------------------------------------------
 */
list *slow_sort_list();
/*    list *l;			*/
/*    int (*orderFn)();	   Should take two *int's as args, return TRUE/FALSE */
                        /* TRUE implies the first element belongs in front of the second */



/*====================================================================================
                       INDEXED LIST (ilist) FUNCTIONS:
 *====================================================================================*/
/* ------------------------------------------------------------------------
 * reverse_copy_ilist: make a copy of an indexed list, inverting the order 
 *                     while maintaining the original indexing
 * ------------------------------------------------------------------------
 */
extern ilist *reverse_copy_ilist();
/*    ilist *l; */
/* ------------------------------------------------------------------------
 * rcopy_ilist: make a copy of an indexed list, inverting order
 * ------------------------------------------------------------------------
 */
extern ilist *rcopy_ilist();
/*    ilist *l; */
/* ------------------------------------------------------------------------
 * copy_ilist: make a copy of an indexed list, preserving order
 * ------------------------------------------------------------------------
 */
extern ilist *copy_ilist();
/*    ilist *l; */
/* ------------------------------------------------------------------------
 * remove_indexed_element remove (by index) an element from a list structure
 * NOTE:  This renumbers the elements following the discovery.
 * ------------------------------------------------------------------------
 */
extern int *remove_indexed_element();
/*    int indx;
 *    ilist **lptr;  */

/* ------------------------------------------------------------------------
 * irem_item - pull from an unknown position within an indexed list
 *             (no renumbering).  This search is limited by non-NULL <howFar>.
 * ------------------------------------------------------------------------
 */
extern int *irem_item();
/*  int *what;
 *  ilist **l;
 *  int howFar;	 	If != NULL, places a limit on the search */

/* ------------------------------------------------------------------------
 * find_indexed_element finds (by index) an element from a list structure
 * NOTE:  This returns the list pointer to the requested element.
 * ------------------------------------------------------------------------
 */
extern ilist *find_indexed_element();
/*    int indx;
 *    ilist *lst;  */

/* ------------------------------------------------------------------------
 * indexed_list_insert  an element to an indexed list structure.
 * ------------------------------------------------------------------------
 */
extern int indexed_list_insert();
/*    int *element;
 *    int indx;
 *    ilist **lptr;  */

/* ------------------------------------------------------------------------
 * ordered_ilist_insert  an element to an indexed list structure.
 * Quite similar to "indexed_list_insert, saving that whenever an element having 
 * the same index value is encountered, rather than replacing the entry (as in
 * the aforementioned function) a new list element is created, and placed in front  
 * of the first such entry encountered.
 * ------------------------------------------------------------------------
 */
extern int ordered_ilist_insert();
/*    int *element;
 *    int indx;
 *    ilist **lptr;  */

/* ------------------------------------------------------------------------
 * ilist_length
 * ------------------------------------------------------------------------
 */
extern int ilist_length();
/*    ilist *l;  */

/* ------------------------------------------------------------------------
 * ipop
 * ------------------------------------------------------------------------
 */
extern int *ipop();
/*    ilist **l;  */

/* ------------------------------------------------------------------------
 * ipush
 * ------------------------------------------------------------------------
 */
extern void ipush();
/* int *what;  */
/* ilist **l;  */

/* ------------------------------------------------------------------------
 * append_ilist: append two generic indexed lists together.
 * Works for either list null.
 * If both lists are null, it returns null.
 * ------------------------------------------------------------------------
 */
extern ilist *append_ilist();
/*    ilist *l1, *l2;*/

/* ------------------------------------------------------------------------
 * free_ilist	wipe this list from memory.  Do not disturb any list elements.
 * ------------------------------------------------------------------------
 */
extern ilist *free_ilist();
/*	ilist *l;   */



/* ------------------------------------------------------------------------
 * END OF FILE
 * ------------------------------------------------------------------------
 */


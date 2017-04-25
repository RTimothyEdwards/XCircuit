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

/* file list.c */
/* ------------------------------------------------------------------------
 * Helper / Utility routines for place and route         spl 7/89, stf 2/90
 * ------------------------------------------------------------------------
 */
#include "list.h"

/*------------------------------------------------------------------------
 * The following list processing functions are based on the following two
 * data structures:
 typedef struct list_struct list;
 typedef struct indexed_list ilist;

     struct list_struct
     {
         int *this;
	 list *next;
     }
 
     struct indexed_list
     {
         int index;
	 int *this;
	 int ilist *next;
     }

 *------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------
 * concat_list: append a generic thing to the head of a list.
 * works for null lists.
 * ------------------------------------------------------------------------
 */

list *concat_list(e,l)
    int *e;
    list *l;
{
    list *p;
    
    p = (list *) calloc(1, sizeof(list));
    p->this = e;
    p->next = l;

    return(p);
}

/* ------------------------------------------------------------------------
 * append_list: append two generic lists together.
 * Works for either list null.
 * If both lists are null, it returns null.
 * ------------------------------------------------------------------------
 */

list *append_list(l1,l2)
    list *l1, *l2;
{
    list *p;
    if (l1 == NULL)
    {
	return (l2);
    }

    for(p = l1; p->next != NULL; p = p->next)
    {
	;
    }
    p->next = l2;
    
    return(l1);
}

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
void remove_list_element(lptr)
    list **lptr;
{
    list *trash;

    trash = *lptr;
    *lptr = trash->next;

    my_free(trash);	/* Trash freed, contents not touched. */

}
/*--------------------------------------------------------------------------
 * delete_if
 * Clip all of the elements of the given list who's ->this field return TRUE 
 * to the given <testFn>.  (Recursive implementation).
 *--------------------------------------------------------------------------
 */
list *delete_if(lptr, testFn)
    list **lptr;
    int (*testFn)();	/* Should take a single *int as its arg, return TRUE/FALSE */
{
    /* Remove all list elements that return TRUE to the <testFn> call. */
    list *ll = *lptr;
    if (*lptr == NULL) return(NULL);
    else if ((*testFn)(ll->this) == TRUE)	 /* The first list item should go */
    {
	remove_list_element(lptr);
	return(delete_if(lptr, testFn));
    }
	
    else 	/* Later list items should get nuked */
    {
	ll = *lptr;
	ll->next = delete_if(&ll->next, testFn);
	return(ll);
    }
}
/* ------------------------------------------------------------------------
 * pushnew - add a unique element to the front of a linked list
 * ------------------------------------------------------------------------
 */
void pushnew(what, l)
    int *what;
    list **l;
{
    list *ll;
    int flag = TRUE;
    
    for (ll = *l; ll != NULL; ll = ll->next)
    {
	if ((int)what == (int)ll->this) flag = FALSE;
    }
    if (flag == TRUE) push(what, l);
}

/* ------------------------------------------------------------------------
 * push - add to the front of a linked list
 * ------------------------------------------------------------------------
 */
void push(what, l)
    int *what;
    list **l;
{
    list *ll;
    if ((ll = (list *)calloc(1, sizeof(list))) == NULL)
    {
	fprintf(stderr,"\nNo more room (push)");	exit(0);
    }
    else
    {
	ll->this = what;
	ll->next = *l;
	*l = ll;
    }
}

/* ------------------------------------------------------------------------
 * pop - (FILO) pop from the front of a linked list
 * ------------------------------------------------------------------------
 */
int *pop(l)
    list **l;
{
    list *ll = *l;
    int * item = ll->this;
    *l = ll->next;
    my_free(ll);
    return(item);
}
/* ------------------------------------------------------------------------
 * queue - add to the back of a linked list
 * ------------------------------------------------------------------------
 */
void queue(what, l)
    int *what;
    list **l;
{
    list *q = *l;
    
    if (q == NULL) /* create the list */
    {
	q = (list *)calloc(1, sizeof(list));
	q->this = what;
	q->next = NULL;
	*l = q;
    }
    else
    {	/* find the end of <l>: */
	for(q = *l; q->next != NULL; q = q->next);
	queue (what, &q->next);
    }
}
/* ------------------------------------------------------------------------
 * trans_item - pull from an unknown position within the first list, push 
 *              onto second list
 * ------------------------------------------------------------------------
 */
extern int *trans_item(what, sList, dList)
    int *what;
    list **sList, **dList;
{
    list *r = *dList, *q = *sList;

    if (q == NULL) return (NULL);
    else if (q->this == what)	/* Front of <sList> */
    {				/* Make the transfer */
	*sList = q->next;			/* Pull from <sList> */
	q->next = *dList;		/* Push onto <dList> */
	*dList = q;
	return(what);
    }
    else
    {	/* find the end of <sList>: */
	for(q = *sList; ((q->next != NULL) && (q->next->this != what)); q = q->next);
	return (trans_item(what, &q->next, dList));
    }
}

/* ------------------------------------------------------------------------
 * rem_item - pull from an unknown position within the list
 * ------------------------------------------------------------------------
 */
int *rem_item(what, l)
    int *what;
    list **l;
{
    list *q = *l;

    if (q == NULL) return (NULL);
    else if (q->this == what)
    {
	*l = q->next;
	my_free(q);
	return(what);
    }
    else
    {	/* find the end of <l>: */
	for(q = *l; ((q->next != NULL) && (q->next->this != what)); q = q->next);
	return (rem_item(what, &q->next));
    }
}
/* ------------------------------------------------------------------------
 * repl_item - pull from an unknown position within the list, replace with 
 * 	       given element.
 * ------------------------------------------------------------------------
 */
int *repl_item(what, new, l)
    int *what, *new;
    list *l;
{
    list *q = l;

    if (q == NULL) return (NULL);
    else if (q->this == what)
    {
	q->this = new;
	return(what);
    }
    else
    {	/* find the end of <l>: */
	for(q = l; ((q->next != NULL) && (q->next->this != what)); q = q->next);
	return (repl_item(what, new, q->next));
    }
}
/* ------------------------------------------------------------------------
 * find_item - see if the item is in the list
 * ------------------------------------------------------------------------
 */
list *find_item(what, l)
    int *what;
    list *l;
{
    list *q;

    for (q = l; q != NULL; q = q->next)
    {
	if (q->this == what) return(q);
    }
    return(NULL);
}
    
/* ------------------------------------------------------------------------
 * nth - Note: Numbering starts at one.
 * ------------------------------------------------------------------------
 */
int *nth(l, n)
    list *l;
    int n;
{
    /* This returns the Nth element of the list <l> given that there are at
       least <n> elements on <l>.		*/
    int i = 1;
    for (; l != NULL; l = l->next)
    {
	if (i == n) return(l->this);
	i++;
    }
    return(NULL);
}
/* ------------------------------------------------------------------------
 * nth_cdr - Note: Numbering starts at one.
 * ------------------------------------------------------------------------
 */
list *nth_cdr(l, n)
    list *l;
    int n;
{
    /* This returns the Nth element of the list <l> given that there are at
       least <n> elements on <l>.		*/
    int i = 1;
    if (n == 0) return(l);		/* simple case... */

    for (; l != NULL; l = l->next)
    {
	if (i == n) return(l);
	i++;
    }
    return(NULL);
}

/* ------------------------------------------------------------------------
 * list_length
 * 
 * ------------------------------------------------------------------------
 */
int list_length(l)
    list *l;
{
    int i;

    for(i= 0; l != NULL; l = l->next)
    {
	i++;
    }
    return(i);
}

/* ------------------------------------------------------------------------
 * member_p - returns TRUE iff the given pointer returns TRUE when compared
 *            with the ->this slot of some list element.
 * ------------------------------------------------------------------------
 */
int member_p(e, l, testFn)
    int *e;
    list *l;
    int (*testFn)();	/* Should take two *int's as args, return TRUE/FALSE */
{
    list *ll;
    
    for(ll = l; ll != NULL; ll = ll->next)
    {
	if ((*testFn)(e, ll->this) == TRUE) return(TRUE);
    }
    return(FALSE);
}

/* ------------------------------------------------------------------------
 * member - returns list pointer to the first item that returns TRUE to 
 *          the testFn(e, ll->this) query.  Returns NULL otherwise.
 * ------------------------------------------------------------------------
 */
list *member(e, l, testFn)
    int *e;
    list *l;
    int (*testFn)();	/* Should take two *int's as args, return TRUE/FALSE */
{
    list *ll;
    
    for(ll = l; ll != NULL; ll = ll->next)
    {
	if ((*testFn)(e, ll->this) == TRUE) return(ll);
    }
    return(NULL);
}

/* ------------------------------------------------------------------------
 * rcopy_list: make a copy of a list, reversing it for speed/convience
 * ------------------------------------------------------------------------
 */

list *rcopy_list(l)
    list *l;
{
    list *ll, *p = NULL;

    for(ll = l; ll != NULL; ll = ll->next) 
    {
	push(ll->this, &p);
    }
    return(p);
}

/* ------------------------------------------------------------------------
 * copy_list: make a copy of a list, preserving order
 * ------------------------------------------------------------------------
 */
list *copy_list(l)
    list *l;
{
    list *p, *pp;

    if(l == NULL)
    {
	/* error: "copy_list passed null list" */
	return(NULL);
    }
    pp = p = (list *) calloc(1, sizeof(list));
    p->this = l->this;
    while(l = l->next)
    {
	p->next = (list *) calloc(1, sizeof(list));
	p = p->next;
	p->this = l->this;
    }
    p->next = NULL;
    return(pp);
}
/* ------------------------------------------------------------------------
 * delete_duplicates  Modify the given list by deleting duplicate elements:
 * ------------------------------------------------------------------------
 */
list *delete_duplicates(l, testFn)
    list **l;
    int (*testFn)();	/* Should take two *int's as args, return TRUE/FALSE */
{
    int *targ;
    list *p, *pp, *trash, *last;

    if (*l == NULL)
    {
	/* error: "delete_duplicates passed null list" */
	return(NULL);
    }
    
    for (p = *l; ((p != NULL) && (p->next != NULL)); p = p->next)
    {
	targ = p->this;
	last = p;
	for (pp = p->next; pp != NULL;)
	{
	    if (testFn(targ, pp->this) == TRUE)
	    {
		last->next = pp->next;
		trash = pp;
		pp = pp->next;
		my_free(trash);
	    }
	    else 
	    {
		last = pp;
		pp = pp->next;
	    }
	}
    }
    return(*l);
}
/* ------------------------------------------------------------------------
 * my_free	wipe this pointer from  memory.
 * ------------------------------------------------------------------------
 */
extern int *my_free(p)
    int *p; 
{
    int q = 1;		/* NOP */
    free(p);
    return(NULL);
}
/* ------------------------------------------------------------------------
 * free_list	wipe this list from memory.  Do not disturb any list elements.
 * ------------------------------------------------------------------------
 */
list *free_list(l)
    list *l;
{
    list *ll, *trash;
    int i;
    char *cptr;

    for(ll = l; ll != NULL;)
    {
	trash = ll;
	ll = ll->next;

	/* Explicitly wipe memory */		/* ??? */
	cptr = (char *)trash;
	for (i = 0; i < sizeof(list); i++)
	{
	    *(cptr + i) = 0;
	}

	my_free(trash);
    }
    return(NULL);
}
/* ------------------------------------------------------------------------
 * sort_list (destructive) sort the given list via orderFn:
 * ------------------------------------------------------------------------
 */
list *sort_list(l, orderFn)
    list *l;
    int (*orderFn)();	/* Should take two *int's as args, return TRUE/FALSE */
                        /* TRUE implies the first element belongs in front of the second */
{
    /* Bubble sort of */
    list *lp;
    int i, j , *temp, stop;
    int exchange, len = list_length(l);
    stop = len - 1;
    
    for (i = len; i > 1; i -= 1)	
    {
	lp = l;
	exchange = FALSE;
	
	for (j = 0; ((lp != NULL) && (lp->next != NULL) && (j < stop)); j += 1)	
	{
	    /* One bubbling pass - Compare lp->this with lp->next->this */
	    if (orderFn(lp->this, lp->next->this) == FALSE)
	    {
		/* Swap lp->this with lp->next->this */
		temp = lp->this;
		lp->this = lp->next->this;
		lp->next->this = temp;
		exchange = TRUE;
	    }
	    lp = lp->next;
	}
	if (exchange == FALSE) break;
	stop -= 1;
    }
    return(l);
}
/* ------------------------------------------------------------------------
 * merge_sort (destructive) sort the given list via orderFn:
 * ------------------------------------------------------------------------
 */
list *merge_sort(l1, l2, n, m, orderFn)
    list *l1, *l2;	/* Two lists to be merge-sorted. */
    int n, m;		/* number of elements from <l1>, <l2> to sort... */
    int (*orderFn)();	/* Ordering function.  Works on list elements */
{
    /* The merge sorter... */
    list *mid1, *mid2, *t1, *t2, *r, *result;
    int i, *temp;

    mid1 = mid2 = t1 = t2 = r = result = NULL;

    if (l1 == l2) result = l1;     		     /* Handle the n = 0 case... */

    else if ((n == 1) && (m == 0)) result = l1;
    else if ((m == 1) && (n == 0)) result = l2;

    else if ((n == 1) && (m == 1))		     /* Handle the n = 1 case... */
    {
	if (orderFn(l1->this, l2->this) == FALSE)
	{
	    /* Swap l1->this with l2->this */
	    temp = l1->this;
	    l1->this = l2->this;
	    l2->this = temp;
	}
	result = l1;
	result->next = l2;
    }

    else                                /* if (l1 != l2) Handle the general case: */
    {
	t1 = nth_cdr(l1, n/2);
	if (t1 != NULL)
	{
	    mid1 = t1->next;				    /* Split <l1> */
	    t1->next = NULL;			            /* Terminate the temp list */
	}

	t2 = nth_cdr(l2, m/2);
	if (t2 != NULL)
	{
	    mid2 = t2->next;				    /* Split <l2> */
	    t2->next = NULL;				    /* Terminate the temp list */
	}

	/* Recursive calls... */
	/* Sort the left sublist  */	
	if ((n == 0) || (n == 1))  t1 = l1;
	else t1 = merge_sort(l1, mid1, n/2, n - n/2, orderFn);   

	/* Sort the right sublist */
	if ((m == 0) || (m == 1)) t2 = l2;
	else t2 = merge_sort(l2, mid2, m/2, m - m/2, orderFn);   

	/* Check for errors: */
	if ((t1 == NULL) && (t2 == NULL)) return(NULL);	             /* Big Problem!! */

	/* Merge the two sorted sublists, <t1> & <t2>... */
	for(i = n+m; i != 0; i--)		/* Merge all of the elements given... */
	{
	    if ((t2 != NULL) && 
		((t1 == NULL) || (orderFn(t1->this, t2->this) == FALSE)))
	    {
		/* The first element of <t2> gets added to the <result> list. */
		if (result != NULL) 
		{ 
		    r->next = t2;
		    t2 = t2->next;
		    r->next->next = NULL;
		    r = r->next;
		}
		else 
		{
		    result = t2;
		    t2 = t2->next;
		    result->next = NULL;
		    r = result;
		}
	    }
	    else /* ((t2 == NULL) || (orderFn(t1->this, t2->this) != FALSE)) */
	    {
		/* The first element of <t1> gets added to the <result> list */
		if (result != NULL) 
		{
		    r->next = t1;
		    t1 = t1->next;
		    r->next->next = NULL;
		    r = r->next;
		}
		else 
		{
		    result = t1;
		    t1 = t1->next;
		    result->next = NULL;
		    r = result;
		}
	    }
	}
    }
    return(result);
}
/* ------------------------------------------------------------------------
 * merge_sort_list (destructive) sort the given list via orderFn:
 * ------------------------------------------------------------------------
 */
list *merge_sort_list(l, orderFn)
    list **l;
    int (*orderFn)(); /* Should take two *int's as args, return TRUE/FALSE */
                      /* TRUE implies the first element belongs in front of the second */
{
    return(*l = merge_sort(*l, NULL, list_length(*l), 0, orderFn));
}


/* ------------------------------------------------------------------------
 * slow_sort_list (destructive) sort the given list via orderFn:
 * ------------------------------------------------------------------------
 */
list *slow_sort_list(l, orderFn)
    list *l;
    int (*orderFn)(); /* Should take two *int's as args, return TRUE/FALSE */
                      /* TRUE implies the first element belongs in front of the second */
{
    /* The slow sorter... */
    list *lp, *lpp;
    int *temp;
    
    for (lp = l; ((lp != NULL) && (lp->next != NULL)); lp = lp->next)
    {
	for (lpp = lp; lpp != NULL; lpp = lpp->next)
	{
	    if (orderFn(lp->this, lpp->this) == FALSE)
	    {
		/* Swap lp->this with lpp->this */
		temp = lp->this;
		lp->this = lpp->this;
		lpp->this = temp;
	    }
	}
    }
    return(l);
}
/* ------------------------------------------------------------------------
 * reverse_ilist: make a copy of an indexed list, inverting order while
 *                maintaining the original indexing
 * ------------------------------------------------------------------------
 */
ilist *reverse_copy_ilist(l)
    ilist *l;
{
    ilist *p, *pp = NULL;

    if(l == NULL)
    {
	/* error: "reverse_copy_ilist passed null list" */
	return(NULL);
    }

    pp = p = (ilist *) calloc(1, sizeof(ilist));
    pp->this = l->this;
    pp->index = l->index;
    pp->next = NULL;

    while(l = l->next)
    {
	p = (ilist *) calloc(1, sizeof(ilist));
	p->next = pp;
	p->this = l->this;
	p->index = l->index;
	pp = p;
    }
    return(p);
}
/* ------------------------------------------------------------------------
 * rcopy_ilist: make a copy of an indexed list, inverting order
 * ------------------------------------------------------------------------
 */
ilist *rcopy_ilist(l)
    ilist *l;
{
    ilist *p, *pp = NULL;

    if(l == NULL)
    {
	/* error: "rcopy_ilist passed null list" */
	return(NULL);
    }

    for (p = l; p != NULL; p = p->next)
    {
	ipush(p->this, &pp);
    }
    return(pp);
}
/* ------------------------------------------------------------------------
 * copy_ilist: make a copy of an indexed list, preserving order
 * ------------------------------------------------------------------------
 */
ilist *copy_ilist(l)
    ilist *l;
{
    ilist *p, *pp;

    if(l == NULL)
    {
	/* error: "copy_ilist passed null list" */
	return(NULL);
    }
    pp = p = (ilist *) calloc(1, sizeof(ilist));
    p->this = l->this;
    p->index = l->index;
    while(l = l->next)
    {
	p->next = (ilist *) calloc(1, sizeof(ilist));
	p = p->next;
	p->this = l->this;
	p->index = l->index;
    }
    p->next = NULL;
    return(pp);
}

/* ------------------------------------------------------------------------
 * remove_indexed_element remove (by index) an element from a list structure
 * NOTE:  This renumbers the elements following the discovery.

             <<<<HAS PROBLEMS>>>>
 * ------------------------------------------------------------------------
 */
int *remove_indexed_element(indx, lptr)
    int indx;
    ilist **lptr;
{
    ilist *l, *lst;
    int *temp = NULL;

    lst = *lptr;
    
    if((lst == NULL)||(lst->index > indx)||((lst->index != indx) && (lst->next == NULL))) 
       return (NULL);
    else
    {
        for(l = lst; ; l = l->next)
       {
            if(l->index == indx)
            {
                /* we already have a list at this index */
		if (l!=lst) lst->next = l -> next; 	/* free trash? */
		else *lptr = l->next;			/* pop off top of list */
                temp = l->this;
            }
            
            /* renumber & assume it's there */
            else if (l->index > indx)  --l->index;  

            /* quit once the list is exausted */            
            if ((l->next == NULL) && (temp)) return(temp);
            else if (l->next == NULL) 
            { /* renumber the list and return null */
                for (l = *lptr; ; l = l->next)
                {
                    if (l->next == NULL) return(temp);
                    else if (l->index > indx) ++l->index;
                }
            }
            lst = l;
        }
    }
}
/* ------------------------------------------------------------------------
 * irem_item - pull from an unknown position within an indexed list
 *             (no renumbering)
 * ------------------------------------------------------------------------
 */
int *irem_item(what, l, howFar)
    int *what;
    ilist **l;
    int howFar;		/* If != NULL, places a limit on the search */    
{
    ilist *q = *l;

    if ((q == NULL) || ((howFar != NULL) && (q->index > howFar))) return (NULL);
    else if (what ==  q->this)		/* Found the little Commie! */
    {
	*l = q->next;	/* Note: No renumbering occurs */
	my_free(q);	/* Nuke it! */
	return(what);
    }
    else
    {	/* find the end of <l>: */
	for(q = *l; ((q->next != NULL) && (q->next->this != what)); q = q->next);
	return (irem_item(what, &q->next, howFar));
    }
}
/* ------------------------------------------------------------------------
 * find_indexed_element finds (by index) an element from a list structure
 * note:  this returns the list pointer to the requested element.
 * ------------------------------------------------------------------------
 */
ilist *find_indexed_element(indx, lst)
    int indx;
    ilist *lst;
{
    ilist *l;

    if((lst == NULL)||(lst->index > indx)) return (NULL);
    
    else
    {
        for(l = lst; ; l = l->next)
       {
           if(l->index == indx) return(l);

           /* quit once the list is exausted */     
           if (l->next == NULL) return(NULL);
           
        }
    }
}
/* ------------------------------------------------------------------------
 * indexed_list_insert  an element to an indexed list structure.
 * ------------------------------------------------------------------------
 */
int indexed_list_insert(element, indx, lptr)
    int *element;
    int indx;
    ilist **lptr;
{
    ilist *lst = *lptr;
    ilist *tl, *l;
    
    if((lst == NULL)||(lst->index > indx))
    {
	tl = (ilist *)calloc(1, sizeof(ilist));
	tl->next = lst;
	tl->index = indx;
	tl->this = element;
	*lptr = tl;
	return (TRUE);
    }
    else
    {
	for(l = lst; ; l = l->next)
	{
	    if(l->index == indx)
	    {
		/* we already have a list at this index */
		l->this = element;	/* replace the contents with the new stuff */
		return(TRUE);
	    }
	    if ( (l->next == NULL) || (l->next->index > indx))
	    {/* note we are counting on order of above exp */
		
		/* we hit the end or fall between two existing lists */
		tl = (ilist *)calloc(1, sizeof(ilist));
		tl->index = indx;
		tl->this = element;
		tl->next = l->next;
		l->next = tl;
		return(TRUE);
	    }
	}
    }
}
/* ------------------------------------------------------------------------
 * ordered_ilist_insert  an element to an indexed list structure.
 * quite similar to "indexed_list_insert, saving that whenever an element having 
 * the same index value is encountered, rather than replacing the entry (as in
 * the aforementioned function) a new list element is created, and placed in front  
 * of the first such entry encountered.
 * ------------------------------------------------------------------------
 */
int ordered_ilist_insert(element, indx, lptr)
    int *element;
    int indx;
    ilist **lptr;
{
    ilist *lst = *lptr;
    ilist *tl, *l;
    
    if((lst == NULL)||(lst->index > indx))
    {
	tl = (ilist *)calloc(1, sizeof(ilist));
	tl->next = lst;
	tl->index = indx;
	tl->this = element;
	*lptr = tl;
	return (TRUE);
    }
    else
    {
	for(l = lst; ; l = l->next)
	{
	    if ( (l->next == NULL) || (l->next->index >= indx))
	    {/* note we are counting on order of above exp */
		
		/* we hit the end or fall between two existing lists */
		tl = (ilist *)calloc(1, sizeof(ilist));
		tl->index = indx;
		tl->this = element;
		tl->next = l->next;
		l->next = tl;
		return(TRUE);
	    }
	}
    }
}
/* ------------------------------------------------------------------------
 * ilist_length
 * 
 * ------------------------------------------------------------------------
 */
int ilist_length(l)
    ilist *l;
{
    int i;

    for(i= 0; l != NULL; l = l->next)
    {
	i++;
    }
    return(i);
}
/* ------------------------------------------------------------------------
 * ipop - (FILO) pop from the front of a indexed linked list (No renumbering)
 * ------------------------------------------------------------------------
 */
int *ipop(l)
    ilist **l;
{
    ilist *ll = *l;
    int *item = ll->this;
    *l = ll->next;
    my_free(ll);
    return(item);
}
/* ------------------------------------------------------------------------
 * ipush - (FILO) push to the front of a indexed linked list (renumbering)
 * ------------------------------------------------------------------------
 */
void ipush(what, l)
    int *what;
    ilist **l;
{
    ilist *ll;
    if ((ll = (ilist *)calloc(1, sizeof(ilist))) == NULL)
    {
	fprintf(stderr,"\nNo more room (ipush)");	exit(0);
    }
    else
    {
	ll->this = what;
	ll->next = *l;
	ll->index = 0;
	*l = ll;
    }
    for (ll = ll->next; ll != NULL; ll = ll->next)
    {
	/* Walk down the rest of the list and renumber */
	ll->index += 1;
    }
    
}
/* ------------------------------------------------------------------------
 * append_ilist: append two generic indexed lists together.
 * Works for either list null.
 * If both lists are null, it returns null.
 * ------------------------------------------------------------------------
 */

ilist *append_ilist(l1,l2)
    ilist *l1, *l2;
{
    ilist *p;
    int i;
    if (l1 == NULL)
    {
	return (l2);
    }

    for(p = l1; p->next != NULL; p = p->next)
    {
	i = p->index;
    }
    p->next = l2;
    i++;
    for (p = l2; p->next != NULL; p = p->next)
    {
	p->index = ++i;
    }
    
    return(l1);
}
/* ------------------------------------------------------------------------
 * free_ilist	wipe this list from memory.  Do not disturb any list elements.
 * ------------------------------------------------------------------------
 */
ilist *free_ilist(l)
    ilist *l;
{
    ilist *ll, *trash;
    
    for(ll = l; ll != NULL;)
    {
	trash = ll;
	ll = ll->next;
	my_free(trash);
    }
    return(NULL);
}
/* ------------------------------------------------------------------------
 * END OF FILE
 * ------------------------------------------------------------------------
 */


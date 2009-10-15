/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5B2int.c
 *			Feb 27 2006
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Internal routines for managing v2 B-trees.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5B2_PACKAGE		/*suppress error about including H5B2pkg  */

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5B2pkg.h"		/* v2 B-trees				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5Vprivate.h"		/* Vectors and arrays 			*/

/****************/
/* Local Macros */
/****************/

/* Number of records that fit into leaf node */
#define H5B2_NUM_LEAF_REC(n, r) \
    (((n) - H5B2_LEAF_PREFIX_SIZE) / (r))

/* Uncomment this macro to enable extra sanity checking */
/* #define H5B2_DEBUG */

/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage B-tree node pages to/from disk */
H5FL_BLK_DEFINE_STATIC(node_page);

/* Declare a free list to manage the 'size_t' sequence information */
H5FL_SEQ_DEFINE_STATIC(size_t);

/* Declare a free list to manage the 'H5B2_node_info_t' sequence information */
H5FL_SEQ_DEFINE(H5B2_node_info_t);



/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_init
 *
 * Purpose:	Allocate & initialize B-tree header info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  2 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_init(H5F_t *f, H5B2_t *bt2, const H5B2_class_t *type,
    unsigned depth, size_t node_size, size_t rrec_size,
    unsigned split_percent, unsigned merge_percent)
{
    size_t sz_max_nrec;                 /* Temporary variable for range checking */
    unsigned u_max_nrec_size;           /* Temporary variable for range checking */
    unsigned u;                         /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B2_hdr_init)

    /* Initialize basic information */
    bt2->f = f;
    bt2->rc = 0;

    /* Assign dynamic information */
    bt2->depth = depth;

    /* Assign user's information */
    bt2->split_percent = split_percent;
    bt2->merge_percent = merge_percent;
    bt2->node_size = node_size;
    bt2->rrec_size = rrec_size;

    /* Assign common type information */
    bt2->type = type;

    /* Allocate "page" for node I/O */
    if(NULL == (bt2->page = H5FL_BLK_MALLOC(node_page, bt2->node_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
#ifdef H5_CLEAR_MEMORY
HDmemset(bt2->page, 0, bt2->node_size);
#endif /* H5_CLEAR_MEMORY */

    /* Allocate array of node info structs */
    if(NULL == (bt2->node_info = H5FL_SEQ_MALLOC(H5B2_node_info_t, (size_t)(bt2->depth + 1))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize leaf node info */
    sz_max_nrec = H5B2_NUM_LEAF_REC(bt2->node_size, bt2->rrec_size);
    H5_ASSIGN_OVERFLOW(/* To: */ bt2->node_info[0].max_nrec, /* From: */ sz_max_nrec, /* From: */ size_t, /* To: */ unsigned)
    bt2->node_info[0].split_nrec = (bt2->node_info[0].max_nrec * bt2->split_percent) / 100;
    bt2->node_info[0].merge_nrec = (bt2->node_info[0].max_nrec * bt2->merge_percent) / 100;
    bt2->node_info[0].cum_max_nrec = bt2->node_info[0].max_nrec;
    bt2->node_info[0].cum_max_nrec_size = 0;
    if(NULL == (bt2->node_info[0].nat_rec_fac = H5FL_fac_init(type->nrec_size * bt2->node_info[0].max_nrec)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create node native key block factory")
    bt2->node_info[0].node_ptr_fac = NULL;

    /* Allocate array of pointers to internal node native keys */
    /* (uses leaf # of records because its the largest) */
    if(NULL == (bt2->nat_off = H5FL_SEQ_MALLOC(size_t, (size_t)bt2->node_info[0].max_nrec)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize offsets in native key block */
    /* (uses leaf # of records because its the largest) */
    for(u = 0; u < bt2->node_info[0].max_nrec; u++)
        bt2->nat_off[u] = type->nrec_size * u;

    /* Compute size to store # of records in each node */
    /* (uses leaf # of records because its the largest) */
    u_max_nrec_size = H5V_limit_enc_size((uint64_t)bt2->node_info[0].max_nrec);
    H5_ASSIGN_OVERFLOW(/* To: */ bt2->max_nrec_size, /* From: */ u_max_nrec_size, /* From: */ unsigned, /* To: */ uint8_t)
    HDassert(bt2->max_nrec_size <= H5B2_SIZEOF_RECORDS_PER_NODE);

    /* Initialize internal node info */
    if(depth > 0) {
        for(u = 1; u < (depth + 1); u++) {
            sz_max_nrec = H5B2_NUM_INT_REC(f, bt2, u);
            H5_ASSIGN_OVERFLOW(/* To: */ bt2->node_info[u].max_nrec, /* From: */ sz_max_nrec, /* From: */ size_t, /* To: */ unsigned)
            HDassert(bt2->node_info[u].max_nrec <= bt2->node_info[u - 1].max_nrec);

            bt2->node_info[u].split_nrec = (bt2->node_info[u].max_nrec * bt2->split_percent) / 100;
            bt2->node_info[u].merge_nrec = (bt2->node_info[u].max_nrec * bt2->merge_percent) / 100;

            bt2->node_info[u].cum_max_nrec = ((bt2->node_info[u].max_nrec + 1) *
                bt2->node_info[u - 1].cum_max_nrec) + bt2->node_info[u].max_nrec;
            u_max_nrec_size = H5V_limit_enc_size((uint64_t)bt2->node_info[u].cum_max_nrec);
            H5_ASSIGN_OVERFLOW(/* To: */ bt2->node_info[u].cum_max_nrec_size, /* From: */ u_max_nrec_size, /* From: */ unsigned, /* To: */ uint8_t)

            if(NULL == (bt2->node_info[u].nat_rec_fac = H5FL_fac_init(bt2->type->nrec_size * bt2->node_info[u].max_nrec)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create node native key block factory")
            if(NULL == (bt2->node_info[u].node_ptr_fac = H5FL_fac_init(sizeof(H5B2_node_ptr_t) * (bt2->node_info[u].max_nrec + 1))))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTINIT, FAIL, "can't create internal 'branch' node node pointer block factory")
        } /* end for */
    } /* end if */

done:
    if(ret_value < 0)
        if(H5B2_hdr_free(bt2) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_CANTFREE, FAIL, "unable to free shared v2 B-tree info")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_init() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_free
 *
 * Purpose:	Free B-tree header info
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  2 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_free(H5B2_t *bt2)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5B2_hdr_free)

    /* Sanity check */
    HDassert(bt2);

    /* Free the B-tree node buffer */
    if(bt2->page)
        (void)H5FL_BLK_FREE(node_page, bt2->page);

    /* Free the array of offsets into the native key block */
    if(bt2->nat_off)
        bt2->nat_off = H5FL_SEQ_FREE(size_t, bt2->nat_off);

    /* Release the node info */
    if(bt2->node_info) {
        unsigned u;             /* Local index variable */

        /* Destroy free list factories */
        for(u = 0; u < (bt2->depth + 1); u++) {
            if(bt2->node_info[u].nat_rec_fac)
                if(H5FL_fac_term(bt2->node_info[u].nat_rec_fac) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy node's native record block factory")
            if(bt2->node_info[u].node_ptr_fac)
                if(H5FL_fac_term(bt2->node_info[u].node_ptr_fac) < 0)
                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTRELEASE, FAIL, "can't destroy node's node pointer block factory")
        } /* end for */

        /* Free the array of node info structs */
        bt2->node_info = H5FL_SEQ_FREE(H5B2_node_info_t, bt2->node_info);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_free() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_incr
 *
 * Purpose:	Increment reference count on B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 13 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_incr(H5B2_t *bt2)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B2_hdr_incr)

    /* Sanity checks */
    HDassert(bt2);
    HDassert(bt2->f);

    /* Mark header as un-evictable when a B-tree node is depending on it */
    if(bt2->rc == 0)
        if(H5AC_pin_protected_entry(bt2->f, bt2) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTPIN, FAIL, "unable to pin v2 B-tree header")

    /* Increment reference count on B-tree header */
    bt2->rc++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_incr_hdr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_decr
 *
 * Purpose:	Decrement reference count on B-tree header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 13 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_decr(H5B2_t *bt2)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B2_hdr_decr)

    /* Sanity check */
    HDassert(bt2);
    HDassert(bt2->f);
    HDassert(bt2->rc > 0);

    /* Decrement reference count on B-tree header */
    bt2->rc--;

    /* Mark header as evictable again when no nodes depend on it */
    if(bt2->rc == 0)
        if(H5AC_unpin_entry(bt2->f, bt2) < 0)
            HGOTO_ERROR(H5E_BTREE, H5E_CANTUNPIN, FAIL, "unable to unpin v2 B-tree header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5B2_hdr_dirty
 *
 * Purpose:	Mark B-tree header as dirty
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Oct 13 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2_hdr_dirty(H5B2_t *bt2)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5B2_hdr_dirty)

    /* Sanity check */
    HDassert(bt2);
    HDassert(bt2->f);

    /* Mark B-tree header as dirty in cache */
    if(H5AC_mark_pinned_or_protected_entry_dirty(bt2->f, bt2) < 0)
        HGOTO_ERROR(H5E_BTREE, H5E_CANTMARKDIRTY, FAIL, "unable to mark v2 B-tree header as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2_hdr_dirty() */

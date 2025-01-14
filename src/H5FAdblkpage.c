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
 * Created:		H5FAdblkpage.c
 *
 * Purpose:		Data block page routines for fixed array.
 *
 *-------------------------------------------------------------------------
 */

/**********************/
/* Module Declaration */
/**********************/

#define H5FA_MODULE


/***********************/
/* Other Packages Used */
/***********************/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FApkg.h"		/* Fixed Arrays				*/
#include "H5FLprivate.h"	/* Free Lists                           */


/****************/
/* Local Macros */
/****************/


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

/* Declare a free list to manage the H5FA_dblk_page_t struct */
H5FL_DEFINE_STATIC(H5FA_dblk_page_t);

/* Declare a free list to manage the page elements */
H5FL_BLK_DEFINE(page_elmts);



/*-------------------------------------------------------------------------
 * Function:	H5FA__dblk_page_alloc
 *
 * Purpose:	Allocate fixed array data block page
 *
 * Return:	Non-NULL pointer to data block on success/NULL on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
H5FA_dblk_page_t *, NULL, NULL,
H5FA__dblk_page_alloc(H5FA_hdr_t *hdr, size_t nelmts))

    /* Local variables */
    H5FA_dblk_page_t *dblk_page = NULL;          /* Fixed array data block page */

    /* Check arguments */
    HDassert(hdr);

    /* Allocate memory for the data block */
    if(NULL == (dblk_page = H5FL_CALLOC(H5FA_dblk_page_t)))
	H5E_THROW(H5E_CANTALLOC, "memory allocation failed for fixed array data block page")

    /* Share common array information */
    if(H5FA__hdr_incr(hdr) < 0)
	H5E_THROW(H5E_CANTINC, "can't increment reference count on shared array header")
    dblk_page->hdr = hdr;

    /* Set non-zero internal fields */
    dblk_page->nelmts = nelmts;

    /* Allocate buffer for elements in data block page */
    if(NULL == (dblk_page->elmts = H5FL_BLK_MALLOC(page_elmts, nelmts * hdr->cparam.cls->nat_elmt_size)))
        H5E_THROW(H5E_CANTALLOC, "memory allocation failed for data block page element buffer")

    /* Set the return value */
    ret_value = dblk_page;

CATCH

    if(!ret_value)
        if(dblk_page && H5FA__dblk_page_dest(dblk_page) < 0)
            H5E_THROW(H5E_CANTFREE, "unable to destroy fixed array data block page")

END_FUNC(PKG)   /* end H5FA__dblk_page_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__dblk_page_create
 *
 * Purpose:	Creates a new fixed array data block page in the file
 *
 * Return:	Valid file address on success/HADDR_UNDEF on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__dblk_page_create(H5FA_hdr_t *hdr, hid_t dxpl_id, haddr_t addr, size_t nelmts))

    /* Local variables */
    H5FA_dblk_page_t *dblk_page = NULL;      /* Fixed array data block page */

#ifdef H5FA_DEBUG
HDfprintf(stderr, "%s: Called, addr = %a\n", FUNC, addr);
#endif /* H5FA_DEBUG */

    /* Sanity check */
    HDassert(hdr);

    /* Allocate the data block page */
    if(NULL == (dblk_page = H5FA__dblk_page_alloc(hdr, nelmts)))
	H5E_THROW(H5E_CANTALLOC, "memory allocation failed for fixed array data block page")

    /* Set info about data block page on disk */
    dblk_page->addr = addr;
    dblk_page->size = H5FA_DBLK_PAGE_SIZE(hdr, nelmts);
#ifdef H5FA_DEBUG
HDfprintf(stderr, "%s: dblk_page->size = %Zu\n", FUNC, dblk_page->size);
#endif /* H5FA_DEBUG */

    /* Clear any elements in data block page to fill value */
    if((hdr->cparam.cls->fill)(dblk_page->elmts, nelmts) < 0)
        H5E_THROW(H5E_CANTSET, "can't set fixed array data block page elements to class's fill value")

    /* Cache the new fixed array data block page */
    if(H5AC_insert_entry(hdr->f, dxpl_id, H5AC_FARRAY_DBLK_PAGE, dblk_page->addr, dblk_page, H5AC__NO_FLAGS_SET) < 0)
	H5E_THROW(H5E_CANTINSERT, "can't add fixed array data block page to cache")

CATCH
    if(ret_value < 0)
        if(dblk_page) {
            /* Destroy data block page */
            if(H5FA__dblk_page_dest(dblk_page) < 0)
                H5E_THROW(H5E_CANTFREE, "unable to destroy fixed array data block page")
        } /* end if */

END_FUNC(PKG)   /* end H5FA__dblk_page_create() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__dblk_page_protect
 *
 * Purpose:	Convenience wrapper around protecting fixed array data
 *              block page
 *
 * Return:	Non-NULL pointer to data block page on success/NULL on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
H5FA_dblk_page_t *, NULL, NULL,
H5FA__dblk_page_protect(H5FA_hdr_t *hdr, hid_t dxpl_id, haddr_t dblk_page_addr,
    size_t dblk_page_nelmts, unsigned flags))

    /* Local variables */
    H5FA_dblk_page_cache_ud_t udata;      /* Information needed for loading data block page */

#ifdef H5FA_DEBUG
HDfprintf(stderr, "%s: Called\n", FUNC);
#endif /* H5FA_DEBUG */

    /* Sanity check */
    HDassert(hdr);
    HDassert(H5F_addr_defined(dblk_page_addr));

    /* only the H5AC__READ_ONLY_FLAG is permitted */
    HDassert((flags & (unsigned)(~H5AC__READ_ONLY_FLAG)) == 0);

    /* Set up user data */
    udata.hdr = hdr;
    udata.nelmts = dblk_page_nelmts;
    udata.dblk_page_addr = dblk_page_addr;

    /* Protect the data block page */
    if(NULL == (ret_value = (H5FA_dblk_page_t *)H5AC_protect(hdr->f, dxpl_id, H5AC_FARRAY_DBLK_PAGE, dblk_page_addr, &udata, flags)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect fixed array data block page, address = %llu", (unsigned long long)dblk_page_addr)

CATCH

END_FUNC(PKG)   /* end H5FA__dblk_page_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__dblk_page_unprotect
 *
 * Purpose:	Convenience wrapper around unprotecting fixed array
 *              data block page
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__dblk_page_unprotect(H5FA_dblk_page_t *dblk_page, hid_t dxpl_id,
    unsigned cache_flags))

    /* Local variables */

#ifdef H5FA_DEBUG
HDfprintf(stderr, "%s: Called\n", FUNC);
#endif /* H5FA_DEBUG */

    /* Sanity check */
    HDassert(dblk_page);

    /* Unprotect the data block page */
    if(H5AC_unprotect(dblk_page->hdr->f, dxpl_id, H5AC_FARRAY_DBLK_PAGE, dblk_page->addr, dblk_page, cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to unprotect fixed array data block page, address = %llu", (unsigned long long)dblk_page->addr)

CATCH

END_FUNC(PKG)   /* end H5FA__dblk_page_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__dblk_page_dest
 *
 * Purpose:	Destroys a fixed array data block page in memory.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__dblk_page_dest(H5FA_dblk_page_t *dblk_page))

    /* Sanity check */
    HDassert(dblk_page);

    /* Check if header field has been initialized */
    if(dblk_page->hdr) {
        /* Check if buffer for data block page elements has been initialized */
        if(dblk_page->elmts) {
            /* Free buffer for data block page elements */
	    dblk_page->elmts = H5FL_BLK_FREE(page_elmts, dblk_page->elmts);
        } /* end if */

        /* Decrement reference count on shared info */
        if(H5FA__hdr_decr(dblk_page->hdr) < 0)
            H5E_THROW(H5E_CANTDEC, "can't decrement reference count on shared array header")
        dblk_page->hdr = NULL;
    } /* end if */

    /* Free the data block page itself */
    dblk_page = H5FL_FREE(H5FA_dblk_page_t, dblk_page);

CATCH

END_FUNC(PKG)   /* end H5FA__dblk_page_dest() */


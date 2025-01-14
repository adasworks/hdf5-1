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
 * Created:		H5FAhdr.c
 *
 * Purpose:		Array header routines for Fixed Array.
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
#include "H5FApkg.h"		/* Fixed Arrays			*/
#include "H5MFprivate.h"	/* File memory management		*/


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

/* Declare a free list to manage the H5FA_hdr_t struct */
H5FL_DEFINE_STATIC(H5FA_hdr_t);


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_alloc
 *
 * Purpose:	Allocate shared Fixed Array header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
H5FA_hdr_t *, NULL, NULL,
H5FA__hdr_alloc(H5F_t *f))

    /* Local variables */
    H5FA_hdr_t *hdr = NULL;          /* Shared Fixed Array header */

    /* Check arguments */
    HDassert(f);

    /* Allocate space for the shared information */
    if(NULL == (hdr = H5FL_CALLOC(H5FA_hdr_t)))
	H5E_THROW(H5E_CANTALLOC, "memory allocation failed for Fixed Array shared header")

    /* Set non-zero internal fields */
    hdr->addr = HADDR_UNDEF;

    /* Set the internal parameters for the array */
    hdr->f = f;
    hdr->sizeof_addr = H5F_SIZEOF_ADDR(f);
    hdr->sizeof_size = H5F_SIZEOF_SIZE(f);

    /* Set the return value */
    ret_value = hdr;

CATCH
    if(!ret_value)
        if(hdr && H5FA__hdr_dest(hdr) < 0)
            H5E_THROW(H5E_CANTFREE, "unable to destroy fixed array header")

END_FUNC(PKG)   /* end H5FA__hdr_alloc() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_init
 *
 * Purpose:	Initialize shared fixed array header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Sunday, November 15, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__hdr_init(H5FA_hdr_t *hdr, void *ctx_udata))

    /* Local variables */

    /* Check arguments */
    HDassert(hdr);

    /* Set size of header on disk (locally and in statistics) */
    hdr->stats.hdr_size = hdr->size = H5FA_HEADER_SIZE_HDR(hdr);

    /* Set number of elements for Fixed Array in statistics */
    hdr->stats.nelmts = hdr->cparam.nelmts;

    /* Create the callback context, if there's one */
    if(hdr->cparam.cls->crt_context) {
        if(NULL == (hdr->cb_ctx = (*hdr->cparam.cls->crt_context)(ctx_udata)))
            H5E_THROW(H5E_CANTCREATE, "unable to create fixed array client callback context")
    } /* end if */

CATCH

END_FUNC(PKG)   /* end H5FA__hdr_init() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_create
 *
 * Purpose:	Creates a new Fixed Array header in the file
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
haddr_t, HADDR_UNDEF, HADDR_UNDEF,
H5FA__hdr_create(H5F_t *f, hid_t dxpl_id, const H5FA_create_t *cparam,
    void *ctx_udata))

    /* Local variables */
    H5FA_hdr_t *hdr = NULL;     /* Fixed array header */

#ifdef H5FA_DEBUG
HDfprintf(stderr, "%s: Called\n", FUNC);
#endif /* H5FA_DEBUG */

    /* Check arguments */
    HDassert(f);
    HDassert(cparam);

#ifndef NDEBUG
{
    /* Check for valid parameters */
    if(cparam->raw_elmt_size == 0)
	H5E_THROW(H5E_BADVALUE, "element size must be greater than zero")
    if(cparam->max_dblk_page_nelmts_bits == 0)
	H5E_THROW(H5E_BADVALUE, "max. # of elements bits must be greater than zero")
    if(cparam->nelmts == 0)
	H5E_THROW(H5E_BADVALUE, "# of elements must be greater than zero")
}
#endif /* NDEBUG */

    /* Allocate space for the shared information */
    if(NULL == (hdr = H5FA__hdr_alloc(f)))
	H5E_THROW(H5E_CANTALLOC, "memory allocation failed for Fixed Array shared header")

    hdr->dblk_addr = HADDR_UNDEF;

    /* Set the creation parameters for the array */
    HDmemcpy(&hdr->cparam, cparam, sizeof(hdr->cparam));

    /* Finish initializing fixed array header */
    if(H5FA__hdr_init(hdr, ctx_udata) < 0)
	H5E_THROW(H5E_CANTINIT, "initialization failed for fixed array header")

    /* Allocate space for the header on disk */
    if(HADDR_UNDEF == (hdr->addr = H5MF_alloc(f, H5FD_MEM_FARRAY_HDR, dxpl_id, (hsize_t)hdr->size)))
	H5E_THROW(H5E_CANTALLOC, "file allocation failed for Fixed Array header")

    /* Cache the new Fixed Array header */
    if(H5AC_insert_entry(f, dxpl_id, H5AC_FARRAY_HDR, hdr->addr, hdr, H5AC__NO_FLAGS_SET) < 0)
	H5E_THROW(H5E_CANTINSERT, "can't add fixed array header to cache")

    /* Set address of array header to return */
    ret_value = hdr->addr;

CATCH
    if(!H5F_addr_defined(ret_value))
        if(hdr) {
            /* Release header's disk space */
            if(H5F_addr_defined(hdr->addr) && H5MF_xfree(f, H5FD_MEM_FARRAY_HDR, dxpl_id, hdr->addr, (hsize_t)hdr->size) < 0)
                H5E_THROW(H5E_CANTFREE, "unable to free Fixed Array header")

            /* Destroy header */
            if(H5FA__hdr_dest(hdr) < 0)
                H5E_THROW(H5E_CANTFREE, "unable to destroy Fixed Array header")
        } /* end if */

END_FUNC(PKG)   /* end H5FA__hdr_create() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_incr
 *
 * Purpose:	Increment component reference count on shared array header
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
H5FA__hdr_incr(H5FA_hdr_t *hdr))

    /* Sanity check */
    HDassert(hdr);

    /* Mark header as un-evictable when something is depending on it */
    if(hdr->rc == 0)
        if(H5AC_pin_protected_entry(hdr) < 0)
            H5E_THROW(H5E_CANTPIN, "unable to pin fixed array header")

    /* Increment reference count on shared header */
    hdr->rc++;

CATCH

END_FUNC(PKG)   /* end H5FA__hdr_incr() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_decr
 *
 * Purpose:	Decrement component reference count on shared array header
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
H5FA__hdr_decr(H5FA_hdr_t *hdr))

    /* Sanity check */
    HDassert(hdr);
    HDassert(hdr->rc);

    /* Decrement reference count on shared header */
    hdr->rc--;

    /* Mark header as evictable again when nothing depend on it */
    if(hdr->rc == 0) {
        HDassert(hdr->file_rc == 0);
        if(H5AC_unpin_entry(hdr) < 0)
            H5E_THROW(H5E_CANTUNPIN, "unable to unpin fixed array header")
    } /* end if */

CATCH

END_FUNC(PKG)   /* end H5FA__hdr_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_fuse_incr
 *
 * Purpose:	Increment file reference count on shared array header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, NOERR,
herr_t, SUCCEED, -,
H5FA__hdr_fuse_incr(H5FA_hdr_t *hdr))

    /* Sanity check */
    HDassert(hdr);

    /* Increment file reference count on shared header */
    hdr->file_rc++;

END_FUNC(PKG)   /* end H5FA__hdr_fuse_incr() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_fuse_decr
 *
 * Purpose:	Decrement file reference count on shared array header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, NOERR,
size_t, 0, -,
H5FA__hdr_fuse_decr(H5FA_hdr_t *hdr))

    /* Sanity check */
    HDassert(hdr);
    HDassert(hdr->file_rc);

    /* Decrement file reference count on shared header */
    hdr->file_rc--;

    /* Set return value */
    ret_value = hdr->file_rc;

END_FUNC(PKG)   /* end H5FA__hdr_fuse_decr() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_modified
 *
 * Purpose:	Mark a fixed array as modified
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__hdr_modified(H5FA_hdr_t *hdr))

    /* Sanity check */
    HDassert(hdr);

    /* Mark header as dirty in cache */
    if(H5AC_mark_entry_dirty(hdr) < 0)
        H5E_THROW(H5E_CANTMARKDIRTY, "unable to mark fixed array header as dirty")

CATCH

END_FUNC(PKG)   /* end H5FA__hdr_modified() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_protect
 *
 * Purpose:	Convenience wrapper around protecting fixed array header
 *
 * Return:	Non-NULL pointer to index block on success/NULL on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug 12 2013
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
H5FA_hdr_t *, NULL, NULL,
H5FA__hdr_protect(H5F_t *f, hid_t dxpl_id, haddr_t fa_addr, void *ctx_udata,
    unsigned flags))

    /* Local variables */
    H5FA_hdr_cache_ud_t udata;  /* User data for cache callbacks */

    /* Sanity check */
    HDassert(f);
    HDassert(H5F_addr_defined(fa_addr));

    /* only the H5AC__READ_ONLY_FLAG is permitted */
    HDassert((flags & (unsigned)(~H5AC__READ_ONLY_FLAG)) == 0);

    /* Set up user data for cache callbacks */
    udata.f = f;
    udata.addr = fa_addr;
    udata.ctx_udata = ctx_udata;

    /* Protect the header */
    if(NULL == (ret_value = (H5FA_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_FARRAY_HDR, fa_addr, &udata, flags)))
        H5E_THROW(H5E_CANTPROTECT, "unable to protect fixed array header, address = %llu", (unsigned long long)fa_addr)

CATCH

END_FUNC(PKG)   /* end H5FA__hdr_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_unprotect
 *
 * Purpose:	Convenience wrapper around unprotecting fixed array header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Aug 12 2013
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__hdr_unprotect(H5FA_hdr_t *hdr, hid_t dxpl_id, unsigned cache_flags))

    /* Local variables */

    /* Sanity check */
    HDassert(hdr);

    /* Unprotect the header */
    if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_FARRAY_HDR, hdr->addr, hdr, cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to unprotect fixed array hdr, address = %llu", (unsigned long long)hdr->addr)

CATCH

END_FUNC(PKG)   /* end H5EA__hdr_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_delete
 *
 * Purpose:	Delete a fixed array, starting with the header
 *
 * Return:	SUCCEED/FAIL
 *
 * Programmer:	Vailin Choi
 *              Thursday, April 30, 2009
 *
 *-------------------------------------------------------------------------
 */
BEGIN_FUNC(PKG, ERR,
herr_t, SUCCEED, FAIL,
H5FA__hdr_delete(H5FA_hdr_t *hdr, hid_t dxpl_id))

    /* Local variables */
    unsigned cache_flags = H5AC__NO_FLAGS_SET;  /* Flags for unprotecting header */

    /* Sanity check */
    HDassert(hdr);
    HDassert(!hdr->file_rc);

#ifndef NDEBUG
{
    unsigned hdr_status = 0;         /* Array header's status in the metadata cache */

    /* Check the array header's status in the metadata cache */
    if(H5AC_get_entry_status(hdr->f, hdr->addr, &hdr_status) < 0)
        H5E_THROW(H5E_CANTGET, "unable to check metadata cache status for array header")

    /* Sanity checks on array header */
    HDassert(hdr_status & H5AC_ES__IN_CACHE);
    HDassert(hdr_status & H5AC_ES__IS_PROTECTED);
} /* end block */
#endif /* NDEBUG */

    /* Check for Fixed Array Data block */
    if(H5F_addr_defined(hdr->dblk_addr)) {
#ifdef H5FA_DEBUG
HDfprintf(stderr, "%s: hdr->dblk_addr = %a\n", FUNC, hdr->dblk_addr);
#endif /* H5FA_DEBUG */

        /* Delete Fixed Array Data block */
        if(H5FA__dblock_delete(hdr, dxpl_id, hdr->dblk_addr) < 0)
            H5E_THROW(H5E_CANTDELETE, "unable to delete fixed array data block")
    } /* end if */

    /* Set flags to finish deleting header on unprotect */
    cache_flags |= H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG;

CATCH

    /* Unprotect the header, deleting it if an error hasn't occurred */
    if(H5AC_unprotect(hdr->f, dxpl_id, H5AC_FARRAY_HDR, hdr->addr, hdr, cache_flags) < 0)
        H5E_THROW(H5E_CANTUNPROTECT, "unable to release fixed array header")

END_FUNC(PKG)   /* end H5FA__hdr_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5FA__hdr_dest
 *
 * Purpose:	Destroys a fixed array header in memory.
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
H5FA__hdr_dest(H5FA_hdr_t *hdr))

    /* Check arguments */
    HDassert(hdr);
    HDassert(hdr->rc == 0);

    /* Destroy the callback context */
    if(hdr->cb_ctx) {
        if((*hdr->cparam.cls->dst_context)(hdr->cb_ctx) < 0)
            H5E_THROW(H5E_CANTRELEASE, "unable to destroy fixed array client callback context")
    } /* end if */
    hdr->cb_ctx = NULL;

    /* Free the shared info itself */
    hdr = H5FL_FREE(H5FA_hdr_t, hdr);

CATCH

END_FUNC(PKG)   /* end H5FA__hdr_dest() */


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

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5F_init_super_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions                    */
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling                       */
#include "H5Fpkg.h"             /* File access                          */
#include "H5FDprivate.h"	/* File drivers                         */
#include "H5Iprivate.h"		/* IDs                                  */
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"		/* Property lists                       */
#include "H5SMprivate.h"        /* Shared Object Header Messages        */


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
static herr_t H5F_super_ext_create(H5F_t *f, hid_t dxpl_id, H5O_loc_t *ext_ptr);


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/

/* Declare a free list to manage the H5F_super_t struct */
H5FL_DEFINE(H5F_super_t);


/*******************/
/* Local Variables */
/*******************/



/*--------------------------------------------------------------------------
NAME
   H5F_init_super_interface -- Initialize interface-specific information
USAGE
    herr_t H5F_init_super_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5F_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5F_init_super_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(H5F_init())
} /* H5F_init_super_interface() */


/*-------------------------------------------------------------------------
 * Function:    H5F_super_ext_create
 *
 * Purpose:     Create the superblock extension
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi; Feb 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_super_ext_create(H5F_t *f, hid_t dxpl_id, H5O_loc_t *ext_ptr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);
    HDassert(!H5F_addr_defined(f->shared->sblock->ext_addr));
    HDassert(ext_ptr);

    /* Check for older version of superblock format that can't support superblock extensions */
    if(f->shared->sblock->super_vers < HDF5_SUPERBLOCK_VERSION_2)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCREATE, FAIL, "superblock extension not permitted with version %u of superblock", f->shared->sblock->super_vers)
    else if(H5F_addr_defined(f->shared->sblock->ext_addr))
        HGOTO_ERROR(H5E_FILE, H5E_CANTCREATE, FAIL, "superblock extension already exists?!?!")
    else {
        /* The superblock extension isn't actually a group, but the
         * default group creation list should work fine.
         * If we don't supply a size for the object header, HDF5 will
         * allocate H5O_MIN_SIZE by default.  This is currently
         * big enough to hold the biggest possible extension, but should
         * be tuned if more information is added to the superblock
         * extension.
         */
        H5O_loc_reset(ext_ptr);
        if(H5O_create(f, dxpl_id, 0, (size_t)1, H5P_GROUP_CREATE_DEFAULT, ext_ptr) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCREATE, FAIL, "unable to create superblock extension")

        /* Record the address of the superblock extension */
        f->shared->sblock->ext_addr = ext_ptr->addr;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_super_ext_create() */


/*-------------------------------------------------------------------------
 * Function:    H5F_super_ext_open
 *
 * Purpose:     Open an existing superblock extension
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi; Feb 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_super_ext_open(H5F_t *f, haddr_t ext_addr, H5O_loc_t *ext_ptr)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);
    HDassert(H5F_addr_defined(ext_addr));
    HDassert(ext_ptr);

    /* Set up "fake" object location for superblock extension */
    H5O_loc_reset(ext_ptr);
    ext_ptr->file = f;
    ext_ptr->addr = ext_addr;

    /* Open the superblock extension object header */
    if(H5O_open(ext_ptr) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, FAIL, "unable to open superblock extension")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_super_ext_open() */


/*-------------------------------------------------------------------------
 * Function:   H5F_super_ext_close
 *
 * Purpose:    Close superblock extension
 *
 * Return:     Success:        non-negative on success
 *             Failure:        Negative
 *
 * Programmer:  Vailin Choi; Feb 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_super_ext_close(H5F_t *f, H5O_loc_t *ext_ptr, hid_t dxpl_id,
    hbool_t was_created)
{
    H5P_genplist_t *dxpl = NULL;        /* DXPL for setting ring */
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(f);
    HDassert(ext_ptr);

    /* Check if extension was created */
    if(was_created) {
        /* Set the ring type in the DXPL */
        if(H5AC_set_ring(dxpl_id, H5AC_RING_SBE, &dxpl, &orig_ring) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set ring value")

        /* Increment link count on superblock extension's object header */
        if(H5O_link(ext_ptr, 1, dxpl_id) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_LINKCOUNT, FAIL, "unable to increment hard link count")

        /* Decrement refcount on superblock extension's object header in memory */
        if(H5O_dec_rc_by_loc(ext_ptr, dxpl_id) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTDEC, FAIL, "unable to decrement refcount on superblock extension");
    } /* end if */

    /* Twiddle the number of open objects to avoid closing the file. */
    f->nopen_objs++;
    if(H5O_close(ext_ptr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEOBJ, FAIL, "unable to close superblock extension")
    f->nopen_objs--;

done:
    /* Reset the ring in the DXPL */
    if(H5AC_reset_ring(dxpl, orig_ring) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set property value")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_super_ext_close() */


/*-------------------------------------------------------------------------
 * Function:    H5F__super_read
 *
 * Purpose:     Reads the superblock from the file or from the BUF. If
 *              ADDR is a valid address, then it reads it from the file.
 *              If not, then BUF must be non-NULL for it to read from the
 *              BUF.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept 12, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F__super_read(H5F_t *f, hid_t dxpl_id)
{
    H5P_genplist_t     *dxpl = NULL;        /* DXPL object */
    H5AC_ring_t         ring, orig_ring = H5AC_RING_INV;
    H5F_super_t *       sblock = NULL;      /* Superblock structure */
    H5F_superblock_cache_ud_t udata;        /* User data for cache callbacks */
    H5P_genplist_t     *c_plist;            /* File creation property list  */
    unsigned            sblock_flags = H5AC__NO_FLAGS_SET;       /* flags used in superblock unprotect call      */
    haddr_t             super_addr;         /* Absolute address of superblock */
    haddr_t             eof;                /* End of file address */
    unsigned      	rw_flags;           /* Read/write permissions for file */
    herr_t              ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE_TAG(dxpl_id, H5AC__SUPERBLOCK_TAG, FAIL)

    /* initialize the drvinfo to NULL -- we will overwrite this if there
     * is a driver information block 
     */
    f->shared->drvinfo = NULL;

    /* Get the DXPL plist object for DXPL ID */
    if(NULL == (dxpl = (H5P_genplist_t *)H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")
    if((H5P_get(dxpl, H5AC_RING_NAME, &orig_ring)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get property value");

    /* Find the superblock */
    if(H5FD_locate_signature(f->shared->lf, dxpl, &super_addr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "unable to locate file signature")
    if(HADDR_UNDEF == super_addr)
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "file signature not found")

    /* Check for userblock present */
    if(H5F_addr_gt(super_addr, 0)) {
        /* Set the base address for the file in the VFD now */
        if(H5F__set_base_addr(f, super_addr) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "failed to set base address for file driver")
    } /* end if */

    /* Determine file intent for superblock protect */

    /* Must tell cache at protect time that the super block is to be
     * flushed last (and collectively in the parallel case).
     */
    rw_flags = H5AC__FLUSH_LAST_FLAG;
#ifdef H5_HAVE_PARALLEL
    rw_flags |= H5C__FLUSH_COLLECTIVELY_FLAG;
#endif /* H5_HAVE_PARALLEL */
    if(!(H5F_INTENT(f) & H5F_ACC_RDWR))
        rw_flags |= H5AC__READ_ONLY_FLAG;

    /* Get the shared file creation property list */
    if(NULL == (c_plist = (H5P_genplist_t *)H5I_object(f->shared->fcpl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_BADTYPE, FAIL, "can't get property list")

    /* Make certain we can read the fixed-size portion of the superblock */
    if(H5F__set_eoa(f, H5FD_MEM_SUPER, 
              H5F_SUPERBLOCK_FIXED_SIZE + H5F_SUPERBLOCK_MINIMAL_VARLEN_SIZE) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "set end of space allocation request failed")

    /* Set up the user data for cache callbacks */
    udata.f = f;
    udata.ignore_drvrinfo = H5F_HAS_FEATURE(f, H5FD_FEAT_IGNORE_DRVRINFO);
    udata.sym_leaf_k = 0;
    if(H5P_get(c_plist, H5F_CRT_BTREE_RANK_NAME, udata.btree_k) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get rank for btree internal nodes")
    udata.stored_eof = HADDR_UNDEF;
    udata.drvrinfo_removed = FALSE;

    /* Set the ring type in the DXPL */
    ring = H5AC_RING_SB;
    if((H5P_set(dxpl, H5AC_RING_NAME, &ring)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set property value");

    /* Look up the superblock */
    if(NULL == (sblock = (H5F_super_t *)H5AC_protect(f, dxpl_id, H5AC_SUPERBLOCK, (haddr_t)0, &udata, rw_flags)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTPROTECT, FAIL, "unable to load superblock")

    /* Pin the superblock in the cache */
    if(H5AC_pin_protected_entry(sblock) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTPIN, FAIL, "unable to pin superblock")

    /* Mark the superblock dirty if it was modified during loading */
    if(((rw_flags & H5AC__READ_ONLY_FLAG) == 0) && udata.ignore_drvrinfo && udata.drvrinfo_removed) {
        HDassert(sblock->super_vers < HDF5_SUPERBLOCK_VERSION_2);
        sblock_flags |= H5AC__DIRTIED_FLAG;
    } /* end if */

    /* The superblock must be flushed last (and collectively in parallel) */
    sblock_flags |= H5AC__FLUSH_LAST_FLAG;
#ifdef H5_HAVE_PARALLEL
    sblock_flags |= H5AC__FLUSH_COLLECTIVELY_FLAG;
#endif /* H5_HAVE_PARALLEL */

    /* Check if superblock address is different from base address and adjust
     * base address and "end of address" address if so.
     */
    if(!H5F_addr_eq(super_addr, sblock->base_addr)) {
        /* Check if the superblock moved earlier in the file */
        if(H5F_addr_lt(super_addr, sblock->base_addr))
            udata.stored_eof -= (sblock->base_addr - super_addr);
        else
            /* The superblock moved later in the file */
            udata.stored_eof += (super_addr - sblock->base_addr);

        /* Adjust base address for offsets of the HDF5 data in the file */
        sblock->base_addr = super_addr;

        /* Set the base address for the file in the VFD now */
        if(H5F__set_base_addr(f, sblock->base_addr) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "failed to set base address for file driver")

        /* Indicate that the superblock should be marked dirty */
        if((rw_flags & H5AC__READ_ONLY_FLAG) == 0)
            sblock_flags |= H5AC__DIRTIED_FLAG;
    } /* end if */

    /* Set information in the file's creation property list */
    if(H5P_set(c_plist, H5F_CRT_SUPER_VERS_NAME, &sblock->super_vers) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set superblock version")
    if(H5P_set(c_plist, H5F_CRT_ADDR_BYTE_NUM_NAME, &sblock->sizeof_addr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set byte number in an address")
    if(H5P_set(c_plist, H5F_CRT_OBJ_BYTE_NUM_NAME, &sblock->sizeof_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set byte number for object size")

    /* Handle the B-tree 'K' values */
    if(sblock->super_vers < HDF5_SUPERBLOCK_VERSION_2) {
        /* Sanity check */
        HDassert(udata.sym_leaf_k != 0);

        /* Set the symbol table internal node 'K' value */
        if(H5P_set(c_plist, H5F_CRT_SYM_LEAF_NAME, &udata.sym_leaf_k) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set rank for symbol table leaf nodes")
        sblock->sym_leaf_k = udata.sym_leaf_k;

        /* Set the B-tree internal node values, etc */
        if(H5P_set(c_plist, H5F_CRT_BTREE_RANK_NAME, udata.btree_k) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set rank for btree internal nodes")
        HDmemcpy(sblock->btree_k, udata.btree_k, sizeof(unsigned) * (size_t)H5B_NUM_BTREE_ID);
    } /* end if */
    else {
        /* Get the (default) B-tree internal node values, etc */
        /* (Note: these may be reset in a superblock extension) */
        if(H5P_get(c_plist, H5F_CRT_BTREE_RANK_NAME, sblock->btree_k) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get rank for btree internal nodes")
        if(H5P_get(c_plist, H5F_CRT_SYM_LEAF_NAME, &sblock->sym_leaf_k) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get rank for btree internal nodes")
    } /* end else */

    /*
     * The user-defined data is the area of the file before the base
     * address.
     */
    if(H5P_set(c_plist, H5F_CRT_USER_BLOCK_NAME, &sblock->base_addr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set userblock size")

    /*
     * Make sure that the data is not truncated. One case where this is
     * possible is if the first file of a family of files was opened
     * individually.
     */
    if(HADDR_UNDEF == (eof = H5FD_get_eof(f->shared->lf, H5FD_MEM_DEFAULT)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to determine file size")

    /* (Account for the stored EOA being absolute offset -QAK) */
    if((eof + sblock->base_addr) < udata.stored_eof)
        HGOTO_ERROR(H5E_FILE, H5E_TRUNCATED, FAIL, "truncated file: eof = %llu, sblock->base_addr = %llu, stored_eoa = %llu", (unsigned long long)eof, (unsigned long long)sblock->base_addr, (unsigned long long)udata.stored_eof)

    /*
     * Tell the file driver how much address space has already been
     * allocated so that it knows how to allocate additional memory.
     */

    /* Set the ring type in the DXPL */
    ring = H5AC_RING_SBE;
    if((H5P_set(dxpl, H5AC_RING_NAME, &ring)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set property value");

    /* Decode the optional driver information block */
    if(H5F_addr_defined(sblock->driver_addr)) {
        H5O_drvinfo_t *drvinfo;         /* Driver info */
        H5F_drvrinfo_cache_ud_t drvrinfo_udata;  /* User data for metadata callbacks */
        unsigned drvinfo_flags = H5AC__NO_FLAGS_SET;    /* Flags used in driver info block unprotect call */

        /* Sanity check - driver info block should only be defined for
         *      superblock version < 2.
         */
        HDassert(sblock->super_vers < HDF5_SUPERBLOCK_VERSION_2);

        /* Set up user data */
        drvrinfo_udata.f           = f;
        drvrinfo_udata.driver_addr = sblock->driver_addr;

        /* extend EOA so we can read at least the fixed sized 
         * portion of the driver info block 
         */
        if(H5FD_set_eoa(f->shared->lf, H5FD_MEM_SUPER, sblock->driver_addr + H5F_DRVINFOBLOCK_HDR_SIZE) < 0) /* will extend eoa later if required */ 
            HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, \
                        "set end of space allocation request failed")

        /* Look up the driver info block */
        if(NULL == (drvinfo = (H5O_drvinfo_t *)H5AC_protect(f, dxpl_id, H5AC_DRVRINFO, sblock->driver_addr, &drvrinfo_udata, rw_flags)))
            HGOTO_ERROR(H5E_FILE, H5E_CANTPROTECT, FAIL, "unable to load driver info block")

        /* Loading the driver info block is enough to set up the right info */

        /* Check if we need to rewrite the driver info block info */
        if ( ( (rw_flags & H5AC__READ_ONLY_FLAG) == 0 ) &&
             ( H5F_HAS_FEATURE(f, H5FD_FEAT_DIRTY_DRVRINFO_LOAD) ) ) {

            drvinfo_flags |= H5AC__DIRTIED_FLAG;
        } /* end if */

        /* set the pin entry flag so that the driver information block 
         * cache entry will be pinned in the cache.
         */
        drvinfo_flags |= H5AC__PIN_ENTRY_FLAG;

        /* Release the driver info block */
        if(H5AC_unprotect(f, dxpl_id, H5AC_DRVRINFO, sblock->driver_addr, drvinfo, drvinfo_flags) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTUNPROTECT, FAIL, "unable to release driver info block")

        /* save a pointer to the driver information cache entry */
        f->shared->drvinfo = drvinfo;
    } /* end if */

    /* (Account for the stored EOA being absolute offset -NAF) */
    if(H5F__set_eoa(f, H5FD_MEM_SUPER, udata.stored_eof - sblock->base_addr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set end-of-address marker for file")

    /* Decode the optional superblock extension info */
    if(H5F_addr_defined(sblock->ext_addr)) {
        H5O_loc_t ext_loc;      /* "Object location" for superblock extension */
        H5O_btreek_t btreek;    /* v1 B-tree 'K' value message from superblock extension */
        H5O_drvinfo_t drvinfo;  /* Driver info message from superblock extension */
	size_t u; 		/* Local index variable */
        htri_t status;          /* Status for message existing */

        /* Sanity check - superblock extension should only be defined for
         *      superblock version >= 2.
         */
        HDassert(sblock->super_vers >= HDF5_SUPERBLOCK_VERSION_2);

        /* Check for superblock extension being located "outside" the stored
         *      'eoa' value, which can occur with the split/multi VFD.
         */
        if(H5F_addr_gt(sblock->ext_addr, udata.stored_eof)) {
            /* Set the 'eoa' for the object header memory type large enough
             *  to give some room for a reasonably sized superblock extension.
             *  (This is _rather_ a kludge -QAK)
             */
            if(H5F__set_eoa(f, H5FD_MEM_OHDR, (haddr_t)(sblock->ext_addr + 1024)) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set end-of-address marker for file")
        } /* end if */

        /* Open the superblock extension */
	if(H5F_super_ext_open(f, sblock->ext_addr, &ext_loc) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENOBJ, FAIL, "unable to open file's superblock extension")

        /* Check for the extension having a 'driver info' message */
        if((status = H5O_msg_exists(&ext_loc, H5O_DRVINFO_ID, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_EXISTS, FAIL, "unable to read object header")
        if(status) {
            /* Check for ignoring the driver info for this file */
            if(!udata.ignore_drvrinfo) {

                /* Retrieve the 'driver info' structure */
                if(NULL == H5O_msg_read(&ext_loc, H5O_DRVINFO_ID, &drvinfo, dxpl_id))
                    HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "driver info message not present")

                /* Validate and decode driver information */
                if(H5FD_sb_load(f->shared->lf, drvinfo.name, drvinfo.buf) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_CANTDECODE, FAIL, "unable to decode driver information")

                /* Reset driver info message */
                H5O_msg_reset(H5O_DRVINFO_ID, &drvinfo);
            } /* end else */
        } /* end if */

        /* Read in the shared OH message information if there is any */
        if(H5SM_get_info(&ext_loc, c_plist, dxpl_id) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to read SOHM table information")

        /* Check for the extension having a 'v1 B-tree "K"' message */
        if((status = H5O_msg_exists(&ext_loc, H5O_BTREEK_ID, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_EXISTS, FAIL, "unable to read object header")
        if(status) {
            /* Retrieve the 'v1 B-tree "K"' structure */
            if(NULL == H5O_msg_read(&ext_loc, H5O_BTREEK_ID, &btreek, dxpl_id))
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "v1 B-tree 'K' info message not present")

            /* Set non-default v1 B-tree 'K' value info from file */
            sblock->btree_k[H5B_CHUNK_ID] = btreek.btree_k[H5B_CHUNK_ID];
            sblock->btree_k[H5B_SNODE_ID] = btreek.btree_k[H5B_SNODE_ID];
            sblock->sym_leaf_k = btreek.sym_leaf_k;

            /* Set non-default v1 B-tree 'K' values in the property list */
            if(H5P_set(c_plist, H5F_CRT_BTREE_RANK_NAME, btreek.btree_k) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set rank for btree internal nodes")
            if(H5P_set(c_plist, H5F_CRT_SYM_LEAF_NAME, &btreek.sym_leaf_k) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set rank for symbol table leaf nodes")
        } /* end if */

        /* Check for the extension having a 'free-space manager info' message */
        if((status = H5O_msg_exists(&ext_loc, H5O_FSINFO_ID, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_EXISTS, FAIL, "unable to read object header")
        if(status) {
            H5O_fsinfo_t fsinfo;    /* Free-space manager info message from superblock extension */

            /* Retrieve the 'free-space manager info' structure */
	    if(NULL == H5O_msg_read(&ext_loc, H5O_FSINFO_ID, &fsinfo, dxpl_id))
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get free-space manager info message")

            /* Check for non-default info */
	    if(f->shared->fs_strategy != fsinfo.strategy) {
		f->shared->fs_strategy = fsinfo.strategy;

		/* Set non-default strategy in the property list */
		if(H5P_set(c_plist, H5F_CRT_FILE_SPACE_STRATEGY_NAME, &fsinfo.strategy) < 0)
		    HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set file space strategy")
	    } /* end if */
	    if(f->shared->fs_threshold != fsinfo.threshold) {
		f->shared->fs_threshold = fsinfo.threshold;

		/* Set non-default threshold in the property list */
		if(H5P_set(c_plist, H5F_CRT_FREE_SPACE_THRESHOLD_NAME, &fsinfo.threshold) < 0)
		    HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set file space strategy")
	    } /* end if */

	    /* Set free-space manager addresses */
	    f->shared->fs_addr[0] = HADDR_UNDEF;
	    for(u = 1; u < NELMTS(f->shared->fs_addr); u++)
		f->shared->fs_addr[u] = fsinfo.fs_addr[u-1];
        } /* end if */

        /* Close superblock extension */
        if(H5F_super_ext_close(f, &ext_loc, dxpl_id, FALSE) < 0)
	    HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEOBJ, FAIL, "unable to close file's superblock extension")
    } /* end if */

    /* Update the driver info if VFD indicated to do so */
    /* (NOTE: only for later versions of superblock, earlier versions are handled
     *          earlier in this routine.
     */
    if(((rw_flags & H5AC__READ_ONLY_FLAG) == 0) &&
            sblock->super_vers >= HDF5_SUPERBLOCK_VERSION_2 &&
            H5F_addr_defined(sblock->ext_addr)) {
        /* Check for modifying the driver info when opening the file */
        if(H5F_HAS_FEATURE(f, H5FD_FEAT_DIRTY_DRVRINFO_LOAD)) {
            size_t          driver_size;    /* Size of driver info block (bytes) */

            /* Check for driver info message */
            H5_CHECKED_ASSIGN(driver_size, size_t, H5FD_sb_size(f->shared->lf), hsize_t);
            if(driver_size > 0) {
                H5O_drvinfo_t drvinfo;      /* Driver info */
                uint8_t dbuf[H5F_MAX_DRVINFOBLOCK_SIZE];  /* Driver info block encoding buffer */

                /* Sanity check */
                HDassert(driver_size <= H5F_MAX_DRVINFOBLOCK_SIZE);

                /* Encode driver-specific data */
                if(H5FD_sb_encode(f->shared->lf, drvinfo.name, dbuf) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to encode driver information")

                /* Set the driver info information for the superblock extension */
                drvinfo.len = driver_size;
                drvinfo.buf = dbuf;

                /* Write driver info information to the superblock extension */

#if 1 /* bug fix test code -- tidy this up if all goes well */ /* JRM */
		/* KLUGE ALERT!!
		 *
		 * H5F_super_ext_write_msg() expects f->shared->sblock to 
		 * be set -- verify that it is NULL, and then set it.
		 * Set it back to NULL when we are done.
		 */
		HDassert(f->shared->sblock == NULL);
		f->shared->sblock = sblock;
#endif /* JRM */

                if(H5F_super_ext_write_msg(f, dxpl_id, &drvinfo, H5O_DRVINFO_ID, FALSE) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_WRITEERROR, FAIL, "error in writing message to superblock extension")

#if 1 /* bug fix test code -- tidy this up if all goes well */ /* JRM */
		f->shared->sblock = NULL;
#endif /* JRM */

            } /* end if */
        } /* end if */
        /* Check for eliminating the driver info block */
        else if(H5F_HAS_FEATURE(f, H5FD_FEAT_IGNORE_DRVRINFO)) {
            /* Remove the driver info message from the superblock extension */
            if(H5F_super_ext_remove_msg(f, dxpl_id, H5O_DRVINFO_ID) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "error in removing message from superblock extension")

            /* Check if the superblock extension was removed */
            if(!H5F_addr_defined(sblock->ext_addr))
                sblock_flags |= H5AC__DIRTIED_FLAG;
        } /* end if */
    } /* end if */

    /* Set the pointer to the pinned superblock */
    f->shared->sblock = sblock;

done:
    /* Reset the ring in the DXPL */
    if(H5AC_reset_ring(dxpl, orig_ring) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set property value")

    /* Release the superblock */
    if(sblock && H5AC_unprotect(f, dxpl_id, H5AC_SUPERBLOCK, (haddr_t)0, sblock, sblock_flags) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTUNPROTECT, FAIL, "unable to close superblock")

    /* If we have failed, make sure no entries are left in the 
     * metadata cache, so that it can be shut down and discarded.
     */
    if(ret_value < 0) { 
        /* Unpin and discard drvinfo cache entry */
        if(f->shared->drvinfo) {
            if(H5AC_unpin_entry(f->shared->drvinfo) < 0)
                HDONE_ERROR(H5E_FILE, H5E_CANTUNPIN, FAIL, "unable to unpin driver info")

            /* Evict the driver info block from the cache */
            if(H5AC_expunge_entry(f, dxpl_id, H5AC_DRVRINFO, sblock->driver_addr, H5AC__NO_FLAGS_SET) < 0)
                HDONE_ERROR(H5E_FILE, H5E_CANTEXPUNGE, FAIL, "unable to expunge driver info block")
        } /* end if */

        /* Unpin & discard superblock */
        if(sblock) {
            /* Unpin superblock in cache */
            if(H5AC_unpin_entry(sblock) < 0)
                HDONE_ERROR(H5E_FILE, H5E_CANTUNPIN, FAIL, "unable to unpin superblock")

            /* Evict the superblock from the cache */
            if(H5AC_expunge_entry(f, dxpl_id, H5AC_SUPERBLOCK, (haddr_t)0, H5AC__NO_FLAGS_SET) < 0)
                HDONE_ERROR(H5E_FILE, H5E_CANTEXPUNGE, FAIL, "unable to expunge superblock")
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5F__super_read() */


/*-------------------------------------------------------------------------
 * Function:    H5F__super_init
 *
 * Purpose:     Allocates the superblock for the file and initializes
 *              information about the superblock in memory.  Writes extension
 *              messages if any are needed.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Sept 15, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F__super_init(H5F_t *f, hid_t dxpl_id)
{
    H5F_super_t    *sblock = NULL;      /* Superblock cache structure                 */
    hbool_t         sblock_in_cache = FALSE;    /* Whether the superblock has been inserted into the metadata cache */
    H5O_drvinfo_t  *drvinfo = NULL;     /* Driver info */
    hbool_t         drvinfo_in_cache = FALSE;   /* Whether the driver info block has been inserted into the metadata cache */
    H5P_genplist_t *plist;              /* File creation property list                */
    H5P_genplist_t *dxpl = NULL;
    H5AC_ring_t ring, orig_ring = H5AC_RING_INV;
    hsize_t         userblock_size;     /* Size of userblock, in bytes                */
    hsize_t         superblock_size;    /* Size of superblock, in bytes               */
    size_t          driver_size;        /* Size of driver info block (bytes)          */
    unsigned super_vers = HDF5_SUPERBLOCK_VERSION_DEF; /* Superblock version for file */
    H5O_loc_t       ext_loc;            /* Superblock extension object location */
    hbool_t         need_ext;           /* Whether the superblock extension is needed */
    hbool_t         ext_created = FALSE; /* Whether the extension has been created */
    herr_t          ret_value = SUCCEED; /* Return Value                              */

    FUNC_ENTER_PACKAGE_TAG(dxpl_id, H5AC__SUPERBLOCK_TAG, FAIL)

    /* Allocate space for the superblock */
    if(NULL == (sblock = H5FL_CALLOC(H5F_super_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize various address information */
    sblock->base_addr = HADDR_UNDEF;
    sblock->ext_addr = HADDR_UNDEF;
    sblock->driver_addr = HADDR_UNDEF;
    sblock->root_addr = HADDR_UNDEF;

    /* Get the shared file creation property list */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(f->shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Initialize sym_leaf_k */
    if(H5P_get(plist, H5F_CRT_SYM_LEAF_NAME, &sblock->sym_leaf_k) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get byte number for object size")

    /* Initialize btree_k */
    if(H5P_get(plist, H5F_CRT_BTREE_RANK_NAME, &sblock->btree_k[0]) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get rank for btree internal nodes")

    /* Bump superblock version if we are to use the latest version of the format */
    if(f->shared->latest_format)
        super_vers = HDF5_SUPERBLOCK_VERSION_LATEST;
    /* Bump superblock version to create superblock extension for SOHM info */
    else if(f->shared->sohm_nindexes > 0)
        super_vers = HDF5_SUPERBLOCK_VERSION_2;
    /* Bump superblock version to create superblock extension for
     * non-default file space strategy or non-default free-space threshold
     */
    else if(f->shared->fs_strategy != H5F_FILE_SPACE_STRATEGY_DEF ||
            f->shared->fs_threshold != H5F_FREE_SPACE_THRESHOLD_DEF)
        super_vers = HDF5_SUPERBLOCK_VERSION_2;
    /* Check for non-default indexed storage B-tree internal 'K' value
     * and set the version # of the superblock to 1 if it is a non-default
     * value.
     */
    else if(sblock->btree_k[H5B_CHUNK_ID] != HDF5_BTREE_CHUNK_IK_DEF)
        super_vers = HDF5_SUPERBLOCK_VERSION_1;

    /* If a newer superblock version is required, set it here */
    if(super_vers != HDF5_SUPERBLOCK_VERSION_DEF) {
        H5P_genplist_t *c_plist;              /* Property list */

        if(NULL == (c_plist = (H5P_genplist_t *)H5I_object(f->shared->fcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not property list")
        if(H5P_set(c_plist, H5F_CRT_SUPER_VERS_NAME, &super_vers) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set superblock version")
    } /* end if */

    /*
     * The superblock starts immediately after the user-defined
     * header, which we have already insured is a proper size. The
     * base address is set to the same thing as the superblock for
     * now.
     */
    if(H5P_get(plist, H5F_CRT_USER_BLOCK_NAME, &userblock_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get userblock size")

    /* Sanity check the userblock size vs. the file's allocation alignment */
    if(userblock_size > 0) {
        if(userblock_size < f->shared->alignment)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "userblock size must be > file object alignment")
        if(0 != (userblock_size % f->shared->alignment))
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "userblock size must be an integral multiple of file object alignment")
    } /* end if */

    sblock->base_addr = userblock_size;
    sblock->status_flags = 0;

    /* Reserve space for the userblock */
    if(H5F__set_eoa(f, H5FD_MEM_SUPER, userblock_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to set EOA value for userblock")

    /* Set the base address for the file in the VFD now, after allocating
     *  space for userblock.
     */
    if(H5F__set_base_addr(f, sblock->base_addr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "failed to set base address for file driver")

    /* Save a local copy of the superblock version number, size of addresses & offsets */
    sblock->super_vers = super_vers;
    sblock->sizeof_addr = f->shared->sizeof_addr;
    sblock->sizeof_size = f->shared->sizeof_size;

    /* Compute the size of the superblock */
    superblock_size = (hsize_t)H5F_SUPERBLOCK_SIZE(sblock);

    /* Compute the size of the driver information block */
    H5_CHECKED_ASSIGN(driver_size, size_t, H5FD_sb_size(f->shared->lf), hsize_t);
    if(driver_size > 0) {
        driver_size += H5F_DRVINFOBLOCK_HDR_SIZE;

        /*
         * The file driver information block begins immediately after the
         * superblock. (relative to base address in file)
         */
        sblock->driver_addr = superblock_size;
    } /* end if */

    /*
     * Allocate space for the superblock & driver info block.
     * We do it with one allocation request because the superblock needs to be
     * at the beginning of the file and only the first allocation request is
     * required to return memory at format address zero.
     */
    if(super_vers < HDF5_SUPERBLOCK_VERSION_2)
        superblock_size += driver_size;

    /* Reserve space in the file for the superblock, instead of allocating it */
    if(H5F__set_eoa(f, H5FD_MEM_SUPER, superblock_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to set EOA value for superblock")

    /* Set the ring type in the DXPL */
    if(H5AC_set_ring(dxpl_id, H5AC_RING_SB, &dxpl, &orig_ring) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set ring value")

    /* Insert superblock into cache, pinned */
    if(H5AC_insert_entry(f, dxpl_id, H5AC_SUPERBLOCK, (haddr_t)0, sblock, H5AC__PIN_ENTRY_FLAG | H5AC__FLUSH_LAST_FLAG | H5AC__FLUSH_COLLECTIVELY_FLAG) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, "can't add superblock to cache")
    sblock_in_cache = TRUE;

    /* Keep a copy of the superblock info */
    f->shared->sblock = sblock;

    /* set the drvinfo filed to NULL -- will overwrite this later if needed */
    f->shared->drvinfo = NULL;

    /*
     * Determine if we will need a superblock extension
     */

    /* Files with SOHM indices always need the superblock extension */
    if(f->shared->sohm_nindexes > 0) {
        HDassert(super_vers >= HDF5_SUPERBLOCK_VERSION_2);
        need_ext = TRUE;
    } /* end if */
    /* Files with non-default free space settings always need the superblock extension */
    else if(f->shared->fs_strategy != H5F_FILE_SPACE_STRATEGY_DEF ||
            f->shared->fs_threshold != H5F_FREE_SPACE_THRESHOLD_DEF) {
        HDassert(super_vers >= HDF5_SUPERBLOCK_VERSION_2);
        need_ext = TRUE;
    } /* end if */
    /* If we're going to use a version of the superblock format which allows
     *  for the superblock extension, check for non-default values to store
     *  in it.
     */
    else if(super_vers >= HDF5_SUPERBLOCK_VERSION_2) {
        /* Check for non-default v1 B-tree 'K' values to store */
        if(sblock->btree_k[H5B_SNODE_ID] != HDF5_BTREE_SNODE_IK_DEF ||
                sblock->btree_k[H5B_CHUNK_ID] != HDF5_BTREE_CHUNK_IK_DEF ||
                sblock->sym_leaf_k != H5F_CRT_SYM_LEAF_DEF)
            need_ext = TRUE;
        /* Check for driver info to store */
        else if(driver_size > 0)
            need_ext = TRUE;
        else
            need_ext = FALSE;
    } /* end if */
    else
        need_ext = FALSE;

    /* Set the ring type in the DXPL */
    ring = H5AC_RING_SBE;
    if((H5P_set(dxpl, H5AC_RING_NAME, &ring)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set property value")

    /* Create the superblock extension for "extra" superblock data, if necessary. */
    if(need_ext) {
        /* The superblock extension isn't actually a group, but the
         * default group creation list should work fine.
         * If we don't supply a size for the object header, HDF5 will
         * allocate H5O_MIN_SIZE by default.  This is currently
         * big enough to hold the biggest possible extension, but should
         * be tuned if more information is added to the superblock
         * extension.
         */
	if(H5F_super_ext_create(f, dxpl_id, &ext_loc) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTCREATE, FAIL, "unable to create superblock extension")
        ext_created = TRUE;

        /* Create the Shared Object Header Message table and register it with
         *      the metadata cache, if this file supports shared messages.
         */
        if(f->shared->sohm_nindexes > 0) {
            /* Initialize the shared message code & write the SOHM message to the extension */
            if(H5SM_init(f, plist, &ext_loc, dxpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to create SOHM table")
        } /* end if */

        /* Check for non-default v1 B-tree 'K' values to store */
        if(sblock->btree_k[H5B_SNODE_ID] != HDF5_BTREE_SNODE_IK_DEF ||
                sblock->btree_k[H5B_CHUNK_ID] != HDF5_BTREE_CHUNK_IK_DEF ||
                sblock->sym_leaf_k != H5F_CRT_SYM_LEAF_DEF) {
            H5O_btreek_t btreek;        /* v1 B-tree 'K' value message for superblock extension */

            /* Write v1 B-tree 'K' value information to the superblock extension */
            btreek.btree_k[H5B_CHUNK_ID] = sblock->btree_k[H5B_CHUNK_ID];
            btreek.btree_k[H5B_SNODE_ID] = sblock->btree_k[H5B_SNODE_ID];
            btreek.sym_leaf_k = sblock->sym_leaf_k;
            if(H5O_msg_create(&ext_loc, H5O_BTREEK_ID, H5O_MSG_FLAG_CONSTANT | H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, &btreek, dxpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to update v1 B-tree 'K' value header message")
        } /* end if */

        /* Check for driver info to store */
        if(driver_size > 0) {
            H5O_drvinfo_t info;      /* Driver info */
            uint8_t dbuf[H5F_MAX_DRVINFOBLOCK_SIZE];  /* Driver info block encoding buffer */

            /* Sanity check */
            HDassert(driver_size <= H5F_MAX_DRVINFOBLOCK_SIZE);

            /* Encode driver-specific data */
            if(H5FD_sb_encode(f->shared->lf, info.name, dbuf) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to encode driver information")

            /* Write driver info information to the superblock extension */
            info.len = driver_size;
            info.buf = dbuf;
            if(H5O_msg_create(&ext_loc, H5O_DRVINFO_ID, H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, &info, dxpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to update driver info header message")
        } /* end if */

        /* Check for non-default free space settings */
	if(f->shared->fs_strategy != H5F_FILE_SPACE_STRATEGY_DEF ||
                f->shared->fs_threshold != H5F_FREE_SPACE_THRESHOLD_DEF) {
	    H5FD_mem_t   type;         	/* Memory type for iteration */
            H5O_fsinfo_t fsinfo;	/* Free space manager info message */

	    /* Write free-space manager info message to superblock extension object header if needed */
	    fsinfo.strategy = f->shared->fs_strategy;
	    fsinfo.threshold = f->shared->fs_threshold;
	    for(type = H5FD_MEM_SUPER; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type))
                fsinfo.fs_addr[type-1] = HADDR_UNDEF;

            if(H5O_msg_create(&ext_loc, H5O_FSINFO_ID, H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, &fsinfo, dxpl_id) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to update free-space info header message")
	} /* end if */
    } /* end if */
    else {
        /* Check for creating an "old-style" driver info block */
        if(driver_size > 0) {
            /* Sanity check */
            HDassert(H5F_addr_defined(sblock->driver_addr));

            /* Allocate space for the driver info */
            if(NULL == (drvinfo = (H5O_drvinfo_t *)H5MM_calloc(sizeof(H5O_drvinfo_t))))
                HGOTO_ERROR(H5E_FILE, H5E_CANTALLOC, FAIL, "memory allocation failed for driver info message")

            /* Set up driver info message */
            /* (NOTE: All the actual information (name & driver information) is
             *          actually based on the VFD info in the file handle and
             *          will be encoded by the VFD's 'encode' callback, so it
             *          doesn't need to be set here. -QAK, 7/20/2013
             */
            H5_CHECKED_ASSIGN(drvinfo->len, size_t, H5FD_sb_size(f->shared->lf), hsize_t);

            /* Insert driver info block into cache */
            if(H5AC_insert_entry(f, dxpl_id, H5AC_DRVRINFO, sblock->driver_addr, drvinfo, H5AC__PIN_ENTRY_FLAG | H5AC__FLUSH_LAST_FLAG | H5AC__FLUSH_COLLECTIVELY_FLAG) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINS, FAIL, "can't add driver info block to cache")
            drvinfo_in_cache = TRUE;
            f->shared->drvinfo = drvinfo;
        } /* end if */
        else
            HDassert(!H5F_addr_defined(sblock->driver_addr));
    } /* end if */

done:
    /* Reset the ring in the DXPL */
    if(H5AC_reset_ring(dxpl, orig_ring) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set property value")

    /* Close superblock extension, if it was created */
    if(ext_created && H5F_super_ext_close(f, &ext_loc, dxpl_id, ext_created) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "unable to close file's superblock extension")

    /* Cleanup on failure */
    if(ret_value < 0) {
        /* Check if the driver info block has been allocated yet */
        if(drvinfo) {
            /* Check if we've cached it already */
            if(drvinfo_in_cache) {
                /* Unpin drvinfo in cache */
                if(H5AC_unpin_entry(drvinfo) < 0)
                    HDONE_ERROR(H5E_FILE, H5E_CANTUNPIN, FAIL, "unable to unpin driver info")

                /* Evict the driver info block from the cache */
                if(H5AC_expunge_entry(f, dxpl_id, H5AC_DRVRINFO, sblock->driver_addr, H5AC__NO_FLAGS_SET) < 0)
                    HDONE_ERROR(H5E_FILE, H5E_CANTEXPUNGE, FAIL, "unable to expunge driver info block")
            } /* end if */
            else
                /* Free driver info block */
                H5MM_xfree(drvinfo);
        } /* end if */

        /* Check if the superblock has been allocated yet */
        if(sblock) {
            /* Check if we've cached it already */
            if(sblock_in_cache) {
                /* Unpin superblock in cache */
                if(H5AC_unpin_entry(sblock) < 0)
                    HDONE_ERROR(H5E_FILE, H5E_CANTUNPIN, FAIL, "unable to unpin superblock")

                /* Evict the superblock from the cache */
                if(H5AC_expunge_entry(f, dxpl_id, H5AC_SUPERBLOCK, (haddr_t)0, H5AC__NO_FLAGS_SET) < 0)
                    HDONE_ERROR(H5E_FILE, H5E_CANTEXPUNGE, FAIL, "unable to expunge superblock")
            } /* end if */
            else
                /* Free superblock */
                if(H5F__super_free(sblock) < 0)
                    HDONE_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "unable to destroy superblock")

            /* Reset variables in file structure */
            f->shared->sblock = NULL;
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI_TAG(ret_value, FAIL)
} /* end H5F__super_init() */


/*-------------------------------------------------------------------------
 * Function:    H5F_super_dirty
 *
 * Purpose:     Mark the file's superblock dirty
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              August 14, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_super_dirty(H5F_t *f)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);

    /* Mark superblock dirty in cache, so change to EOA will get encoded */
    if(H5AC_mark_entry_dirty(f->shared->sblock) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTMARKDIRTY, FAIL, "unable to mark superblock as dirty")

    /* if the driver information block exists, mark it dirty as well 
     * so that the change in eoa will be reflected there as well if 
     * appropriate.
     */
    if ( f->shared->drvinfo )
        if(H5AC_mark_entry_dirty(f->shared->drvinfo) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTMARKDIRTY, FAIL, "unable to mark drvinfo as dirty")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_super_dirty() */


/*-------------------------------------------------------------------------
 * Function:    H5F__super_free
 *
 * Purpose:     Destroyer the file's superblock
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              April 1, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F__super_free(H5F_super_t *sblock)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    HDassert(sblock);

    /* Free root group symbol table entry, if any */
    sblock->root_ent = (H5G_entry_t *)H5MM_xfree(sblock->root_ent);

    /* Free superblock */
    sblock = (H5F_super_t *)H5FL_FREE(H5F_super_t, sblock);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5F__super_free() */


/*-------------------------------------------------------------------------
 * Function:    H5F__super_size
 *
 * Purpose:     Get storage size of the superblock and superblock extension
 *
 * Return:      Success:        non-negative on success
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi
 *              July 11, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F__super_size(H5F_t *f, hid_t dxpl_id, hsize_t *super_size, hsize_t *super_ext_size)
{
    H5P_genplist_t *dxpl = NULL;        /* DXPL for setting ring */
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);

    /* Set the superblock size */
    if(super_size)
	*super_size = (hsize_t)H5F_SUPERBLOCK_SIZE(f->shared->sblock);

    /* Set the superblock extension size */
    if(super_ext_size) {
        if(H5F_addr_defined(f->shared->sblock->ext_addr)) {
            H5O_loc_t ext_loc;          /* "Object location" for superblock extension */
            H5O_hdr_info_t hdr_info;    /* Object info for superblock extension */

            /* Set up "fake" object location for superblock extension */
            H5O_loc_reset(&ext_loc);
            ext_loc.file = f;
            ext_loc.addr = f->shared->sblock->ext_addr;

            /* Set the ring type in the DXPL */
            if(H5AC_set_ring(dxpl_id, H5AC_RING_SBE, &dxpl, &orig_ring) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set ring value")

            /* Get object header info for superblock extension */
            if(H5O_get_hdr_info(&ext_loc, dxpl_id, &hdr_info) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to retrieve superblock extension info")

            /* Set the superblock extension size */
            *super_ext_size = hdr_info.space.total;
        } /* end if */
        else
            /* Set the superblock extension size to zero */
            *super_ext_size = (hsize_t)0;
    } /* end if */

done:
    /* Reset the ring in the DXPL */
    if(H5AC_reset_ring(dxpl, orig_ring) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set property value")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F__super_size() */


/*-------------------------------------------------------------------------
 * Function:    H5F_super_ext_write_msg()
 *
 * Purpose:     Write the message with ID to the superblock extension
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; Feb 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_super_ext_write_msg(H5F_t *f, hid_t dxpl_id, void *mesg, unsigned id, hbool_t may_create)
{
    H5P_genplist_t *dxpl = NULL;        /* DXPL for setting ring */
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    hbool_t     ext_created = FALSE;   /* Whether superblock extension was created */
    hbool_t     ext_opened = FALSE;    /* Whether superblock extension was opened */
    H5O_loc_t 	ext_loc; 	/* "Object location" for superblock extension */
    htri_t 	status;       	/* Indicate whether the message exists or not */
    herr_t 	ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->sblock);

    /* Set the ring type in the DXPL */
    if(H5AC_set_ring(dxpl_id, H5AC_RING_SBE, &dxpl, &orig_ring) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set ring value")

    /* Open/create the superblock extension object header */
    if(H5F_addr_defined(f->shared->sblock->ext_addr)) {
	if(H5F_super_ext_open(f, f->shared->sblock->ext_addr, &ext_loc) < 0)
	    HGOTO_ERROR(H5E_FILE, H5E_CANTOPENOBJ, FAIL, "unable to open file's superblock extension")
    } /* end if */
    else {
        HDassert(may_create);
	if(H5F_super_ext_create(f, dxpl_id, &ext_loc) < 0)
	    HGOTO_ERROR(H5E_FILE, H5E_CANTCREATE, FAIL, "unable to create file's superblock extension")
        ext_created = TRUE;
    } /* end else */
    HDassert(H5F_addr_defined(ext_loc.addr));
    ext_opened = TRUE;

    /* Check if message with ID does not exist in the object header */
    if((status = H5O_msg_exists(&ext_loc, id, dxpl_id)) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to check object header for message or message exists")

    /* Check for creating vs. writing */
    if(may_create) {
	if(status)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "Message should not exist")

	/* Create the message with ID in the superblock extension */
	if(H5O_msg_create(&ext_loc, id, H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, mesg, dxpl_id) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to create the message in object header")
    } /* end if */
    else {
	if(!status)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "Message should exist")

	/* Update the message with ID in the superblock extension */
	if(H5O_msg_write(&ext_loc, id, H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, mesg, dxpl_id) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to write the message in object header")
    } /* end else */

done:
    /* Reset the ring in the DXPL */
    if(H5AC_reset_ring(dxpl, orig_ring) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set property value")

    /* Close the superblock extension, if it was opened */
    if(ext_opened && H5F_super_ext_close(f, &ext_loc, dxpl_id, ext_created) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "unable to close file's superblock extension")

    /* Mark superblock dirty in cache, if superblock extension was created */
    if(ext_created && H5AC_mark_entry_dirty(f->shared->sblock) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTMARKDIRTY, FAIL, "unable to mark superblock as dirty")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_super_ext_write_msg() */


/*-------------------------------------------------------------------------
 * Function:    H5F_super_ext_remove_msg
 *
 * Purpose:     Remove the message with ID from the superblock extension
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; Feb 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_super_ext_remove_msg(H5F_t *f, hid_t dxpl_id, unsigned id)
{
    H5P_genplist_t *dxpl = NULL;        /* DXPL for setting ring */
    H5AC_ring_t orig_ring = H5AC_RING_INV;      /* Original ring value */
    H5O_loc_t 	ext_loc; 	/* "Object location" for superblock extension */
    hbool_t     ext_opened = FALSE;     /* Whether the superblock extension was opened */
    int		null_count = 0;	/* # of null messages */
    htri_t      status;         /* Indicate whether the message exists or not */
    herr_t 	ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Make sure that the superblock extension object header exists */
    HDassert(H5F_addr_defined(f->shared->sblock->ext_addr));

    /* Set the ring type in the DXPL */
    if(H5AC_set_ring(dxpl_id, H5AC_RING_SBE, &dxpl, &orig_ring) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set ring value")

    /* Open superblock extension object header */
    if(H5F_super_ext_open(f, f->shared->sblock->ext_addr, &ext_loc) < 0)
	HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "error in starting file's superblock extension")
    ext_opened = TRUE;

    /* Check if message with ID exists in the object header */
    if((status = H5O_msg_exists(&ext_loc, id, dxpl_id)) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to check object header for message")
    else if(status) { /* message exists */
	H5O_hdr_info_t 	hdr_info;       /* Object header info for superblock extension */

	/* Remove the message */
	if(H5O_msg_remove(&ext_loc, id, H5O_ALL, TRUE, dxpl_id) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete free-space manager info message")

	/* Get info for the superblock extension's object header */
	if(H5O_get_hdr_info(&ext_loc, dxpl_id, &hdr_info) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to retrieve superblock extension info")

	/* If the object header is an empty base chunk, remove superblock extension */
	if(hdr_info.nchunks == 1) {
	    if((null_count = H5O_msg_count(&ext_loc, H5O_NULL_ID, dxpl_id)) < 0)
		HGOTO_ERROR(H5E_SYM, H5E_CANTCOUNT, FAIL, "unable to count messages")
	    else if((unsigned)null_count == hdr_info.nmesgs) {
		HDassert(H5F_addr_defined(ext_loc.addr));
		if(H5O_delete(f, dxpl_id, ext_loc.addr) < 0)
		    HGOTO_ERROR(H5E_SYM, H5E_CANTCOUNT, FAIL, "unable to count messages")
		f->shared->sblock->ext_addr = HADDR_UNDEF;
	    } /* end if */
	} /* end if */
    } /* end if */

done:
    /* Reset the ring in the DXPL */
    if(H5AC_reset_ring(dxpl, orig_ring) < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "unable to set property value")

    /* Close superblock extension object header, if opened */
    if(ext_opened && H5F_super_ext_close(f, &ext_loc, dxpl_id, FALSE) < 0)
       HDONE_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "unable to close file's superblock extension")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5F_super_ext_remove_msg() */


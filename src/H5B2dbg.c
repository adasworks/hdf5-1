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
 * Created:		H5B2dbg.c
 *			Feb  2 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Dump debugging information about a v2 B-tree.
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


/*-------------------------------------------------------------------------
 * Function:	H5B2__hdr_debug
 *
 * Purpose:	Prints debugging info about a B-tree header.
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
H5B2__hdr_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent, int fwidth,
    const H5B2_class_t *type, haddr_t obj_addr)
{
    H5B2_hdr_t	*hdr = NULL;            /* B-tree header info */
    unsigned    u;                      /* Local index variable */
    char        temp_str[128];          /* Temporary string, for formatting */
    H5B2_hdr_cache_ud_t cache_udata;    /* User-data for callback */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(H5F_addr_defined(obj_addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(type);

    /*
     * Load the B-tree header.
     */
    cache_udata.f = f;
    cache_udata.addr = addr;
    cache_udata.ctx_udata = f;
    if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_BT2_HDR, addr, &cache_udata, H5AC__READ_ONLY_FLAG)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL, "unable to load B-tree header")

    /* Set file pointer for this B-tree operation */
    hdr->f = f;

    /* Print opening message */
    HDfprintf(stream, "%*sv2 B-tree Header...\n", indent, "");

    /*
     * Print the values.
     */
    HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
	      "Tree type ID:", hdr->cls->name, (unsigned)hdr->cls->id);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Size of node:",
	      (unsigned)hdr->node_size);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Size of raw (disk) record:",
	      (unsigned)hdr->rrec_size);
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
	      "Dirty flag:",
	      hdr->cache_info.is_dirty ? "True" : "False");
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Depth:",
	      hdr->depth);
    HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
	      "Number of records in tree:",
	      hdr->root.all_nrec);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Number of records in root node:",
	      hdr->root.node_nrec);
    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
	      "Address of root node:",
	      hdr->root.addr);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Split percent:",
	      hdr->split_percent);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Merge percent:",
	      hdr->merge_percent);

    /* Print relevant node info */
    HDfprintf(stream, "%*sNode Info: (max_nrec/split_nrec/merge_nrec)\n", indent, "");
    for(u = 0; u < (unsigned)(hdr->depth + 1); u++) {
        HDsnprintf(temp_str, sizeof(temp_str), "Depth %u:", u);
        HDfprintf(stream, "%*s%-*s (%u/%u/%u)\n", indent + 3, "", MAX(0, fwidth - 3),
            temp_str,
            hdr->node_info[u].max_nrec, hdr->node_info[u].split_nrec, hdr->node_info[u].merge_nrec);
    } /* end for */

done:
    if(hdr) {
        hdr->f = NULL;
        if(H5AC_unprotect(f, dxpl_id, H5AC_BT2_HDR, addr, hdr, H5AC__NO_FLAGS_SET) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_PROTECT, FAIL, "unable to release B-tree header")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__hdr_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__int_debug
 *
 * Purpose:	Prints debugging info about a B-tree internal node
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  4 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2__int_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent, int fwidth,
    const H5B2_class_t *type, haddr_t hdr_addr, unsigned nrec, unsigned depth, haddr_t obj_addr)
{
    H5B2_hdr_t	*hdr = NULL;            /* B-tree header */
    H5B2_internal_t	*internal = NULL;   /* B-tree internal node */
    unsigned	u;                      /* Local index variable */
    char        temp_str[128];          /* Temporary string, for formatting */
    H5B2_hdr_cache_ud_t cache_udata;    /* User-data for callback */
    herr_t      ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_PACKAGE

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(type);
    HDassert(H5F_addr_defined(hdr_addr));
    HDassert(H5F_addr_defined(obj_addr));
    HDassert(nrec > 0);

    /*
     * Load the B-tree header.
     */
    cache_udata.f = f;
    cache_udata.addr = hdr_addr;
    cache_udata.ctx_udata = f;
    if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_BT2_HDR, hdr_addr, &cache_udata, H5AC__READ_ONLY_FLAG)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL, "unable to load B-tree header")

    /* Set file pointer for this B-tree operation */
    hdr->f = f;

    /*
     * Load the B-tree internal node
     */
    H5_CHECK_OVERFLOW(nrec, unsigned, uint16_t);
    H5_CHECK_OVERFLOW(depth, unsigned, uint16_t);
    if(NULL == (internal = H5B2__protect_internal(hdr, dxpl_id, addr, (uint16_t)nrec, (uint16_t)depth, H5AC__READ_ONLY_FLAG)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL, "unable to load B-tree internal node")

    /* Print opening message */
    HDfprintf(stream, "%*sv2 B-tree Internal Node...\n", indent, "");

    /*
     * Print the values.
     */
    HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
	      "Tree type ID:", hdr->cls->name, (unsigned)hdr->cls->id);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Size of node:",
	      (unsigned)hdr->node_size);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Size of raw (disk) record:",
	      (unsigned)hdr->rrec_size);
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
	      "Dirty flag:",
	      internal->cache_info.is_dirty ? "True" : "False");
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Number of records in node:",
	      internal->nrec);

    /* Print all node pointers and records */
    for(u = 0; u < internal->nrec; u++) {
        /* Print node pointer */
        HDsnprintf(temp_str, sizeof(temp_str), "Node pointer #%u: (all/node/addr)", u);
        HDfprintf(stream, "%*s%-*s (%Hu/%u/%a)\n", indent + 3, "", MAX(0, fwidth - 3),
                  temp_str,
                  internal->node_ptrs[u].all_nrec,
                  internal->node_ptrs[u].node_nrec,
                  internal->node_ptrs[u].addr);

        /* Print record */
        HDsnprintf(temp_str, sizeof(temp_str), "Record #%u:", u);
        HDfprintf(stream, "%*s%-*s\n", indent + 3, "", MAX(0, fwidth - 3),
                  temp_str);
        HDassert(H5B2_INT_NREC(internal, hdr, u));
        (void)(type->debug)(stream, indent + 6, MAX (0, fwidth-6),
            H5B2_INT_NREC(internal, hdr, u));
    } /* end for */

    /* Print final node pointer */
    HDsnprintf(temp_str, sizeof(temp_str), "Node pointer #%u: (all/node/addr)", u);
    HDfprintf(stream, "%*s%-*s (%Hu/%u/%a)\n", indent + 3, "", MAX(0, fwidth - 3),
              temp_str,
              internal->node_ptrs[u].all_nrec,
              internal->node_ptrs[u].node_nrec,
              internal->node_ptrs[u].addr);

done:
    if(hdr) {
        hdr->f = NULL;
        if(H5AC_unprotect(f, dxpl_id, H5AC_BT2_HDR, hdr_addr, hdr, H5AC__NO_FLAGS_SET) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_PROTECT, FAIL, "unable to release B-tree header")
    } /* end if */
    if(internal && H5AC_unprotect(f, dxpl_id, H5AC_BT2_INT, addr, internal, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_PROTECT, FAIL, "unable to release B-tree internal node")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__int_debug() */


/*-------------------------------------------------------------------------
 * Function:	H5B2__leaf_debug
 *
 * Purpose:	Prints debugging info about a B-tree leaf node
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Feb  7 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5B2__leaf_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream, int indent, int fwidth,
    const H5B2_class_t *type, haddr_t hdr_addr, unsigned nrec, haddr_t obj_addr)
{
    H5B2_hdr_t	*hdr = NULL;            /* B-tree header */
    H5B2_leaf_t	*leaf = NULL;           /* B-tree leaf node */
    H5B2_hdr_cache_ud_t cache_udata;    /* User-data for callback */
    unsigned	u;                      /* Local index variable */
    char        temp_str[128];          /* Temporary string, for formatting */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);
    HDassert(type);
    HDassert(H5F_addr_defined(hdr_addr));
    HDassert(H5F_addr_defined(obj_addr));
    HDassert(nrec > 0);

    /*
     * Load the B-tree header.
     */
    cache_udata.f = f;
    cache_udata.addr = hdr_addr;
    cache_udata.ctx_udata = f;
    if(NULL == (hdr = (H5B2_hdr_t *)H5AC_protect(f, dxpl_id, H5AC_BT2_HDR, hdr_addr, &cache_udata, H5AC__READ_ONLY_FLAG)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect B-tree header")

    /* Set file pointer for this B-tree operation */
    hdr->f = f;

    /*
     * Load the B-tree leaf node
     */
    H5_CHECK_OVERFLOW(nrec, unsigned, uint16_t);
    if(NULL == (leaf = H5B2__protect_leaf(hdr, dxpl_id, addr, (uint16_t)nrec, H5AC__READ_ONLY_FLAG)))
	HGOTO_ERROR(H5E_BTREE, H5E_CANTPROTECT, FAIL, "unable to protect B-tree leaf node")

    /* Print opening message */
    HDfprintf(stream, "%*sv2 B-tree Leaf Node...\n", indent, "");

    /*
     * Print the values.
     */
    HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
	      "Tree type ID:", hdr->cls->name, (unsigned)hdr->cls->id);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Size of node:",
	      (unsigned)hdr->node_size);
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Size of raw (disk) record:",
	      (unsigned)hdr->rrec_size);
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
	      "Dirty flag:",
	      leaf->cache_info.is_dirty ? "True" : "False");
    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
	      "Number of records in node:",
	      leaf->nrec);

    /* Print all node pointers and records */
    for(u = 0; u < leaf->nrec; u++) {
        /* Print record */
        HDsnprintf(temp_str, sizeof(temp_str), "Record #%u:", u);
        HDfprintf(stream, "%*s%-*s\n", indent + 3, "", MAX(0, fwidth - 3),
                  temp_str);
        HDassert(H5B2_LEAF_NREC(leaf, hdr, u));
        (void)(type->debug)(stream, indent + 6, MAX (0, fwidth-6),
            H5B2_LEAF_NREC(leaf, hdr, u));
    } /* end for */

done:
    if(hdr) {
        hdr->f = NULL;
        if(H5AC_unprotect(f, dxpl_id, H5AC_BT2_HDR, hdr_addr, hdr, H5AC__NO_FLAGS_SET) < 0)
            HDONE_ERROR(H5E_BTREE, H5E_PROTECT, FAIL, "unable to release B-tree header")
    } /* end if */
    if(leaf && H5AC_unprotect(f, dxpl_id, H5AC_BT2_LEAF, addr, leaf, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_BTREE, H5E_PROTECT, FAIL, "unable to release B-tree leaf node")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5B2__leaf_debug() */


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

/*
 * generate plist file
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "H5private.h"
#include "hdf5.h"

static int encode_plist(hid_t plist_id, int little_endian, const char *filename_le, const char *filename_be);

int
main(void)
{
    hid_t dcpl1;	       	/* dataset create prop. list */
    hid_t dapl1;	       	/* dataset access prop. list */
    hid_t dxpl1;	       	/* dataset xfer prop. list */
    hid_t gcpl1;	       	/* group create prop. list */
    hid_t ocpypl1;		/* object copy prop. list */
    hid_t ocpl1;	        /* object create prop. list */
    hid_t lcpl1;	       	/* link create prop. list */
    hid_t lapl1;	       	/* link access prop. list */
    hid_t fapl1;	       	/* file access prop. list */
    hid_t fcpl1;	       	/* file create prop. list */
    hid_t strcpl1;	       	/* string create prop. list */
    hid_t acpl1;	       	/* attribute create prop. list */

    herr_t ret = 0;
    hsize_t chunk_size = 16384;	/* chunk size */ 
    int fill = 2;            /* Fill value */
    hsize_t max_size[1];        /* data space maximum size */
    size_t nslots = 521 * 2;
    size_t nbytes = 1048576 * 10;
    double w0 = 0.5f;
    unsigned max_compact;
    unsigned min_dense;
    const char* c_to_f = "x+32";
    int little_endian;
    H5AC_cache_config_t my_cache_config = {
        H5AC__CURR_CACHE_CONFIG_VERSION,
        1 /*TRUE*/,
        0 /*FALSE*/,
        0 /*FALSE*/,
        "temp",
        1 /*TRUE*/,
        0 /*FALSE*/,
        ( 2 * 2048 * 1024),
        0.3,
        (64 * 1024 * 1024),
        (4 * 1024 * 1024),
        60000,
        H5C_incr__threshold,
        0.8,
        3.0,
        1 /*TRUE*/,
        (8 * 1024 * 1024),
        H5C_flash_incr__add_space,
        2.0,
        0.25,
        H5C_decr__age_out_with_threshold,
        0.997,
        0.8,
        1 /*TRUE*/,
        (3 * 1024 * 1024),
        3,
        0 /*FALSE*/,
        0.2,
        (256 * 2048),
        H5AC_METADATA_WRITE_STRATEGY__PROCESS_0_ONLY};

    /* check endianess */
    {
        short int word = 0x0001;
        char *byte = (char *) &word;

        if(byte[0] == 1)
            /* little endian */
            little_endian = 1;
        else
            /* big endian */
            little_endian = 0;
    }

    /* Explicitly initialize the library, since we are including the private header file */
    H5open();

    /******* ENCODE/DECODE DCPLS *****/
    if((dcpl1 = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        assert(dcpl1 > 0);

    if((ret = H5Pset_chunk(dcpl1, 1, &chunk_size)) < 0)
        assert(ret > 0);

    if((ret = H5Pset_alloc_time(dcpl1, H5D_ALLOC_TIME_LATE)) < 0)
        assert(ret > 0);

    ret = H5Tconvert(H5T_NATIVE_INT, H5T_STD_I32BE, (size_t)1, &fill, NULL, H5P_DEFAULT);
    assert(ret >= 0);
    if((ret = H5Pset_fill_value(dcpl1, H5T_STD_I32BE, &fill)) < 0)
        assert(ret > 0);

    max_size[0] = 100;
    if((ret = H5Pset_external(dcpl1, "ext1.data", (off_t)0, 
                         (hsize_t)(max_size[0] * sizeof(int)/4))) < 0)
        assert(ret > 0);
    if((ret = H5Pset_external(dcpl1, "ext2.data", (off_t)0, 
                         (hsize_t)(max_size[0] * sizeof(int)/4))) < 0)
        assert(ret > 0);
    if((ret = H5Pset_external(dcpl1, "ext3.data", (off_t)0, 
                         (hsize_t)(max_size[0] * sizeof(int)/4))) < 0)
        assert(ret > 0);
    if((ret = H5Pset_external(dcpl1, "ext4.data", (off_t)0, 
                         (hsize_t)(max_size[0] * sizeof(int)/4))) < 0)
        assert(ret > 0);

    if((ret = encode_plist(dcpl1, little_endian, "testfiles/plist_files/dcpl_le", "testfiles/plist_files/dcpl_be")) < 0)
        assert(ret > 0);
        
    /* release resource */
    if((ret = H5Pclose(dcpl1)) < 0)
         assert(ret > 0);


    /******* ENCODE/DECODE DAPLS *****/
    if((dapl1 = H5Pcreate(H5P_DATASET_ACCESS)) < 0)
        assert(dapl1 > 0);

    if((ret = H5Pset_chunk_cache(dapl1, nslots, nbytes, w0)) < 0)
        assert(ret > 0);

    if((ret = encode_plist(dapl1, little_endian, "testfiles/plist_files/dapl_le", "testfiles/plist_files/dapl_be")) < 0)
        assert(ret > 0);
        
    /* release resource */
    if((ret = H5Pclose(dapl1)) < 0)
         assert(ret > 0);

    /******* ENCODE/DECODE DXPLS *****/
    if((dxpl1 = H5Pcreate(H5P_DATASET_XFER)) < 0)
        assert(dxpl1 > 0);
    if((ret = H5Pset_btree_ratios(dxpl1, 0.2f, 0.6f, 0.2f)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_hyper_vector_size(dxpl1, 5)) < 0)
        assert(ret > 0);
#ifdef H5_HAVE_PARALLEL
    if((ret = H5Pset_dxpl_mpio(dxpl1, H5FD_MPIO_COLLECTIVE)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_dxpl_mpio_collective_opt(dxpl1, H5FD_MPIO_INDIVIDUAL_IO)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_dxpl_mpio_chunk_opt(dxpl1, H5FD_MPIO_CHUNK_MULTI_IO)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_dxpl_mpio_chunk_opt_ratio(dxpl1, 30)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_dxpl_mpio_chunk_opt_num(dxpl1, 40)) < 0)
        assert(ret > 0);
#endif/* H5_HAVE_PARALLEL */
    if((ret = H5Pset_edc_check(dxpl1, H5Z_DISABLE_EDC)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_data_transform(dxpl1, c_to_f)) < 0)
        assert(ret > 0);

    if((ret = encode_plist(dxpl1, little_endian, "testfiles/plist_files/dxpl_le", "testfiles/plist_files/dxpl_be")) < 0)
        assert(ret > 0);
        
    /* release resource */
    if((ret = H5Pclose(dxpl1)) < 0)
         assert(ret > 0);


    /******* ENCODE/DECODE GCPLS *****/
    if((gcpl1 = H5Pcreate(H5P_GROUP_CREATE)) < 0)
        assert(gcpl1 > 0);

    if((ret = H5Pset_local_heap_size_hint(gcpl1, 256)) < 0)
         assert(ret > 0);

    if((ret = H5Pset_link_phase_change(gcpl1, 2, 2)) < 0)
         assert(ret > 0);

    /* Query the group creation properties */
    if((ret = H5Pget_link_phase_change(gcpl1, &max_compact, &min_dense)) < 0)
         assert(ret > 0);

    if((ret = H5Pset_est_link_info(gcpl1, 3, 9)) < 0)
         assert(ret > 0);

    if((ret = H5Pset_link_creation_order(gcpl1, (H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED))) < 0)
         assert(ret > 0);

    if((ret = encode_plist(gcpl1, little_endian, "testfiles/plist_files/gcpl_le", "testfiles/plist_files/gcpl_be")) < 0)
        assert(ret > 0);
        
    /* release resource */
    if((ret = H5Pclose(gcpl1)) < 0)
         assert(ret > 0);

    /******* ENCODE/DECODE LCPLS *****/
    if((lcpl1 = H5Pcreate(H5P_LINK_CREATE)) < 0)
        assert(lcpl1 > 0);

    if((ret = H5Pset_create_intermediate_group(lcpl1, 1 /*TRUE*/)) < 0)
        assert(ret > 0);

    if((ret = encode_plist(lcpl1, little_endian, "testfiles/plist_files/lcpl_le", "testfiles/plist_files/lcpl_be")) < 0)
        assert(ret > 0);
        
    /* release resource */
    if((ret = H5Pclose(lcpl1)) < 0)
        assert(ret > 0);

    /******* ENCODE/DECODE OCPYLS *****/
    if((ocpypl1 = H5Pcreate(H5P_OBJECT_COPY)) < 0)
        assert(ocpypl1 > 0);

    ret = H5Pset_copy_object(ocpypl1, H5O_COPY_EXPAND_EXT_LINK_FLAG);
    assert(ret >= 0);

    ret = H5Padd_merge_committed_dtype_path(ocpypl1, "foo");
    assert(ret >= 0);

    ret = H5Padd_merge_committed_dtype_path(ocpypl1, "bar");
    assert(ret >= 0);

    if((ret = encode_plist(ocpypl1, little_endian, "testfiles/plist_files/ocpypl_le", "testfiles/plist_files/ocpypl_be")) < 0)
        assert(ret > 0);
        
    /* release resource */
    if((ret = H5Pclose(ocpypl1)) < 0)
         assert(ret > 0);

    /******* ENCODE/DECODE OCPLS *****/
    if((ocpl1 = H5Pcreate(H5P_OBJECT_CREATE)) < 0)
        assert(ocpl1 > 0);

    if((ret = H5Pset_attr_creation_order(ocpl1, (H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED))) < 0)
         assert(ret > 0);

    if((ret = H5Pset_attr_phase_change (ocpl1, 110, 105)) < 0)
         assert(ret > 0);

    if((ret = H5Pset_filter (ocpl1, H5Z_FILTER_FLETCHER32, 0, (size_t)0, NULL)) < 0)
        assert(ret > 0);

    if((ret = encode_plist(ocpl1, little_endian, "testfiles/plist_files/ocpl_le", "testfiles/plist_files/ocpl_be")) < 0)
        assert(ret > 0);

    /* release resource */
    if((ret = H5Pclose(ocpl1)) < 0)
        assert(ret > 0);

    /******* ENCODE/DECODE LAPLS *****/
    if((lapl1 = H5Pcreate(H5P_LINK_ACCESS)) < 0)
        assert(lapl1 > 0);

    if((ret = H5Pset_nlinks(lapl1, (size_t)134)) < 0)
        assert(ret > 0);

    if((ret = H5Pset_elink_acc_flags(lapl1, H5F_ACC_RDONLY)) < 0)
        assert(ret > 0);

    if((ret = H5Pset_elink_prefix(lapl1, "/tmpasodiasod")) < 0)
        assert(ret > 0);

    /* Create FAPL for the elink FAPL */
    if((fapl1 = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        assert(fapl1 > 0);
    if((ret = H5Pset_alignment(fapl1, 2, 1024)) < 0)
        assert(ret > 0);

    if((ret = H5Pset_elink_fapl(lapl1, fapl1)) < 0)
        assert(ret > 0);

    /* Close the elink's FAPL */
    if((ret = H5Pclose(fapl1)) < 0)
        assert(ret > 0);

    if((ret = encode_plist(lapl1, little_endian, "testfiles/plist_files/lapl_le", "testfiles/plist_files/lapl_be")) < 0)
        assert(ret > 0);

    /* release resource */
    if((ret = H5Pclose(lapl1)) < 0)
        assert(ret > 0);

    /******* ENCODE/DECODE FAPLS *****/
    if((fapl1 = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        assert(fapl1 > 0);

    if((ret = H5Pset_family_offset(fapl1, 1024)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_meta_block_size(fapl1, 2098452)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_sieve_buf_size(fapl1, 1048576)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_alignment(fapl1, 2, 1024)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_cache(fapl1, 1024, 128, 10485760, 0.3f)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_elink_file_cache_size(fapl1, 10485760)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_gc_references(fapl1, 1)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_small_data_block_size(fapl1, 2048)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_libver_bounds(fapl1, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_fclose_degree(fapl1, H5F_CLOSE_WEAK)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_multi_type(fapl1, H5FD_MEM_GHEAP)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_mdc_config(fapl1, &my_cache_config)) < 0)
        assert(ret > 0);
    if((ret = H5Pset_core_write_tracking(fapl1, TRUE, (size_t)(1024 * 1024))) < 0)
        assert(ret > 0);

    if((ret = encode_plist(fapl1, little_endian, "testfiles/plist_files/fapl_le", "testfiles/plist_files/fapl_be")) < 0)
        assert(ret > 0);

    /* release resource */
    if((ret = H5Pclose(fapl1)) < 0)
        assert(ret > 0);

    /******* ENCODE/DECODE FCPLS *****/
    if((fcpl1 = H5Pcreate(H5P_FILE_CREATE)) < 0)
        assert(fcpl1 > 0);

    if((ret = H5Pset_userblock(fcpl1, 1024) < 0))
         assert(ret > 0);

    if((ret = H5Pset_istore_k(fcpl1, 3) < 0))
         assert(ret > 0);

    if((ret = H5Pset_sym_k(fcpl1, 4, 5) < 0))
         assert(ret > 0);

    if((ret = H5Pset_shared_mesg_nindexes(fcpl1, 8) < 0))
         assert(ret > 0);

    if((ret = H5Pset_shared_mesg_index(fcpl1, 1,  H5O_SHMESG_SDSPACE_FLAG, 32) < 0))
         assert(ret > 0);

   if((ret = H5Pset_shared_mesg_phase_change(fcpl1, 60, 20) < 0))
         assert(ret > 0);

    if((ret = H5Pset_sizes(fcpl1, 8, 4) < 0))
         assert(ret > 0);

    if((ret = encode_plist(fcpl1, little_endian, "testfiles/plist_files/fcpl_le", "testfiles/plist_files/fcpl_be")) < 0)
        assert(ret > 0);

    /* release resource */
    if((ret = H5Pclose(fcpl1)) < 0)
        assert(ret > 0);

    /******* ENCODE/DECODE STRCPLS *****/
    strcpl1 = H5Pcreate(H5P_STRING_CREATE);
    assert(strcpl1 > 0);

    ret = H5Pset_char_encoding(strcpl1, H5T_CSET_UTF8);
    assert(ret >= 0);

    ret = encode_plist(strcpl1, little_endian, "testfiles/plist_files/strcpl_le", "testfiles/plist_files/strcpl_be");
    assert(ret > 0);

    /* release resource */
    ret = H5Pclose(strcpl1);
    assert(ret >= 0);

    /******* ENCODE/DECODE ACPLS *****/
    acpl1 = H5Pcreate(H5P_ATTRIBUTE_CREATE);
    assert(acpl1 > 0);

    ret = H5Pset_char_encoding(acpl1, H5T_CSET_UTF8);
    assert(ret >= 0);

    ret = encode_plist(acpl1, little_endian, "testfiles/plist_files/acpl_le", "testfiles/plist_files/acpl_be");
    assert(ret > 0);

    /* release resource */
    ret = H5Pclose(acpl1);
    assert(ret >= 0);

    return 0;
}

static int
encode_plist(hid_t plist_id, int little_endian, const char *filename_le, const char *filename_be)
{
    int fd = 0; /* file descriptor */
    herr_t ret = 0;
    void *temp_buf = NULL;
    size_t temp_size = 0;
    ssize_t write_size;

    /* first call to encode returns only the size of the buffer needed */
    if((ret = H5Pencode(plist_id, NULL, &temp_size)) < 0)
        assert(ret > 0);

    temp_buf = (void *)HDmalloc(temp_size);
    assert(temp_buf);

    if((ret = H5Pencode(plist_id, temp_buf, &temp_size)) < 0)
        assert(ret > 0);

    if(little_endian)
        fd = HDopen(filename_le, O_RDWR | O_CREAT | O_TRUNC, 0666);
    else
        fd = HDopen(filename_be, O_RDWR | O_CREAT | O_TRUNC, 0666);
    assert(fd > 0);

    write_size = HDwrite(fd, temp_buf, temp_size);
    assert(write_size == (ssize_t)temp_size);

    HDclose(fd);
    
    HDfree(temp_buf);

    return 1;
}


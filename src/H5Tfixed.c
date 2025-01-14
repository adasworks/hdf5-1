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
 * Module Info: This module contains the functionality for fixed-point (i.e.
 *      integer) datatypes in the H5T interface.
 */

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T_init_fixed_interface


#include "H5private.h"		/*generic functions			  */
#include "H5Eprivate.h"		/*error handling			  */
#include "H5Iprivate.h"		/*ID functions		   		  */
#include "H5Tpkg.h"		/*data-type functions			  */


/*--------------------------------------------------------------------------
NAME
   H5T_init_fixed_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_fixed_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_fixed_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_fixed_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_sign
 *
 * Purpose:	Retrieves the sign type for an integer type.
 *
 * Return:	Success:	The sign type.
 *
 *		Failure:	H5T_SGN_ERROR (Negative)
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *	Robb Matzke, 22 Dec 1998
 *	Also works with derived datatypes.
 *-------------------------------------------------------------------------
 */
H5T_sign_t
H5Tget_sign(hid_t type_id)
{
    H5T_t		*dt = NULL;
    H5T_sign_t		ret_value;

    FUNC_ENTER_API(H5T_SGN_ERROR)
    H5TRACE1("Ts", "i", type_id);

    /* Check args */
    if (NULL == (dt = (H5T_t *)H5I_object_verify(type_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_SGN_ERROR, "not an integer datatype")

    ret_value = H5T_get_sign(dt);

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5T_get_sign
 *
 * Purpose:	Private function for H5Tget_sign.  Retrieves the sign type
 *              for an integer type.
 *
 * Return:	Success:	The sign type.
 *
 *		Failure:	H5T_SGN_ERROR (Negative)
 *
 * Programmer:	Raymond Lu
 *		October 8, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_sign_t
H5T_get_sign(H5T_t const *dt)
{
    H5T_sign_t		ret_value;

    FUNC_ENTER_NOAPI(H5T_SGN_ERROR)

    HDassert(dt);

    /* Defer to parent */
    while(dt->shared->parent)
        dt = dt->shared->parent;

    /* Check args */
    if (H5T_INTEGER!=dt->shared->type)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_SGN_ERROR, "operation not defined for datatype class")

    /* Sign */
    ret_value = dt->shared->u.atomic.u.i.sign;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}



/*-------------------------------------------------------------------------
 * Function:	H5Tset_sign
 *
 * Purpose:	Sets the sign property for an integer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 * 	Robb Matzke, 22 Dec 1998
 *	Also works with derived datatypes.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_sign(hid_t type_id, H5T_sign_t sign)
{
    H5T_t	*dt = NULL;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iTs", type_id, sign);

    /* Check args */
    if (NULL == (dt = (H5T_t *)H5I_object_verify(type_id,H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an integer datatype")
    if (H5T_STATE_TRANSIENT!=dt->shared->state)
        HGOTO_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "datatype is read-only")
    if (sign < H5T_SGN_NONE || sign >= H5T_NSGN)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal sign type")
    if (H5T_ENUM==dt->shared->type && dt->shared->u.enumer.nmembs>0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not allowed after members are defined")
    while (dt->shared->parent)
        dt = dt->shared->parent; /*defer to parent*/
    if (H5T_INTEGER!=dt->shared->type)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "operation not defined for datatype class")

    /* Commit */
    dt->shared->u.atomic.u.i.sign = sign;

done:
    FUNC_LEAVE_API(ret_value)
}


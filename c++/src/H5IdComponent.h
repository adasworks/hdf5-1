// C++ informative line for the emacs editor: -*- C++ -*-
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

#ifndef __IdComponent_H
#define __IdComponent_H

#ifndef H5_NO_NAMESPACE
namespace H5 {
#endif

class DataSpace;
/*! \class IdComponent
    \brief Class IdComponent provides wrappers of the C functions that
     operate on an HDF5 identifier.

    In most cases, the C library handles these operations and an application
    rarely needs them.
*/
class H5_DLLCPP IdComponent {
   public:
	// Increment reference counter.
	void incRefCount(const hid_t obj_id) const;
	void incRefCount() const;

	// Decrement reference counter.
	void decRefCount(const hid_t obj_id) const;
	void decRefCount() const;

	// Get the reference counter to this identifier.
	int getCounter(const hid_t obj_id) const;
	int getCounter() const;

	// Returns an HDF5 object type, given the object id.
	static H5I_type_t getHDFObjType(const hid_t obj_id);

	// Returns an HDF5 object type of this object.
	H5I_type_t getHDFObjType() const;

	// Assignment operator.
	IdComponent& operator=( const IdComponent& rhs );

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	// Gets the identifier of this object.
	virtual hid_t getId () const = 0;
#endif // DOXYGEN_SHOULD_SKIP_THIS

	// Sets the identifier of this object to a new value.
	void setId(const hid_t new_id);

	// *** Deprecation warning ***
	// The following two constructors are no longer appropriate after the
	// data member "id" had been moved to the sub-classes.
	// The copy constructor is a noop and is removed in 1.8.15 and the
	// other will be removed from 1.10 release, and then from 1.8 if its
	// removal does not raise any problems in two 1.10 releases.

	// Creates an object to hold an HDF5 identifier.
	IdComponent( const hid_t h5_id );

	// Copy constructor: makes copy of the original IdComponent object.
	// IdComponent( const IdComponent& original );

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	// Pure virtual function for there are various H5*close for the
	// subclasses.
	virtual void close() = 0;

	// Makes and returns the string "<class-name>::<func_name>";
	// <class-name> is returned by fromClass().
	H5std_string inMemFunc(const char* func_name) const;

	///\brief Returns this class name.
	virtual H5std_string fromClass() const { return("IdComponent");}

#endif // DOXYGEN_SHOULD_SKIP_THIS

	// Destructor
	virtual ~IdComponent();

   protected:
	// Default constructor.
	IdComponent();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	// Gets the name of the file, in which an HDF5 object belongs.
	H5std_string p_get_file_name() const;

	// Verifies that the given id is valid.
	static bool p_valid_id(const hid_t obj_id);

	// Sets the identifier of this object to a new value. - this one
	// doesn't increment reference count
	virtual void p_setId(const hid_t new_id) = 0;
	//virtual void p_setId(const hid_t new_id);

#endif // DOXYGEN_SHOULD_SKIP_THIS

}; // end class IdComponent

#ifndef H5_NO_NAMESPACE
}
#endif
#endif // __IdComponent_H

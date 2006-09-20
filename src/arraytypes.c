#include "arraytypes.h"
#include "utils.h"

/* Get the correct HDF5 type for a format code.  We can't manage to do
 * the mapping with a table because the HDF5 types are not constant
 * values and are defined by executing a function.  So we do that in a
 * switch case. */

hid_t convArrayType(int fmt, size_t size, char *byteorder)
{
   hid_t type_id;

   switch(fmt) {
    case NPY_STRING:
      /*      return H5T_NATIVE_CHAR; */
      /* An H5T_NATIVE_CHAR is interpreted as a signed byte by HDF5
       * so, we have to create a string type of length 1 so as to
       * represent a char.
       */
      type_id = H5Tcopy(H5T_C_S1);
      H5Tset_size(type_id, size);

      return type_id;
    /* The next two maps are for time datatypes. */
    case 't':
      type_id = H5Tcopy(H5T_UNIX_D32BE);
      break;
    case 'T':
      type_id = H5Tcopy(H5T_UNIX_D64BE);
      break;
    case NPY_BOOL:
      /* The solution below choose a 8 bits bitfield and set a
	 precision of 1. It seems as if H5T_STD_B8LE and H5T_STD_B8BE
	 both return a type little endian (at least on Intel platforms).
	 Anyway, for a 8-bit type that should not matter.
	 */
      printf("Hello!\n");
      type_id = H5Tcopy(H5T_NATIVE_B8);
      H5Tset_precision(type_id, 1);
      /* These calls does not reduce the storage needs,
	 so it would be better to comment them? */
      /* Definitely yes, they did not give more information to the type */
/*       H5Tset_offset(type_id, 0); */
/*       H5Tset_pad(type_id, H5T_PAD_ZERO, H5T_PAD_ZERO); */
      break;
    case NPY_INT8:
      type_id = H5Tcopy(H5T_NATIVE_SCHAR);
      break;
    case NPY_UINT8:
      type_id = H5Tcopy(H5T_NATIVE_UCHAR);
      break;
    case NPY_INT16:
      type_id = H5Tcopy(H5T_NATIVE_SHORT);
      break;
    case NPY_UINT16:
      type_id = H5Tcopy(H5T_NATIVE_USHORT);
      break;
    case NPY_INT32:
      type_id = H5Tcopy(H5T_NATIVE_INT);
      break;
    case NPY_UINT32:
      type_id = H5Tcopy(H5T_NATIVE_UINT);
      break;
    case NPY_INT64:
      type_id = H5Tcopy(H5T_NATIVE_LLONG);
      break;
    case NPY_UINT64:
      type_id = H5Tcopy(H5T_NATIVE_ULLONG);
      break;
    case NPY_FLOAT32:
      type_id = H5Tcopy(H5T_NATIVE_FLOAT);
      break;
    case NPY_FLOAT64:
      type_id = H5Tcopy(H5T_NATIVE_DOUBLE);
      break;
    case NPY_COMPLEX64:
      type_id = create_native_complex64(byteorder);
      break;
    case NPY_COMPLEX128:
      type_id = create_native_complex128(byteorder);
      break;
    default:
#ifdef DEBUG
      printf("Error: bad char <%c> in array format\n", fmt);
#endif /* DEBUG */
      return -1;
   }

   /* Set the byteorder datatype */
   if (set_order(type_id, byteorder) < 0) return -1;

   return type_id;
}


/* Routine to map the atomic type to a numpy typecode
 */
size_t getArrayType(hid_t type_id,
		    int *fmt)
{
  H5T_class_t class_id;
  size_t type_size;
  /*   size_t type_precision; */  /* No used, but why? */
  H5T_sign_t sign;

  /* Get the necessary info from the type */
  class_id = H5Tget_class( type_id );
  type_size = H5Tget_size( type_id );
/*   type_precision = H5Tget_precision( type_id ); */
  if ( (class_id == H5T_INTEGER) ) /* Only class integer can be signed */
    sign = H5Tget_sign( type_id );
  else
    sign = -1;		/* Means no sign */

  switch(class_id) {
  case H5T_BITFIELD:
    *fmt = NPY_BOOL;              /* boolean */
    break;
  case H5T_INTEGER:           /* int (bool, byte, short, long, long long) */
    switch (type_size) {
    case 1:                        /* byte */
      if ( sign )
	*fmt = NPY_INT8;                /* signed byte */
      else
	*fmt = NPY_UINT8;             /* unsigned byte */
      break;
    case 2:                        /* short */
      if ( sign )
	 *fmt = NPY_INT16;                /* signed short */
      else
	*fmt = NPY_UINT16;                /* unsigned short */
      break;
    case 4:                        /* long */
      if ( sign )
	*fmt = NPY_INT32;                /* signed long */
      else
	*fmt = NPY_UINT32;                /* unsigned long */
      break;
    case 8:                        /* long long */
      if ( sign )
	*fmt = NPY_INT64;                /* signed long long */
      else
	*fmt = NPY_UINT64;                /* unsigned long long */
      break;
    default:
      /* This should never happen */
      goto out;
    }
    break; /* case H5T_INTEGER */
  case H5T_FLOAT:                   /* float (single or double) */
    switch (type_size) {
    case 4:
	*fmt = NPY_FLOAT32;                 /* float */
	break;
    case 8:
	*fmt = NPY_FLOAT64;                 /* double */
	break;
    default:
      /* This should never happen */
      goto out;
    }
    break; /* case H5T_FLOAT */
  case H5T_COMPOUND:                /* might be complex (single or double) */
    if (is_complex(type_id)) {
      switch (get_complex_precision(type_id)) {
      case 32:
	*fmt = NPY_COMPLEX64;               /* float complex */
	break;
      case 64:
	*fmt = NPY_COMPLEX128;               /* double complex */
	break;
      default:
	/* This should never happen */
	goto out;
      }
    } else {
      fprintf(stderr, "this H5T_COMPOUND class is not a complex number\n");
      goto out;
    }
    break; /* case H5T_COMPOUND */
  case H5T_STRING:                  /* char or string */
      *fmt = NPY_STRING;                   /* chararray */
    break; /* case H5T_STRING */
  case H5T_TIME:                    /* time (integer or double) */
    switch (type_size) {
    case 4:
	*fmt = (int)'t';            /* integer */
	break;
    case 8:
	*fmt = (int)'T';            /* double */
	break;
    default:
      /* This should never happen */
      goto out;
    }
    break; /* case H5T_TIME */
  case H5T_ENUM:                    /* enumerated type */
    *fmt = (int)('e');              /* will get type from enum description */
    break; /* case H5T_ENUM */
  default: /* Any other class type */
    /* This should never happen with Numeric arrays */
    fprintf(stderr, "class %d not supported. Sorry!\n", class_id);
    goto out;
  }

  return type_size;

 out:
  /* If we reach this line, there should be an error */
  return -1;

}

#ifdef MAIN
int main(int args, char *argv[])
{
   char *byteorder;
   int fmt;
   size_t size;

   printf("args # --> %d\n", args);
   printf("arg 0 --> %s\n", argv[0]);
   printf("arg 1 --> %s\n", argv[1]);
   printf("arg 2 --> %s\n", argv[2]);

   fmt = atoi(argv[1]);
   size = atoi(argv[2]);
   byteorder = "little";
   printf("The array format is %c\n", fmt);
   printf("The correspondent HDF5 variable is %d \n",
	  convArrayType(fmt, size, byteorder));
}
#endif /* MAIN */

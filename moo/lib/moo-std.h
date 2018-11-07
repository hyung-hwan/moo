#ifndef _MOO_STD_H_
#define _MOO_STD_H_

#include <moo.h>

#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT moo_t* moo_openstd (
	moo_oow_t     xtnsize, 
	moo_oow_t     memsize,
	moo_errnum_t* errnum
);

MOO_EXPORT void* moo_getxtnstd (
	moo_t*        moo
);

#if defined(__cplusplus)
}
#endif


#endif

#ifndef _MOO_STD_H_
#define _MOO_STD_H_

#include <moo.h>

enum moo_cfg_type_t
{
	MOO_CFG_TYPE_B,
	MOO_CFG_TYPE_U
};
typedef enum moo_cfg_type_t moo_cfg_type_t;

struct moo_cfg_t
{
	moo_cfg_type_t type;

	moo_oow_t memsize;
	int large_pages;

	const void* logopt;
	const void* dbgopt;
};
typedef struct moo_cfg_t moo_cfg_t;

#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT moo_t* moo_openstd (
	moo_oow_t        xtnsize, 
	const moo_cfg_t* cfg,
	moo_errinf_t*    errinfo
);

MOO_EXPORT void* moo_getxtnstd (
	moo_t*        moo
);

#if defined(__cplusplus)
}
#endif


#endif

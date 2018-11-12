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


enum moo_iostd_type_t
{
	MOO_IOSTD_FILE,
	MOO_IOSTD_FILEB,
	MOO_IOSTD_FILEU
};
typedef enum moo_iostd_type_t moo_iostd_type_t;

struct moo_iostd_t
{
	moo_iostd_type_t type;
	union
	{
		struct
		{
			const moo_ooch_t* path;
		} file;

		struct
		{
			const moo_bch_t* path;
		} fileb;

		struct
		{
			const moo_uch_t* path;
		} fileu;
	} u;
};
typedef struct moo_iostd_t moo_iostd_t;

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

MOO_EXPORT int moo_compilestd(
	moo_t*             moo,
	const moo_iostd_t* instd,
	moo_oow_t          count
);
#if defined(__cplusplus)
}
#endif


#endif

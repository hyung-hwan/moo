#ifndef _MOO_STD_H_
#define _MOO_STD_H_

#include <moo.h>

enum moo_cfgstd_type_t
{
	MOO_CFGSTD_OPT,
	MOO_CFGSTD_OPTB,
	MOO_CFGSTD_OPTU
};
typedef enum moo_cfgstd_type_t moo_cfgstd_type_t;

struct moo_cfgstd_t
{
	moo_cfgstd_type_t type;

	moo_oow_t memsize;
	int large_pages;
	moo_cmgr_t* cmgr;

	union
	{
		struct
		{
			const moo_ooch_t* log;
			const moo_ooch_t* dbg;
		} opt;

		struct 
		{
			const moo_bch_t* log;
			const moo_bch_t* dbg;
		} optb;

		struct
		{
			const moo_uch_t* log;
			const moo_uch_t* dbg;
		} optu;
	} u;
};
typedef struct moo_cfgstd_t moo_cfgstd_t;


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

MOO_EXPORT void moo_start_ticker (
	void
);

MOO_EXPORT void moo_stop_ticker (
	void
);

MOO_EXPORT moo_t* moo_openstd (
	moo_oow_t           xtnsize, 
	const moo_cfgstd_t* cfg,
	moo_errinf_t*       errinfo
);

MOO_EXPORT void* moo_getxtnstd (
	moo_t*        moo
);

MOO_EXPORT void moo_abortstd (
	moo_t*        moo
);

MOO_EXPORT int moo_compilestd(
	moo_t*             moo,
	const moo_iostd_t* in,
	moo_oow_t          count
);

MOO_EXPORT void moo_rcvtickstd (
	moo_t*             moo,
	int                v
);

#if defined(__cplusplus)
}
#endif


#endif

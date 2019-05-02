/* test endian conversion macros */

#include <moo-utl.h>
#include <stdio.h>
#include "t.h"

int main ()
{
	{
		union {
			moo_uint16_t u16;
			moo_uint8_t arr[2];
		} x;

		x.arr[0] = 0x11;
		x.arr[1] = 0x22;

		printf("x.u16 = 0x%04x\n", x.u16);
		printf("htole16(x.u16) = 0x%04x\n", moo_htole16(x.u16));
		printf("htobe16(x.u16) = 0x%04x\n", moo_htobe16(x.u16));

		T_ASSERT1 (x.u16 != moo_htole16(x.u16) || x.u16 != moo_htobe16(x.u16), "u16 endian conversion #0");
		T_ASSERT1 (x.u16 == moo_le16toh(moo_htole16(x.u16)), "u16 endian conversion #1");
		T_ASSERT1 (x.u16 == moo_be16toh(moo_htobe16(x.u16)), "u16 endian conversion #2");
		T_ASSERT1 (x.u16 == moo_ntoh16(moo_hton16(x.u16)), "u16 endian conversion #3");

		#define X_CONST (0x1122)
		T_ASSERT1 (X_CONST != MOO_CONST_HTOLE16(X_CONST) || X_CONST != MOO_CONST_HTOBE16(X_CONST), "u16 constant endian conversion #0");
		T_ASSERT1 (X_CONST == MOO_CONST_LE16TOH(MOO_CONST_HTOLE16(X_CONST)), "u16 constant endian conversion #1");
		T_ASSERT1 (X_CONST == MOO_CONST_BE16TOH(MOO_CONST_HTOBE16(X_CONST)), "u16 constant endian conversion #2");
		T_ASSERT1 (X_CONST == MOO_CONST_NTOH16(MOO_CONST_HTON16(X_CONST)), "u16 constant endian conversion #3");
		#undef X_CONST
	}


	{
		union {
			moo_uint32_t u32;
			moo_uint8_t arr[4];
		} x;

		x.arr[0] = 0x11;
		x.arr[1] = 0x22;
		x.arr[2] = 0x33;
		x.arr[3] = 0x44;

		printf("x.u32 = 0x%08x\n", x.u32);
		printf("htole32(x.u32) = 0x%08x\n", moo_htole32(x.u32));
		printf("htobe32(x.u32) = 0x%08x\n", moo_htobe32(x.u32));

		T_ASSERT1 (x.u32 != moo_htole32(x.u32) || x.u32 != moo_htobe32(x.u32), "u32 endian conversion #0");
		T_ASSERT1 (x.u32 == moo_le32toh(moo_htole32(x.u32)), "u32 endian conversion #1");
		T_ASSERT1 (x.u32 == moo_be32toh(moo_htobe32(x.u32)), "u32 endian conversion #2");
		T_ASSERT1 (x.u32 == moo_ntoh32(moo_hton32(x.u32)), "u32 endian conversion #3");

		#define X_CONST (0x11223344)
		T_ASSERT1 (X_CONST != MOO_CONST_HTOLE32(X_CONST) || X_CONST != MOO_CONST_HTOBE32(X_CONST), "u32 constant endian conversion #0");
		T_ASSERT1 (X_CONST == MOO_CONST_LE32TOH(MOO_CONST_HTOLE32(X_CONST)), "u32 constant endian conversion #1");
		T_ASSERT1 (X_CONST == MOO_CONST_BE32TOH(MOO_CONST_HTOBE32(X_CONST)), "u32 constant endian conversion #2");
		T_ASSERT1 (X_CONST == MOO_CONST_NTOH32(MOO_CONST_HTON32(X_CONST)), "u32 constant endian conversion #3");
		#undef X_CONST
	}

#if defined(MOO_HAVE_UINT64_T)
	{
		union {
			moo_uint64_t u64;
			moo_uint8_t arr[8];
		} x;

		x.arr[0] = 0x11;
		x.arr[1] = 0x22;
		x.arr[2] = 0x33;
		x.arr[3] = 0x44;
		x.arr[4] = 0x55;
		x.arr[5] = 0x66;
		x.arr[6] = 0x77;
		x.arr[7] = 0x88;

		printf("x.u64 = 0x%016llx\n", (unsigned long long)x.u64);
		printf("htole64(x.u64) = 0x%016llx\n", (unsigned long long)moo_htole64(x.u64));
		printf("htobe64(x.u64) = 0x%016llx\n", (unsigned long long)moo_htobe64(x.u64));

		T_ASSERT1 (x.u64 != moo_htole64(x.u64) || x.u64 != moo_htobe64(x.u64), "u64 endian conversion #0");
		T_ASSERT1 (x.u64 == moo_le64toh(moo_htole64(x.u64)), "u64 endian conversion #1");
		T_ASSERT1 (x.u64 == moo_be64toh(moo_htobe64(x.u64)), "u64 endian conversion #2");
		T_ASSERT1 (x.u64 == moo_ntoh64(moo_hton64(x.u64)), "u64 endian conversion #3");

		#define X_CONST (((moo_uint64_t)0x11223344 << 32) | (moo_uint64_t)0x55667788)
		T_ASSERT1 (X_CONST != MOO_CONST_HTOLE64(X_CONST) || X_CONST != MOO_CONST_HTOBE64(X_CONST), "u64 constant endian conversion #0");
		T_ASSERT1 (X_CONST == MOO_CONST_LE64TOH(MOO_CONST_HTOLE64(X_CONST)), "u64 constant endian conversion #1");
		T_ASSERT1 (X_CONST == MOO_CONST_BE64TOH(MOO_CONST_HTOBE64(X_CONST)), "u64 constant endian conversion #2");
		T_ASSERT1 (X_CONST == MOO_CONST_NTOH64(MOO_CONST_HTON64(X_CONST)), "u64 constant endian conversion #3");
		#undef X_CONST
	}
#endif

#if defined(MOO_HAVE_UINT128_T)
	{
		union {
			moo_uint128_t u128;
			moo_uint8_t arr[16];
		} x;
		moo_uint128_t tmp;

		x.arr[0] = 0x11;
		x.arr[1] = 0x22;
		x.arr[2] = 0x33;
		x.arr[3] = 0x44;
		x.arr[4] = 0x55;
		x.arr[5] = 0x66;
		x.arr[6] = 0x77;
		x.arr[7] = 0x88;
		x.arr[8] = 0x99;
		x.arr[9] = 0xaa;
		x.arr[10] = 0xbb;
		x.arr[11] = 0xcc;
		x.arr[12] = 0xdd;
		x.arr[13] = 0xee;
		x.arr[14] = 0xff;
		x.arr[15] = 0xfa;

		printf("x.u128 = 0x%016llx%016llx\n", (unsigned long long)(moo_uint64_t)(x.u128 >> 64), (unsigned long long)(moo_uint64_t)(x.u128 >> 0));

		tmp = moo_htole128(x.u128);
		printf("htole128(tmp) = 0x%016llx%016llx\n", (unsigned long long)(moo_uint64_t)(tmp >> 64), (unsigned long long)(moo_uint64_t)(tmp >> 0));

		tmp = moo_htobe128(x.u128);
		printf("htobe128(tmp) = 0x%016llx%016llx\n", (unsigned long long)(moo_uint64_t)(tmp >> 64), (unsigned long long)(moo_uint64_t)(tmp >> 0));

		T_ASSERT1 (x.u128 != moo_htole128(x.u128) || x.u128 != moo_htobe128(x.u128), "u128 endian conversion #0");
		T_ASSERT1 (x.u128 == moo_le128toh(moo_htole128(x.u128)), "u128 endian conversion #1");
		T_ASSERT1 (x.u128 == moo_be128toh(moo_htobe128(x.u128)), "u128 endian conversion #2");
		T_ASSERT1 (x.u128 == moo_ntoh128(moo_hton128(x.u128)), "u128 endian conversion #3");

		#define X_CONST (((moo_uint128_t)0x11223344 << 96) | ((moo_uint128_t)0x55667788 << 64) | ((moo_uint128_t)0x99aabbcc << 32)  | ((moo_uint128_t)0xddeefffa))
		T_ASSERT1 (X_CONST != MOO_CONST_HTOLE128(X_CONST) || X_CONST != MOO_CONST_HTOBE128(X_CONST), "u128 constant endian conversion #0");
		T_ASSERT1 (X_CONST == MOO_CONST_LE128TOH(MOO_CONST_HTOLE128(X_CONST)), "u128 constant endian conversion #1");
		T_ASSERT1 (X_CONST == MOO_CONST_BE128TOH(MOO_CONST_HTOBE128(X_CONST)), "u128 constant endian conversion #2");
		T_ASSERT1 (X_CONST == MOO_CONST_NTOH128(MOO_CONST_HTON128(X_CONST)), "u128 constant endian conversion #3");
		#undef X_CONST
	}
#endif

	return 0;

oops:
	return -1;
}

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
	}
#endif

	return 0;

oops:
	return -1;
}

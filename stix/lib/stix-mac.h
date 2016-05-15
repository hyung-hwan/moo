/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is for class Mac OS */

/* Mac OS on PPC and m68k uses the big endian mode */
#define STIX_ENDIAN_BIG 

#if defined(__MWERKS__)
#	define STIX_SIZEOF_CHAR        1
#	define STIX_SIZEOF_SHORT       2
#	define STIX_SIZEOF_INT         4
#	define STIX_SIZEOF_LONG        4
#	define STIX_SIZEOF_LONG_LONG   8
#	define STIX_SIZEOF_VOID_P      4
#	define STIX_SIZEOF_FLOAT       4
#	define STIX_SIZEOF_DOUBLE      8
#	define STIX_SIZEOF_LONG_DOUBLE 8
#	define STIX_SIZEOF_WCHAR_T     2

#	define STIX_SIZEOF___INT8      1
#	define STIX_SIZEOF___INT16     2
#	define STIX_SIZEOF___INT32     4
#	define STIX_SIZEOF___INT64     8
#	define STIX_SIZEOF___INT128    0

#	define STIX_SIZEOF_OFF64_T     0
#	define STIX_SIZEOF_OFF_T       8

#	define STIX_SIZEOF_MBSTATE_T   STIX_SIZEOF_LONG
#	define STIX_MBLEN_MAX          16

#else
#	error Define the size of various data types.
#endif


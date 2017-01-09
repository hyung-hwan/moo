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

/*
Macro Meaning 
_WIN64 A 64-bit platform. 
_WIN32 A 32-bit platform.  This value is also defined by the 64-bit 
       compiler for backward compatibility. 
_WIN16 A 16-bit platform 

The following macros are specific to the architecture.

Macro Meaning 
_M_IA64 Intel Itanium Processor Family 
_M_IX86 x86 platform 
_M_X64 x64 platform 
*/

/* windows for most of non-x86 platforms dropped. 
 * make it selective to support old non-x86 windows platforms. */
#define MOO_ENDIAN_LITTLE 

#if defined(__WATCOMC__)
#	define MOO_SIZEOF_CHAR        1
#	define MOO_SIZEOF_SHORT       2
#	define MOO_SIZEOF_INT         4
#	define MOO_SIZEOF_LONG        4
#	if (__WATCOMC__ < 1200)
#		define MOO_SIZEOF_LONG_LONG   0
#	else
#		define MOO_SIZEOF_LONG_LONG   8
#	endif

#	if defined(_WIN64)
#		define MOO_SIZEOF_VOID_P      8
#	else
#		define MOO_SIZEOF_VOID_P      4
#	endif
#	define MOO_SIZEOF_FLOAT       4
#	define MOO_SIZEOF_DOUBLE      8
#	define MOO_SIZEOF_LONG_DOUBLE 8
#	define MOO_SIZEOF_WCHAR_T     2

#	define MOO_SIZEOF___INT8      1
#	define MOO_SIZEOF___INT16     2
#	define MOO_SIZEOF___INT32     4
#	define MOO_SIZEOF___INT64     8
#	define MOO_SIZEOF___INT128    0

#	define MOO_SIZEOF_OFF64_T     0
#	define MOO_SIZEOF_OFF_T       8

#	define MOO_SIZEOF_MBSTATE_T   MOO_SIZEOF_LONG
#	define MOO_MBLEN_MAX          16

#elif defined(__GNUC__) || defined(__DMC__) || defined(__POCC__)
#	define MOO_SIZEOF_CHAR        1
#	define MOO_SIZEOF_SHORT       2
#	define MOO_SIZEOF_INT         4
#	define MOO_SIZEOF_LONG        4
#	define MOO_SIZEOF_LONG_LONG   8

#	if defined(_WIN64)
#		define MOO_SIZEOF_VOID_P      8
#	else
#		define MOO_SIZEOF_VOID_P      4
#	endif
#	define MOO_SIZEOF_FLOAT       4
#	define MOO_SIZEOF_DOUBLE      8
#	define MOO_SIZEOF_LONG_DOUBLE 16
#	define MOO_SIZEOF_WCHAR_T     2

#	define MOO_SIZEOF___INT8      0
#	define MOO_SIZEOF___INT16     0
#	define MOO_SIZEOF___INT32     0
#	define MOO_SIZEOF___INT64     0
#	define MOO_SIZEOF___INT128    0

#	define MOO_SIZEOF_OFF64_T     0
#	define MOO_SIZEOF_OFF_T       8

#	define MOO_SIZEOF_MBSTATE_T   MOO_SIZEOF_LONG
#	define MOO_MBLEN_MAX          16

#elif defined(_MSC_VER)
#	define MOO_SIZEOF_CHAR        1
#	define MOO_SIZEOF_SHORT       2
#	define MOO_SIZEOF_INT         4
#	define MOO_SIZEOF_LONG        4
#	if (_MSC_VER>=1310)
#		define MOO_SIZEOF_LONG_LONG   8
#	else
#		define MOO_SIZEOF_LONG_LONG   0
#	endif

#	if defined(_WIN64)
#		define MOO_SIZEOF_VOID_P      8
#	else
#		define MOO_SIZEOF_VOID_P      4
#	endif
#	define MOO_SIZEOF_FLOAT       4
#	define MOO_SIZEOF_DOUBLE      8
#	define MOO_SIZEOF_LONG_DOUBLE 8
#	define MOO_SIZEOF_WCHAR_T     2

#	define MOO_SIZEOF___INT8      1
#	define MOO_SIZEOF___INT16     2
#	define MOO_SIZEOF___INT32     4
#	define MOO_SIZEOF___INT64     8
#	define MOO_SIZEOF___INT128    0

#	define MOO_SIZEOF_OFF64_T     0
#	define MOO_SIZEOF_OFF_T       8

#	define MOO_SIZEOF_MBSTATE_T   MOO_SIZEOF_LONG
#	define MOO_MBLEN_MAX          8

#elif defined(__BORLANDC__)

#	define MOO_SIZEOF_CHAR        1
#	define MOO_SIZEOF_SHORT       2
#	define MOO_SIZEOF_INT         4
#	define MOO_SIZEOF_LONG        4
#	define MOO_SIZEOF_LONG_LONG   0

#	if defined(_WIN64)
#		define MOO_SIZEOF_VOID_P      8
#	else
#		define MOO_SIZEOF_VOID_P      4
#	endif
#	define MOO_SIZEOF_FLOAT       4
#	define MOO_SIZEOF_DOUBLE      8
#	define MOO_SIZEOF_LONG_DOUBLE 8
#	define MOO_SIZEOF_WCHAR_T     2

#	define MOO_SIZEOF___INT8      1
#	define MOO_SIZEOF___INT16     2
#	define MOO_SIZEOF___INT32     4
#	define MOO_SIZEOF___INT64     8
#	define MOO_SIZEOF___INT128    0

#	define MOO_SIZEOF_OFF64_T     0
#	define MOO_SIZEOF_OFF_T       8

#	define MOO_SIZEOF_MBSTATE_T   MOO_SIZEOF_LONG
#	define MOO_MBLEN_MAX          8

#else
#	error Define the size of various data types.
#endif


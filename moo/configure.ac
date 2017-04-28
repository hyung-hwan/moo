dnl AC_PREREQ([2.67])

AC_INIT([moo],[0.1.0],[Chung, Hyung-Hwan (hyunghwan.chung@gmail.com)],[],[http://code.abiyo.net/@moo])

AC_CONFIG_HEADER([lib/moo-cfg.h])
AC_CONFIG_AUX_DIR([ac])
AC_CONFIG_MACRO_DIR([m4])

dnl use CC and CPP for compilation tests with .c test programs.
AC_LANG(C)

dnl initialize automake
AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects foreign])
AM_PROG_AR

dnl enable silent rules if automake supports them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])
dnl m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl determine a C compiler to use
AC_PROG_CC

dnl determine a C++ compiler to use
AC_PROG_CXX

dnl check if the C++ compiler exists in PATH
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

dnl check if the C++ compiler supports namespace
AX_CXX_NAMESPACE_STD

dnl determine some key programs
AC_PROG_INSTALL
AC_PROG_EGREP
AC_PROG_LN_S
AC_PROG_SED

dnl 'recursive' type seems to have problems in setting LT_CONFIG_H
dnl so i use the classic 'subproject' type here
LT_CONFIG_LTDL_DIR([libltdl])

dnl initialize libtool
LT_INIT
AC_SUBST(LIBTOOL_DEPS)

save_CFLAGS="$CFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"
LTDL_INIT
LDFLAGS="$save_LDFLAGS"
CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"

dnl The lt_dladvise_init symbol was added with libtool-2.2
if test "x$with_included_ltdl" != "xyes"; then
	save_CFLAGS="$CFLAGS"
	save_LDFLAGS="$LDFLAGS"
	save_LIBS="$LIBS"
	CFLAGS="$CFLAGS $LTDLINCL"
	LDFLAGS="$LDFLAGS $LIBLTDL"
	AC_CHECK_LIB([ltdl], [lt_dladvise_init], [],
	             [AC_MSG_ERROR([installed libltdl is too old. install a newer libltdl or specify --with-included-ltdl])])
	LDFLAGS="$save_LDFLAGS"
	CFLAGS="$save_CFLAGS"
	LIBS="$save_LIBS"
fi


dnl overrides the default CFLAGS setting
if test "$ac_test_CFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CFLAGS="$CFLAGS -Wall"
	fi
fi

if test "$ac_test_CXXFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CXXFLAGS="$CXXFLAGS -Wall"
	fi
fi

dnl indicate the existence of config.h
CFLAGS="$CFLAGS -DMOO_HAVE_CFG_H"
CXXFLAGS="$CXXFLAGS -DMOO_HAVE_CFG_H"

dnl make visible the 64-bit interface to the file system
CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE"
CXXFLAGS="$CXXFLAGS -D_LARGEFILE64_SOURCE"

dnl define the WIN32 conditional if necessary
case "$host" in
*-*-mingw*|*-*-cygwin*)
	#CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0400"
	platform_win32=yes
	;;
*)
	platform_win32=no
	;;
esac
AM_CONDITIONAL(WIN32, test "x${platform_win32}" = "xyes" )

dnl check the math library (check if -lm is needed)
LT_LIB_M
AC_SUBST(LIBM, $LIBM)

dnl check some compiler builtins
AC_MSG_CHECKING([for __builtin_memset])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [__builtin_memset ((void*)1, ' ', 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMSET], [1], [__builtin_memset])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __builtin_memcpy])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [__builtin_memcpy ((void*)1, (void*)2, 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMCPY], [1], [__builtin_memcpy])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __builtin_memmove])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [__builtin_memmove ((void*)1, (void*)2, 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMMOVE], [1], [__builtin_memmove])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __builtin_memcmp])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [int a = __builtin_memcmp ((void*)1, (void*)2, 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMCMP], [1], [__builtin_memcmp])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([computed goto usability])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM( [], [[void *jp = &&jpt; goto *jp; 1; jpt: 2; ]])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE(HAVE_COMPUTED_GOTO, 1, [computed gotos])],
	[AC_MSG_RESULT(no)]
)

dnl check header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h errno.h signal.h fcntl.h dirent.h])
AC_CHECK_HEADERS([time.h sys/time.h utime.h spawn.h execinfo.h ucontext.h])
AC_CHECK_HEADERS([dyncall.h])
AC_CHECK_HEADERS([xcb/xcb.h])

dnl check data types
dnl AC_CHECK_TYPE([wchar_t], 
dnl 	[ AC_DEFINE([HAVE_WCHAR_T_IN_STDDEF_H],
dnl	           [],[wchar_t is available in stddef.h]) ],
dnl	[],
dnl	[#include <stddef.h>])

dnl check functions
AC_CHECK_FUNCS([gettimeofday settimeofday clock_gettime clock_settime getitimer setitimer])
AC_CHECK_FUNCS([backtrace backtrace_symbols])
AC_CHECK_FUNCS([makecontext swapcontext getcontext setcontext])
AC_CHECK_FUNCS([snprintf _vsnprintf _vsnwprintf])

AC_CHECK_LIB([dyncall_s], [dcNewCallVM], 
	[
		DYNCALL_LIBS="-ldyncall_s"
		AC_DEFINE([HAVE_DYNCALL_LIB], [1], [dyncall library is available])	
	]
)
AC_SUBST(DYNCALL_LIBS)

dnl check is the import library for unicows.dll exists
dnl this check doesn't look for a particular symbol
dnl but for the symbol 'main' since i want to check 
dnl the existence of the libaray.
AC_CHECK_LIB([unicows], [main], [UNICOWS_LIBS="-lunicows"])
AC_SUBST(UNICOWS_LIBS)

AC_CHECK_LIB([rt], [clock_gettime], [LIBS="$LIBS -lrt"])

AC_MSG_CHECKING([for va_copy])
AC_TRY_LINK(
	[#include <stdarg.h>],
	[
		va_list x, y;
		va_copy(x,y);
	],
	[
		AC_MSG_RESULT(yes)
		AC_DEFINE([HAVE_VA_COPY], [1], [va_copy is available])
	],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __va_copy])
AC_TRY_LINK(
	[#include <stdarg.h>],
	[
		va_list x, y;
		__va_copy(x,y); 
	],
	[
		AC_MSG_RESULT(yes)
		AC_DEFINE([HAVE___VA_COPY], [1], [__va_copy is available])
	],
	[AC_MSG_RESULT(no)]
)

dnl check the size of primitive data types
AC_CHECK_SIZEOF(char,,[[]])
AC_CHECK_SIZEOF(short,,[[]])
AC_CHECK_SIZEOF(int,,[[]])
AC_CHECK_SIZEOF(long,,[[]])
AC_CHECK_SIZEOF(long long,,[[]])
AC_CHECK_SIZEOF(__int8,,[[]])
AC_CHECK_SIZEOF(__int16,,[[]])
AC_CHECK_SIZEOF(__int32,,[[]])
AC_CHECK_SIZEOF(__int64,,[[]])
AC_CHECK_SIZEOF(__int128,,[[]])
AC_CHECK_SIZEOF(__int8_t,,[[]])
AC_CHECK_SIZEOF(__int16_t,,[[]])
AC_CHECK_SIZEOF(__int32_t,,[[]])
AC_CHECK_SIZEOF(__int64_t,,[[]])
AC_CHECK_SIZEOF(__int128_t,,[[]])
AC_CHECK_SIZEOF(__uint128_t,,[[]])
dnl AC_CHECK_SIZEOF doesn't work without white-space between void and *
AC_CHECK_SIZEOF(void *,,[[]])
AC_CHECK_SIZEOF(float,,[[]])
AC_CHECK_SIZEOF(double,,[[]])
AC_CHECK_SIZEOF(long double,,[[]])
AC_CHECK_SIZEOF(__float128,,[[]])

AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(off64_t)

AC_CHECK_SIZEOF(mbstate_t,,[#include <wchar.h>])
AX_CHECK_NUMVALOF(MB_LEN_MAX,[32],[#include <limits.h>])

dnl gcc 3.4.3 on opensolaris x86 gave this warning without -msse or 
dnl something similar.
dnl    SSE vector argument without SSE enabled changes the ABI
dnl it also ended with an internal compiler error while compiling 
dnl some expressions like 'v % base' where v is of the __int128_t type.
dnl so i've added this check to determine if the compiler has
dnl proper support for __int128_t.
dnl 
dnl Changing ac_cv_sizeof___int128_t here doesn't corret SIZEOF___INT128_T
dnl since it's already generated. It only affects MOO_SIZEOF___INT128_T below.
if test ${ac_cv_sizeof___int128_t} -gt 0
then
	AC_MSG_CHECKING([__int128_t with %])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([], [
			volatile __int128_t x, base;
			x = (__int128_t)&base;
			base = (__int128_t)&x;
			x = x % base;
			return (int)x;
		])],

		[
			AC_MSG_RESULT(yes)
		],
		[
			AC_MSG_RESULT(no)
			ac_cv_sizeof___int128_t=0
			ac_cv_sizeof___uint128_t=0
		]
	)
fi

dnl Some compilers doesn't seem to have full support for __float128 
dnl even if the type is available.
if test ${ac_cv_sizeof___float128} -gt 0
then
	AC_MSG_CHECKING([__float128 with linking])
	AC_TRY_LINK(
		[#include <stdio.h>],
		[
			__float128 x, y, z;
			z = x * y + x;
			printf ("%p", (char*)(int)z);
		],
		[AC_MSG_RESULT(yes)],
		[
			AC_MSG_RESULT(no)
			ac_cv_sizeof___float128=0
		]
	)
fi

dnl Quadmath functions may not be in the default c library.
dnl Find a library containing them. Disable the float128 type
dnl if they are not available.
if test ${ac_cv_sizeof___float128} -gt 0
then
	AC_CHECK_FUNCS([quadmath_snprintf])
	if test "$ac_cv_func_quadmath_snprintf" = "no"
	then
		OLDLIBS="$LIBS"
		LIBS="$LIBM $LIBS"
		AC_CHECK_FUNCS([quadmath_snprintf])
		LIBS="$OLDLIBS"
	
		if test "$ac_cv_func_quadmath_snprintf" = "no"
		then
			dnl quadmath_snprintf not avalable in the 
			dnl standard math lib.

			AC_CHECK_LIB([quadmath], [quadmath_snprintf], [
				QUADMATH_LIBS="-lquadmath"
				LIBM="$LIBM -lquadmath"
				AC_DEFINE(HAVE_QUADMATH_SNPRINTF, 1)
			])

			if test "$ac_cv_lib_quadmath_quadmath_snprintf" = "no"
			then
				dnl quadmath_snprintf not avalable in the 
				dnl quadmath lib.

				ac_cv_sizeof___float128=0
			fi

		else
			QUADMATH_LIBS="$LIBM"
		fi
	fi

	if test ${ac_cv_sizeof___float128} -gt 0
	then
		OLDLIBS="$LIBS"
		LIBS="$LIBM $LIBS"
		AC_CHECK_FUNCS([powq fmodq sinq cosq tanq sinhq coshq tanhq asinq acosq atanq atan2q logq log10q expq sqrtq ceilq floorq roundq])
		AC_CHECK_FUNCS([strtoflt128])
		LIBS="$OLDLIBS"
	fi
fi
AC_SUBST(QUADMATH_LIBS)


AC_DEFINE_UNQUOTED(MOO_SIZEOF_CHAR, ${ac_cv_sizeof_char}, [sizeof(char)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_SHORT, ${ac_cv_sizeof_short}, [sizeof(short)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_INT, ${ac_cv_sizeof_int}, [sizeof(int)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_LONG, ${ac_cv_sizeof_long}, [sizeof(long)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_LONG_LONG, ${ac_cv_sizeof_long_long}, [sizeof(long long)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___INT8, ${ac_cv_sizeof___int8}, [sizeof(__int8)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___INT16, ${ac_cv_sizeof___int16}, [sizeof(__int16)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___INT32, ${ac_cv_sizeof___int32}, [sizeof(__int32)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___INT64, ${ac_cv_sizeof___int64}, [sizeof(__int64)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___INT128, ${ac_cv_sizeof___int128}, [sizeof(__int128)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___INT128_T, ${ac_cv_sizeof___int128_t}, [sizeof(__int128_t)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___UINT128_T, ${ac_cv_sizeof___uint128_t}, [sizeof(__uint128_t)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_VOID_P, ${ac_cv_sizeof_void_p}, [sizeof(void*)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_FLOAT, ${ac_cv_sizeof_float}, [sizeof(float)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_DOUBLE, ${ac_cv_sizeof_double}, [sizeof(double)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_LONG_DOUBLE, ${ac_cv_sizeof_long_double}, [sizeof(long double)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF___FLOAT128, ${ac_cv_sizeof___float128}, [sizeof(__float128)])

AC_DEFINE_UNQUOTED(MOO_SIZEOF_WCHAR_T, ${ac_cv_sizeof_wchar_t}, [sizeof(wchar_t)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_OFF_T, ${ac_cv_sizeof_off_t}, [sizeof(off_t)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_OFF64_T, ${ac_cv_sizeof_off64_t}, [sizeof(off64_t)])

AC_DEFINE_UNQUOTED(MOO_SIZEOF_MBSTATE_T, ${ac_cv_sizeof_mbstate_t}, [sizeof(mbstate_t)])
AC_DEFINE_UNQUOTED(MOO_MBLEN_MAX, ${ax_cv_numvalof_MB_LEN_MAX}, [MB_LEN_MAX])

moo_package_version_major="`echo ${PACKAGE_VERSION} | cut -d. -f1`"
moo_package_version_minor="`echo ${PACKAGE_VERSION} | cut -d. -f2`"
moo_package_version_patch="`echo ${PACKAGE_VERSION} | cut -d. -f3`"
AC_SUBST(PACKAGE_VERSION_MAJOR, ${moo_package_version_major})
AC_SUBST(PACKAGE_VERSION_MINOR, ${moo_package_version_minor})
AC_SUBST(PACKAGE_VERSION_PATCH, ${moo_package_version_patch})

AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION, "${PACKAGE_VERSION}", [Package version])
dnl AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION_MAJOR, $(echo ${VERSION} | cut -d. -f1), [Major version number])
dnl AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION_MINOR, $(echo ${VERSION} | cut -d. -f2), [Minor version number])
dnl AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION_PATCH, $(echo ${VERSION} | cut -d. -f3), [Patch level])
AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION_MAJOR, ${moo_package_version_major}, [Major version number])
AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION_MINOR, ${moo_package_version_minor}, [Minor version number])
AC_DEFINE_UNQUOTED(MOO_PACKAGE_VERSION_PATCH, ${moo_package_version_patch}, [Patch level])

AC_DEFINE_UNQUOTED(MOO_PACKAGE_AUTHOR,"${PACKAGE_BUGREPORT}", [Author])
AC_DEFINE_UNQUOTED(MOO_PACKAGE_NAME,"${PACKAGE_NAME}", [package name])
AC_DEFINE_UNQUOTED(MOO_PACKAGE_URL, "${PACKAGE_URL}", [Project URL])

AC_C_BIGENDIAN(
	[AC_DEFINE([MOO_ENDIAN_BIG],[],[Big Endian])],
	[AC_DEFINE([MOO_ENDIAN_LITTLE],[],[Little Endian])],
	[AC_DEFINE([MOO_ENDIAN_UNKNOWN],[],[Unknown Endian])])

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug],[build the library in the debug mode (default. no)])],
	enable_debug_is=$enableval,enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG -DMOO_BUILD_DEBUG -U_FORTIFY_SOURCE"]
	[CXXFLAGS="$CXXFLAGS -g -D_DEBUG -UNDEBUG -DMOO_BUILD_DEBUG -U_FORTIFY_SOURCE"]
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG -DMOO_BUILD_RELEASE -U_FORTIFY_SOURCE"]
	[CXXFLAGS="$CXXFLAGS -DNDEBUG -U_DEBUG -DMOO_BUILD_RELEASE -U_FORTIFY_SOURCE"]
	AC_SUBST(BUILD_MODE, "release")
fi

AC_ARG_ENABLE([cxx], 
	[AS_HELP_STRING([--enable-cxx],[build the library for C++ if a C++ compiler is available (default. yes)])],
	enable_cxx_is=$enableval,
	enable_cxx_is=yes
)

dnl disable c++ if no c++ compiler was found
test "${HAVE_CXX}" = "yes" || enable_cxx_is="no"
dnl disable c++ if the compiler is too old.
test "${ax_cv_cxx_have_std_namespace}" = "yes" || enable_cxx_is="no"

AM_CONDITIONAL(ENABLE_CXX, test "${enable_cxx_is}" = "yes" )

dnl ===== enable-dynamic-module =====
AC_ARG_ENABLE([dynamic-module],
	[AS_HELP_STRING([--enable-dynamic-module],[enable dynamic module capability(default. yes)])],
	enable_dynamic_module_is=$enableval,
	enable_dynamic_module_is=yes
)
test "${enable_shared}" = "no" && enable_dynamic_module_is="no"

if test "${enable_dynamic_module_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_DYNAMIC_MODULE],[],[enable dynamic module capability])
fi
AM_CONDITIONAL(ENABLE_DYNAMIC_MODULE, test "${enable_dynamic_module_is}" = "yes")

dnl ===== enable-static-module =====
AC_ARG_ENABLE([static-module],
	[AS_HELP_STRING([--enable-static-module],[build modules statically into the main library(default. no)])],
	enable_static_module_is=$enableval,
	enable_static_module_is=no
)
test "${enable_shared}" = "no" -a "${enable_static}" = "yes" && enable_static_module_is="yes"

if test "${enable_static_module_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_STATIC_MODULE],[],[link modules statically into the main library])
fi
AM_CONDITIONAL(ENABLE_STATIC_MODULE, test "${enable_static_module_is}" = "yes")

dnl ===== enable-libltdl =====
AC_ARG_ENABLE([libltdl],
	[AS_HELP_STRING([--enable-libltdl],[use libltdl(default. yes)])],
	enable_libltdl_is=$enableval,
	enable_libltdl_is=yes
)
if test "${enable_libltdl_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_LIBLTDL],[],[use libltdl when loading a dynamic module])
fi
AM_CONDITIONAL(ENABLE_LIBLTDL, test "${enable_libltdl_is}" = "yes")

dnl ===== enable-mod-ffi =====
AC_ARG_ENABLE([mod-ffi],
	[AS_HELP_STRING([--enable-mod-ffi],[enable mod/ffi (default. yes)])],
	enable_mod_ffi_is=$enableval,
	enable_mod_ffi_is=yes
)
if test "${enable_mod_ffi_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_MOD_FFI],[],[build mod/ffi])
fi
AM_CONDITIONAL(ENABLE_MOD_FFI, test "${enable_mod_ffi_is}" = "yes")

dnl ===== enable-mod-x11 =====
AC_ARG_ENABLE([mod-x11],
	[AS_HELP_STRING([--enable-mod-x11],[enable mod/x11 (default. yes)])],
	enable_mod_x11_is=$enableval,
	enable_mod_x11_is=yes
)
if test "${enable_mod_x11_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_MOD_X11],[],[build mod/x11])
fi
AM_CONDITIONAL(ENABLE_MOD_X11, test "${enable_mod_x11_is}" = "yes")

AC_SUBST(MOO_SIZEOF_WCHAR_T, $ac_cv_sizeof_wchar_t)
AC_SUBST(MOO_SIZEOF_LONG_LONG, $ac_cv_sizeof_long_long)
AC_SUBST(MOO_SIZEOF_LONG, $ac_cv_sizeof_long)
AC_SUBST(MOO_SIZEOF_INT, $ac_cv_sizeof_int)
AC_SUBST(MOO_SIZEOF_SHORT, $ac_cv_sizeof_short)
AC_SUBST(MOO_SIZEOF_CHAR, $ac_cv_sizeof_char)
AC_SUBST(MOO_SIZEOF_VOID_P, $ac_cv_sizeof_void_p)
AC_SUBST(MOO_SIZEOF_FLOAT, $ac_cv_sizeof_float)
AC_SUBST(MOO_SIZEOF_DOUBLE, $ac_cv_sizeof_double)
AC_SUBST(MOO_SIZEOF_LONG_DOUBLE, $ac_cv_sizeof_long_double)
AC_SUBST(MOO_SIZEOF_OFF_T, $ac_cv_sizeof_off_t)
AC_SUBST(MOO_SIZEOF_OFF64_T, $ac_cv_sizeof_off64_t)

AC_SUBST(MOO_PROJECT_AUTHOR, "${PACKAGE_BUGREPORT}")
AC_SUBST(MOO_PROJECT_URL, "${PACKAGE_URL}")

AC_CONFIG_FILES([
	Makefile 
	lib/Makefile 
	mod/Makefile 
])
AC_OUTPUT

[
echo 
echo "-[SUMMARY]---------------------------------------------------------------"
echo "Configured for ${host}"
echo "  Build mode : ${BUILD_MODE}"
echo "  Source directory: ${srcdir}"
echo "  Installation directory: ${prefix}"
echo "  C compiler: ${CC} ${CFLAGS}"
echo "  C++ compiler: ${CXX} ${CXXFLAGS}"
echo "  C++ support: ${enable_cxx_is}"
echo "  Math library: ${LIBM}"
echo "-------------------------------------------------------------------------"
]

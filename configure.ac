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

dnl determine some key programs
AC_PROG_INSTALL
AC_PROG_EGREP
AC_PROG_LN_S
AC_PROG_SED

dnl initialize libtool
LT_INIT
AC_SUBST(LIBTOOL_DEPS)

dnl overrides the default CFLAGS setting
if test "$ac_test_CFLAGS" != "set"
then
	if test "$GCC" = "yes"
	then
		CFLAGS="$CFLAGS -Wall"
	fi
fi

dnl indicate the existence of config.h
CFLAGS="$CFLAGS -DMOO_HAVE_CFG_H"

dnl make visible the 64-bit interface to the file system
AC_SYS_LARGEFILE()

dnl define the WIN32 conditional if necessary
case "$host" in
*-*-mingw*|*-*-cygwin*)
	#CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0400"
	platform_win32=yes
	platform_macosx=no
	;;
*-*-darwin*)
	platform_win32=no
	platform_macosx=yes
	;;
*)
	platform_win32=no
	platform_macosx=no
	;;
esac
AM_CONDITIONAL(WIN32, test "x${platform_win32}" = "xyes" )
AM_CONDITIONAL(MACOSX, test "x${platform_macosx}" = "xyes" )

dnl check the math library (check if -lm is needed)
LT_LIB_M
AC_SUBST(LIBM, $LIBM)

dnl check some compiler builtins
AC_MSG_CHECKING([for __builtin_memset])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([], [__builtin_memset ((void*)1, ' ', 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMSET], [1], [__builtin_memset])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __builtin_memcpy])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([], [__builtin_memcpy ((void*)1, (void*)2, 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMCPY], [1], [__builtin_memcpy])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __builtin_memmove])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([], [__builtin_memmove ((void*)1, (void*)2, 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMMOVE], [1], [__builtin_memmove])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([for __builtin_memcmp])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([], [int a = __builtin_memcmp ((void*)1, (void*)2, 10);])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE___BUILTIN_MEMCMP], [1], [__builtin_memcmp])],
	[AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING([labels as values])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM( [], [[void* jp[] = { &&jpt1, &&jpt2, &&jpt3 }; goto *jp[1]; jpt1: 1; jpt2: 2; jpt3: 3; ]])],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE(HAVE_LABELS_AS_VALUES, 1, [labels as values])],
	[AC_MSG_RESULT(no)]
)

dnl check header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h wchar.h wctype.h errno.h signal.h fcntl.h dirent.h])
AC_CHECK_HEADERS([time.h sys/time.h utime.h spawn.h execinfo.h ucontext.h])
AC_CHECK_HEADERS([dlfcn.h ltdl.h sys/mman.h])
AC_CHECK_HEADERS([sys/devpoll.h sys/epoll.h poll.h sys/event.h])
AC_CHECK_HEADERS([netinet/in.h sys/un.h netpacket/packet.h net/if_dl.h])
AC_CHECK_HEADERS([xcb/xcb.h])
AC_CHECK_HEADERS([ffi.h dyncall.h libunwind.h quadmath.h])

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
AC_CHECK_FUNCS([clock_nanosleep nanosleep usleep])
AC_CHECK_FUNCS([localtime_r gmtime_r])
AC_CHECK_FUNCS([sigaction signal])
AC_CHECK_FUNCS([snprintf _vsnprintf _vsnwprintf strerror_r])
AC_CHECK_FUNCS([accept4 pipe2 epoll_create epoll_create1 kqueue kqueue1])
AC_CHECK_FUNCS([isatty mmap munmap])

save_LIBS="$LIBS"
AC_SEARCH_LIBS([dlopen], [dl dld], [
	DL_LIBS="$ac_cv_search_dlopen"
	if test "x${DL_LIBS}" = "xnone required"
	then
		DL_LIBS=""
	fi
])
LIBS="$save_LIBS"
AC_SUBST(DL_LIBS)

save_LIBS="$LIBS"
AC_SEARCH_LIBS([lt_dlopen], [ltdl], [
	LTDL_LIBS="$ac_cv_search_lt_dlopen"
	if test "x${LTDL_LIBS}" = "xnone required"
	then
		LTDL_LIBS=""
	fi
])
LIBS="$save_LIBS"
AC_SUBST(LTDL_LIBS)

AC_CHECK_LIB([dyncall_s], [dcNewCallVM], 
	[
		DYNCALL_LIBS="-ldyncall_s"
		AC_DEFINE([HAVE_DYNCALL_LIB], [1], [dyncall library is available])
	]
)
AC_SUBST(DYNCALL_LIBS)

AC_CHECK_LIB([ffi], [ffi_call], 
	[
		FFI_LIBS="-lffi"
		AC_DEFINE([HAVE_FFI_LIB], [1], [libffi library is available])
	]
)
AC_SUBST(FFI_LIBS)

if test "x${ac_cv_header_libunwind_h}" = "xyes" 
then
	AC_CHECK_LIB([unwind], [unw_backtrace], 
		[
			UNWIND_LIBS="-lunwind"
			AC_DEFINE([HAVE_UNWIND_LIB], [1], [libunwind is available])
		]
	)
	AC_SUBST(UNWIND_LIBS)
fi


save_LIBS="$LIBS"
AC_SEARCH_LIBS([setupterm], [tinfo termcap curses ncurses ncursesw], [TERMINAL_LIBS="$ac_cv_search_setupterm"])
LIBS="$save_LIBS"
AC_SUBST(TERMINAL_LIBS)


dnl OLDLIBS="$LIBS"
dnl AC_SEARCH_LIBS([connect], [socket])
dnl LIBS="$OLDLIBS"
dnl if test "$ac_cv_search_connect" != "none required" -a "$ac_cv_search_connect" != "no"
dnl then
dnl     SOCKET_LIBS="$SOCKET_LIBS -lsocket"
dnl fi
dnl AC_SUBST(SOCKET_LIBS)
AC_CHECK_FUNCS([connect gethostbyname])
if test "x$ac_cv_func_gethostbyname" = "xno"
then
	AC_CHECK_LIB([nsl], [gethostbyname], [
		SOCKET_LIBS="$SOCKET_LIBS -lnsl"
		AC_DEFINE(HAVE_GETHOSTBYNAME, 1)
	])
fi
if test "x$ac_cv_func_connect" = "xno"
then
	AC_CHECK_LIB([socket], [connect], [
		SOCKET_LIBS="$SOCKET_LIBS -lsocket"
		AC_DEFINE(HAVE_CONNECT, 1)
	])

	if test "x$ac_cv_lib_socket_connect" = xno
	then
		AC_CHECK_FUNCS([t_connect])
		if test "x$ac_cv_func_t_connect" = "xno"
		then
			AC_CHECK_LIB([nsl_s], [t_connect], [
				SOCKET_LIBS="$SOCKET_LIBS -lnsl_s"
				AC_DEFINE(HAVE_T_CONNECT, 1)
			])
		fi
	fi
fi
if test "x${platform_win32}" = "xyes"
then
	SOCKET_LIBS="$SOCKET_LIBS -lws2_32"
fi
AC_SUBST(SOCKET_LIBS)

AX_PTHREAD
AC_CHECK_LIB([pthread], [pthread_mutex_timedlock],  [
	AC_DEFINE([HAVE_PTHREAD_MUTEX_TIMEDLOCK],1,[pthreads has pthread_mutex_timedlock()])
])
AC_CHECK_LIB([pthread], [pthread_mutex_trylock],  [
	AC_DEFINE([HAVE_PTHREAD_MUTEX_TRYLOCK],1,[pthreads has pthread_mutex_trylock()])
])

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

AC_MSG_CHECKING([for strftime %z])
AC_TRY_RUN(
	[#include <string.h>
	 #if defined(HAVE_SYS_TIME_H)
	 #include <sys/time.h>
	 #endif
	 #if defined(HAVE_TIME_H)
	 #include <time.h>
	 #endif
	int main ()
	{
		char buf[100];
		time_t t = 0;
		strftime (buf, sizeof(buf), "%z", gmtime(&t));
		return (strcmp(buf, "%z") == 0 || strcmp(buf, "z") == 0)? 1: 0;
	}],
	[
	 	AC_MSG_RESULT(yes)
		AC_DEFINE([HAVE_STRFTIME_SMALL_Z], [1], [strftime supports %z])
	],
	[AC_MSG_RESULT(no)],
	[
	 	dnl cross-compiling, assume yes
	 	AC_MSG_RESULT(unknown)
		AC_DEFINE([HAVE_STRFTIME_SMALL_Z], [1], [strftime supports %z])
	]
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
AX_CHECK_NUMVALOF(PATH_MAX,[0],[#include <limits.h>])
AX_CHECK_NUMVALOF(NSIG,[32],[#include <signal.h>])

if test "x${platform_win32}" = "xyes"
then
	AC_CHECK_SIZEOF(struct sockaddr_in,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_in6,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_un,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_ll,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	AC_CHECK_SIZEOF(struct sockaddr_dl,,[
		#include <winsock2.h>
		#include <ws2tcpip.h>])

	AC_CHECK_SIZEOF(socklen_t,, [
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	if test ${ac_cv_sizeof_socklen_t} -gt 0
	then
		AX_CHECK_SIGN([socklen_t],
			[ AC_DEFINE(MOO_SOCKLEN_T_IS_SIGNED, 1, [Define if socklen_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <winsock2.h>
			 #include <ws2tcpip.h>])
	fi

	AC_CHECK_SIZEOF(sa_family_t,, [
		#include <winsock2.h>
		#include <ws2tcpip.h>])
	if test ${ac_cv_sizeof_sa_family_t} -gt 0
	then
		AX_CHECK_SIGN([sa_family_t],
			[ AC_DEFINE(MOO_SA_FAMILY_T_IS_SIGNED, 1, [Define if sa_family_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <winsock2.h>
			 #include <ws2tcpip.h>])
	fi

	AC_MSG_CHECKING([offset of sa_family in struct sockaddr])
	AC_COMPUTE_INT([ac_cv_offsetof_sa_family],
		[[offsetof(struct sockaddr, sa_family)]],
		[#include <stddef.h>
		 #include <winsock2.h>
		 #include <ws2tcpip.h>], [ac_cv_offsteof_sa_family=0])
	AC_MSG_RESULT([$ac_cv_offsetof_sa_family])
	AC_DEFINE_UNQUOTED(MOO_OFFSETOF_SA_FAMILY, ${ac_cv_offsetof_sa_family}, [offsetof(struct sockaddr, sa_family)])

else
	AC_CHECK_SIZEOF(struct sockaddr_in,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>])
	AC_CHECK_SIZEOF(struct sockaddr_in6,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>])
	AC_CHECK_SIZEOF(struct sockaddr_un,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <sys/un.h>])
	AC_CHECK_SIZEOF(struct sockaddr_ll,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netpacket/packet.h>])
	AC_CHECK_SIZEOF(struct sockaddr_dl,,[
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <net/if_dl.h>])


	AC_CHECK_SIZEOF(socklen_t,, [
		#include <sys/types.h>
		#include <sys/socket.h>])
	if test ${ac_cv_sizeof_socklen_t} -gt 0
	then
		AX_CHECK_SIGN([socklen_t],
			[ AC_DEFINE(MOO_SOCKLEN_T_IS_SIGNED, 1, [Define if socklen_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <sys/types.h>
			 #include <sys/socket.h>])
	fi

	AC_CHECK_SIZEOF(sa_family_t,, [
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>])
	if test ${ac_cv_sizeof_sa_family_t} -gt 0
	then
		AX_CHECK_SIGN([sa_family_t],
			[ AC_DEFINE(MOO_SA_FAMILY_T_IS_SIGNED, 1, [Define if sa_family_t is signed]) ],
			[ AS_ECHO_N("") ],
			[#include <sys/types.h>
			 #include <sys/socket.h>
			 #include <netinet/in.h>])
	fi

	AC_MSG_CHECKING([offset of sa_family in struct sockaddr])
	AC_COMPUTE_INT([ac_cv_offsetof_sa_family],
		[[offsetof(struct sockaddr, sa_family)]],
		[#include <stddef.h>
		 #include <sys/types.h>
		 #include <sys/socket.h>], [ac_cv_offsteof_sa_family=0])
	AC_MSG_RESULT([$ac_cv_offsetof_sa_family])
	AC_DEFINE_UNQUOTED(MOO_OFFSETOF_SA_FAMILY, ${ac_cv_offsetof_sa_family}, [offsetof(struct sockaddr, sa_family)])
fi

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
	AC_MSG_CHECKING([__int128_t with % and va_arg])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <stdarg.h>], [
			volatile __int128_t x, base;
			va_list ap;
			va_arg(ap, __int128_t);
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
if test ${ax_cv_numvalof_PATH_MAX} -gt 0
then
	AC_DEFINE_UNQUOTED(MOO_PATH_MAX, ${ax_cv_numvalof_PATH_MAX}, [PATH_MAX])
fi
AC_DEFINE_UNQUOTED(MOO_NSIG, ${ax_cv_numvalof_NSIG}, [NSIG])

AC_DEFINE_UNQUOTED(MOO_SIZEOF_STRUCT_SOCKADDR_IN, ${ac_cv_sizeof_struct_sockaddr_in}, [sizeof(struct sockaddr_in)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_STRUCT_SOCKADDR_IN6, ${ac_cv_sizeof_struct_sockaddr_in6}, [sizeof(struct sockaddr_in6)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_STRUCT_SOCKADDR_LL, ${ac_cv_sizeof_struct_sockaddr_ll}, [sizeof(struct sockaddr_ll)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_STRUCT_SOCKADDR_UN, ${ac_cv_sizeof_struct_sockaddr_un}, [sizeof(struct sockaddr_un)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_STRUCT_SOCKADDR_DL, ${ac_cv_sizeof_struct_sockaddr_dl}, [sizeof(struct sockaddr_dl)])

AC_DEFINE_UNQUOTED(MOO_SIZEOF_SOCKLEN_T, ${ac_cv_sizeof_socklen_t}, [sizeof(socklen_t)])
AC_DEFINE_UNQUOTED(MOO_SIZEOF_SA_FAMILY_T, ${ac_cv_sizeof_sa_family_t}, [sizeof(sa_family_t)])

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

dnl ===== enable-experimental =====
AC_ARG_ENABLE([experimental], 
	[AS_HELP_STRING([--enable-experimental],[enable experimental feature(default. no)])],
	enable_experimental_is=$enableval,
	enable_experimental_is=no)
AM_CONDITIONAL(ENABLE_EXPERIMENTAL, test "${enable_experimental_is}" = "yes" )

dnl ===== enable-debug =====
AC_ARG_ENABLE([debug], 
	[AS_HELP_STRING([--enable-debug],[compile the code in the debug mode (default. no)])],
	enable_debug_is=$enableval,
	enable_debug_is=no)
if test "$enable_debug_is" = "yes"
then
	[CFLAGS="$CFLAGS -g -D_DEBUG -UNDEBUG -U_FORTIFY_SOURCE"]
	AC_DEFINE([MOO_BUILD_DEBUG],[1],[build release/debug])
	AC_SUBST(BUILD_MODE, "debug")
else
	[CFLAGS="$CFLAGS -DNDEBUG -U_DEBUG -U_FORTIFY_SOURCE"]
	AC_DEFINE([MOO_BUILD_RELEASE],[1],[build release/debug])
	AC_SUBST(BUILD_MODE, "release")
fi

dnl ===== enable-wide-char =====
AC_ARG_ENABLE([wide-char], 
	[AS_HELP_STRING([--enable-wide-char],[Use the wide-character type as the default character type. one of yes, no, yes:2, yes:4, no:2, no:4 (default. yes)])],
	enable_wide_char=$enableval,
	enable_wide_char=yes
)
if test "${enable_wide_char}" = "yes:4"
then
	AC_DEFINE([MOO_WIDE_CHAR_SIZE],[4],[Wide-character type size])
	AC_DEFINE([MOO_ENABLE_WIDE_CHAR],[1],[Use the wide-character type as the default character type])
	if test "${platform_win32}" = "yes"
	then
		[CFLAGS="$CFLAGS -DUNICODE -D_UNICODE"]
	fi 
elif test "${enable_wide_char}" = "yes:2"
then
	AC_DEFINE([MOO_WIDE_CHAR_SIZE],[2],[Wide-character type size])
	AC_DEFINE([MOO_ENABLE_WIDE_CHAR],[1],[Use the wide-character type as the default character type])
	if test "${platform_win32}" = "yes"
	then
		[CFLAGS="$CFLAGS -DUNICODE -D_UNICODE"]
	fi
elif test "${enable_wide_char}" = "yes"
then
	AC_DEFINE([MOO_WIDE_CHAR_SIZE],[1],[Wide-character type size])
	AC_DEFINE([MOO_ENABLE_WIDE_CHAR],[1],[Use the wide-character type as the default character type])
	if test "${platform_win32}" = "yes"
	then
		[CFLAGS="$CFLAGS -DUNICODE -D_UNICODE"]
	fi
elif test "${enable_wide_char}" = "no:4"
then
	AC_DEFINE([MOO_WIDE_CHAR_SIZE],[4],[Wide-character type size])
elif test "${enable_wide_char}" = "no:2"
then
	AC_DEFINE([MOO_WIDE_CHAR_SIZE],[2],[Wide-character type size])
else
	AC_DEFINE([MOO_WIDE_CHAR_SIZE],[1],[Wide-character type size])
fi

dnl ===== enable-full-liw =====
AC_ARG_ENABLE([full-liw], 
	[AS_HELP_STRING([--enable-full-liw],[Use a full word as a large integer word (default. no)])],
	enable_full_liw=$enableval,
	enable_full_liw=no
)
if test "${enable_full_liw}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_FULL_LIW],[1],[Use a full word as a large integer word])
fi

dnl ===== enable-dynamic-module =====
AC_ARG_ENABLE([dynamic-module],
	[AS_HELP_STRING([--enable-dynamic-module],[enable dynamic module capability(default. yes)])],
	enable_dynamic_module_is=$enableval,
	enable_dynamic_module_is=yes
)
if test "x${enable_shared}" = "xno"
then
	enable_dynamic_module_is="no"
fi

if test "x${enable_dynamic_module_is}" = "xyes"
then
	AC_DEFINE([MOO_ENABLE_DYNAMIC_MODULE],[1],[enable dynamic module capability])
fi
AM_CONDITIONAL(ENABLE_DYNAMIC_MODULE, test "x${enable_dynamic_module_is}" = "xyes")

dnl ===== enable-static-module =====
AC_ARG_ENABLE([static-module],
	[AS_HELP_STRING([--enable-static-module],[build modules statically into the main library(default. no)])],
	enable_static_module_is=$enableval,
	enable_static_module_is=no
)
if test "x${enable_shared}" = "xno" -a "x${enable_static}" = "xyes" 
then
	enable_static_module_is="yes"
fi

if test "${enable_static_module_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_STATIC_MODULE],[1],[link modules statically into the main library])
fi
AM_CONDITIONAL(ENABLE_STATIC_MODULE, test "x${enable_static_module_is}" = "xyes")

dnl ===== enable-libltdl =====
AC_ARG_ENABLE([libltdl],
	[AS_HELP_STRING([--enable-libltdl],[use libltdl(default. no)])],
	enable_libltdl_is=$enableval,
	enable_libltdl_is=no
)
if test "x${enable_libltdl_is}" = "xyes"
then
	if test "x${ac_cv_header_ltdl_h}" = "xyes"  -a "${LTDL_LIBS}" != ""
	then
		AC_DEFINE([MOO_ENABLE_LIBLTDL],[],[use libltdl when loading a dynamic module])
	else
		enable_libltdl_is="no"
	fi
fi
AM_CONDITIONAL(ENABLE_LIBLTDL, test "x${enable_libltdl_is}" = "xyes")

dnl ===== enable-libunwind =====
AC_ARG_ENABLE([libunwind],
	[AS_HELP_STRING([--enable-libunwind],[use libunwind(default. no)])],
	enable_libunwind_is=$enableval,
	enable_libunwind_is=no
)
if test "x${enable_libunwind_is}" = "xyes"
then
	if test "x${ac_cv_header_libunwind_h}" = "xyes"  -a "${UNWIND_LIBS}" != ""
	then
		AC_DEFINE([MOO_ENABLE_LIBUNWIND],[],[use libunwind for backtracing stack frames])
	else
		enable_libunwind_is="no"
	fi
fi
AM_CONDITIONAL(ENABLE_LIBUNWIND, test "x${enable_libunwind_is}" = "xyes")

dnl ===== enable-mod-con =====
AC_ARG_ENABLE([mod-con],
	[AS_HELP_STRING([--enable-mod-con],[enable mod/con (default. yes)])],
	enable_mod_con_is=$enableval,
	enable_mod_con_is=yes
)
if test "${enable_mod_con_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_MOD_CON],[1],[build mod/con])
fi
AM_CONDITIONAL(ENABLE_MOD_CON, test "${enable_mod_con_is}" = "yes")

dnl ===== enable-mod-ffi =====
AC_ARG_ENABLE([mod-ffi],
	[AS_HELP_STRING([--enable-mod-ffi],[enable mod/ffi (default. yes)])],
	enable_mod_ffi_is=$enableval,
	enable_mod_ffi_is=yes
)
if test "${enable_mod_ffi_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_MOD_FFI],[1],[build mod/ffi])
fi
AM_CONDITIONAL(ENABLE_MOD_FFI, test "${enable_mod_ffi_is}" = "yes")

dnl ===== enable-mod-sck =====
AC_ARG_ENABLE([mod-sck],
	[AS_HELP_STRING([--enable-mod-sck],[enable mod/sck (default. yes)])],
	enable_mod_sck_is=$enableval,
	enable_mod_sck_is=yes
)
if test "${enable_mod_sck_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_MOD_SCK],[1],[build mod/sck])
fi
AM_CONDITIONAL(ENABLE_MOD_SCK, test "${enable_mod_sck_is}" = "yes")

dnl ===== enable-mod-x11 =====
AC_ARG_ENABLE([mod-x11],
	[AS_HELP_STRING([--enable-mod-x11],[enable mod/x11 (default. yes)])],
	enable_mod_x11_is=$enableval,
	enable_mod_x11_is=yes
)
if test "${enable_mod_x11_is}" = "yes"
then
	AC_DEFINE([MOO_ENABLE_MOD_X11],[1],[build mod/x11])
fi
AM_CONDITIONAL(ENABLE_MOD_X11, test "${enable_mod_x11_is}" = "yes")

dnl ==== include pthread options to the default flags ====
dnl keep this as  the last option as it changes the default compile flags.
dnl otherwise, other tests may get affected if this option is on.
AC_ARG_ENABLE([pthread-flags], [AS_HELP_STRING([--enable-pthread-flags],[add thread flags to CFLAGS, LIBS(default. yes)])],
	enable_pthread_flags_is=$enableval,enable_pthread_flags_is=yes)
if test "$enable_pthread_flags_is" = "yes"
then
	[CFLAGS="$CFLAGS $PTHREAD_CFLAGS"]
	[LIBS="$LIBS $PTHREAD_LIBS"]
fi

dnl ====  subsititution of some key items ====
AC_SUBST(MOO_PROJECT_AUTHOR, "${PACKAGE_BUGREPORT}")
AC_SUBST(MOO_PROJECT_URL, "${PACKAGE_URL}")

dnl === configure arguments
if test `expr " [$]0" : " '.*"` = 0; then
	moo_configure_cmd="'[$]0'"
else
	moo_configure_cmd="[$]0"
fi

AC_DEFINE_UNQUOTED(MOO_CONFIGURE_ARGS, ["$ac_configure_args"], [configure arguments])
AC_DEFINE_UNQUOTED(MOO_CONFIGURE_CMD, ["$moo_configure_cmd"], [configure command])

AC_CONFIG_FILES([
	Makefile 
	tools/Makefile 
	lib/Makefile 
	mod/Makefile 
	bin/Makefile
	t/Makefile
	wasm/Makefile
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
echo "  Math library: ${LIBM}"
echo "  Socket library: ${SOCKET_LIBS}"
echo "  Native function call library: ${FFI_LIBS} ${DYNCALL_LIBS}"
echo "  Terminal library: ${TERMINAL_LIBS}"
echo "  Thraed library: ${PTHREAD_LIBS}"
echo "-------------------------------------------------------------------------"
]

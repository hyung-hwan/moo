/*
 * $Id$
 *
    Copyright (c) 2014-2018 Chung, Hyung-Hwan. All rights reserved.

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

#include <moo-std.h>
#include <moo-utl.h>
#include <moo-opt.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#if (defined(__unix) || defined(__linux) || defined(__ultrix) || defined(_AIX) || defined(__hpux) || defined(__sgi)) && defined(HAVE_SIGNAL_H)
#	include <signal.h>
#endif

static void print_syntax_error (moo_t* moo, const char* main_src_file)
{
	moo_synerr_t synerr;

	moo_getsynerr (moo, &synerr);

	moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: ");
	if (synerr.loc.file)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "%js", synerr.loc.file);
	}
	else
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "%s", main_src_file);
	}

	moo_logbfmt (moo, MOO_LOG_STDERR, "[%zu,%zu] %js", 
		synerr.loc.line, synerr.loc.colm,
		(moo_geterrmsg(moo) != moo_geterrstr(moo)? moo_geterrmsg(moo): moo_geterrstr(moo))
	);

	if (synerr.tgt.len > 0)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, " - %.*js", synerr.tgt.len, synerr.tgt.ptr);
	}

	moo_logbfmt (moo, MOO_LOG_STDERR, "\n");
}

/* ========================================================================= */

#define MIN_MEMSIZE 2048000ul

int main (int argc, char* argv[])
{
	static moo_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' }; /*TODO: make this an argument */
	static moo_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };

	moo_t* moo;
	moo_cfgstd_t cfg;
	moo_errinf_t errinf;

	moo_oocs_t objname;
	moo_oocs_t mthname;
	moo_oow_t memsize;
	int i, xret;

	moo_bci_t c;
	static moo_bopt_lng_t lopt[] =
	{
		{ ":log",         'l' },
		{ ":memsize",     'm' },
		{ "large-pages",  '\0' },
	#if defined(_WIN32)
		{ "utf8",         '\0' },
		{ "utf16",        '\0' },
	#endif
	#if defined(MOO_BUILD_DEBUG)
		{ ":debug",       '\0' }, /* NOTE: there is no short option for --debug */
	#endif
		{ MOO_NULL,       '\0' }
	};
	static moo_bopt_t opt =
	{
		"l:m:",
		lopt
	};

	setlocale (LC_ALL, "");

#if !defined(macintosh)
	if (argc < 2)
	{
	print_usage:
		fprintf (stderr, "Usage: %s [options] filename ...\n", argv[0]);
		fprintf (stderr, " --log filename[,logopts]\n");
		fprintf (stderr, " --memsize number\n");
		fprintf (stderr, " --large-pages\n");
	#if defined(_WIN32)
		fprintf (stderr, " --utf16\n");
		fprintf (stderr, " --utf8\n");
	#endif
	#if defined(MOO_BUILD_DEBUG)
		fprintf (stderr, " --debug dbgopts\n");
	#endif
		return -1;
	}

	memset (&cfg, 0, MOO_SIZEOF(cfg));
	cfg.type = MOO_CFGSTD_OPTB;
	cfg.cmgr = moo_get_utf8_cmgr();

	memsize = MIN_MEMSIZE;

	while ((c = moo_getbopt(argc, argv, &opt)) != MOO_BCI_EOF)
	{
		switch (c)
		{
			case 'l':
				cfg.u.optb.log = opt.arg;
				break;

			case 'm':
				memsize = strtoul(opt.arg, MOO_NULL, 0);
				if (memsize <= MIN_MEMSIZE) memsize = MIN_MEMSIZE;
				break;

			case '\0':
				if (moo_comp_bcstr(opt.lngopt, "large-pages") == 0)
				{
					cfg.large_pages = 1;
					break;
				}
			#if defined(_WIN32)
				else if (moo_comp_bcstr(opt.lngopt, "utf8") == 0)
				{
					cfg.cmgr = moo_get_utf8_cmgr();
					break;
				}
				else if (moo_comp_bcstr(opt.lngopt, "utf16") == 0)
				{
					cfg.cmgr = moo_get_utf16_cmgr();
					break;
				}
			#endif
			#if defined(MOO_BUILD_DEBUG)
				else if (moo_comp_bcstr(opt.lngopt, "debug") == 0)
				{
					cfg.u.optb.dbg = opt.arg;
					break;
				}
			#endif
				goto print_usage;

			case ':':
				if (opt.lngopt)
					fprintf (stderr, "bad argument for '%s'\n", opt.lngopt);
				else
					fprintf (stderr, "bad argument for '%c'\n", opt.opt);

				return -1;

			default:
				goto print_usage;
		}
	}

	if (opt.ind >= argc) goto print_usage;
#endif

	moo = moo_openstd(0, &cfg, &errinf);
	if (!moo)
	{
	#if defined(MOO_OOCH_IS_BCH)
		fprintf (stderr, "ERROR: cannot open moo - [%d] %s\n", (int)errinf.num, errinf.msg);
	#elif (MOO_SIZEOF_UCH_T == MOO_SIZEOF_WCHAR_T)
		fprintf (stderr, "ERROR: cannot open moo - [%d] %ls\n", (int)errinf.num, errinf.msg);
	#else
		moo_bch_t bcsmsg[MOO_COUNTOF(errinf.msg) * 2]; /* error messages may get truncated */
		moo_oow_t wcslen, bcslen;
		bcslen = MOO_COUNTOF(bcsmsg);
		moo_conv_ucstr_to_utf8 (errinf.msg, &wcslen, bcsmsg, &bcslen);
		fprintf (stderr, "ERROR: cannot open moo - [%d] %s\n", (int)errinf.num, bcsmsg);
	#endif
		return -1;
	}

	{
		moo_oow_t tab_size;

		tab_size = 5000;
		moo_setoption (moo, MOO_SYMTAB_SIZE, &tab_size);
		tab_size = 5000;
		moo_setoption (moo, MOO_SYSDIC_SIZE, &tab_size);
		tab_size = 600;
		moo_setoption (moo, MOO_PROCSTK_SIZE, &tab_size);
	}

	{
		moo_bitmask_t bm = 0;

		/*bm |= MOO_NOGC;*/
		bm |= MOO_AWAIT_PROCS;
		moo_setoption (moo, MOO_TRAIT, &bm);

		/* disable GC logs */
		bm = ~MOO_LOG_GC;
		moo_setoption (moo, MOO_LOG_MASK, &bm);
	}

	if (moo_ignite(moo, memsize) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot ignite moo - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
		moo_close (moo);
		return -1;
	}

/*
#if defined(macintosh)
	i = 20;
	xtn->source_path = "test.moo";
	goto compile;
#endif
*/

	for (i = opt.ind; i < argc; i++)
	{
		moo_iostd_t in;

#if 1
		in.type = MOO_IOSTD_FILEB;
		in.u.fileb.path = argv[i];
#else
		moo_uch_t tmp[1000];
		moo_oow_t bcslen, ucslen;
		ucslen = MOO_COUNTOF(tmp);
		moo_conv_utf8_to_ucstr(argv[i], &bcslen, tmp, &ucslen);
		in.type = MOO_IOSTD_FILEU;
		in.u.fileu.path = tmp;
#endif

	/*compile:*/
		if (moo_compilestd(moo, &in, 1) <= -1)
		{
			if (moo->errnum == MOO_ESYNERR)
			{
				print_syntax_error (moo, argv[i]);
			}
			else
			{
				moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot compile code - [%d] %js\n", moo_geterrnum(moo), moo_geterrmsg(moo));
			}

			moo_close (moo);
			return -1;
		}
	}

	MOO_DEBUG0 (moo, "COMPILE OK. STARTING EXECUTION...\n");
	xret = 0;

#if defined(SIGINT) && defined(HAVE_SIGNAL)
	/* i'd like the program to ignore the interrupt signal 
	 * before moo_catch_termreq() and after moo_uncatch_termreq() */
	signal (SIGINT, SIG_IGN);
	signal (SIGTERM, SIG_IGN);
#endif

	moo_catch_termreq ();
	moo_start_ticker ();

	moo_rcvtickstd (moo, 1);

	objname.ptr = str_my_object;
	objname.len = 8;
	mthname.ptr = str_main;
	mthname.len = 4;
	if (moo_invoke(moo, &objname, &mthname) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot execute code - [%d] %js\n", moo_geterrnum(moo), moo_geterrmsg(moo));
		xret = -1;
	}

	moo_stop_ticker ();
	moo_uncatch_termreq ();

	/*moo_dumpsymtab(moo);
	 *moo_dumpdic(moo, moo->sysdic, "System dictionary");*/

	moo_close (moo);
	return xret;
}

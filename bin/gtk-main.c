/* 
 * EXPERIMENTAL CODE TO DEMONSTRATE HOW TO CREATE A GUI LOG CONSOLE.
 * The purpose of this application is not to create a full gui environment.
 * I just want to create a autonomous gui log console like the text terminal.
 * 
 * 1. create a general program that reads from a pipe and show contents.
 *    - the moo's log write doesn't need to be modified.
 * 
 * 
 * 2. override the log_write callback to call the gtk text output gui function.
 *  not possible to use moo as a main loop as gtk doesn't expose the event loop file descriptor
 *  must use thread. the thread can't make a gui call.
 *
 * 3. execute moo when idle?
 *   need to break down moo_invoke() to smaller pieces 
 *   no looping in moo_invoke(). it should be made to allow stepping from the caller side.
 */

#include <gtk/gtk.h>
#include <moo-std.h>
#include <moo-utl.h>
#include <string.h>

GtkWidget* log_view = NULL;
GThread* moo_thread = NULL;
moo_t* moo_vm = NULL;

/* ========================================================================= */

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

struct log_data_t 
{
	GtkTextBuffer *buffer;
	moo_bch_t* ptr;
	moo_oow_t len;
};

static gboolean append_log_textbuffer(struct log_data_t *data)
{
	gtk_text_buffer_insert_at_cursor(data->buffer, data->ptr, data->len);
	g_free (data->ptr);
	g_free(data);
	return G_SOURCE_REMOVE;
}

void write_log (GtkWidget* buffer, const moo_bch_t* ptr, moo_oow_t len)
{
	/* cannot call gtk_text_buffer_insert_at_cursor() in a non-GUI thread.
	 * use gdk_threads_add_idle() to append the task. i don't like it. */
	struct log_data_t* data = g_new0(struct log_data_t, 1);
	data->ptr = g_strndup(ptr, len);
	data->len = len;
	data->buffer = GTK_TEXT_BUFFER(buffer);
	gdk_threads_add_idle(append_log_textbuffer, data);
}

static void log_write (moo_t* moo, moo_bitmask_t mask, const moo_ooch_t* msg, moo_oow_t len)
{
	GtkWidget* buffer;
	moo_bch_t buf[256];
	moo_oow_t ucslen, bcslen, msgidx;
	moo_cmgr_t* log_cmgr = moo_get_utf8_cmgr();
	int n;

	time_t now;
#if defined(MOO_OOCH_IS_UCH)
	char ts[32 * MOO_BCSIZE_MAX];
#else
	char ts[32];
#endif
	size_t tslen;
	struct tm tm, *tmp;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));

	now = time(MOO_NULL);
#if defined(_WIN32)
	tmp = localtime(&now);
	tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", tmp);
	if (tslen == 0) 
	{
		tslen = sprintf(ts, "%04d-%02d-%02d %02d:%02d:%02d ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	}
#elif defined(__OS2__)
	#if defined(__WATCOMC__)
	tmp = _localtime(&now, &tm);
	#else
	tmp = localtime(&now);
	#endif

	#if defined(__BORLANDC__)
	/* the borland compiler doesn't handle %z properly - it showed 00 all the time */
	tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %Z ", tmp);
	#else
	tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", tmp);
	#endif
	if (tslen == 0) 
	{
		tslen = sprintf(ts, "%04d-%02d-%02d %02d:%02d:%02d ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	}

#elif defined(__DOS__)
	tmp = localtime(&now);
	/* since i know that %z/%Z is not available in strftime, i switch to sprintf immediately */
	tslen = sprintf(ts, "%04d-%02d-%02d %02d:%02d:%02d ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
#else
	#if defined(HAVE_LOCALTIME_R)
	tmp = localtime_r(&now, &tm);
	#else
	tmp = localtime(&now);
	#endif

	#if defined(HAVE_STRFTIME_SMALL_Z)
	tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", tmp);
	#else
	tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %Z ", tmp); 
	#endif
	if (tslen == 0) 
	{
		tslen = sprintf(ts, "%04d-%02d-%02d %02d:%02d:%02d ", tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	}
#endif

#if defined(MOO_OOCH_IS_UCH)
	if (moo_getcmgr(moo) != log_cmgr)
	{
		moo_uch_t tsu[32];
		moo_oow_t tsulen;

		/* the timestamp is likely to contain simple ascii characters only.
		 * conversion is not likely to fail regardless of encodings. 
		 * so i don't check errors here */
		tsulen = MOO_COUNTOF(tsu);
		moo_convbtooochars (moo, ts, &tslen, tsu, &tsulen);
		tslen = MOO_COUNTOF(ts);
		moo_conv_uchars_to_bchars_with_cmgr (tsu, &tsulen, ts, &tslen, log_cmgr);
	}
#endif

	write_log (buffer, ts, tslen);

#if defined(MOO_OOCH_IS_UCH)
	msgidx = 0;
	while (len > 0)
	{
		ucslen = len;
		bcslen = MOO_COUNTOF(buf);

		/*n = moo_convootobchars(moo, &msg[msgidx], &ucslen, buf, &bcslen);*/
		n = moo_conv_uchars_to_bchars_with_cmgr(&msg[msgidx], &ucslen, buf, &bcslen, log_cmgr);
		if (n == 0 || n == -2)
		{
			/* n = 0: 
			 *   converted all successfully 
			 * n == -2: 
			 *    buffer not sufficient. not all got converted yet.
			 *    write what have been converted this round. */

			MOO_ASSERT (moo, ucslen > 0); /* if this fails, the buffer size must be increased */

			/* attempt to write all converted characters */
			write_log (buffer, buf, bcslen);

			if (n == 0) break;
			else
			{
				msgidx += ucslen;
				len -= ucslen;
			}
		}
		else if (n <= -1)
		{
			/* conversion error but i just stop here but don't treat it as a hard error. */
			break;
		}
	}
#else
	write_log (buffer, msg, len);
#endif
}

static gboolean clear_moo_thread (gpointer user_data)
{
	if (moo_thread)
	{
		g_thread_join (moo_thread);
		moo_thread = NULL;
		moo_vm = NULL;
	}
	return G_SOURCE_REMOVE;
}
/* ========================================================================= */

#define MIN_MEMSIZE 2048000ul

gpointer moo_thread_func (gpointer ctx)
{
	static moo_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' }; /*TODO: make this an argument */
	static moo_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };

	moo_t* moo;
	moo_cfgstd_t cfg;
	moo_errinf_t errinf;

	moo_iostd_t in;

	moo_oocs_t objname;
	moo_oocs_t mthname;
	int i, xret;

	memset (&cfg, 0, MOO_SIZEOF(cfg));
	cfg.type = MOO_CFGSTD_OPTB;
	cfg.cmgr = moo_get_utf8_cmgr();
	cfg.input_cmgr = cfg.cmgr;
	cfg.log_cmgr = cfg.cmgr;
	cfg.u.optb.log = "/dev/stderr,warn+";
	cfg.log_write = log_write;

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
		moo_setoption (moo, MOO_OPTION_SYMTAB_SIZE, &tab_size);
		tab_size = 5000;
		moo_setoption (moo, MOO_OPTION_SYSDIC_SIZE, &tab_size);
		tab_size = 600;
		moo_setoption (moo, MOO_OPTION_PROCSTK_SIZE, &tab_size);
	}

	if (moo_ignite(moo, MIN_MEMSIZE) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot ignite moo - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
		moo_close (moo);
		return -1;
	}


	memset (&in, 0, MOO_SIZEOF(in));
#if 1
	in.type = MOO_IOSTD_FILEB;
	in.u.fileb.path = "../../kernel/test-001.moo";
#else
	moo_uch_t tmp[1000];
	moo_oow_t bcslen, ucslen;
	ucslen = MOO_COUNTOF(tmp);
	moo_conv_utf8_to_ucstr("../../kernel/test-001.moo", &bcslen, tmp, &ucslen);
	in.type = MOO_IOSTD_FILEU;
	in.u.fileu.path = tmp;
#endif
	in.cmgr = MOO_NULL;

/*compile:*/
	if (moo_compilestd(moo, &in, 1) <= -1)
	{
		if (moo->errnum == MOO_ESYNERR)
		{
			print_syntax_error (moo, in.u.fileb.path);
		}
		else
		{
			moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot compile code - [%d] %js\n", moo_geterrnum(moo), moo_geterrmsg(moo));
		}

		moo_close (moo);
		return -1;
	}

	MOO_DEBUG0 (moo, "COMPILE OK. STARTING EXECUTION...\n");
	xret = 0;


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

	/*moo_dumpsymtab(moo);
	moo_dumpdic(moo, moo->sysdic, "System dictionary");*/

	moo_close (moo);

	gdk_threads_add_idle(clear_moo_thread, NULL);
	return xret;
}


static void start_moo (GtkWidget* widget, gpointer user_data)
{
	if (!moo_thread)
		moo_thread = g_thread_new("moo", moo_thread_func, NULL);
}

static void abort_moo ()
{
	if (moo_vm) moo_abortstd (moo_vm);
}

static void activate (GtkApplication *app, gpointer user_data)
{
	/* Declare variables */
	GtkWidget *window;
	/*GtkWidget *log_view; */
	GtkWidget *scrolled_window;
	GtkWidget *button;
	GtkWidget* table;

	GtkTextBuffer *buffer;

	/* Create a window with a title, and a default size */
	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "MOO LOGS");
	gtk_window_set_default_size (GTK_WINDOW (window), 720, 200);


	/* The text buffer represents the text being edited */
	buffer = gtk_text_buffer_new(NULL);

	/* Text view is a widget in which can display the text buffer. 
	* The line wrapping is set to break lines in between words.
	*/
	log_view = gtk_text_view_new_with_buffer(buffer);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD); 
	gtk_text_view_set_editable (GTK_TEXT_VIEW(log_view), FALSE);

	/* Create the scrolled window. Usually NULL is passed for both parameters so 
	* that it creates the horizontal/vertical adjustments automatically. Setting 
	* the scrollbar policy to automatic allows the scrollbars to only show up 
	* when needed. 
	*/
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC); 
	/* The function directly below is used to add children to the scrolled window 
	* with scrolling capabilities (e.g log_view), otherwise, 
	* gtk_scrolled_window_add_with_viewport() would have been used
	*/
	gtk_container_add (GTK_CONTAINER(scrolled_window), log_view);
	gtk_container_set_border_width (GTK_CONTAINER(scrolled_window), 5);

	button = gtk_button_new_with_label("Start");
	g_signal_connect (button, "clicked", G_CALLBACK(start_moo), NULL);


	table = gtk_table_new(6, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 3);
	gtk_table_set_row_spacing(GTK_TABLE(table), 0, 3);

	gtk_table_attach(GTK_TABLE(table), scrolled_window, 0, 2, 0, 4, 
		GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 1, 1);
	gtk_table_attach(GTK_TABLE(table), button, 1, 2, 5, 6, 
		0/*GTK_FILL | GTK_EXPAND*/, 0/*GTK_FILL | GTK_EXPAND*/, 0, 0);

	gtk_container_add (GTK_CONTAINER(window), table);

	/*g_signal_connect (window, "destroy", G_CALLBACK(quit_app), NULL);*/


	gtk_widget_show_all (window);
}


int main (int argc, char **argv)
{
	GtkApplication *app;
	int status;

	app = gtk_application_new ("moo.log", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run (G_APPLICATION(app), argc, argv);
	g_object_unref (app);

	if (moo_thread) 
	{
		g_thread_join (moo_thread);
		moo_thread = NULL;
	}
	return status;
}

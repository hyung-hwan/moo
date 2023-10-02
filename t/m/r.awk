### THIS SCRIPT NEEDS MUCH MORE ENHANCEMENT.
### THE CURRENT CODE IS JUST TEMPORARY.

BEGIN {
	@local pid, workfile, d, f, cmd;

	pid = sys::getpid();
	workfile = "moo-test." pid;

	d = dir::open (".", dir::SORT);
	while (dir::read(d, f) > 0)
	{
		if (f !~ /.txt$/) continue;

		print "#include \"../../kernel/Moo.moo\"." > workfile;
		print "class MyObject(Object) {" >> workfile;
		print "\tmethod(#class) main" >> workfile;
		print "\t{" >> workfile;

		while ((getline x < f) > 0)
		{
			print x >> workfile;
		}
		close (f);

		print "\t}" >> workfile;
		print "\}" >> workfile;
		close (workfile);

		cmd = "/home/hyung-hwan/xxx/bin/moo " workfile;
		print "<" f ">";
		##sys::system ("cat " workfile);
		sys::system (cmd);
	}

	dir::close(d);
	sys::unlink (workfile);
}

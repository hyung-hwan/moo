class(#limited) InputOutputStud(Object) from "io"
{
	var(#get) handle. // you must keep handle as the first field for consitency with the io module.

	method(#primitive,#lenient) _close.
	method(#primitive) _readBytesInto: buffer.
	method(#primitive) _readBytesInto: buffer startingAt: offset for: count.
	method(#primitive) _writeBytesFrom: buffer.
	method(#primitive) _writeBytesFrom: buffer startingAt: offset for: count.
}

class FileAccessor(InputOutputStud) from "io.file"
{
	pooldic Flag
	{
		//O_RDONLY := 0,
		//O_WRONLY := 1
		O_CLOEXEC  from "O_CLOEXEC",
		O_CREAT    from "O_CREAT",
		O_EXCL     from "O_EXCL",
		O_NOFOLLOW from "O_NOFOLLOW",
		O_NONBLOCK from "O_NONBLOCK",
		O_RDONLY   from "O_RDONLY",
		O_RDWR     from "O_RDWR",
		O_TRUNC    from "O_TRUNC",
		O_WRONLY   from "O_WRONLY",

		SEEK_CUR   from "SEEK_CUR",
		SEEK_END   from "SEEK_END",
		SEEK_SET   from "SEEK_SET"
	}

	method(#primitive,#lenient) _open: path flags: flags.
	method(#primitive) _seek: offset whence: whence.

	method(#class) on: path for: flags
	{
		| fa |
		fa := self new _open: path flags: flags.
		if (fa isError) { self error: "Unable to open file %s - %s" strfmt(path, thisProcess primErrorMessage) }.
		fa addToBeFinalized.
		^fa.
	}

	method close
	{
		self _close.
		self removeToBeFinalized.
	}

	method finalize
	{
		self close.
	}
}

/*
class UnixFileAccessor(FileAccessor) from "io.file.unix"
{
	method(#primitive) _open: path flags: flags mode: mode.
}
*/

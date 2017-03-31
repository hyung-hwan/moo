class _FFI(Object) from 'ffi'
{
	method(#primitive) open(name).
	method(#primitive) close.
	method(#primitive) getsym(name).
	 
	(* TODO: make call variadic? method(#primitive,#variadic) call (func, sig). *)
	method(#primitive) call(func, sig, args).
}

class FFI(Object)
{
	dcl name ffi funcs.

	method(#class) new: aString
	{
		^self new open: aString.
	}

	method initialize 
	{
		self.funcs := Dictionary new.
		self.ffi := _FFI new.
	}

	method open: name
	{
		| x |
		self.funcs removeAllKeys.
		self.name := name.

		x := self.ffi open(name).
		(x isError) ifTrue: [^x].

		^self.
	}

	method close
	{
		self.ffi close.
	}

	method call: name signature: sig arguments: args
	{
		| f |
		f := self.funcs at: name.
		(f isError) ifTrue: [
			f := self.ffi getsym(name).
			(f isError) ifTrue: [^f].
			self.funcs at: name put: f.
		].

		^self.ffi call(f, sig, args)
	}
}

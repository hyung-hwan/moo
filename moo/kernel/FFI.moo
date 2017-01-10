class(#byte) _FFI(Module) from 'ffi'
{
	(* 
	 * the ffi module installs the following methods 
	 *   method(#class) _newInstSize
	 *   method open: name
	 *   method close
	 *   method call
	 *   method call: func sig: sig with: args.
	 *)
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

		x := self.ffi open: name.
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
			f := self.ffi getsym: name.
			(f isError) ifTrue: [^f].
			self.funcs at: name put: f.
		].

		(*^self.ffi call: f sig: sig with: args*)
		^self.ffi call(f, sig, args)
	}
}

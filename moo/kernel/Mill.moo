#include 'Moo.moo'.

class Mill(Object)
{
	dcl registrar.
dcl obj.

	method initialize
	{
		self.registrar := Dictionary new.
	}

	method register: name call: callable
	{
		| k | 
		(k := self.registrar at: name) isNil ifTrue: [
			self.registrar at: name put: [
				self.obj isNil ifTrue: [
					self.obj := callable value.
				].
				self.obj
			].
		].

		^k
	}

	method make: name
	{
		| k |

		(k := self.registrar at: name) notNil ifTrue: [
			^k value.
		].

		^nil.
	}
}

class MyObject(Object)
{
	method(#class) mainxx
	{
		| d a ffi |

		(*k := Mill new.
		k register: #abc call: [ Dictionary new ].
		a := k make: #abc.
		##a dump.*)
		
		d := Dictionary new.
		d at: #abc put: 20.
		d at: #dddd put: 40.
		d at: #jjjj put: 'hello world'.
		d at: #moo put: 'good?'.
		d at: #moo put: 'not good?'.

		(* (d at: #abc) dump.
		(* (d at: #dddd) dump. *)
		(*d do: [:v | v dump].*)

		d keysAndValuesDo: [:k :v| System logNl: (k asString) & ' => ' & (v asString) ].

		##(d includesAssociation: (Association key: #moo value: 'not good?')) dump.
		'-------------------------' dump.
		##(System at: #MyObject) dump.
		(d removeKey: #moo) dump.
		d removeKey: #abc ifAbsent: [System logNl: '#moo not found'].
		d keysAndValuesDo: [:k :v| System logNl: (k asString) & ' => ' & (v asString) ].

		'-------------------------' dump.
		(d at: #jjjj) dump.
		(d at: #jjjja) dump.
		
		(* 
		System keysAndValuesDo: [:k :v| 
				System logNl: (k asString) & ' => ' & (v asString) 
		].
		*)
		
		##System keysDo: [:k| System logNl: (k asString)]
		
		(*[
			[Exception hash dump] ensure: ['xxxx' dump].
		] on: Exception do: [:ex | ('Exception caught - ' & ex asString) dump ].*)

		ffi := FFI new: '/lib64/libc.so.6'.
		(*
		(ffi isError) 
			ifTrue: [System logNl: 'cannot open libc.so' ]
			ifFalse: [
				(ffi call: #getpid  signature: ')i' arguments: nil) dump.
				(ffi call: #printf signature: 's|iis)i' arguments: #(S'A=>%d B=>%d Hello, world %s\n' 1 2 'fly away')) dump.
				(ffi call: #printf signature: 's|iis)i' arguments: #(S'A=>%d B=>%d Hello, world %s\n' 1 2 'jump down')) dump.
				ffi close.
			].
		*)
		if (ffi isError)
		{
			System logNl: 'cannot open libc.so'
		}
		else
		{
			(ffi call: #getpid  signature: ')i' arguments: nil) dump.
			(ffi call: #printf signature: 's|iis)i' arguments: #(S'A=>%d B=>%d Hello, world %s\n' 1 2 'fly away')) dump.
			(ffi call: #printf signature: 's|iis)i' arguments: #(S'A=>%d B=>%d Hello, world %s\n' 1 2 'jump down')) dump.
			ffi close.
		}.
		(('abcd' == 'abcd') ifTrue: [1] ifFalse: [2]) dump.

	}
	
	method(#class) main
	{
		|a k|
		
		
		'BEGINNING OF main...........' dump.
		a := 
			if ([System logNl: 'xxxx'. 'abcd' == 'bcde'. false] value) 
			{ 
				System logNl: 'XXXXXXXXX'.
				1111 
			}
			elsif ('abcd' ~= 'abcd')
			{
				System logNl: 'second if'.
			}
			elsif ([k := 20. System logNl: 'k => ' & (k asString). k + 20. true] value)
			{
				System logNl: 'THIRID forever.............' & (k asString)
			}
			elsif (true = true)
			{
				System logNl: 'forever.............'
			}
			else
			{
				System logNl: 'NO MATCH'.
				[true] value.
			}.
		
		a dump.
		
		(if (false) { 10 } else { 16rFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF}) dump.
		
		System logNl: 'DONE DONE DONE...'.
		
		
		(if (false) {} else {1.} ) dump.
		
		'TESTING ^^....' dump.
		a := 10.
		([
			a := a + 3.
			if (a > 10) {^^99 }
		] value) dump.

		a := 5.
		##((a < 20) ifTrue: [ 1. if (a < 20) { ^^50 }. 90. ]) dump.
		([true] whileTrue: [
			[true] whileTrue: [
				[
				'aaa' dump.
				if (a > 20) { ^^506070 }.
				a := a + 1.
				'bbb' dump.
				] ensure: [('xxxxx' & a asString) dump].
			]
		]) dump.

		(*
		a := 5.
		while (true) {
			System logNl: a asString.
			a := a + 100000000000000.
		}.*)
		
		
		a := if(false) { 10 } elsif (false) { 20 } elsif (false) { 30} else { 40}.
		##a := if(false) { 999 } else { 888 }.
		a dump.

		a := 5.
		a := while (true)
		{
			a := a + 1.
			##if (a > 20) { break if (true) { break. }. }.
			if (a > 20) { 
				^if (true) { break } else {10}.   ## return gets ignored for break.
			}.
			a dump.
		}.
		a dump.
		a := 5.
	do {
		a := do {
			('in loop.....' & a asString) dump.
			##if (a > 5) { break }.
			a := a + 1.
		} while(a < 10).
	} while (false).
		a dump.

		## these should be elimited by the compiler.
		nil.
		1.
		0.
		self.
		thisProcess.
		thisContext.
		nil.
		nil.
		## end of elimination.
		
		'---------- END ------------' dump.
		##Processor sleepFor: 20.
	}
}

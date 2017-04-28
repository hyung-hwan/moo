#include 'Moo.moo'.

class Mill(Object)
{
	var registrar.
	var obj.

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
	
	
	method(#class) main_xx
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

		## PROBLEM: the following double loop will exhaust the stack 
		while (true)
		{
			while (true) 
			{ 
				##[:j :q | (j + q) dump] value: (if (true) { 20 }) value: (if (true) { break }).
				(1 + (if (false) {} else { break })) dump.
			}.
		}.

		
		a :=999.
		a := #{
			1, 
			2,
			a,
			4,
			1 + 1,
			#{ 1, 2, #{a, a := a + 1, a, if (a > 10) { a + 20 } }, 3},
			2 + 2,
			#'a b c'
		}.
		
		(* Dictionary ???
		a := #{
			key -> value ,
			key -> value ,
			key -> value ,
			key -> value
		}
		
		a := :{
			key -> value,
		}
		*)

		a do: [ :v | v dump].

(*
		## how to handle return inside 'if' like the following???
		## what happens to the stack?
		a := if ((k := 20) == 10) {99} else { 100}.
		k dump.
		a dump.
*)
		'---------- END ------------' dump.
		##Processor sleepFor: 20.
	}
	
	
	
	method(#class) main
	{
		|a i|

###self main_xx.

		a := 100.
			## PROBLEM: the following double loop will exhaust the stack 
			(*
		while (true)
		{
		##111 dump.
			while (true) 
			{ 
				##[:j :q | (j + q) dump] value: (if (true) { 20 }) value: (if (true) { break }).
				(1 + (if (false) {} else { break })) dump.
				##break.
				##[:j :q | (j + q) dump] value: 10 value: 20.
				##if (false) {} else { break }.
			}.
		}.*)


		a := :{
			'aaa' -> 10,
			'bbb' -> 20,
			'bbb' -> 30,
			#bbb -> 40,
			Association key: 12343 value: 129908123,
			##5 -> 99,
			'ccc' -> 890
		}.

		(*a removeKey: 'bbb'.
		a remove: :(#bbb).*)

		##1 to: 100 do: [ :i | a at: i put: (i * 2) ].
		a keysAndValuesDo: [:k :v |
			k dump.
			v dump.
			'------------' dump.
		].

		(a associationAt: (#aaa -> nil)) dump.
		(*
		while (true) 
		{
			while (true)
			{
				[:j :q | (j + q) dump] value: (if (true) { 20 }) value: (if (true) { break }).
				(1 + (if (false) {} else { break })) dump.
			}
		}*)

		
		

		(* basicAt: 12 to access the nsdic field. use a proper method once it's defined in Class *)
	##	(System nsdic) keysAndValuesDo: [:k :v |
	##		k dump.
	##		v dump.
	##		'------------' dump.
	##	].

		
		(System at: #Processor) dump.
		System logNl: 'Sleeping start now....'.

		
a := System _malloc(200).
i := 0.
while (i < 26)
{
	a putUint8(i, ($A asInteger + i)).
	System _putUint8(a, i, ($A asInteger + i)).
	i := i + 1.
}.
while (i > 0)
{
	i := i - 1.
	a getUint8(i) asCharacter dump.
}.
a getUint32(0) dump.
a getUint32(1) dump.

##a dump.
##System _free(a).
a free.

		Processor sleepFor: 2.
	}
	
	(*
	#method(#class) main
	{
		| event |
		
		Timer fire: [ 'Timer job' dump. ] in: 3000.
		
		GUI on: #XXX do: [:evt | ... ].
		GUI on: #YYYY do: [:evt | ... ].

		while (true)
		{
			event := GUI waitForEvent.
			GUI dispatchEvent: event.
		}
	}*)
}

(*
pooldic XXD {
	#abc := #(1 2 3).
	#def := #{ 1, 3, 4 }. ## syntax error - literal expected where #{ is
}
*)

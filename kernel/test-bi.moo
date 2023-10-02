
#include "Moo.moo".

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

class MyObject(Object)
{
	var(#class) a := 100.
	var(#classinst) K := 200.

	method(#class) testBigintDiv
	{
		| i q r divd divr divd_ubound divr_ubound |

		divr_ubound := 16rFFFFFFFFFFFFFFFFFFFFFFFF.
		divd_ubound := 16rFFFFFFFFFFFFFFFFFFFFFFFF.

		divr := 1.
		while (divr <= divr_ubound)
		{
			("divr => " & divr asString) dump.

			divd := 0.
			while (divd <= divd_ubound)
			{
				q := divd div: divr.
				r := divd rem: divr.
				if (divd ~= (q * divr + r)) { i dump. divd dump. divr dump. q dump. r dump. (q * divr + r) dump. ^false. }.
				divd := divd + 1.
			}.
			divd := divr + 1.
		}.
		^true
	}

	method(#class) testBigintDiv2
	{
		| ffi now |


		[ ffi := FFI new: "libc.so.6". ] on: Exception do: [:ex | 
			[ ffi := FFI new: "libc.so" ] on: Exception do: [ :ex2 | ffi := FFI new: "msvcrt.dll" ]
		].

		now := ffi call: #time signature: "l>i" arguments: #(0).
		ffi call: #srand signature: "i>" arguments: ##(now).
		///ffi call: #srandom signature: "i>" arguments: ##(now).

		[
			| i q r divd divr divd_ubound divr_ubound x |

			divr_ubound := 16rFFFFFFFFFFFFFFFFFFFFFFFF.
			divd_ubound := 16rFFFFFFFFFFFFFFFFFFFFFFFF.
			i := 0.

			while (true)
			{
				x := (ffi call: #rand signature: ">i" arguments: nil) rem: 20.
				divd := (ffi call: #rand signature: ">i" arguments: nil).
				///x := (ffi call: #random signature: ">l" arguments: nil) rem: 20.
				///divd := (ffi call: #random signature: ">l" arguments: nil).
				while (x > 0)
				{
					divd := (divd bitShift: 7) bitOr: (ffi call: #rand signature: ">i" arguments: nil).
					///divd := (divd bitShift: 7) bitOr: (ffi call: #random signature: ">l" arguments: nil).
					x := x - 1.
				}.

				x := (ffi call: #rand signature: ">i" arguments: nil) rem: 20.
				divr := (ffi call: #rand signature: ">i" arguments: nil).
				///x := (ffi call: #random signature: ">l" arguments: nil) rem: 20.
				///divr := (ffi call: #random signature: ">l" arguments: nil).
				while (x > 0)
				{
					divr := (divr bitShift: 7) bitOr: (ffi call: #rand signature: ">i" arguments: nil).
					//divr := (divr bitShift: 7) bitOr: (ffi call: #random signature: ">l" arguments: nil).
					x := x - 1.
				}.
				if (divr = 0) { divr := 1 }.

				i := i + 1.
				ffi call: #printf signature: "s | l > l" arguments: ##("%d\r", i).

				q := divd div: divr.
				r := divd rem: divr.
				if (divd ~= (q * divr + r)) 
				{ 
					// dump numbers if result is wrong
					divd dump.
					divr dump.
					q dump.
					r dump.
					(q * divr + r) dump. 
					^false. 
				}.
				////((q asString) & " " & (r asString)) dump 
			}.
		] ensure: [ ffi close. ].
		^true
	}

	method(#class) main
	{
		self testBigintDiv2.
	}
}



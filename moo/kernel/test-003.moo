//
// TEST CASES for basic methods
//

#include "Moo.moo".

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

class MyObject(Object)
{
	var(#classinst) t1, t2.

	method(#class) xxxx
	{
		| g1 g2 |
		t1 dump.
		t2 := [ g1 := 50. g2 := 100. ^g1 + g2 ].
		(t1 < 100) ifFalse: [ ^self ].
		t1 := t1 + 1. 
		^self xxxx.
	}
	method(#class) test1
	{
		//// TODO: add this to the test case list.
		| rec results |

		//rec := [ :y :z | (108.0000000000000000000000 - (815.000000000000000000 - (1500.0000000000000000 div: z) div: y)) truncate: 18. ].
		//rec := [ :y :z | (108.0000000000000000000000 - (815.000000000000000000 - (1500.0000000000000000 div: z) div: y)) truncate: 16. ].
		//rec := [ :y :z | 108.0000000000000000000000 - (815.000000000000000000 - (1500.0000000000000000 div: z) div: y) ].
		//rec := [ :y :z | (108.0 scale: 22) - ((815 scale: 18) - ((1500 scale: 16) div: z) div: y) ].
		//rec := [ :y :z | 108.000000000000000000000000000000 - (815.00000000000000000000000000 - (1500.0000000000000000 div: z) div: y) ].
		rec := [ :y :z | 22p108 - (18p815 - (16p1500 div: z) div: y) ].


		// results := ##( 4.0, 425 div: 100.0 ) asOrderedCollection.
		results := OrderedCollection new.
		results add: 4.0; add: (425.00 div: 100.00).


		3 to: 100 do: [ :i |
//(results at: i - 2) dump.
//(results at: i - 3) dump.
//"----------" dump.
			results add: (rec value: (results at: i - 2) value: (results at: i - 3)).
		].

		results doWithIndex: [ :each :i |
			System log: (i asString); log: "\t";
			       log: each; logNl: "".
		].
	}


	method(#class) test_local_return_001
	{
		| a b c d|
		a := 10.
		c := 2.
		d := 3.
		b := ([
			[
				a := a + 3.
				if (a > 10) { ^^77 } // ^^ must return to the calling method despite 2 blocks nested.
			] value.
			d := 99. // this is skipped because of ^^77 above.
		] ensure: [ c := 88 ]). // however, the ensured block must be executed.

		^a == 13 and b == 77 and c == 88 and d == 3.
	}

	method(#class) test_if_001
	{
		| a b c d e x |

		x := if (false) { X02: a := 55. goto Z02 }
		     elif (false) { b := 66.}
		     elif (2 > 1) { c := 77. goto X02 }
		     elif (true) { d := 88 }
		     else { Z02:  e := 99 }.

		^c == 77 and a == 55 and e == 99 and b == nil and d == nil and x == 99.
	}

	method(#class) test_while_001
	{
		| a b i |

		a := 0.
		b := #(0 0 0 0 0) copy.
		i := 0.

		while (false)
		{
		X02:
			a := a + 1.
			b at: i put: 1.
			i := i + 1.
			goto X03.
		}.
		goto X02.

		until (false)
		{
		X03:
			a := a + 2.
			b at: i put: 2.
			i := i + 1.
			goto Y01.
		}.

		do
		{
		X04:
			a := a + 3.
			b at: i put: 3.
			i := i + 1.
			goto X05.
		}
		while (true).
	Y01:
		goto X04.

		do
		{
			a := a + 4.
			b at: i put: 4.
			i := i + 1.
			break.
		X05:
			a := a + 5.
			b at: i put: 5.
			i := i + 1.
		}
		until (false).

		^a == 15 and b = #(1 2 3 5 4).
	}

method(#class) q
{
	| v |
	
	v := 0.
start:
	if (v > 100) { ^nil }.
	v dump.
	v := v + 1.
	goto start.
}

	method(#class) main
	{
		| tc limit |

		tc := ##(
			// 0 - 4
			[(Object isKindOf: Class) == true],
			[(Object isKindOf: Apex) == true],
			[(Class isKindOf: Class) == true],
			[(Class isKindOf: Apex) == true],
			[(Class isKindOf: Object) == false],

			// 5-9
			[(Apex isKindOf: Class) == true],
			[(Apex isKindOf: Apex) == true],
			[(SmallInteger isKindOf: Integer) == false],
			[(SmallInteger isKindOf: SmallInteger) == false],
			[(Object isKindOf: SmallInteger) == false],

			// 10-14
			[(10 isKindOf: Integer) == true],
			[(10 isKindOf: 20) == false],
			[([] isKindOf: BlockContext) == true],
			[([] isKindOf: MethodContext) == false],
			[([] isKindOf: Context) == true],

			// 15-19
			[("string" isKindOf: String) == true],
			[(#symbol isKindOf: String) == true],
			[("string" isKindOf: Symbol) == false],
			[(#symbol isKindOf: Symbol) == true],
			[ [] value == nil ],
			
			// 20-24
			[ self test_local_return_001 ],
			[ self test_if_001 ],
			[ self test_while_001 ],
			[ (if (1 > 2) { } else { }) == nil. ],
			[ (if (1 < 2) { } else { }) == nil. ],
			[ (if (1 > 2) { } else { goto A01. A01: }) == nil ],

			// 25-29
			[ (if (1 > 2) { } else { 9876. goto A02. A02: }) == 9876 ],
			[ [ | a3 | a3:= 20. if (a3 == 21) { a3 := 4321. goto L03 } else { a3 := 1234. goto L03 }. a3 := 8888. L03: a3 ] value == 1234 ],
			[ [ | a4 | a4:= 21. if (a4 == 21) { a4 := 4321. goto L04 } else { a4 := 1234. goto L04 }. a4 := 8888. L04: a4 ] value == 4321 ]
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { " PASS" } else { " FAIL" }), "\n").
		].


//	(if (true) { a: 10. b: 1p1000. c: 20000 }) dump.
//	[goto B02. A01: 10. B02: 1000. ] value class dump.
self q.

//[ | a | a := 21. if (a = 21) { goto X02 }. X02: ] value dump. // this causes a stack depletion problem... TODO:

                //[ a := 4. while (1) { X44: goto X33 }. X33: 1233 dump. if (a < 10) { a := a + 1. 'X44' dump. goto X44 } else { 'ELSE' dump. a := a * 10}. ] value dump.


/*
this is horrible... the stack won't be cleared when goto is made... 
System should never be popped out
	(if (2 > 1) { 20. System log: (if (2 > 1) {goto X2}) }) dump.
X2:
	a dump.
*/

EXCEPTION_TEST:
Exception signal: 'experiment with exception signalling'.

		// TODO: 
		String format("%s", " 나	는\\\"") dump.
		#"a b\nc" dump.
	}
}

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

			// 15-20
			[("string" isKindOf: String) == true],
			[(#symbol isKindOf: String) == true],
			[("string" isKindOf: Symbol) == false],
			[(#symbol isKindOf: Symbol) == true]
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { " PASS" } else { " FAIL" }), "\n").
		].
	(if (true) { a: 10. b: 1p1000. c: 20000 }) dump.
	[goto B02. A01: 10. B02: 1000. ] value class dump.
self q.

EXCEPTION_TEST:
Exception signal: 'experiment with exception signalling'.

		// TODO: 
		String format("%s", " 나	는\\\"") dump.
		#"a b\nc" dump.
	}
}

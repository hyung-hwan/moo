##
## TEST CASES for basic methods
##

#include 'Moo.moo'.

#################################################################
## MAIN
#################################################################

## TODO: use #define to define a class or use #class to define a class.
##       use #extend to extend a class
##       using #class for both feels confusing.

extend Apex
{

}

extend SmallInteger
{
	method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	method inc
	{
		^self + 1.
	}
}

class TestObject(Object)
{
	var(#class) Q, R.
	var(#classinst) a1, a2.
}


class MyObject(TestObject)
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

	method(#class) main
	{
		| tc limit |

		tc := %(
			## 0 - 4
			[(Object isKindOf: Class) == true],
			[(Object isKindOf: Apex) == true],
			[(Class isKindOf: Class) == true],
			[(Class isKindOf: Apex) == true],
			[(Class isKindOf: Object) == false],

			## 5-9
			[(Apex isKindOf: Class) == true],
			[(Apex isKindOf: Apex) == true],
			[(SmallInteger isKindOf: Integer) == false],
			[(10 isKindOf: Integer) == true],
			[(10 isKindOf: 20) == false],

			## 10-14
			[([] isKindOf: BlockContext) == true],
			[([] isKindOf: MethodContext) == false],
			[([] isKindOf: Context) == true],
			[('string' isKindOf: String) == true],
			[(#symbol isKindOf: String) == true],

			[('string' isKindOf: Symbol) == false],
			[(#symbol isKindOf: Symbol) == true]
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		]
	}
}

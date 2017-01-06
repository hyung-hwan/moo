
#include 'Stix.st'.

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
	dcl(#class) Q R.
	dcl(#classinst) t1 t2.
}


class MyObject(TestObject)
{
	dcl(#class) C B A.

	method getTrue
	{
		^true.
	}

	method getTrue: anInteger
	{
		^ anInteger
	}
	method getFalse
	{
		^false
	}

	method a { ^ 10 }
	method b { ^ 20 }
	method c { ^ 30 }

	method(#class) a: a b: b c: c
	{
		^ a + b + c.
	}

	method(#class) getBlock
	{
		| a |
		a := 7777777.
"
		^[1 + [^a]].
		1234567 dump.
"

		^[self a: a b: 3 c: ([[[^6] value] value ] value)].
	}

	method(#class) main
	{
"
		| k | 

		k := 30.
		k := k + 10; + 20.
		k dump.
		(self a: 1 b: 2 c: 3) dump.
		[self a: 1 b: 2 c: 3] value dump.
		[self a: 1 b: 2 c: 3] value dump.
		[self a: 9 b: 10 c: 11] value dump.


		((k = 2) ifTrue: [11111] ifFalse: [2222])dump.

		self getBlock value dump.

		[10 + [^20]] value dump.
"


		'START OF MAIN' dump.
		[2 + 3 + 1 + [[[^6] value] value ] value] value dump.
	##	^(self a: (self new a) b: ([:a :b | a + b] value: 10 value: 20) c: (self new c)) dump.
		##self getBlock value dump.
		'END OF MAIN' dump.
	}

}

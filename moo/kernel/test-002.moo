
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
	dcl(#class) Q R.
	dcl(#classinst) a1 a2.
}


class MyObject(TestObject)
{
	#declare(#classinst) t1 t2.
	method(#class) xxxx
	{
		| g1 g2 |
		t1 dump.
		t2 := [ g1 := 50. g2 := 100. ^g1 + g2 ].
		(t1 < 100) ifFalse: [ ^self ].
		t1 := t1 + 1. 
		^self xxxx.
	}

	method(#class) zzz
	{
		'zzzzzzzzzzzzzzzzzz' dump.
		^self.
	}
	method(#class) yyy
	{
		^[123456789 dump. ^200].
	}

	method(#class) main2
	{
		'START OF MAIN2' dump.
		##[thisContext dump. ^100] newProcess resume.
		[ |k| thisContext dump. self zzz. "k := self yyy. k value." ['ok' dump. ^100] value] newProcess resume.
		'1111' dump.
		'1111' dump.
		'1111' dump.
		'1111' dump.
		'1111' dump.
		'EDN OF MAIN2' dump.
	}

	method(#class) main1
	{
		'START OF MAIN1' dump.
		self main2.
		'END OF MAIN1' dump.
	}

	method(#class) main
	{
		'START OF MAIN' dump.
		self main1.
		'EDN OF MAIN' dump.
	}

}

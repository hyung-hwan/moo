
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
	dcl(#classinst) a1 a2.
}


class MyObject(TestObject)
{
	#declare(#classinst) t1 t2.

	method(#class) main2
	{
		'START OF MAIN2' dump.
		t1 value.
		'END OF MAIN2' dump.
	}

	method(#class) main1
	{
		'START OF MAIN1' dump.
		self main2.
		t2 := [ 'BLOCK #2' dump. ^200].
		'END OF MAIN1' dump.
		
	}

	method(#class) main
	{
		'START OF MAIN' dump.
		t1 := ['BLOCK #1' dump. ^100].
		self main1.
		'END OF MAIN' dump.
	}

}

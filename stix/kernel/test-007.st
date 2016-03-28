
#include 'Stix.st'.

#################################################################
## MAIN
#################################################################

## TODO: use #define to define a class or use #class to define a class.
##       use #extend to extend a class
##       using #class for both feels confusing.

#extend Apex
{

}

#extend SmallInteger
{
	#method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	#method inc
	{
		^self + 1.
	}
}

#class TestObject(Object)
{
	#dcl(#class) Q R.
	#dcl(#classinst) t1 t2.
}


#class MyObject(TestObject)
{
	#dcl(#class) C B A.

	#method getTrue
	{
		^true.
	}

	#method getTrue: anInteger
	{
		^ anInteger
	}
	#method getFalse
	{
		^false
	}

	#method a { ^ 10 }
	#method b { ^ 20 }
	#method c { ^ 30 }

	#method(#class) a: a b: b c: c
	{
		^ a + b + c.
	}

	#method(#class) main
	{
		| p p2 |
                'START OF MAIN' dump.
 		##p := [ :a :b :c :d | a dump. b dump. (c + d) dump. ^10. ] newProcessWith: #(abc def 10 20).
 		p := [ :a :b :c :d | a dump. b dump. (c + d) dump. ] newProcessWith: #(abc def 10 20).
 		p2 := [ :a :b :c :d | a dump. b dump. a dump. b dump. (c + d) dump. ^10000 ] newProcessWith: #(
			AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
			BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB 
			1000000000000000
			299999999999999999999999999999999999999999
		).

                p resume.
		p2 resume.

                'MIDDLE OF MAIN' dump.
		Processor activeProcess terminate.

		##p terminate.
                '999999999999999999' dump.
                '999999999999999999' dump.
                '999999999999999999' dump.
                '999999999999999999' dump.
                '999999999999999999' dump.
                '999999999999999999' dump.
                '999999999999999999' dump.
##		p resume.
                '999999999999999999' dump.
                '999999999999999999' dump.
                '999999999999999999' dump.
		'END OF MAIN' dump.
	}

}

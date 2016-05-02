
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
	#dcl(#classinst) a1 a2.
}


#class MyObject(TestObject)
{

	#method(#class) main2
	{
		| k |
		k := ['this is test-011' dump. Exception signal: 'main2 screwed...'. 8888 dump. ] 
			on: Exception do: [ :ex | 'Exception occurred' dump.  ex messageText dump. 'Getting back to....' dump. "ex return: 9999." ex pass. 'AFTER RETURN' dump. ].

		k dump.
		'END OF test-011' dump.
	}

	#method(#class) raise_exception
	{
		Exception signal: 'bad exceptinon'.
	}

	#method(#class) test3
	{
		| k j |
		j := 20.
		k := [
			'>>> TEST3 METHOD >>> ' dump.
			j dump.
			(j < 25) ifTrue: [ | t | 
				t := Exception signal: 'bad exceptinon'.  ## when resume, t should get Exception.
				t := self raise_exception.  ## when resumed, t should get 'self'
				'RESUMED???' dump. 
				t dump.
				j dump.
			]. 

			'OOOOOOOOOOOOOOOOOOOOOOO' dump.
			'JJJJJJJJJJJJJJ' dump. 
		] on: Exception do: [ :ex | 'Exception occurred' dump. ex messageText dump. j := j + 1. ex resume. ].

		k dump.
		'END OF TEST3' dump.
	}

	#method(#class) test4_1
	{
		| k j |
		j := 20.
		k := [
			'>>> TEST4_1 METHOD >>> ' dump.
			j dump.
			(j < 25) ifTrue: [ | t | 
				##t := Exception signal: 'bad exceptinon'.  ## when resume, t should get Exception.
				t := self raise_exception.  ## when resumed, t should get 'self'
				'RESUMED???' dump. 
				t dump.
				j dump.
			]. 

			'OOOOOOOOOOOOOOOOOOOOOOO' dump.
			'JJJJJJJJJJJJJJ' dump. 
		] on: Exception do: [ :ex | 'Exception occurred' dump. ex messageText dump. j := j + 1. ex pass. ].

		k dump.
		'END OF TEST4_1' dump.
	}

	#method(#class) test4
	{
		'BEGINNING OF TEST4' dump.
		[ self test4_1 ] on: Exception do: [:ex | 'Excepton in test4_1' dump. ex messageText dump. ex resume].
		'END OF TEST4' dump.
	}


	#method(#class) test5
	{
		| k j |
		'BEGINNING OF TEST5' dump.
		j := 20.
		k := [
			'>>> TEST5 BLOCK >>> ' dump.
			j dump.
			(j < 25) ifTrue: [ | t | 
				##t := Exception signal: 'bad exceptinon'.  ## when resume, t should get Exception.
				t := self raise_exception.  ## when resumed, t should get 'self'
			]. 

			'OOOOOOOOOOOOOOOOOOOOOOO' dump.
			'JJJJJJJJJJJJJJ' dump. 
		] on: Exception do: [ :ex | 'Exception occurred' dump. ex messageText dump. j := j + 1. ex retry. ].

		k dump.
		'END OF TEST5' dump.
	}

	#method(#class) test11
	{
		## exception is raised in a new process. it can't be captured
		## by an exception handler of a calling process.
		## exception handling must not cross the process boundary.
		[ 
			|p | 
			p := [Exception signal: 'Sample Exception' ] newProcess.
			'JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ' dump.
			p resume.
		] on: Exception do: [:ex | 'EXCEPTION ----------' dump. ex messageText dump ].
	}

	#method(#class) test12
	{
		[ 
			|p | 
			p := [ 
				[ Exception signal: 'Sample Exception' ] on: Exception do: [:ex | 'EXCEPTION CAUGHT...' dump. ex messageText dump. ]
			] newProcess.
			'JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ' dump.
			p resume.
		] on: Exception do: [:ex | 'EXCEPTION ----------' dump. ex messageText dump ].
	}

	#method(#class) main
	{

		'>>>>> BEGINNING OF MAIN' dump.

	##	[ self main2 ] on: Exception do: [ :ex | 'EXCEPTION CAUGHT IN MAIN....' dump. ex messageText dump. "ex pass." ex resume. ].

		'##############################' dump.
	##	self test3.
	##	self test4.

		self test5.
	##	self test11.
	##	self test12.

		##100 timesRepeat: ['>>>>> END OF MAIN' dump].
		'>>>>> END OF MAIN' dump.
	}
}

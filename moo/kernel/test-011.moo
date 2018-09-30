
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

	method(#class) main2
	{
		| k q |
		'BEGINNING OF main2' dump.
		k := [ 'this is test-011' dump. q := Exception signal: 'main2 screwed...'. q dump. 8888 dump.  ] 
			on: Exception do: [ :ex | 
				'Exception occurred' dump.  
				ex messageText dump.
				'Getting back to....' dump.
				"ex return: 9999."
				ex pass.
				'AFTER RETURN' dump. 
			].

		'k=>>> ' dump.
		k dump.
		'END OF main2' dump.
	}

	method(#class) raise_exception
	{
		Exception signal: 'bad exceptinon'.
	}

	method(#class) test3
	{
		| k j g_ex |
		j := 20.
		k := [
			'>>> TEST3 METHOD >>> ' dump.
			j dump.
			(j < 25) ifTrue: [ | t | 
				t := Exception signal: 'bad exceptinon'.  ## when resumed, t should get Exception, the leftover in the stack...
				t signal: 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA'. ## so it should be ok to signal again..
				##t := self raise_exception.  ## when resumed, t should get 'self'

				##g_ex retry. # You should not do these as the following 3 lines make things very complicated.
				##g_ex signal.
				##g_ex pass.

				'RESUMED???' dump. 
				t dump.
				j dump.
			]. 

			'OOOOOOOOOOOOOOOOOOOOOOO' dump.
			'JJJJJJJJJJJJJJ' dump. 
		] on: Exception do: [ :ex | 'Exception occurred' dump. ex messageText dump. j := j + 1. g_ex := ex. ex resume. ].

		k dump.
		'END OF TEST3' dump.
	}

	method(#class) test4_1
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

	method(#class) test4
	{
		'BEGINNING OF TEST4' dump.
		[ self test4_1 ] on: Exception do: [:ex | 'Excepton in test4_1' dump. ex messageText dump. ex resume].
		'END OF TEST4' dump.
	}


	method(#class) test5
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

	method(#class) test11
	{
		## exception is raised in a new process. it can't be captured
		## by an exception handler of a calling process.
		## exception handling must not cross the process boundary.
		'BEGINNING OF test11' dump.
		[ 
			|p | 
			p := [ 'TEST11 IN NEW PROCESS' dump. Exception signal: 'Exception raised in a new process of test11'.  ] newProcess.
			'JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ' dump.
			p resume. ## resume the new process
		] on: Exception do: [:ex | '---- EXCEPTION IN TEST11. THIS MUST NOT BE PRINTED----------' dump. ex messageText dump ].
		'END OF test11' dump.
	}

	method(#class) test12
	{
		'BEGINNING OF test12' dump.
		[ 
			|p | 
			p := [ 
				[ Exception signal: 'Exception in a new process of test12' ] 
					on: Exception do: [:ex | 
						('EXCEPTION CAUGHT...in test12 ==> ' & (ex messageText)) dump.
					]
			] newProcess.
			'JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ' dump.
			p resume.
		] on: Exception do: [:ex | 'EXCEPTION ----------' dump. ex messageText dump ].
		'END OF test12' dump.
	}

	method(#class) main
	{

		'>>>>> BEGINNING OF MAIN' dump.

		[ self main2 ] on: Exception do: [ :ex | 
			'EXCEPTION CAUGHT IN MAIN....' dump. 
			ex messageText dump. 
			##ex pass.
			'Returning back to where the exception has signalled in main2...' dump.
			##ex resume. 
			ex resume: 'RESUMED WITH THIS STRING'.
		].

		'##############################' dump.
	##	self test3.
	##	self test4.

	##	self test5.
		self test11.
	##	self test12.
	##	100 timesRepeat: ['>>>>> END OF MAIN' dump].


(*
(Exception isKindOf: Apex) dump.
(Exception isMemberOf: Apex) dump.
(Exception isMemberOf: Class) dump.
(1 isMemberOf: SmallInteger) dump.
(1 isKindOf: Integer) dump.
(1 isKindOf: Class) dump.
(1 isKindOf: Apex) dump.
(Exception isKindOf: Class) dump.
(Exception isKindOf: Apex) dump.
(Exception isKindOf: Object) dump.
(Exception isKindOf: (Apex new)) dump.
(Exception isKindOf: (Object new)) dump.
*)


		'@@@@@@@@@@@@@@@@@@@@@@@@@@@@' dump.
		## the following line(return:to:) must cause primitive failure...
		##[ Processor return: 10 to: 20. ] on: Exception do: [:ex | ex messageText dump].

		##[ Processor return: 10 to: 20. ] 
		##	on: PrimitiveFailureException do: [:ex | 'PRIMITIVE FAILURE CAUGHT HERE HERE HERE' dump]
		##	on: Exception do: [:ex | ex messageText dump].

		'SLEEPING FOR 10 seconds ....' dump.
		System sleepForSecs: 10.

		'>>>>> END OF MAIN' dump.
	}
}

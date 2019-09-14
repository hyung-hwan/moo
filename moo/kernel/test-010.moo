
#include 'Moo.moo'.

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

// TODO: use #define to define a class or use #class to define a class.
//       use #extend to extend a class
//       using #class for both feels confusing.

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

	method test999
	{
		^self.Q
	}
}

class B.TestObject(Object)
{
	var(#class) Q, R.
	var(#classinst) a1, a2.

	method test000
	{
		^self.Q
	}
}

pooldic ABC 
{
	KKK := 20.
}


class MyObject(TestObject)
{
	method(#class) main111
	{
		| s3 |
		s3 := Semaphore new.
		System signal: s3 afterSecs: 1 nanosecs: 50.
		s3 wait.
		'END OF MAIN' dump.
	}

	
	method(#class) main987
	{
		|t1 t2 s1 s2 s3|

		'START OF MAIN' dump.

		s1 := Semaphore new.
		s2 := Semaphore forMutualExclusion.

		t1 := [ 
			//1000 timesRepeat: ['BLOCK #1' dump].
			s2 critical: [
				10000 timesRepeat: ['BLOCK #1' dump ].
				Exception signal: 'Raised Exception at process t1'.
			]
		] newProcess.
		t2 := [ 
			//1000 timesRepeat: ['BLOCK #2' dump].
			s2 critical: [
				10000 timesRepeat: ['BLOCK #2' dump. ]
			].

			s1 signal.
		] newProcess.

		t1 resume.
		t2 resume.

		s1 wait.

		ABC.KKK dump.
		'END OF MAIN' dump.   


"

		|s1|
		s1 := Semaphore new.
		s1 signal.
		'XXXXXXXXXXXXXXXX' dump.
		s1 wait.
"

	}

	method(#class) aaa_123
	{
		| v1 |
		v1 := [
			| k |
			k := 99.
			[
				[
					//[ Exception signal: 'simulated error' ] ensure: [('ensure 1 ' & (k asString)) dump ].
					[ ^ 20 ] ensure: [ ('ensure 1 ' & (k asString)) dump. ].
				] ensure: ['ensure 2' dump ].
			] ensure: ['ensure 3' dump ].
		] on: Exception do: [:ex | 
			('EXCETION - ' & ex messageText) dump.
			// Exception signal: 'qqq'.
		].
		^v1
	}
	method(#class) main
	{
		| v1 |
		'START OF MAIN' dump.
		//[1 xxx] ifCurtailed: ['XXXXXXXX CURTAILED XXXXXXXXX' dump].
		//['ENSURE TEST' dump] ensure: ['XXXXXXXXX ENSURE XXXXXXXXXXXXXx' dump].

		//v1 := [ ['kkk' dump.] ensure: ['XXXXXXXXX ENSURE XXXXXXXXXXXXXx' dump. 30] ] on: Exception do: [:ex | 'EXCEPTION OUTSIDE ENSURE...' dump.  ].
		//v1 dump.


		//[ Exception signal: 'simulated error' ] on: Exception do: [:ex | 'CAUGHT...' dump. Exception signal: 'jjjjjjj' ].
		
		//[
		//		[ Exception signal: 'simulated error' ] ensure: ['ensure 1' dump ].
		//] on: Exception do: [:ex | ('EXCETION - ' & ex messageText) dump. Exception signal: 'qqq'. ].

		//[1 xxx] ifCurtailed: ['XXXXXXXX CURTAILED XXXXXXXXX' dump. Exception signal: 'jjjj'].

		/*
		v1 := [
			| k |
			k := 99.
			[
				[
					//[ Exception signal: 'simulated error' ] ensure: [('ensure 1 ' & (k asString)) dump ].
					[ ^20 ] ensure: [('ensure 1 ' & (k asString)) dump ].
				] ensure: ['ensure 2' dump ].
			] ensure: ['ensure 3' dump ].
		] on: Exception do: [:ex | 
			('EXCETION - ' & ex messageText) dump.
			// Exception signal: 'qqq'.
		].
		*/

		v1 := self aaa_123.
		'--------------------------------' dump.
		v1 dump.
		'--------------------------------' dump.
		'END OF MAIN' dump.
	}

	method(#class) main22222
	{
		|t1 t2 s1 s2 s3|

		'START OF MAIN' dump.

		s1 := Semaphore new.
		s2 := Semaphore new.
		s3 := Semaphore new.

		t1 := [ 
			10 timesRepeat: ['BLOCK #1' dump. System sleepForSecs: 1.].
			s1 signal
		] newProcess.
		t2 := [ 
			5 timesRepeat: ['BLOCK #2' dump. "System sleepForSecs: 1." ].
			'SIGNALLING S2...' dump. s2 signal. 
		] newProcess.

		t1 resume.
		t2 resume.

		System signal: s3 after: 10.

		'STARTED t1 and t2' dump.

		s2 wait.
		's2 WAITED....' dump.
		s1 wait.
		's1 WAITED....' dump.
		

		'WAITING ON S3...' dump.
		//System unsignal: s3.
		s3 wait.

		10 timesRepeat: ['WAITED t1 and t2' dump].
		'END OF MAIN' dump.
	}

	method(#class) test_semaphore_heap
	{
		| sempq a |
		sempq := SemaphoreHeap new.

		'--------------------------' dump.

		1 to: 200 by: 1 do: [ :i |  
			| sem |
			sem := Semaphore new. 
			sem fireTime: (200 - i).
			sempq insert: sem
		].

		'--------------------------' dump.
		sempq removeAt: 40.
		sempq removeAt: 50.

		[sempq size > 0] whileTrue: [
			| sem |
			sem := sempq popTop.
			sem fireTime dump.
		]
	}
}


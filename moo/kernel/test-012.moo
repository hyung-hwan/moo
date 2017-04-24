
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
	method(#class) aaa_123
	{
		| v1 |
		v1 := [
			| k |
			k := 99.
			[
				[
					##[ Exception signal: 'simulated error' ] ensure: [('ensure 1 ' & (k asString)) dump ].
					[ ^ 20 ] ensure: [ ('ensure 1 ' & (k asString)) dump. ].
				] ensure: ['ensure 2' dump ].
			] ensure: ['ensure 3' dump ].
		] on: Exception do: [:ex | 
			('EXCETION - ' & ex messageText) dump.
			## Exception signal: 'qqq'.
		].
		^v1
	}
	method(#class) main
	{
		| v1 |
		'START OF MAIN' dump.
		##[1 xxx] ifCurtailed: ['XXXXXXXX CURTAILED XXXXXXXXX' dump].
		##['ENSURE TEST' dump] ensure: ['XXXXXXXXX ENSURE XXXXXXXXXXXXXx' dump].

		##v1 := [ ['kkk' dump.] ensure: ['XXXXXXXXX ENSURE XXXXXXXXXXXXXx' dump. 30] ] on: Exception do: [:ex | 'EXCEPTION OUTSIDE ENSURE...' dump.  ].
		##v1 dump.


		##[ Exception signal: 'simulated error' ] on: Exception do: [:ex | 'CAUGHT...' dump. Exception signal: 'jjjjjjj' ].
		
		(*[
				[ Exception signal: 'simulated error' ] ensure: ['ensure 1' dump ].
		] on: Exception do: [:ex | ('EXCETION - ' & ex messageText) dump. Exception signal: 'qqq'. ].

		[1 xxx] ifCurtailed: ['XXXXXXXX CURTAILED XXXXXXXXX' dump. Exception signal: 'jjjj']. *)

		v1 := [ 
			[
				| k |
				k := 99.
				[
					[
						[ '@@@@@' dump. 
						  Exception signal: 'simulated error'.
						  '^^^^^^' dump. ] ensure: [
							('ensure 11 ' & (k asString)) dump. 
							Exception signal: 'qqq'.
						].

						##[ ^20 ] ensure: [('ensure 1 ', (k asString)) dump ].

						'KKKKKKKKKKKKKKKKKKKK' dump.
					] ensure: ['ensure 2' dump ].

					'JJJJJJJJJJJJJJJJJJJJJJJJJJJ' dump.
				] ensure: ['ensure 3' dump ].
			] on: Exception do: [:ex | 
				('>>>> EXCETION - ' & ex messageText) dump.
				ex pass.

				##Exception signal: 'XXXXXXXXXXXXx'.
				## ex return: 10.
				## Exception signal: 'qqq'.
			].
			'ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ' dump.
		]
		on: PrimitiveFailureException do: [:ex | 
			('PRIMITIVE FAILURE EXCETION AT OUTER - ' & ex messageText) dump. 
		]
		on: Exception do: [:ex |
			('>>>> EXCETION AT OUTER - ' & ex messageText) dump.
		].


##		v1 := self aaa_123.
		'--------------------------------' dump.
		v1 dump.
		'--------------------------------' dump.
		'END OF MAIN' dump.
	}

}


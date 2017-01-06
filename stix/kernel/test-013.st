
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

	method test999
	{
		^self.Q
	}
}

class B.TestObject(Object)
{
	dcl(#class) Q R.
	dcl(#classinst) a1 a2.

	method test000
	{
		^self.Q
	}
}

pooldic ABC 
{
	#KKK := 20.
}


class MyObject(TestObject)
{
	method(#class) main
	{
		| v1 v2 |
		System logNl: 'START OF MAIN'.
		v2 := [ 
			[ v1 := [ System logNl: 'xxxxxxxxxxxxxxxxc'. Exception signal: 'qqqqq' ] value.
			'OK OK OK' dump. ] ensure: [ System logNl: 'ENSURE ENSURE ENSURE'].
		]
		on: Exception
		do: [:ex |
			System logNl: ('Exception: ', ex messageText).
			ex return: 10.
			##ex retry.
			System logNl: '--- THIS MUST NOT BE PRINTED ---'.
		].


		System logNl: '---------------------'.
		System log: 'v1=>'; log: v1; log: ' v2=>'; logNl: v2.

		v1 := [
			[    
				[ 
					##1 to: 20000 by: 1 do: [:i | System logNl: i asString. "Processor sleepFor: 1." ]  
					Processor activeProcess terminate.
				] ensure: [ System logNl: '<<<PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP>>>' ].

			] ensure: [ System logNl: '<<--------------------->>' ].
		] newProcess.

		System logNl: 'RESUMING v1'.
		v1 resume.
		Processor sleepFor: 1.
		v1 terminate.

 		##[    
		##	[ Processor activeProcess terminate. ] ensure: [System logNl: '<<<PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP>>>' ].
		##] ensure: [ System logNl: '<<--------------------->>' ].

		System logNl: S'\0\0\0END OF MAIN\0AB\0\0\0C\0\0\0'.
	}

}


##
## TEST CASES for basic methods
##

#include 'Moo.moo'.

#################################################################
## MAIN
#################################################################

class MyObject(Object)
{
	var(#class) t := 20.

	method(#class) test_terminate
	{
		| a s |
		s := Semaphore new.
		a := [ self.t := self.t * 9. 
		       s signal.
		       Processor activeProcess terminate.
		       self.t := self.t + 20 ] fork.
		s wait.	
		^self.t
	}

	method(#class) main
	{
		| tc limit |

		tc := %(
			## 0 - 4
			[self test_terminate == 180]
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		]
	}
}

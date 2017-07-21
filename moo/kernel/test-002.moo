
#include 'Moo.moo'.

#################################################################
## MAIN
#################################################################

class MyObject(Object)
{
## TODO: support import in extend??

	var(#class) a := 100.

	method(#class) proc1
	{
		[ Processor sleepFor: 1. a := a + 100 ] newProcess resume.
		^a
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
		sempq deleteAt: 40.
		sempq deleteAt: 50.
		sempq deleteAt: 100.

	
		a := -100.
		[sempq size > 0] whileTrue: [
			| sem b |
			sem := sempq popTop.
			b := sem fireTime.
			if (a > b) { ^false }.
			a := b.
		].
		
		^true
	}

	method(#class) main
	{
		| tc limit |

		tc := %(
			## 0 - 4
			[ self proc1 == 100 ], 
			[ Processor sleepFor: 2.  self proc1 == 200 ],
			[ self test_semaphore_heap == true ]
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		].
	}

}

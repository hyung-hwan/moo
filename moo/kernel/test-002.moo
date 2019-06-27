
#include 'Moo.moo'.

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

class MyObject(Object)
{
	var(#class) a := 100.
	var(#class) ensure_tester_v := 0.

	method(#class) test_quicksort: anArray
	{
		| quicksort |
		quicksort := nil.
		quicksort := [ :data :from :to |
			(from < to)
				ifTrue: [ | pivot index |
					pivot := data at: from.
					data swap: from with: to.
					index := from.
					from to: (to - 1) do: [ :each | 
						(data at: each) < pivot
						ifTrue: [ 
							data swap: each with: index. 
							index := index + 1
						] 
					].

					data swap: to with: index.
					quicksort value: data value: from value: index - 1.
					quicksort value: data value: index + 1 value: to 
				].
			data 
		].

		^(quicksort value: anArray value: 0 value: (anArray size - 1))
	}

	method(#class) test_on_do_with: dividend with: divisor
	{
		// it returns 0 upon an exception otherwise, division result.
		^[ dividend div: divisor ]  on: Exception do: [:ex | 0 ]
	}

	method(#class) test_ensure_with: v
	{
		[ 
			[ Exception signal: "test ensure exception" ] ensure: [ self.ensure_tester_v := v ].
		] on: Exception do: [:ex | /* do nothing */ ].
	}

	method(#class) test_ensure2_with: v
	{
		[ ^v * v ] ensure: [ self.ensure_tester_v := v ].
	}

	method(#class) proc1
	{
		[ System sleepForSecs: 1. a := a + 100 ] newProcess resume.
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

	method(#class) test_mutex
	{
		| mtx sem v p q |

		mtx := Mutex new.
		sem := Semaphore new.

		p := 0.

		[ mtx lock.
		  v := 0.
		  2000 timesRepeat: [v := v + 1. p := p + 1. ].
		  q := v.
		  mtx unlock.
		  sem signal.
		] fork.

		[ mtx critical: [
		     v := 0.
		     2000 timesRepeat: [v := v + 1. p := p + 1. ].
		     q := v.
		  ].
		  sem signal.
		] fork.

		mtx lock.
		v := 0.
		2000 timesRepeat: [v := v + 1. p := p + 1. ].
		mtx unlock.

		sem wait.
		sem wait.

		^##( v, p ) // v must be 2000, p must be 6000
	}

/*
	method(#class) test_sem_sig
	{
		| s |
		s := Semaphore new.
		s signalAction: [:sem | 'SIGNAL ACTION............' dump. ].
		[ System sleepForSecs: 1. s signal ] fork.
		s wait.
	}

	method(#class) test_semgrp
	{
		| sg |
		sg := SemaphoreGroup new.
		sg add: s1 withAction: [].
		sg add: s2 withAction: [].
		sg add: s3 withAction: [].
		sg wait.
	}
*/

	method(#class) main
	{
		| tc limit |

		tc := ##(
			// 0 - 4
			[ (self test_quicksort: #(7 12 3 20 5 8 2) copy) = #(2 3 5 7 8 12 20)],
			[ (self test_quicksort: ##(99, 12, 18, 7, 12, 3, 20, 5, 8, 2)) = #(2 3 5 7 8 12 12 18 20 99)],
			[ (self test_on_do_with: 10 with: 2) == 5 ],
			[ (self test_on_do_with: -10 with: 0) == 0 ],
			[ self test_ensure_with: -20945. self.ensure_tester_v == -20945 ],

			// 5-9
			[ ((self test_ensure2_with: 8) == 64) and: [self.ensure_tester_v == 8] ],
			[ self proc1 == 100 ], 
			[ System sleepForSecs: 2.  self proc1 == 200 ],
			[ self test_semaphore_heap == true ],
			[ self test_mutex = #(2000 6000) ],

			// 10-14
			////[ self test_sem_sig ],
			[ System sleepForSecs: 2. self.a == 300 ] // sleep before checking self.a to prevent different result depending on process switching frequency and speed
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		].
	}
}


/*
s1 := TcpSocket new.

s1 onEvent: #connected do: [
	s1 waitToRead.
	//s1 beginWrite: C'GET / HTTP/1.0\n\r'.
] 
s1 onEvent: #written do: [
].

s1 onEvent: #readyToRead do: [
	
].

s1 beginConnect: '1.2.3.4:45' onConnected: [ :result | xxxx].


////
s1 beginConnect: destination onConnected: 
s1 endConnect --> return what?
s1 endReceive
s1 beginReceive: buffer callback: [xxxx].
s1 beginSend: data onEnd: [do this].
s1 endSend

s1 beginAccept: [callback]
s1 endAccept -> returns the actual socket
*/

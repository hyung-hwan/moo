
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

	method(#class) main
	{
		| tc limit |

		tc := #{
			## 0 - 4
			[ self proc1 == 100 ], 
			[ Processor sleepFor: 2.  self proc1 == 200 ]
		}.

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		]
	}

}
